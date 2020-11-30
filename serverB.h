#ifndef SERVERB_H
#define SERVERB_H

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <set>
#include <vector>
#include <sstream>
#include <iterator>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <signal.h>

//Opens data2.txt and parses the file
void parseFile();

//setup UDP socket
void setup_UDP_socket();

//sends country mapping to mainserver after receiving request
void send_UDP_countries();

//receives query from the mainserver, runs the recommendation algorithm, and replies
void process_user_query();

/*
    Recommendation algorithm based on common neighbors
    @param country - name of the country
    @param userID - User's ID
    @return -1 if the user is already connected to all other users
            otherwise return the user ID of the recommendation
*/
int getRecommendation(const std::string& country, const int& userID);

//signal handler for SIGINT, closes all sockets
void sig_handler(int s);

#endif