//
// Created by J on 2017. 3. 7..
//

#ifndef A03_MESSAGE_LIB_H
#define A03_MESSAGE_LIB_H

#include "rpc.h"
#include <iostream>
#include <vector>

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

struct argT {
    bool input;
    bool output;
    int type;
    bool array;

    argT(bool input, bool output, int type, bool array);
};

struct SkeletonData {
    char name[64];
    skeleton f;
    std::vector< argT* > argTv;
    int num_argTv;

    SkeletonData(char *n, int *argTypes, skeleton f);
    ~SkeletonData();
};

void generateArgTvector(int *argTypes, std::vector< argT* > &v);

bool sameName(char* n1, char* n2);

// returns index of database that matches name and argTypes otherwise, return -1
template<typename T> int matchingArgT(char* name, int *argTypes, T *database) {
    std::vector< argT* > v;
    std::cout << "Received name and argTypes:" << std::endl;
    printf("name = %s\n", name);
    generateArgTvector(argTypes, v);
    int vsize = v.size();

    // makes an entry in a local database, associating the server skeleton with
    // name and list of argument types.
    int localDatabaseSize = database->size();
    int sameDataIndex = -1;

    for (int i = 0; i < localDatabaseSize; ++i) {
        // check if function name is same
        if (!sameName(name, (*database)[i]->name)) continue; // move to next data

        // check if number of arguments is same
        if (vsize != (*database)[i]->num_argTv) continue; // move to next data

        bool flag_sameArg = true;

        // check if argument types are same
        for (int j = 0; j < vsize; ++j) {
            if (v[j]->type != ((*database)[i]->argTv)[j]->type ||
                v[j]->array != ((*database)[i]->argTv)[j]->array) { // has different arg type
                flag_sameArg = false;
                break;
            }
        }
        if (!flag_sameArg) continue; // move to next data

        sameDataIndex = i;
        break;
    }

    return sameDataIndex;
}

//int getMessageSize(const char* name, int* argTypes, void**args); // execute, exe success
//int getMessageSize(const char* name, int* argTypes); // loc request
//int getMessageSize(char * server_identifer, int port, const char* name, int* argTypes); // register
//int getMessageSize(char * server_identifer, int port); // loc success
//int getMessageSize(int reasonCode); // loc fail, exe fail, reg success&fail
//
//void getMessage(unsigned int messageLength, MessageType msgType, char * message, const char* name, int* argTypes, void**args);
//void getMessage(unsigned int messageLength, MessageType msgType, char * message, const char* name, int* argTypes);
//void getMessage(unsigned int messageLength, MessageType msgType, char * message, char * server_identifier, int port, const char* name, int* argTypes);
//void getMessage(unsigned int messageLength, MessageType msgType, char * message, char * server_identifer, int port);
//void getMessage(unsigned int messageLength, MessageType msgType, int reasonCode);

int sendRegRequestAfterFormatting(int socket, char * server_identifier, int port, char * name, int * argTypes);
int sendRegSuccessAfterFormatting(int socket, int reasonCode);
int sendRegFailureAfterFormatting(int socket, int reasonCode);
int sendLocRequestAfterFormatting(int socket, char * name, int* argTypes);
int sendLocSuccessAfterFormatting(int socket, char * server_identifier, int port);
int sendLocFailureAfterFormatting(int socket, int reasonCode);
int sendExecRequestAfterFormatting(int socket, char* name, int* argTypes, void** args);
int sendExecSuccessAfterFormatting(int socket, char * name, int * argTypes, void** args);
int sendExecFailureAfterFormatting(int socket, int reasonCode);
int sendTerminateAfterFormatting(int socket);

int receiveLengthAndType(int socket, int &length, int &msgType);
void receiveServerIdentifierAndPortAndNameAndArgType(int msgLength, char * message, char * server_identifier, int &port, char * name, int * argTypes);
void receiveNameAndArgType(int msgLength, char * message, char * name, int * argTypes);
void receiveServerIdentifierAndPort(int msgLength, char * message, char * server_identifier, int &port);
void receiveReasonCode(int msgLength, char * message, int &reasonCode);
void receiveNameAndArgTypeAndArgs(int msgLength, char * message, char * name, int * argTypes, void** args);



#endif //A03_MESSAGE_LIB_H
