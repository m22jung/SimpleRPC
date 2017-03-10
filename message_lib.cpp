//
// Created by J on 2017. 3. 8..
//

#include "message_lib.h"
#include "type_lib.h"
#include <iostream>
#include <sys/socket.h>

int getMessageSize(const char * name, int * argTypes, void** args) {
    int nameLength = 64;
    int argTypesLength = sizeof(argTypes)/sizeof(*argTypes);
    int argLength = argTypesLength - 1;

    return nameLength + (argTypesLength * 4) + (argLength * 4);

}
int getMessageSize(const char * name, int * argTypes) {
    int nameLength = 64;
    int argTypesLength = sizeof(argTypes)/sizeof(*argTypes);

    return nameLength + (argTypesLength * 4);
}

int getMessageSize(char * server_identifer, int port, const char* name, int* argTypes) {
    int server_identifier_Length = 1024;
    int portLength = 4;
    int nameLength = 64;
    int argTypesLength = sizeof(argTypes);

    return server_identifier_Length + portLength + nameLength + argTypesLength;
}

void getMessage(unsigned int messageLength, MessageType msgType, char * message, const char* name, int* argTypes, void**args) {
    message[0] = (messageLength >> 24) & 0xFF;
    message[1] = (messageLength >> 16) & 0xFF;
    message[2] = (messageLength >> 8) & 0xFF;
    message[3] = messageLength & 0xFF;

    int msgType_int = static_cast<int>(msgType);
    message[4] = (msgType_int >> 24) & 0xFF;
    message[5] = (msgType_int >> 16) & 0xFF;
    message[6] = (msgType_int >> 8) & 0xFF;
    message[7] = msgType_int & 0xFF;

    memcpy(message + 8, name, 64);

    int argTypesSize = sizeof(argTypes);
    memcpy(message + 72, argTypes, argTypesSize);

    memcpy(message + argTypesSize, args, argTypesSize - 1);
}
void getMessage(unsigned int messageLength, MessageType msgType, char * message, const char* name, int* argTypes) {
    message[0] = (messageLength >> 24) & 0xFF;
    message[1] = (messageLength >> 16) & 0xFF;
    message[2] = (messageLength >> 8) & 0xFF;
    message[3] = messageLength & 0xFF;

    int msgType_int = static_cast<int>(msgType);
    message[4] = (msgType_int >> 24) & 0xFF;
    message[5] = (msgType_int >> 16) & 0xFF;
    message[6] = (msgType_int >> 8) & 0xFF;
    message[7] = msgType_int & 0xFF;

    memcpy(message + 8, name, 64);

    memcpy(message + 72, argTypes, sizeof(argTypes));
}
void getMessage(unsigned int messageLength, MessageType msgType, char * message, char * server_identifier, int port, const char* name, int* argTypes) {
    message[0] = (messageLength >> 24) & 0xFF;
    message[1] = (messageLength >> 16) & 0xFF;
    message[2] = (messageLength >> 8) & 0xFF;
    message[3] = messageLength & 0xFF;

    int msgType_int = static_cast<int>(msgType);
    message[4] = (msgType_int >> 24) & 0xFF;
    message[5] = (msgType_int >> 16) & 0xFF;
    message[6] = (msgType_int >> 8) & 0xFF;
    message[7] = msgType_int & 0xFF;

    memcpy(message + 8, server_identifier, 1024);

    message[1032] = (port >> 24) & 0xFF;
    message[1033] = (port >> 16) & 0xFF;
    message[1034] = (port >> 8) & 0xFF;
    message[1035] = port & 0xFF;

    memcpy(message + 1036, name, 64);

    memcpy(message + 1100, argTypes, sizeof(argTypes));
}

void getMessage(MessageType msgType, char * message) {
    int msgType_int = static_cast<int>(msgType);

    message[0] = (msgType_int >> 24) & 0xFF;
    message[1] = (msgType_int >> 16) & 0xFF;
    message[2] = (msgType_int >> 8) & 0xFF;
    message[3] = msgType_int & 0xFF;
}

int sendRegRequestAfterFormatting(int socket, char * server_identifier, int port, char * name, int * argTypes) {
    int msgSize = getMessageSize(server_identifier, port, name, argTypes);
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, REGISTER, msg, server_identifier, port, name, argTypes);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}

int sendLocRequestAfterFormatting(int socket, char * name, int argTypes[]) {
    int msgSize = getMessageSize(name, argTypes);
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, LOC_REQUEST, msg, name, argTypes);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}

int sendExecRequestAfterFormatting(int socket, char* name, int* argTypes, void** args) {
    int msgSize = getMessageSize(name, argTypes, args);
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, EXECUTE, msg, name, argTypes, args);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}

int sendTerminateAfterFormatting(int socket) {
    int msgSize = 4;
    char msg[msgSize];
    getMessage(TERMINATE, msg);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}
