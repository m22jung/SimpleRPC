//
// Created by J on 2017. 3. 8..
//

#include "message_lib.h"
#include "type_lib.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> // FD_SET, FD_ISSET, FD_ZERO macros
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "string.h"

argT::argT(bool input, bool output, int type, bool array)
: input(input), output(output), type(type), array(array) {}

SkeletonData::SkeletonData(char *n, int *argTypes, skeleton f) : f(f) {
    memcpy(name, n, 64);
    name[64] = '\0';
    printf("SkeletonData::name = %s\n", name);

    generateArgTvector(argTypes, argTv);
    num_argTv = argTv.size();
}

SkeletonData::~SkeletonData() {
    for (int i = 0; i < num_argTv; ++i) {
        delete argTv[i];
    }
}

FunctionData::FunctionData(char *n, int *argTypes) {
    memcpy(name, n, 64);
    name[64] = '\0';
    printf("FunctionData::name = %s\n", name);

    generateArgTvector(argTypes, argTv);
    num_argTv = argTv.size();
}

FunctionData::~FunctionData() {
    for (int i = 0; i < num_argTv; ++i) {
        delete argTv[i];
    }
}

ServerData::ServerData(char* hn, int port) : port(port) {
    memcpy(hostname, hn, 1024);
}

ServerData::~ServerData() {
    for (int i = 0; i < num_fns; ++i) {
        delete fns[i];
    }
}

bool ServerData::functionInList(FunctionData* fdata) {
    for (int i = 0; i < num_fns; ++i) {
        if (matchingArgT(fdata->name, &(fdata->argTv), &fns) != -1) {
            return true;
        }
    }
    return false;
}

void ServerData::addFunctionToList(FunctionData* fdata) {
    fns.push_back(fdata);
    num_fns++;
}

void generateArgTvector(int *argTypes, vector< argT* > &v) {
    for (int i = 0; ; ++i) {
        if (argTypes[i] == 0) break; // end of argTypes

        bool input, output, array;
        int type;

        int io = (argTypes[i] >> ARG_OUTPUT) & 0x00000003;
        //cout << "int io=" << io;
        switch (io) {
            case 3:
                input = true;
                output = true;
                break;
            case 2:
                input = true;
                break;
            case 1:
                output = true;
                break;
            case 0: // false for both
                break;
        }

        type = (argTypes[i] >> 16) & 0x00000006;
        //cout << " type=" << type;

        int arraysize = argTypes[i] & 0x0000FFFF;
        //cout << " arraysize=" << arraysize << endl;
        if (arraysize != 0) array = true;

        v.push_back(new argT(input, output, type, array));
    }
}

bool sameName(char* n1, char* n2) {
    bool flag_samename = true;

    for (int j = 0; j < 64; ++j) {
        if (n1[j] == '\0' && n2[j] == '\0') break; // has same name
        if (n1[j] != n2[j]) {
            flag_samename = false;
            break;
        }
    }
    return flag_samename;
}

bool sameServerName(char* n1, char* n2) {
    bool flag_samename = true;

    for (int j = 0; j < 1024; ++j) {
        if (n1[j] == '\0' && n2[j] == '\0') break; // has same name
        if (n1[j] != n2[j]) {
            flag_samename = false;
            break;
        }
    }
    return flag_samename;
}

