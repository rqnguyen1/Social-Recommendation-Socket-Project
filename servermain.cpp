#include "servermain.h"

//mainserver TCP port#
#define MAINSERVER_PORT 33481
//mainserver UDP port#
#define MAINSERVER_UDP_PORT 32481
//backend server A port#
#define SERVERA_PORT 30481
//backend server B port#
#define SERVERB_PORT 31481
//loopback address
#define LOOPBACK_ADDR "127.0.0.1"

//mainserver TCP socket
int sockfd;

//mainserver TCP child socket
int childsock;

//mainserver UDP socket
int udpSock;

//backend server mapping
std::unordered_map<std::string, int> country_backend_mapping;

//mainserver TCP address, mainserver UDP address, server A address, server B address
struct sockaddr_in mainserver_addr, mainserver_UDP_addr, serverA_addr, serverB_addr;

//process id
int pid;

//sets up TCP socket and accepts connection from client
void setup_TCP_socket()
{
    //create servermain socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failure");
    }
    
    mainserver_addr.sin_family = AF_INET;
    mainserver_addr.sin_addr.s_addr = inet_addr(LOOPBACK_ADDR);
    mainserver_addr.sin_port = htons(MAINSERVER_PORT);

    //bind socket
    if (bind(sockfd, (struct sockaddr* )& mainserver_addr, sizeof(mainserver_addr)) < 0)
    {
        perror("bind failure");
    }

    //listen
    if (listen(sockfd, 2) < 0)
    {
        perror("listen failure");
    }

    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    //accept
    if ((childsock = accept(sockfd, (struct sockaddr* )& clientAddr,(socklen_t *) &clientAddrLen)) < 0)
    {
        perror("accept failure");
    }

    //create child process to handle connection
    //close child socket and continue to accept if main process
    if (pid = fork() > 0)
    {
        close(childsock);

        if ((childsock = accept(sockfd, (struct sockaddr* )& clientAddr,(socklen_t *) &clientAddrLen)) < 0)
        {
            perror("accept failure");
        }
    }

}

//receives TCP query from the client and contacts the backend server for recommendation
void process_TCP_query()
{
    char buf[1024];

    //receive country and userID from client
    int buf_len = recv(childsock, buf, 1024, 0);
    buf[buf_len] = '\0';

    char* token;
    std::string country;
    std::string userID;
    if (buf_len > 0)
    {
        token = strtok(buf, " ");
        country = token;
        token = strtok(NULL, " ");
        userID = token;
        
        
        int client = 0;
        if (pid == 0)
        {
            client = 1;
        }
        else
        {   
            client = 2;
        }

        std::cout << "The Main server has received the request on User" << userID << " in " << country << " from client" << client << " using TCP over port " << MAINSERVER_PORT << std::endl;
        //can't find country name
        if (country_backend_mapping.find(country) == country_backend_mapping.end())
        {
            std::cout << country << " does not show up in server A&B" << std::endl;
            std::string msg = country + ": not found";
            send_TCP_resp(msg);
            std::cout << "The Main Server has sent “" << country <<  ": Not found” to client" << client << " using TCP over port " << MAINSERVER_PORT << std::endl;
        }
        //found country name
        else
        {   
            std::string server;
            int backendServer = country_backend_mapping[country];
            if (backendServer == 0)
            {
                server = "A";
            }
            else
            {
                server = "B";
            }

            std::cout << country << " shows up in server "<< server << std::endl;
            send_UDP_query(country, userID, backendServer);

            std::cout << "The Main Server has sent request from User" << userID << " to server "<< server << " using UDP over port " << MAINSERVER_PORT << std::endl;
            receive_UDP_recommendation(country, userID, backendServer);
        }
    }
}

/*
    sends query response to the client
    @param msg - the response messge to the client's query
*/
void send_TCP_resp(const std::string& msg)
{
    send(childsock, msg.c_str(), strlen(msg.c_str()), 0);
}

