#include "serverB.h"

//server B port#
#define SERVERB_PORT 31481
//mainserver UDP port#
#define MAINSERVER_UDP_PORT 32481
//loopback address
#define LOOPBACK_ADDR "127.0.0.1"

//social network graphs
std::unordered_map<std::string, std::unordered_map< int, std::set<int> >> graphs;

//backend server B UDP socket
int sockfd;

//server B address, mainserver UDP address
struct sockaddr_in serverB_addr, mainserver_UDP_addr;

//opens and parses data2.txt
void parseFile()
{
    std::ifstream data1_file;
    data1_file.open("./testcase1/data2.txt");
    
    std::string line;
    if (data1_file.is_open())
    {
        //read each line in the file
        std::string currCountry;
        while ( getline(data1_file, line) )
        {
            //country
            if (isalpha(line[0]))
            {
                currCountry = line;
                std::unordered_map<int, std::set<int>> graph;
                graphs.insert({currCountry, graph});
            }

            //user
            else
            {
                int numTokens = 0;

                std::stringstream ss(line);
                std::string node;
                int key = 0;
                std::set<int> neighbors;

                //read each token in the line
                while (ss >> node)
                {   
                    if (numTokens == 0)
                    {
                        key = std::stoi(node);
                        graphs[currCountry].insert({key, neighbors});
                    }
                    else
                    {
                        graphs[currCountry][key].insert(std::stoi(node));
                    }
                    ++numTokens;
                  
                }
            }

        }
        data1_file.close();
    }
}

//setup UDP socket and sends country mapping to mainserver
void setup_UDP_socket()
{
    //create serverB UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
    {
        perror("socket failure");
    }

    serverB_addr.sin_family = AF_INET;
    serverB_addr.sin_addr.s_addr = inet_addr(LOOPBACK_ADDR);
    serverB_addr.sin_port = htons(SERVERB_PORT);

    //bind socket
    if (bind(sockfd, (struct sockaddr* ) &serverB_addr, sizeof(serverB_addr)) < 0)
    {
        perror("bind failure");
    }

    //configure mainserver's address
    mainserver_UDP_addr.sin_family = AF_INET;
    mainserver_UDP_addr.sin_addr.s_addr = inet_addr(LOOPBACK_ADDR);
    mainserver_UDP_addr.sin_port = htons(MAINSERVER_UDP_PORT);

}

void send_UDP_countries()
{
    char buf[1024];

    //receive request for country mapping from mainserver
    int mainserver_addr_len = sizeof(mainserver_UDP_addr);
    int buf_len = recvfrom(sockfd, buf, 1024, 0,(struct sockaddr*) &mainserver_UDP_addr, (socklen_t *) &mainserver_addr_len);
    buf[buf_len] = '\0';

    std::string s(buf);

    //send # of countries to mainserver
    int graph_size = graphs.size();
    char* num_countries = (char*) &graph_size;
    sendto(sockfd, num_countries, sizeof(num_countries), 0, (struct sockaddr *) &mainserver_UDP_addr, mainserver_addr_len);

    //send countries to mainserver
    for (const auto& pair: graphs)
    {
        std::string country = pair.first;
        sendto(sockfd, country.c_str(), strlen(country.c_str()), 0, (struct sockaddr *) &mainserver_UDP_addr, mainserver_addr_len);
    }

    std::cout << "The server B has sent a country list to Main Server" << std::endl;
}

