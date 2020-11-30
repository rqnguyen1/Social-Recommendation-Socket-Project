#ifndef SERVERMAIN_H
#define SERVERMAIN_H

#include <iostream>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <signal.h>

//sets up TCP socket and accepts connection from client
void setup_TCP_socket();

//receives TCP query from the client and contacts the backend server for recommendation
void process_TCP_query();

/*
    sends query response to the client
    @param msg - the response messge to the client's query
*/
void send_TCP_resp(const std::string& msg);


//setup UDP socket to communicate with backend servers
void setup_UDP_socket();

/*
    receive countries from backend servers
    @param server - specifies which backend server
                    0 = backend server A
                    1 = backend server B
*/
void receive_UDP_countries(const int& server);

//prints out the country mapping
void print_country_map();

/*
    sends client's query to the corresponding backend server
    @param country - the country to query
    @param userID - the userID to query
    @param backendServer - specifies which backend server
                            0 = backend server A
                            1 = backend server B
*/
void send_UDP_query(const std::string& country, const std::string& userID, const int& backendServer);


/*
    receives recommendation from the backend server
    @param country - country that was queried
    @param userID - userID that was queried
    @param backendServer - specifies which backend server
                            0 = backend server A
                            1 = backend server B
*/
void receive_UDP_recommendation(const std::string& country, const std::string& userID, const int& backendServer);

//signal handler for SIGINT, closes all sockets
void sig_handler(int s);


#endif