// returns index of database that matches name and argTypes otherwise, return -1
int matchingArgT(char* name, int *argTypes, std::vector<SkeletonData*> *database) {
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

int matchingArgT(char* name, std::vector<argT*> *argv, std::vector<FunctionData*> *database) {
    std::cout << "Received FunctionData:" << std::endl;
    printf("name = %s\n", name);
    
    int vsize = argv->size();
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
            if ((*argv)[j]->type != ((*database)[i]->argTv)[j]->type ||
                (*argv)[j]->array != ((*database)[i]->argTv)[j]->array) { // has different arg type
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

int getMessageSize(const char * name, int * argTypes, void** args) {
    int nameLength = 64;
    int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    int argLength = argTypesLength - 1;

    return nameLength + (argTypesLength * 4) + (argLength * 4);

}
int getMessageSize(const char * name, int * argTypes) {
    int nameLength = 64;
    int argTypesLength = 0;
    while (argTypes[argTypesLength++]);

    return nameLength + (argTypesLength * 4);
}

int getMessageSize(char * server_identifer, int port, const char* name, int* argTypes) {
    int server_identifier_Length = 1024;
    int portLength = sizeof(port);
    int nameLength = 64;

    int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
//    int argTypesLength = sizeof(argTypes);

    return server_identifier_Length + portLength + nameLength + (argTypesLength * 4);
}

int getMessageSize(char * server_identifer, int port) {
    int server_identifier_Length = 1024;
    int portLength = sizeof(port);

    return server_identifier_Length + portLength;
}

int getMessageSize(int reasonCode) {
    return sizeof(reasonCode); // Just an int
}

void putMsglengthAndMsgType(int messageLength, MessageType msgType, char * message) {
    message[0] = (messageLength >> 24) & 0xFF;
    message[1] = (messageLength >> 16) & 0xFF;
    message[2] = (messageLength >> 8) & 0xFF;
    message[3] = messageLength & 0xFF;

    int msgType_int = static_cast<int>(msgType);
    message[4] = (msgType_int >> 24) & 0xFF;
    message[5] = (msgType_int >> 16) & 0xFF;
    message[6] = (msgType_int >> 8) & 0xFF;
    message[7] = msgType_int & 0xFF;
}

void getMessage(int messageLength, MessageType msgType, char * message, const char* name, int* argTypes, void**args) {
    putMsglengthAndMsgType(messageLength, msgType, message);

    memcpy(message + 8, name, 64);

    int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    int argTypesSize = argTypesLength * 4;
    memcpy(message + 72, argTypes, argTypesSize);

    memcpy(message + argTypesSize, args, argTypesSize - 4);
}
void getMessage(unsigned int messageLength, MessageType msgType, char * message, const char* name, int* argTypes) {
    putMsglengthAndMsgType(messageLength, msgType, message);

    memcpy(message + 8, name, 64);

    memcpy(message + 72, argTypes, sizeof(argTypes));
}
void getMessage(int messageLength, MessageType msgType, char * message, char * server_identifier, int port, const char* name, int* argTypes) {
    putMsglengthAndMsgType(messageLength, msgType, message);

    memcpy(message + 8, server_identifier, 1024);

    message[1032] = (port >> 24) & 0xFF;
    message[1033] = (port >> 16) & 0xFF;
    message[1034] = (port >> 8) & 0xFF;
    message[1035] = port & 0xFF;

    memcpy(message + 1036, name, 64);


    int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    int argTypesSize = argTypesLength * 4;
    memcpy(message + 1100, argTypes, argTypesSize);
}

void getMessage(int messageLength, MessageType msgType, char * message) {
    putMsglengthAndMsgType(messageLength, msgType, message);
}

void getMessage(int messageLength, MessageType msgType, char * message, char * server_identifer, int port) {
    putMsglengthAndMsgType(messageLength, msgType, message);

    memcpy(message, server_identifer, 1024);

    message[1032] = (port >> 24) & 0xFF;
    message[1033] = (port >> 16) & 0xFF;
    message[1034] = (port >> 8) & 0xFF;
    message[1035] = port & 0xFF;
}

void getMessage(int messageLength, MessageType msgType, char * message, int reasonCode) {
    putMsglengthAndMsgType(messageLength, msgType, message);

    message[8] = (reasonCode >> 24) & 0xFF;
    message[9] = (reasonCode >> 16) & 0xFF;
    message[10] = (reasonCode >> 8) & 0xFF;
    message[11] = reasonCode & 0xFF;
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

int sendRegSuccessAfterFormatting(int socket, int reasonCode) {
    int msgSize = getMessageSize(reasonCode);
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, REGISTER_SUCCESS, msg, reasonCode);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}

int sendRegFailureAfterFormatting(int socket, int reasonCode) {
    int msgSize = getMessageSize(reasonCode);
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, REGISTER_FAILURE, msg, reasonCode);

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

int sendLocSuccessAfterFormatting(int socket, char *server_identifier, int port) {
    int msgSize = getMessageSize(server_identifier, port);
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, LOC_SUCCESS, msg, server_identifier, port);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}

int sendLocFailureAfterFormatting(int socket, int reasonCode) {
    int msgSize = getMessageSize(reasonCode);
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, LOC_FAILURE, msg, reasonCode);

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

int sendExecSuccessAfterFormatting(int socket, char *name, int *argTypes, void **args) {
    int msgSize = getMessageSize(name, argTypes, args);
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, EXECUTE_SUCCESS, msg, name, argTypes, args);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}

int sendExecFailureAfterFormatting(int socket, int reasonCode) {
    int msgSize = getMessageSize(reasonCode);
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, EXECUTE_FAILURE, msg, reasonCode);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}

