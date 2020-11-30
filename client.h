#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <string>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <signal.h>

//sets up TCP socket and connects with the main server
void setup_TCP_socket();

/*
    sends the country and userID query to the mainserver
    @param country - the country inputted by the user
    @param userID - the user ID inputted by the user
*/
void send_TCP_query(const std::string& country, const std::string& userID);

/*
    receives query response from mainserver
    @param country - the country inputted by the user
    @param userID - the user ID inputted by the user
*/
void receive_TCP_resp(const std::string& country, const std::string& userID);

//signal handler for SIGINT, closes all sockets
void sig_handler();

#endif