#include "rpc.h"
#include "message_lib.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> // FD_SET, FD_ISSET, FD_ZERO macros
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <vector>
using namespace std;

struct FunctionData {
	char name[64];
	int *argTypes;
	char hostname[1024];
	int port;

	FunctionData(char *n, int *argTypes, skeleton f, char *hn, int port);
};

// setup() opens and binds socket, prints address and port number
void setup(int *sockfd, struct sockaddr_in *address) {
    socklen_t addrlen = sizeof(*address);

    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd < 0) {
        cerr << "ERROR opening socket" << endl;
        exit(EXIT_FAILURE);
    }

    address->sin_family = AF_INET;
    address->sin_port = 0;
    address->sin_addr.s_addr = INADDR_ANY;

    if ( bind(*sockfd, (struct sockaddr *) address, addrlen) < 0) {
        cerr << "ERROR on binding" << endl;
        close(*sockfd);
        exit(EXIT_FAILURE);
    }

    getsockname(*sockfd, (struct sockaddr *) address, &addrlen);
    int port = ntohs(address->sin_port);
    string BINDER_PORT = to_string(port);

    char BINDER_ADDRESS[1024];
    gethostname(BINDER_ADDRESS, 1024);
    
    printf("BINDER_ADDRESS %s\n", BINDER_ADDRESS);
    cout << "BINDER_PORT " << BINDER_PORT << endl;
}


int main() {
	int sockfd, newsockfd, fd, maxfd, socket_n;
	struct sockaddr_in address;
	fd_set readfds;
	socklen_t addrlen = sizeof(address);
	bool flag_terminate = false;
	
	setup(&sockfd, &address); // opens and binds socket, prints address and port number

	if ( listen(sockfd, 5) < 0 ) {
        cerr << "ERROR on listen" << endl;
        exit(EXIT_FAILURE);
    }

	vector<int> socket_connected; // list of sockets connected. servers stay connected whole time.
    vector<FunctionData*> database; // database for server functions registered by servers

    while ( !flag_terminate ) { // for as many requests
      begin:
        FD_ZERO(&readfds); // reset readfds
        FD_SET(sockfd, &readfds);
        maxfd = sockfd;
        socket_n = socket_connected.size();
        cout << "socket_n: " << socket_n << endl;

        for (int i = 0 ; i < socket_n ; ++i) { // set readfds
            fd = socket_connected[i];
            if(fd > 0) FD_SET(fd , &readfds);
            if(fd > maxfd) maxfd = fd;
        }

        if ( select( maxfd + 1 , &readfds , NULL , NULL , NULL) < 0 ) {
            cerr << "ERROR on select" << endl;
            continue;
        }
        
        if (FD_ISSET(sockfd, &readfds)) { // New connection
            if ( (newsockfd = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0 ){
                cerr << "ERROR on accept" << endl;
                continue;
            }
			socket_connected.push_back(newsockfd);
            continue;
        }

        for (int i = 0; i < socket_n; i++) { // Request
            fd = socket_connected[i];
            if (FD_ISSET(fd, &readfds)) {
                char buffer[8];
                int valread = read(fd , buffer, 8);
                
                if (valread == 0) { // disconnected
                    close( fd );
                    cout << "fd=" << fd << " disconnected" << endl;
                    socket_connected.erase(socket_connected.begin() + i);
                } else if (valread < 0) {
                    cerr << "ERROR reading from socket" << endl;
                    goto begin;
                } else { // read
                	cout << "-READ- ";
                    int len = (int)((unsigned char)(buffer[0]) << 24 |
                        (unsigned char)(buffer[1]) << 16 |
                        (unsigned char)(buffer[2]) << 8 |
                        (unsigned char)(buffer[3]) );

                    int type = (int)((unsigned char)(buffer[4]) << 24 |
                        (unsigned char)(buffer[5]) << 16 |
                        (unsigned char)(buffer[6]) << 8 |
                        (unsigned char)(buffer[7]) );

                    cout << "Length: " << len << ", TYPE: " << type << endl;

                    char *message = new char[len];
                    memcpy(message, buffer, 8);

                    if ( read(fd, message + 8, len - 8) < 0 ) {
                        cerr << "ERROR reading from socket" << endl;
                        goto begin;
                    }

                    char *server_identifier = new char[1024];
                    int port;
                    char *name = new char[64];
                    int argTypeslen = (len - 8 - 1024 - 4 - 64) / 4;
                    int* argTypes = new int[argTypeslen];
                    int receiveResult;

                    switch (type) {
                    	case REGISTER:
                    		cout << "REGISTER" << endl;
                            receiveResult = receiveRegRequest(len, message, server_identifier, port, name, argTypes);
                            printf("Server_ideitifier: %s\n", server_identifier);
                            cout << "port: " << port << endl;
                            printf("name: %s\n", name);
                            
                            for (int j = 0; j < argTypeslen; ++j) {
                                cout << "arg type " << j << " is " << argTypes[j] << endl;
                            }
                            if (receiveResult < 0) {
                                // error
                                return receiveResult;
                            }
                    		break;
                    	case LOC_REQUEST:
                    		cout << "LOC_REQUEST" << endl;
                    		break;
                    	case TERMINATE:
                    		cout << "TERMINATE" << endl;
                    		flag_terminate = true;
                    		break;
                    }
                    
                    delete [] message;

                    /*
                    if ( write(fd, message, len + 4) < 0) {
                        cerr << "ERROR sending to socket" << endl;
                        goto begin;
                    }

                    char message[17] = "request received";
                    if ( write(fd, message, 16) < 0) {
                        cerr << "ERROR sending to socket" << endl;
                        goto begin;
                    }
                    */
                } // else
            } // if
        } // for

    } // while
} // main
