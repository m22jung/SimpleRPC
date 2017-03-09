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
using namespace std;

int main() {
    int sockfd, newsockfd, data_n, fd;
    vector<int> socket_connected;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    fd_set readfds;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "ERROR opening socket" << endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = INADDR_ANY;

    if ( bind(sockfd, (struct sockaddr *) &address, addrlen) < 0) {
        cerr << "ERROR on binding" << endl;
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    getsockname(sockfd, (struct sockaddr *) &address, &addrlen);
    int port = ntohs(address.sin_port);
    string BINDER_PORT = to_string(port);

    char BINDER_ADDRESS[1024];
    gethostname(BINDER_ADDRESS, 1024);
    
    printf("BINDER_ADDRESS %s\n", BINDER_ADDRESS);
    cout << "BINDER_PORT " << BINDER_PORT << endl;

    if ( listen(sockfd, 5) < 0 ) {
        cerr << "ERROR on listen" << endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
      begin:
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        maxfd = sockfd;
        socket_n = socket_connected.size();

        for (int i = 0 ; i < socket_n ; ++i) {
            fd = *socket_connected[i];
            if(fd > 0) FD_SET(fd , &readfds);
            if(fd > maxfd) maxfd = fd;
        }

        if ( select( maxfd + 1 , &readfds , NULL , NULL , NULL) < 0 ) {
            cerr << "ERROR on select" << endl;
            goto begin;
        }

        if (FD_ISSET(sockfd, &readfds)) { // New connection
        	cout << "New connection" << endl;
            if ( (newsockfd = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0 ){
                cerr << "ERROR on accept" << endl;
                goto begin;
            }

	        int *fdp = new int;
	        FD_SET(*fdp, &readfds);
	        if (*fdp > maxfd) maxfd = *fdp;
	        socket_connected.push_back(fdp);
            break;
        }

        cout << "Request" << endl;
        for (int i = 0; i < socket_n; i++) {
            fd = *socket_connected[i];
            if (FD_ISSET(fd, &readfds)) {
                char buffer[4];
                int valread = read(fd , buffer, 4);
                
                if (valread == 0) { // disconnected
                    close( fd );
                    socket_connected.erase(i);
                } else if (valread < 0) {
                    cerr << "ERROR reading from socket" << endl;
                    goto begin;
                } else { // read
                	cout << "READ" << endl;
                    /*int len = (int)((unsigned char)(buffer[0]) |
                        (unsigned char)(buffer[1]) << 8 |
                        (unsigned char)(buffer[2]) << 16 |
                        (unsigned char)(buffer[3]) << 24 );

                    char *message = new char[len + 4];
                    memcpy(message, buffer, 4);

                    if ( read(fd, message + 4, len) < 0 ) {
                        //cerr << "ERROR reading from socket" << endl;
                        goto begin;
                    }
                    printf("%s\n", message + 4);

                    titlecase(message + 4);

                    if ( write(fd, message, len + 4) < 0) {
                        cerr << "ERROR sending to socket" << endl;
                        goto begin;
                    }

                    delete [] message;
                    */

                    char *message = new char("request received");
                    if ( write(fd, message, 16) < 0) {
                        cerr << "ERROR sending to socket" << endl;
                        goto begin;
                    }

                    delete [] message;
                }
            }
        }

    } // while
}