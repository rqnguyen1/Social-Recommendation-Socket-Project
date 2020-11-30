#include "client.h"

//mainserver port#
#define MAINSERVER_PORT 33481
//loopback address
#define LOOPBACK_ADDR "127.0.0.1"

//client TCP socket
int sockfd;

//mainserver address
struct sockaddr_in mainserver_addr;

//sets up TCP  socket and connects with the mainserver
void setup_TCP_socket()
{
    //create client TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failure");
    }

    mainserver_addr.sin_family = AF_INET;
    mainserver_addr.sin_addr.s_addr = inet_addr(LOOPBACK_ADDR);
    mainserver_addr.sin_port = htons(MAINSERVER_PORT);
   
    //connect to mainserver
    if (connect(sockfd, (struct sockaddr *) &mainserver_addr, sizeof(mainserver_addr)) < 0)
    {
        perror("connection failure");
    }

}

/*
    sends the country and userID query to the mainserver
    @param country - the country inputted by the user
    @param userID - the user ID inputted by the user
*/
void send_TCP_query(const std::string& country, const std::string& userID)
{
    std::string msg = country + " " + userID;
    send(sockfd, msg.c_str(), strlen(msg.c_str()), 0);

    std::cout << "The client has sent User" << userID << " and " << country << " to Main Server using TCP" << std::endl;
    receive_TCP_resp(country, userID);
}

/*
    receives query response from mainserver
    @param country - the country inputted by the user
    @param userID - the user ID inputted by the user
*/
void receive_TCP_resp(const std::string& country, const std::string& userID)
{
    char buf[1024];
    int buf_len = recv(sockfd, buf, 1024, 0);
    buf[buf_len] = '\0';

    std::string recommendation(buf);

    // country not found
    if (recommendation == (country + ": not found"))
    {
        std::cout << country << " not found" << std::endl;
    }

    // user not found
    else if (recommendation == ("User" + userID + ": not found"))
    {
        std::cout << "User" << userID << " not found" << std::endl;
    }

    // received recommendation
    else
    {
        // recommendation is none
        if (recommendation == "-1")
        {
            recommendation = " None";
        }
        std::cout << "The client has received results from Main Server: User" << recommendation 
        << " is a possible friend of User" << userID << " in " << country << std::endl;
    }
}

//signal handler for SIGINT, closes all sockets
void sig_handler(int s)
{
    close(sockfd);
    exit(1);
}

int main()
{
    std::cout << "The client is up and running" << std::endl;

    setup_TCP_socket();

    struct sigaction sa;
    sa.sa_handler = sig_handler;

    sigaction(SIGINT, &sa, NULL );

    while (true)
    {
        std::cout << "Enter country name: ";
        std::string country;
        std::cin >> country;

        std::cout << "Enter user ID: ";
        std::string userID;
        std::cin >> userID;

        send_TCP_query(country, userID);
    }
    

    return 0;
}

