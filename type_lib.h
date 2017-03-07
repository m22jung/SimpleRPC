//
// Created by J on 2017. 3. 7..
//

#ifndef A03_TYPE_LIB_H
#define A03_TYPE_LIB_H

#include <string>

using namespace std;

enum MessageType {
    ERROR = 0,
    REGISTER = 1,
    REGISTER_SUCCESS = 2,
    REGISTER_FAILURE = 3,
    LOC_REQUEST = 4,
    LOC_SUCCESS = 5,
    LOC_FAILURE = 6,
    EXECUTE = 7,
    EXECUTE_SUCCESS = 8,
    EXECUTE_FAILURE = 9,
    TERMINATE = 10
};

enum ErrorCode {
    SUCCESS = 0,
    FAILURE = -1,

};

struct server {
    string hostName;
    int portNum;

    server(string hostName_in, int portNum_in) {
        hostName = hostName_in;
        portNum = portNum_in;
    }
};

struct client {
    string hostName;
    int portNum;

    client(string hostName_in, int portNum_in) {
        hostName = hostName_in;
        portNum = portNum_in;
    }
};

#endif //A03_TYPE_LIB_H
