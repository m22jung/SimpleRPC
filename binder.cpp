#include "rpc.h"
#include "message_lib.h"
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
#include <vector>
#include <utility>

using namespace std;

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

    if (bind(*sockfd, (struct sockaddr *) address, addrlen) < 0) {
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

// returns index of database that matches server/port otherwise, return -1
int matchingServerPort(char *server_identifier, int port, vector<ServerData*> *database) {
    int sameServerIndex = -1;
    int databasesize = database->size();

    for (int i = 0; i < databasesize; ++i) {
        if (sameServerName(server_identifier, (*database)[i]->hostname)
            && port == (*database)[i]->port) {
            sameServerIndex = i;
            break;
        }
    }
    return sameServerIndex;
}

int main() {
    int sockfd, newsockfd, fd, maxfd, socket_n;
    struct sockaddr_in address;
    fd_set readfds;
    socklen_t addrlen = sizeof(address);
    bool flag_terminate = false;

    setup(&sockfd, &address); // opens and binds socket, prints address and port number

    if (listen(sockfd, 5) < 0) {
        cerr << "ERROR on listen" << endl;
        exit(EXIT_FAILURE);
    }

    vector<int> socket_connected; // list of sockets connected. servers stay connected whole time.
    vector<ServerData*> database; // database for serverData registered by servers

    while (!flag_terminate) { // for as many requests
        begin:
        FD_ZERO(&readfds); // reset readfds
        FD_SET(sockfd, &readfds);
        maxfd = sockfd;
        socket_n = socket_connected.size();
        cout << "socket_n: " << socket_n << endl;

        for (int i = 0; i < socket_n; ++i) { // set readfds
            fd = socket_connected[i];
            if (fd > 0) FD_SET(fd, &readfds);
            if (fd > maxfd) maxfd = fd;
        }

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            cerr << "ERROR on select" << endl;
            continue;
        }

        if (FD_ISSET(sockfd, &readfds)) { // New connection
            if ((newsockfd = accept(sockfd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
                cerr << "ERROR on accept" << endl;
                continue;
            }
            socket_connected.push_back(newsockfd);
            continue;
        }

        for (int i = 0; i < socket_n; i++) { // Request
            fd = socket_connected[i];
            if (FD_ISSET(fd, &readfds)) {
                cout << "-READ- ";

                int len, type;
                int receiveLengthAndTypeResult = receiveLengthAndType(fd, len, type);
                if (receiveLengthAndTypeResult == SOCKET_CONNECTION_FINISHED) {
                    close(fd);
                    cout << "fd=" << fd << " disconnected" << endl;
                    socket_connected.erase(socket_connected.begin() + i);
                } else if (receiveLengthAndTypeResult == READING_SOCKET_ERROR) {
                    goto begin;
                } else {
                    cout << "Length: " << len << ", TYPE: " << type << endl;

                    char *message = new char[len];

                    if (read(fd, message + 8, len - 8) < 0) {
                        cerr << "ERROR reading from socket" << endl;
                        goto begin;
                    }

                    char *server_identifier = new char[1024];
                    int port;
                    char *name = new char[64];
                    int argTypeslen = (len - 8 - 1024 - 4 - 64) / 4;
                    int *argTypes = new int[argTypeslen];
                    int receiveResult;
                    int sameServerIndex;

                    if (type == REGISTER) {
                            cout << "REGISTER" << endl;

                            receiveServerIdentifierAndPortAndNameAndArgType(len, message, server_identifier, port, name, argTypes);

                            printf("Server_ideitifier: %s", server_identifier);
                            cout << " port: " << port << endl;
                            
                            // new FunctionData from REGISTER
                            FunctionData *fdata = new FunctionData(name, argTypes);

                            // check if same server/port exists in database 
                            sameServerIndex = matchingServerPort(server_identifier, port, &database);
                            if (sameServerIndex == -1) {
                                cout << "ServerData added" << endl;
                                database.push_back(new ServerData(server_identifier, port));
                                (database.back())->addFunctionToList(fdata);
                            } else {
                                // check if FunctionData is in function list
                                if (database[sameServerIndex]->functionInList(fdata)) {
                                    cout << "Same FunctionData exists in database" << endl;
                                    delete fdata;
                                } else {
                                    cout << "non-existing FunctionData added to server: ";
                                    printf("%s port=%d\n", database[sameServerIndex]->hostname, database[sameServerIndex]->port);
                                    database[sameServerIndex]->addFunctionToList(fdata);
                                }
                            }

                            //TODO: send reg sucess back

                    } else if (type == LOC_REQUEST) {
                            cout << "LOC_REQUEST" << endl;

                            receiveNameAndArgType(len, message, name, argTypes);

                            // TODO: Find matching function, reply loc_success

                    } else if (type == TERMINATE) {
                            cout << "TERMINATE" << endl;

                            // TODO: send termination msg to all servers

                            flag_terminate = true;
                    }
                    delete[] message;
                }
            } // if
        } // for
        cout << endl;
    } // while
} // main
