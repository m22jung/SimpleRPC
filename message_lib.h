//
// Created by J on 2017. 3. 7..
//

#ifndef A03_MESSAGE_LIB_H
#define A03_MESSAGE_LIB_H

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

int sendMessage();


#endif //A03_MESSAGE_LIB_H
