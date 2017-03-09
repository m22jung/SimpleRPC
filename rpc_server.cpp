//
// Created by J on 2017. 3. 7..
//

#include "rpc.h"
#include "type_lib.h"
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

int sockfd_client, sockfd_binder, binder_socket, client_socket;

int rpcInit() {
	int port, binder_port;
	struct sockaddr_in binder_addr, address;
	struct hostnet *binder;

	// connection socket for clients
    socklen_t addrlen = sizeof(address);

    sockfd_client = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_client < 0) {
        cerr << "ERROR opening socket" << endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = INADDR_ANY;

    if ( bind(sockfd_client, (struct sockaddr *) &address, addrlen) < 0) {
        cerr << "ERROR on binding" << endl;
        close(sockfd_client);
        exit(EXIT_FAILURE);
    }

    getsockname(sockfd_client, (struct sockaddr *) &address, &addrlen);
    port = ntohs(address.sin_port);
    string SERVER_PORT = to_string(port);

    char SERVER_ADDRESS[1024];
    gethostname(SERVER_ADDRESS, 1024);
    
    printf("SERVER_ADDRESS %s\n", SERVER_ADDRESS);
    cout << "SERVER_PORT " << SERVER_PORT << endl;

    if ( listen(sockfd_client, 5) < 0 ) {
        cerr << "ERROR on listen" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "socket for clients done" << endl;

    // open connection to binder
    char *BINDER_PORT = getenv("BINDER_PORT");
    if (BINDER_PORT == NULL) {
        cerr << "ERROR, BINDER_PORT does not exist" << endl;
        exit(EXIT_FAILURE);
    }
    portno = atoi(BINDER_PORT);
    
    char *BINDER_ADDRESS = getenv("BINDER_ADDRESS");
    if (BINDER_ADDRESS == NULL) {
        cerr << "ERROR, BINDER_ADDRESS does not exist" << endl;
        exit(EXIT_FAILURE);
    }
    binder = gethostbyname( BINDER_ADDRESS );
    if (binder == NULL) {
        cerr << "ERROR, no such host" << endl;
        exit(EXIT_FAILURE);
    }
    
    sockfd_binder = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_binder < 0) {
        cerr << "ERROR opening socket" << endl;
        exit(EXIT_FAILURE);
    }

    bzero((char *) &binder_addr, sizeof(binder_addr));
    binder_addr.sin_family = AF_INET;

    bcopy((char *)binder->h_addr, (char *)&binder_addr.sin_addr.s_addr, binder->h_length);
    binder_addr.sin_port = htons(portno);

    if (connect(sockfd_binder,(struct sockaddr *) &binder_addr,sizeof(binder_addr)) < 0) {
        cerr << "ERROR connecting" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "open connection to binder done" << endl;
}

int main() {
	rpcInit();
}