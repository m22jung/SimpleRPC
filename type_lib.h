//
// Created by J on 2017. 3. 7..
//

#ifndef A03_TYPE_LIB_H
#define A03_TYPE_LIB_H

#include <string>

using namespace std;

enum ErrorCode {
    SUCCESS = 0,
    FAILURE = -1,
    BINDER_ADDR_NOT_FOUND = -2,
    BINDER_PORT_NOT_FOUND = -3,
    BINDER_NOT_SETUP = -4,
    SERVER_SOCKET_NOT_SETUP = -5,
    SOCKET_NOT_SETUP = -6,
    SOCKET_CONNECTION_FAILED = -7,
    DATA_SEND_FAILED = -8

};

struct server_identifier {
    string hostName;
    int portNum;

    server_identifier(string hostName_in, int portNum_in) {
        hostName = hostName_in;
        portNum = portNum_in;
    }
};

struct client_identifier {
    string hostName;
    int portNum;

    client_identifier(string hostName_in, int portNum_in) {
        hostName = hostName_in;
        portNum = portNum_in;
    }
};

#endif //A03_TYPE_LIB_H