int sendTerminateAfterFormatting(int socket) {
    int msgSize = 4;
    msgSize += 8;
    char msg[msgSize];
    getMessage(msgSize, TERMINATE, msg);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}

int receiveLengthAndType(int socket, int &length, int &msgType) {
    char buffer[8];
    int valread = read(socket, buffer, 8);

    if (valread == 0) { // disconnected
        return SOCKET_CONNECTION_FINISHED;
    } else if (valread < 0) {
        cerr << "ERROR reading from socket" << endl;
        return READING_SOCKET_ERROR;

    } else { // read
        length = (int)((unsigned char)(buffer[0]) << 24 |
                        (unsigned char)(buffer[1]) << 16 |
                        (unsigned char)(buffer[2]) << 8 |
                        (unsigned char)(buffer[3]) );

        msgType = (int)((unsigned char)(buffer[4]) << 24 |
                         (unsigned char)(buffer[5]) << 16 |
                         (unsigned char)(buffer[6]) << 8 |
                         (unsigned char)(buffer[7]) );
    }
    return 0;
}

void receiveServerIdentifierAndPortAndNameAndArgType(int msgLength, char *message, char *server_identifier, int &port,
                                                    char *name, int *argTypes) {

    // extract server_identifier
    memcpy(server_identifier, message + 8, 1024);

    // extract port
    port = (int)((unsigned char)(message[1032]) << 24 |
                 (unsigned char)(message[1033]) << 16 |
                 (unsigned char)(message[1034]) << 8 |
                 (unsigned char)(message[1035]) );

    // extract name
    memcpy(name, message + 1036, 64);

    int argTypesLength = msgLength - 8 - 1024 - 4 - 64;
    // extract argTypes
    memcpy(argTypes, message + 1100, argTypesLength);

}

void receiveNameAndArgType(int msgLength, char *message, char *name, int *argTypes) {

    // extract name
    memcpy(name, message + 8, 64);

    // extract server_identifier
    int argTypesLength = msgLength - 8 - 64;
    memcpy(argTypes, message + 72, argTypesLength);

}

void receiveServerIdentifierAndPort(int msgLength, char *message, char *server_identifier, int &port) {

    //extract server_identifier
    memcpy(server_identifier, message + 8, 1024);

    //extract port
    port = (int)((unsigned char)(message[1032]) << 24 |
                 (unsigned char)(message[1033]) << 16 |
                 (unsigned char)(message[1034]) << 8 |
                 (unsigned char)(message[1035]) );

}

void receiveReasonCode(int msgLength, char *message, int &reasonCode) {
    //extract reasonCode
    reasonCode = (int)((unsigned char)(message[8]) << 24 |
                       (unsigned char)(message[9]) << 16 |
                       (unsigned char)(message[10]) << 8 |
                       (unsigned char)(message[11]) );

}
