//
// Created by J on 2017. 3. 8..
//

#include "message_lib.h"
#include "type_lib.h"
#include <iostream>
#include <sys/socket.h>

int sendDataAfterFormatting(int socket, unsigned int messageSize = 0, MessageType msgType, char *message = NULL) {


}

int getMessageSize(const char * name, int * argTypes, void** args) {
    string name_string(name);

    ((sizeof(argTypes)/sizeof(*argTypes)) * 4);

}
int getMessageSize(const char * name, int * argTypes) {
    string name_string(name);

    return 4 + name_string.size() + 1 + ((sizeof(argTypes)/sizeof(*argTypes)) * 4);
}

int getMessage(char * message, const char* name, int* argTypes, void**args) {

}
int getMessage(char * message, const char* name, int* argTypes) {

}

int sendLocRequestAfterFormatting(int socket, char * name, int argTypes[]) {
    int msgSize = getMessageSize(name, argTypes);
    char msg[msgSize];
    getMessage(msg, name, argTypes);

    sendDataAfterFormatting(socket, msgSize, LOC_REQUEST, msg);
}

int sendExecRequestAfterFormatting(int socket, char* name, int* argTypes, void** args) {
    int msgSize = getMessageSize(name, argTypes, args);
    char msg[msgSize];
    getMessage(msg, name, argTypes, args);

    sendDataAfterFormatting(socket, msgSize, EXECUTE, msg);
}

int sendTerminateAfterFormatting(int socket) {
    sendDataAfterFormatting(socket, TERMINATE);
}
