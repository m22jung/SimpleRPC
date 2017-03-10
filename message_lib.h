//
// Created by J on 2017. 3. 7..
//

#ifndef A03_MESSAGE_LIB_H
#define A03_MESSAGE_LIB_H

#include <iostream>

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

int getMessageSize(const char* name, int* argTypes, void**args); // execute, exe success
int getMessageSize(const char* name, int* argTypes); // loc request
int getMessageSize(char * server_identifer, int port, const char* name, int* argTypes); // register
//int getMessageSize(char * serverId, int port); // loc success
//int getMessageSize(int reasonCode); // loc fail, exe fail, reg success&fail

void getMessage(char * message, const char* name, int* argTypes, void**args);
void getMessage(char * message, const char* name, int* argTypes);
void getMessage(char * message, char * server_identifier, int port, const char* name, int* argTypes);

int sendRegRequestAfterFormatting(int socket, char * server_identifier, int port, char * name, int * argTypes);
int sendLocRequestAfterFormatting(int socket, char * name, int* argTypes);
int sendExecRequestAfterFormatting(int serverSocket, char* name, int* argTypes, void** args);
int sendTerminateAfterFormatting(int socket);

int sendDataAfterFormatting(int socket, unsigned int messageSize = 0, MessageType msgType, char *message = NULL);

int receiveMessage(int socket);


#endif //A03_MESSAGE_LIB_H
