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
//        cerr << "ERROR opening socket" << endl;
        exit(EXIT_FAILURE);
    }

    address->sin_family = AF_INET;
    address->sin_port = 0;
    address->sin_addr.s_addr = INADDR_ANY;

    if (bind(*sockfd, (struct sockaddr *) address, addrlen) < 0) {
//        cerr << "ERROR on binding" << endl;
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
    int databaseGlobalIndex = 0;
    int sockfd, newsockfd, fd, maxfd, socket_n;
    struct sockaddr_in address;
    fd_set readfds;
    socklen_t addrlen = sizeof(address);
    bool flag_terminate = false;

    setup(&sockfd, &address); // opens and binds socket, prints address and port number

    if (listen(sockfd, SOMAXCONN) < 0) {
//        cerr << "ERROR on listen" << endl;
        exit(EXIT_FAILURE);
    }

    vector<pair<int, bool>> socket_connected; // list of sockets connected. servers stay connected whole time.
    vector<ServerData*> database; // database for serverData registered by servers

    while (!flag_terminate) { // for as many requests
        begin:
        FD_ZERO(&readfds); // reset readfds
        FD_SET(sockfd, &readfds);
        maxfd = sockfd;
        socket_n = socket_connected.size();
//        cout << "socket_n: " << socket_n << endl;

        for (int i = 0; i < socket_n; ++i) { // set readfds
            fd = socket_connected[i].first;
            if (fd > 0) FD_SET(fd, &readfds);
            if (fd > maxfd) maxfd = fd;
        }

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
//            cerr << "ERROR on select" << endl;
            continue;
        }

        if (FD_ISSET(sockfd, &readfds)) { // New connection
            if ((newsockfd = accept(sockfd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
//                cerr << "ERROR on accept" << endl;
                continue;
            }
            socket_connected.push_back(make_pair(newsockfd, false));
            continue;
        }

        for (int i = 0; i < socket_n; i++) { // Request
            fd = socket_connected[i].first;
            if (FD_ISSET(fd, &readfds)) {
//                cout << "-READ- ";

                int len, type;
                int receiveLengthAndTypeResult = receiveLengthAndType(fd, len, type);
                if (receiveLengthAndTypeResult == SOCKET_CONNECTION_FINISHED) {
                    close(fd);
//                    cout << "fd=" << fd << " disconnected" << endl;
                    socket_connected.erase(socket_connected.begin() + i);
                } else if (receiveLengthAndTypeResult == READING_SOCKET_ERROR) {
                    goto begin;
                } else {
//                    cout << "Length: " << len << ", TYPE: " << type << endl;

                    char message[len];

                    if (read(fd, message + 8, len - 8) < 0) {
//                        cerr << "ERROR reading from socket" << endl;
                        goto begin;
                    }

                    char server_identifier[1024];
                    int port;
                    char name[64];
                    int argTypeslen;
                    int receiveResult;
                    int sameServerIndex;

                    if (type == REGISTER) {
//                            cout << "REGISTER" << endl;
                            argTypeslen = (len - 8 - 1024 - 4 - 64) / 4;
                            int argTypes[argTypeslen];
                            socket_connected[i].second = true;

                            receiveServerIdentifierAndPortAndNameAndArgType(len, message, server_identifier, port, name, argTypes);

//                            printf("Server_ideitifier: %s", server_identifier);
//                            cout << " port: " << port << endl;
                            
                            // new FunctionData from REGISTER
                            FunctionData *fdata = new FunctionData(name, argTypes);

                            // check if same server/port exists in database 
                            sameServerIndex = matchingServerPort(server_identifier, port, &database);
                            if (sameServerIndex == -1) {
//                                cout << "ServerData added" << endl;
                                database.push_back(new ServerData(server_identifier, port));
                                (database.back())->addFunctionToList(fdata);
                                sendRegSuccessAfterFormatting(fd, REG_SUCCESS_NEW_SERVER);
                            } else {
                                // check if FunctionData is in function list
                                if (database[sameServerIndex]->functionInList(fdata)) {
//                                    cout << "Same FunctionData exists in database" << endl;
                                    delete fdata;
                                    sendRegSuccessAfterFormatting(fd, REG_SAME_FUNCTION_EXIST);
                                } else {
//                                    cout << "non-existing FunctionData added to server: ";
//                                    printf("%s port=%d\n", database[sameServerIndex]->hostname, database[sameServerIndex]->port);
                                    database[sameServerIndex]->addFunctionToList(fdata);
                                    sendRegSuccessAfterFormatting(fd, REG_SUCCESS_EXISTING_SERVER);
                                }
                            }

                    } else if (type == LOC_REQUEST) {
//                        cout << "LOC_REQUEST" << endl;
                        if (database.size() == 0) { // client called before any server
                            sendLocFailureAfterFormatting(fd, FUNCTION_LOCATION_DOES_NOT_EXIST_IN_THIS_BINDER);
                            continue;
                        }

                        argTypeslen = (len - 8 - 64) / 4;
                        int argTypes[argTypeslen];

                        receiveNameAndArgType(len, message, name, argTypes);

                        int originalIndex = databaseGlobalIndex;
                        bool searching = true;

                        FunctionData * searchingFunction = new FunctionData(name, argTypes);
                        while(searching) {
//                            cout << "Databse Index is " << databaseGlobalIndex << endl;

                            if (database[databaseGlobalIndex]->functionInList(searchingFunction)) {
//                                cout << "Databse instance found" << endl;
                                int logSuccessMsgResult = sendLocSuccessAfterFormatting(fd, database[databaseGlobalIndex]->hostname, database[databaseGlobalIndex]->port);
                                if (logSuccessMsgResult < 0) {
//                                    cerr << "Sending Loc Success Msg Failed" << endl;
                                }
                                searching = false;
                            } else {
//                                cout << "Databse instance not found, iterate the pointer" << endl;
                                databaseGlobalIndex++;
                                if (databaseGlobalIndex == database.size()) {
//                                    cout << "Going back to the start of the index" << endl;
                                    databaseGlobalIndex = databaseGlobalIndex % database.size();
                                }

                                if (databaseGlobalIndex == originalIndex) { // fail function doesn't exist
//                                    cout << "one full loop done" << endl;
                                    sendLocFailureAfterFormatting(fd, FUNCTION_LOCATION_DOES_NOT_EXIST_IN_THIS_BINDER);
                                    searching = false;
                                }
                            }
                        }
                        delete searchingFunction;

                    } else if (type == TERMINATE) {
//                      cout << "TERMINATE" << endl;
                        for (int index = 0; index < socket_n; index++) {
                            if (socket_connected[index].second) {
//                                    cout << "terminate sent to index = " << index << endl;
                                sendTerminateAfterFormatting(socket_connected[index].first);
                            }
                        }

                        for (int l = 0; l < database.size(); ++l) {
                            delete database[l];
                        }
                        flag_terminate = true;
                    }
                } // else
            } // if
        } // for
//        cout << endl;
    } // while
} // main