//receives query from the mainserver and runs the recommendation algorithm
void process_user_query()
{
    char country_buf[1024];
    char userID_buf[1024];

    //receive user query from mainserver
    int mainserver_addr_len = sizeof(mainserver_UDP_addr);
    int buf_len = recvfrom(sockfd, country_buf, 1024, 0,(struct sockaddr*) &mainserver_UDP_addr, (socklen_t *) &mainserver_addr_len);
    country_buf[buf_len] = '\0';

    buf_len = recvfrom(sockfd, userID_buf, 1024, 0,(struct sockaddr*) &mainserver_UDP_addr, (socklen_t *) &mainserver_addr_len);
    userID_buf[buf_len] = '\0';

    std::string country(country_buf);
    std::string userID(userID_buf);

    std::cout << "The server B has received request for finding possible friends of User" << userID << " in " << country << std::endl;
    int id = std::stoi(userID);

    //cannot find userID in this country
    if (graphs[country].find(id) == graphs[country].end())
    {
        std::cout << "User" << userID << " does not show up in " << country << std::endl;

        std::string notFound = "User" + userID + ": not found";
        sendto(sockfd, notFound.c_str(), strlen(notFound.c_str()), 0, (struct sockaddr *) &mainserver_UDP_addr, mainserver_addr_len);
        std::cout << "The server B has sent “User" << userID << " not found” to Main Server" << std::endl;
    }

    //found the userID in this country
    else
    {
        std::cout << "The server B is searching possible friends for User" << userID << " ..." << std::endl;
        int recommendation = getRecommendation(country, id);
        if (recommendation == -1)
        {
            std::cout << "Here is the result: User None" << std::endl;
        }
        else
        {
            std::cout << "Here is the result: User" << recommendation <<std::endl;
        }

        std::string rec = std::to_string(recommendation);
        sendto(sockfd, rec.c_str(), strlen(rec.c_str()), 0, (struct sockaddr *) &mainserver_UDP_addr, mainserver_addr_len);
        std::cout << "The server B has sent the result to Main Server" << std::endl;
    }


}

/*
    Recommendation algorithm based on common neighbors
    @param country - name of the country
    @param userID - User's ID
    @return -1 if the user is already connected to all other users
            otherwise return the user ID of the recommended user
*/
int getRecommendation(const std::string& country, const int& userID)
{
    //1. user is already connected to all the other users in the country C
    int numConnections = graphs[country][userID].size();
    int totalNumUsers = graphs[country].size();

    if ((numConnections == totalNumUsers - 1) || totalNumUsers == 1)
    {
        return -1;
    }

    //2. user is not yet connected to some users in country C
    else
    {
        //a. no n shares any common neighbors with user
        //b. n shares common neighbors with user
        
        int maxCommonNeighbors = 0;
        int recommendation = -1;
        int highestDegree = 0;
        int userWithHighestDegree = -1;

        //for each user in the country
        std::unordered_map< int, std::set<int>>& countryGraph = graphs[country]; 
        for (const auto& user: countryGraph)
        {
            int currCommonNeighbors = 0;
            //not the same user and also not already a connection of the user
            if (user.first != userID && countryGraph[userID].find(user.first) == countryGraph[userID].end())
            {
                //for each of this user's neighbors
                for (const auto& neighbor: user.second)
                {
                    //found a common neighbor
                    if (countryGraph[userID].find(neighbor) != countryGraph[userID].end())
                    {
                        ++currCommonNeighbors;
                    }
                }
            
                //update max so far
                if (currCommonNeighbors > maxCommonNeighbors)
                {
                    maxCommonNeighbors = currCommonNeighbors;
                    recommendation = user.first;
                }

                //tie
                else if ((currCommonNeighbors == maxCommonNeighbors) && (currCommonNeighbors != 0))
                {
                    //recommend the user with the smallest ID to break the tie
                    recommendation = std::min(recommendation, user.first);
                }

                //keep track of highest degree in case there are no common neighbors
                int degree = countryGraph[user.first].size();
                if (degree > highestDegree)
                {
                    highestDegree = degree;
                    userWithHighestDegree = user.first;
                }

                else if (degree == highestDegree)
                {
                    userWithHighestDegree = std::min(userWithHighestDegree, user.first);
                }
            }
        }

        //there were no common neighbors
        if (recommendation == -1)
        {
            return userWithHighestDegree;
        }

        return recommendation;

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
    std::cout << "The server B is up and running using UDP on port 31481" << std::endl;
    parseFile();

    setup_UDP_socket();
    send_UDP_countries();

    struct sigaction sa;
    sa.sa_handler = sig_handler;

    sigaction(SIGINT, &sa, NULL );

    while (true)
    {
        process_user_query();
    }


    return 0;
}