/*
    setup UDP socket with backend server
    @param server - specifies which backend server
                    0 = backend server A
                    1 = backend server B
*/
void setup_UDP_socket()
{
    mainserver_UDP_addr.sin_family = AF_INET;
    mainserver_UDP_addr.sin_addr.s_addr = inet_addr(LOOPBACK_ADDR);
    mainserver_UDP_addr.sin_port = htons(MAINSERVER_UDP_PORT);

    //create socket
    if ((udpSock = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
    {
        perror("socket failure");
    }

    //bind socket
    if (bind(udpSock, (struct sockaddr* ) &mainserver_UDP_addr, sizeof(mainserver_UDP_addr)) < 0)
    {
        perror("bind failure");
    }

}

/*
    receive countries from backend servers
    @param server - specifies which backend server
                    0 = backend server A
                    1 = backend server B
*/
void receive_UDP_countries(const int& server)
{
    //backend server A
    if (server == 0)
    {
        serverA_addr.sin_addr.s_addr = inet_addr(LOOPBACK_ADDR);
        serverA_addr.sin_port = htons(SERVERA_PORT);

        //sends country mapping request to backend server
        std::string msg = "Requesting country mapping";
        int serverA_addr_len = sizeof(serverA_addr);
        sendto(udpSock, msg.c_str(), strlen(msg.c_str()), 0, (struct sockaddr *) &serverA_addr, serverA_addr_len);

        int num_countries = 0;
        recvfrom(udpSock, (char*) &num_countries, sizeof(num_countries), 0,(struct sockaddr*) &serverA_addr, (socklen_t *) &serverA_addr_len);

        char buf[1024];
        
        //receive countries from serverA and store it in country_backend_mapping
        for (int i = 0; i < num_countries; ++i)
        {
            int buf_len = recvfrom(udpSock, buf, 1024, 0,(struct sockaddr*) &serverA_addr, (socklen_t *) &serverA_addr_len);
            buf[buf_len] = '\0';
            std::string s(buf);
            country_backend_mapping.insert({s,0});
            char buf[1024];
        }

        std::cout << "The Main server has received the country list from server A using UDP over port " << MAINSERVER_PORT << std::endl;
    }
    //backend server B
    else
    {
        serverB_addr.sin_addr.s_addr = inet_addr(LOOPBACK_ADDR);
        serverB_addr.sin_port = htons(SERVERB_PORT);

        //sends country mapping request to backend server
        std::string msg = "Requesting country mapping";
        int serverB_addr_len = sizeof(serverB_addr);
        sendto(udpSock, msg.c_str(), strlen(msg.c_str()), 0, (struct sockaddr *) &serverB_addr, serverB_addr_len);

        int num_countries = 0;
        recvfrom(udpSock, (char*) &num_countries, sizeof(num_countries), 0,(struct sockaddr*) &serverB_addr, (socklen_t *) &serverB_addr_len);

        char buf[1024];
        
        //receive countries from serverA and store it in country_backend_mapping
        for (int i = 0; i < num_countries; ++i)
        {
            int buf_len = recvfrom(udpSock, buf, 1024, 0,(struct sockaddr*) &serverB_addr, (socklen_t *) &serverB_addr_len);
            buf[buf_len] = '\0';
            std::string s(buf);
            country_backend_mapping.insert({s,1});
            char buf[1024];
        }

        std::cout << "The Main server has received the country list from server B using UDP over port " << MAINSERVER_PORT << std::endl;
    }

}

//prints out the country mapping
void print_country_map()
{
    std::cout << "Server A" << std::endl;
    for (const auto& pair: country_backend_mapping )
    {
        if (pair.second == 0)
        {
            std::cout << pair.first << std::endl;
        }
    }
    std::cout << std::endl;

    std::cout << "Server B" << std::endl;
    for (const auto& pair: country_backend_mapping )
    {
        if (pair.second == 1)
        {
            std::cout << pair.first << std::endl;
        }
    }

}

/*
    sends client's query to the corresponding backend server
    @param country - the country to query
    @param userID - the userID to query
    @param backendServer - specifies which backend server
                            0 = backend server A
                            1 = backend server B
*/
void send_UDP_query(const std::string& country,const std::string& userID, const int& backendServer)
{
    //backend server A
    if (backendServer == 0)
    {
        int serverA_addr_len = sizeof(serverA_addr);
        sendto(udpSock, country.c_str(), strlen(country.c_str()), 0, (struct sockaddr *) &serverA_addr, serverA_addr_len);
        sendto(udpSock, userID.c_str(), strlen(userID.c_str()), 0, (struct sockaddr *) &serverA_addr, serverA_addr_len);
    }
    //backend server B
    else
    {
        int serverB_addr_len = sizeof(serverB_addr);
        sendto(udpSock, country.c_str(), strlen(country.c_str()), 0, (struct sockaddr *) &serverB_addr, serverB_addr_len);
        sendto(udpSock, userID.c_str(), strlen(userID.c_str()), 0, (struct sockaddr *) &serverB_addr, serverB_addr_len);
    }

}

/*
    receives recommendation from the backend server
    @param country - country that was queried
    @param userID - userID that was queried
    @param backendServer - specifies which backend server
                            0 = backend server A
                            1 = backend server B
*/
void receive_UDP_recommendation(const std::string& country, const std::string& userID, const int& backendServer)
{
    char buf[1024];
    int buf_len;
    std::string server;

    //backend server A
    if (backendServer == 0)
    {
        server = "Server A";
        int serverA_addr_len = sizeof(serverA_addr);
        buf_len = recvfrom(udpSock, buf, 1024, 0,(struct sockaddr*) &serverA_addr, (socklen_t *) &serverA_addr_len);
    }

    //backend server B
    else
    {
        server = "Server B";
        int serverB_addr_len = sizeof(serverB_addr);
        buf_len = recvfrom(udpSock, buf, 1024, 0,(struct sockaddr*) &serverB_addr, (socklen_t *) &serverB_addr_len);
    }

    buf[buf_len] = '\0';

    std::string recommendation(buf);
    
    //Received search results
    if (!isalpha(recommendation[0]))
    {
        std::cout << "The Main server has received searching result(s) of User" << userID << " from " << server << std::endl;
        send_TCP_resp(recommendation);
        std::cout << "The Main Server has sent searching result(s) to client using TCP over port " << MAINSERVER_PORT << std::endl;

    }
    //User not found
    else
    {
        std::cout << "The Main Server has received \"" << recommendation << "\" from " << server << std::endl;
        send_TCP_resp(recommendation);
        std::cout << "The Main Server has sent error to client using TCP over " << MAINSERVER_PORT << std::endl;
    }
    
}

//signal handler for SIGINT, closes all sockets
void sig_handler(int s)
{
    //close mainserver UDP socket
    close(udpSock);

    //close child TCP socket
    close(childsock);

    //close parent TCP socket
    close(sockfd);

    exit(1);
}

int main()
{
    std::cout << "The Main server is up and running." << std::endl;

    setup_UDP_socket();

    //server A
    receive_UDP_countries(0);
    //server B
    receive_UDP_countries(1);
    
    print_country_map();

    //client
    setup_TCP_socket();

    struct sigaction sa;
    sa.sa_handler = sig_handler;

    sigaction(SIGINT, &sa, NULL );

    while (true)
    {
        process_TCP_query();
    }

    return 0;
}