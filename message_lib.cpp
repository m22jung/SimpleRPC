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
#include <string.h>

void generateArgTvector(int *argTypes, vector< argT* > &v) {
    for (int i = 0; ; ++i) {
        if (argTypes[i] == 0) break; // end of argTypes

        bool input, output;
        int type, arraysize;

        int io = (argTypes[i] >> ARG_OUTPUT) & 0x00000003;
        cout << "int io=" << io;
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

        type = (argTypes[i] >> 16) & 0x00000007;
        cout << " type=" << type;

        arraysize = argTypes[i] & 0x0000FFFF;
        cout << " arraysize=" << arraysize << endl;

        v.push_back(new argT(input, output, type, arraysize));
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

    //std::cout << "Received name and argTypes:" << std::endl;
    //printf("name = %s\n", name);

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
            if (v[j]->type != ((*database)[i]->argTv)[j]->type) { // has different type
                flag_sameArg = false;
                break;
            }
            if (!((v[j]->arraysize == 0 && ((*database)[i]->argTv)[j]->arraysize == 0) ||
                (v[j]->arraysize != 0 && ((*database)[i]->argTv)[j]->arraysize != 0))) { // same type but different array size
                flag_sameArg = false;
                break;
            }
        }
        if (!flag_sameArg) continue; // move to next data

        sameDataIndex = i;
        cout << "Function name=";
        printf("%s, ", name);
        cout << "matched at index=" << i << endl;
        break;
    }

    return sameDataIndex;
}

int matchingArgT(char* name, std::vector<argT*> *argv, std::vector<FunctionData*> *database) {
    //std::cout << "Received FunctionData:" << std::endl;
    //printf("name = %s\n", name);
    
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
            if ((*argv)[j]->type != ((*database)[i]->argTv)[j]->type) { // has different type
                flag_sameArg = false;
                break;
            }
            if (!(((*argv)[j]->arraysize == 0 && ((*database)[i]->argTv)[j]->arraysize == 0) ||
                ((*argv)[j]->arraysize != 0 && ((*database)[i]->argTv)[j]->arraysize != 0))) { // same type but different array size
                flag_sameArg = false;
                break;
            }
        }
        if (!flag_sameArg) continue; // move to next data

        sameDataIndex = i;
        cout << "Function name=";
        printf("%s, ", name);
        cout << "matched at FunctionData index=" << i << endl;
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
    int portLength = 4;
    int nameLength = 64;

    int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
//    int argTypesLength = sizeof(argTypes);

    return server_identifier_Length + portLength + nameLength + (argTypesLength * 4);
}

int getMessageSize(char * server_identifer, int port) {
    int server_identifier_Length = 1024;
    int portLength = 4;

    return server_identifier_Length + portLength;
}

int getMessageSize(int reasonCode) {
    return sizeof(reasonCode); // Just an int
}

void put4byteToCharArray(char *dest, int value) {
    char *ptr = (char*)(&value);

    dest[0] = *ptr;
    dest[1] = *(++ptr);
    dest[2] = *(++ptr);
    dest[3] = *(++ptr);
}

void get4byteFromCharArray(int *dest, char *from) {
    char *ptr = (char*)(dest);

    ptr[0] = *from;
    ptr[1] = *(++from);
    ptr[2] = *(++from);
    ptr[3] = *(++from);
}

void putMsglengthAndMsgType(int messageLength, MessageType msgType, char * message) {
    put4byteToCharArray(message, messageLength);

    int msgType_int = static_cast<int>(msgType);
    put4byteToCharArray(message+4, msgType_int);
}

void getMessage(int messageLength, MessageType msgType, char * message, const char* name, int* argTypes, void**args) {
    putMsglengthAndMsgType(messageLength, msgType, message);

    memcpy(message + 8, name, 64);

    int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    int argTypesSize = argTypesLength * 4;
    memcpy(message + 72, argTypes, argTypesSize);

    memcpy(message + 72 + argTypesSize, args, argTypesSize - 4);
}
void getMessage(unsigned int messageLength, MessageType msgType, char * message, const char* name, int* argTypes) {
    putMsglengthAndMsgType(messageLength, msgType, message);

    memcpy(message + 8, name, 64);

    int argTypesLength = 0;
    while (argTypes[argTypesLength++]);
    int argTypesSize = argTypesLength * 4;
    memcpy(message + 72, argTypes, argTypesSize);
}
void getMessage(int messageLength, MessageType msgType, char * message, char * server_identifier, int port, const char* name, int* argTypes) {
    putMsglengthAndMsgType(messageLength, msgType, message);

    memcpy(message + 8, server_identifier, 1024);

    put4byteToCharArray(message+1032, port);

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

    memcpy(message + 8, server_identifer, 1024);

    put4byteToCharArray(message+1032, port);
}

void getMessage(int messageLength, MessageType msgType, char * message, int reasonCode) {
    putMsglengthAndMsgType(messageLength, msgType, message);

    put4byteToCharArray(message+8, reasonCode);
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
    std::vector< argT* > argTypeVector;

    generateArgTvector(argTypes, argTypeVector);

    int argTypesLength = argTypeVector.size() + 1;
    int argTypesSize = argTypesLength * 4;

    // Calculate msgSize
    int msgSize = 0;
    msgSize += 8; // Length and MsgType
    msgSize += argTypesSize; // ArgTypes
    msgSize += 64; // Name

    for (int i = 0; i < argTypeVector.size(); i++) {
        if (argTypeVector[i]->arraysize == 0) {
            switch (argTypeVector[i]->type) {
                case ARG_CHAR:
                    msgSize += 1; break;
                case ARG_SHORT:
                    msgSize += 2; break;
                case ARG_INT:
                    msgSize += 4; break;
                case ARG_LONG:
                    msgSize += 4; break;
                case ARG_DOUBLE:
                    msgSize += 8; break;
                case ARG_FLOAT:
                    msgSize += 4; break;
            }
        } else {
            switch (argTypeVector[i]->type) {
                case ARG_CHAR:
                    msgSize += (argTypeVector[i]->arraysize * 1); break;
                case ARG_SHORT:
                    msgSize += (argTypeVector[i]->arraysize * 2); break;
                case ARG_INT:
                    msgSize += (argTypeVector[i]->arraysize * 4); break;
                case ARG_LONG:
                    msgSize += (argTypeVector[i]->arraysize * 4); break;
                case ARG_DOUBLE:
                    msgSize += (argTypeVector[i]->arraysize * 8); break;
                case ARG_FLOAT:
                    msgSize += (argTypeVector[i]->arraysize * 4); break;
            }
        }
    }
    cout << "msg Size from client before sending is " << msgSize << endl;

    char msg[msgSize];
    putMsglengthAndMsgType(msgSize, EXECUTE, msg);

    memcpy(msg + 8, name, 64);

    cout << "argTypesSize=" << argTypesSize << endl;
    memcpy(msg + 72, argTypes, argTypesSize);

    int argTypeslen = 72;
    for (;;) {
        int temp;
        get4byteFromCharArray(&temp, msg + argTypeslen);
        cout << "pointer=" << argTypeslen << " temp=" << temp << endl;
        argTypeslen += 4;
        if (temp == 0) break;
    }

    char * msgPointer = msg + 8 + 64 + argTypesSize;

    marshallData(msgPointer, argTypes, args, argTypesLength, true);

    int sentData = send(socket, msg, msgSize, 0);

    if (sentData == -1) {
        return DATA_SEND_FAILED;
    }

    return sentData;
}

int sendExecSuccessAfterFormatting(int socket, char *name, int *argTypes, void **args) {
    std::vector< argT* > argTypeVector;

    generateArgTvector(argTypes, argTypeVector);

    int argTypesLength = argTypeVector.size() + 1;
    int argTypesSize = argTypesLength * 4;

    // Calculate msgSize
    int msgSize = 0;
    msgSize += 8; // Length and MsgType
    msgSize += argTypesSize; // ArgTypes
    msgSize += 64; // Name

    for (int i = 0; i < argTypeVector.size(); i++) {
        if (argTypeVector[i]->arraysize == 0) {
            switch (argTypeVector[i]->type) {
                case ARG_CHAR:
                    msgSize += 1; break;
                case ARG_SHORT:
                    msgSize += 2; break;
                case ARG_INT:
                    msgSize += 4; break;
                case ARG_LONG:
                    msgSize += 4; break;
                case ARG_DOUBLE:
                    msgSize += 8; break;
                case ARG_FLOAT:
                    msgSize += 4; break;
            }
        } else {
            switch (argTypeVector[i]->type) {
                case ARG_CHAR:
                    msgSize += (argTypeVector[i]->arraysize * 1); break;
                case ARG_SHORT:
                    msgSize += (argTypeVector[i]->arraysize * 2); break;
                case ARG_INT:
                    msgSize += (argTypeVector[i]->arraysize * 4); break;
                case ARG_LONG:
                    msgSize += (argTypeVector[i]->arraysize * 4); break;
                case ARG_DOUBLE:
                    msgSize += (argTypeVector[i]->arraysize * 8); break;
                case ARG_FLOAT:
                    msgSize += (argTypeVector[i]->arraysize * 4); break;
            }
        }
    }
    cout << "msg Size from client before sending is " << msgSize << endl;

    char msg[msgSize];
    putMsglengthAndMsgType(msgSize, EXECUTE_SUCCESS, msg);

    memcpy(msg + 8, name, 64);

    cout << "argTypesSize=" << argTypesSize << endl;
    memcpy(msg + 72, argTypes, argTypesSize);

    int argTypeslen = 72;
    for (;;) {
        int temp;
        get4byteFromCharArray(&temp, msg + argTypeslen);
        cout << "pointer=" << argTypeslen << " temp=" << temp << endl;
        argTypeslen += 4;
        if (temp == 0) break;
    }

    char * msgPointer = msg + 8 + 64 + argTypesSize;

    marshallData(msgPointer, argTypes, args, argTypesLength, false);

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
        get4byteFromCharArray(&length, buffer);
        get4byteFromCharArray(&msgType, buffer+4);
    }
    return 0;
}

int receiveRegisterResult(int socket, int &msgType, int &reasonCode) {
    char buffer[12];
    int valread = read(socket, buffer, 12);

    if (valread == 0) {
        return SOCKET_CONNECTION_FINISHED;
    } else if (valread < 0) {
        cerr << "ERROR reading from socket" << endl;
        return READING_SOCKET_ERROR;

    } else { // read
        get4byteFromCharArray(&msgType, buffer+4);
        get4byteFromCharArray(&reasonCode, buffer+8);
    }
    return 0;
}

void receiveServerIdentifierAndPortAndNameAndArgType(int msgLength, char *message, char *server_identifier, int &port,
                                                    char *name, int *argTypes) {

    // extract server_identifier
    memcpy(server_identifier, message + 8, 1024);

    // extract port
    get4byteFromCharArray(&port, message+1032);

    // extract name
    memcpy(name, message + 1036, 64);

    int argTypesLength = msgLength - 8 - 1024 - 4 - 64;
    // extract argTypes
    memcpy(argTypes, message + 1100, argTypesLength);

}

void receiveNameAndArgType(int msgLength, char *message, char *name, int *argTypes) {

    // extract name
    memcpy(name, message + 8, 64);

    // extract argType
    int argTypesLength = msgLength - 8 - 64;
    memcpy(argTypes, message + 72, argTypesLength);

}

void receiveServerIdentifierAndPort(int msgLength, char *message, char *server_identifier, int &port) {

    //extract server_identifier
    memcpy(server_identifier, message + 8, 1024);

    //extract port
    get4byteFromCharArray(&port, message+1032);

}

void receiveReasonCode(int msgLength, char *message, int &reasonCode) {
    //extract reasonCode
    get4byteFromCharArray(&reasonCode, message+8);

}

void receiveNameAndArgTypeForRPCCall(char *message, char *name, int *argTypes, int argTypesLength) {
    // extract name
    memcpy(name, message + 8, 64);
    cout << "function name=" << name << endl;

    // extract argType
    cout << "argTypesLength=" << argTypesLength << endl;
    memcpy(argTypes, message + 72, argTypesLength);
}

int marshallData(char * msgPointer, int * argTypes, void ** args, int argTypesLength, bool fromClient) {
    cout << "----------marshaling-----------" << endl;
    std::vector< argT* > argTypeVector;

    generateArgTvector(argTypes, argTypeVector);

    for (int i = 0; i < argTypesLength - 1; i++) {
        argT * argType = argTypeVector[i];

        void * singleArgument = args[i];

        char * chars;
        short * shorts;
        int * ints;
        long * longs;
        double * doubles;
        float * floats;

        cout << "arg" << i << " ";
        switch (argType->type) {
            case ARG_CHAR:
                cout << "type=char : ";
                chars = (char *)singleArgument;
                if (argType->arraysize == 0) {
                    *msgPointer = chars[0];
                    msgPointer += 1;
                    cout << *chars << " " << endl;
                }
                else {
                    for(int j = 0; j < argType->arraysize; j++) {
                        if (argType->input && argType->output) {
                            cout << chars[j] << " ";
                            *msgPointer = chars[j];
                        }
                        else if (argType->input) {
                            cout << chars[j] << " ";
                            *msgPointer = chars[j];

                        } else if (argType->output) {
                            if (fromClient) {
                                cout << " ";
                                // do nothing
                            } else {
                                cout << chars[j] << " ";
                                *msgPointer = chars[j];
                            }
                        }
                        msgPointer += 1;

                    } cout << endl;
                }
                break;
            case ARG_SHORT: // 2 byte
                cout << "type=short : ";
                shorts = (short *)singleArgument;
                if (argType->arraysize == 0) {
                    msgPointer[1] = (*shorts >> 8) & 0xFF;
                    msgPointer[0] = *shorts & 0xFF;
                    msgPointer += 2;
                    cout << *shorts << " " << endl;
                } else {
                    for (int j = 0; j < argType->arraysize; j++) {
                        if (argType->input && argType->output) {
                            msgPointer[1] = (shorts[j] >> 8) & 0xFF;
                            msgPointer[0] = shorts[j] & 0xFF;
                            cout << shorts[j] << " ";
                        }
                        else if (argType->input) {
                            msgPointer[1] = (shorts[j] >> 8) & 0xFF;
                            msgPointer[0] = shorts[j] & 0xFF;
                            cout << shorts[j] << " ";
                        } else if (argType->output) {
                            if (fromClient) {
                                cout << " ";
                                // do nothing
                            } else {
                                msgPointer[1] = (shorts[j] >> 8) & 0xFF;
                                msgPointer[0] = shorts[j] & 0xFF;

                                cout << shorts[j] << " ";
                            }
                        }
                        msgPointer += 2;
                    } cout << endl;
                }
                break;
            case ARG_INT: // 4byte
                cout << "type=int : ";
                ints = (int *)singleArgument;
                if (argType->arraysize == 0) {
                    put4byteToCharArray(msgPointer, *ints);
                    msgPointer += 4;
                    cout << *ints << " " << endl;
                } else {
                    for (int j = 0; j < argType->arraysize; j++) {
                        put4byteToCharArray(msgPointer, ints[j]);
                        msgPointer += 4;
                        cout << ints[j] << " ";
                    } cout << endl;
                }
                break;
            case ARG_LONG: // 4 byte
                cout << "type=long : ";
                longs = (long *)singleArgument;
                if (argType->arraysize == 0) {
                    msgPointer[3] = (*longs >> 24) & 0xFF;
                    msgPointer[2] = (*longs >> 16) & 0xFF;
                    msgPointer[1] = (*longs >> 8) & 0xFF;
                    msgPointer[0] = *longs & 0xFF;
                    msgPointer += 4;
                    cout << *longs << " ";
                } else {
                    for (int j = 0; j < argType->arraysize; j++) {

                        if (argType->input && argType->output) {
                            msgPointer[3] = (longs[j] >> 24) & 0xFF;
                            msgPointer[2] = (longs[j] >> 16) & 0xFF;
                            msgPointer[1] = (longs[j] >> 8) & 0xFF;
                            msgPointer[0] = longs[j] & 0xFF;
                            cout << longs[j] << " ";
                        }
                        else if (argType->input) {
                            msgPointer[3] = (longs[j] >> 24) & 0xFF;
                            msgPointer[2] = (longs[j] >> 16) & 0xFF;
                            msgPointer[1] = (longs[j] >> 8) & 0xFF;
                            msgPointer[0] = longs[j] & 0xFF;
                            cout << longs[j] << " ";
                        } else if (argType->output) {
                            if (fromClient) {
                                cout << " ";
                                // do nothing
                            } else {
                                msgPointer[3] = (longs[j] >> 24) & 0xFF;
                                msgPointer[2] = (longs[j] >> 16) & 0xFF;
                                msgPointer[1] = (longs[j] >> 8) & 0xFF;
                                msgPointer[0] = longs[j] & 0xFF;
                                cout << longs[j] << " ";
                            }
                        }
                        msgPointer += 4;
                    } cout << endl;
                }
                break;
            case ARG_DOUBLE: // 8byte
                cout << "type=double : ";
                doubles = (double *)singleArgument;
                if (argType->arraysize == 0) {

                    char *ptr = (char *)(doubles);

                    msgPointer[7] = *ptr;
                    msgPointer[6] = *(++ptr);
                    msgPointer[5] = *(++ptr);
                    msgPointer[4] = *(++ptr);
                    msgPointer[3] = *(++ptr);
                    msgPointer[2] = *(++ptr);
                    msgPointer[1] = *(++ptr);
                    msgPointer[0] = *(++ptr);

                    msgPointer += 8;
                    cout << *doubles << " " << endl;
                } else {
                    for (int j = 0; j < argType->arraysize; j++) {

                        if (argType->input && argType->output) {
                            char *ptr = (char*)(&doubles[j]);

                            msgPointer[7] = *ptr;
                            msgPointer[6] = *(++ptr);
                            msgPointer[5] = *(++ptr);
                            msgPointer[4] = *(++ptr);
                            msgPointer[3] = *(++ptr);
                            msgPointer[2] = *(++ptr);
                            msgPointer[1] = *(++ptr);
                            msgPointer[0] = *(++ptr);

                            cout << doubles[j] << " ";
                        }
                        else if (argType->input) {
                            char *ptr = (char*)(&doubles[j]);

                            msgPointer[7] = *ptr;
                            msgPointer[6] = *(++ptr);
                            msgPointer[5] = *(++ptr);
                            msgPointer[4] = *(++ptr);
                            msgPointer[3] = *(++ptr);
                            msgPointer[2] = *(++ptr);
                            msgPointer[1] = *(++ptr);
                            msgPointer[0] = *(++ptr);

                            cout << doubles[j] << " ";
                        } else if (argType->output) {
                            if (fromClient) {
                                cout << " ";
                                // do nothing
                            } else {
                                char *ptr = (char*)(&doubles[j]);

                                msgPointer[7] = *ptr;
                                msgPointer[6] = *(++ptr);
                                msgPointer[5] = *(++ptr);
                                msgPointer[4] = *(++ptr);
                                msgPointer[3] = *(++ptr);
                                msgPointer[2] = *(++ptr);
                                msgPointer[1] = *(++ptr);
                                msgPointer[0] = *(++ptr);


                                cout << doubles[j] << " ";
                            }
                        }
                        msgPointer += 8;
                    } cout << endl;
                }
                break;
            case ARG_FLOAT: // 4 byte
                cout << "type=float : ";
                floats = (float *)singleArgument;
                if (argType->arraysize == 0) {

                    char *ptr = (char *)(floats);

                    msgPointer[3] = *ptr;
                    msgPointer[2] = *(++ptr);
                    msgPointer[1] = *(++ptr);
                    msgPointer[0] = *(++ptr);

                    msgPointer += 4;
                    cout << *floats << " " << endl;

                } else {
                    for (int j = 0; j < argType->arraysize; j++) {
                        char *ptr = (char*)(&floats[j]);

                        if (argType->input && argType->output) {
                            msgPointer[3] = *ptr;
                            msgPointer[2] = *(++ptr);
                            msgPointer[1] = *(++ptr);
                            msgPointer[0] = *(++ptr);

                            cout << floats[j] << " ";
                        }
                        else if (argType->input) {
                            msgPointer[3] = *ptr;
                            msgPointer[2] = *(++ptr);
                            msgPointer[1] = *(++ptr);
                            msgPointer[0] = *(++ptr);

                            cout << floats[j] << " ";
                        } else if (argType->output) {
                            if (fromClient) {
                                cout << " ";
                                // do nothing
                            } else {
                                msgPointer[3] = *ptr;
                                msgPointer[2] = *(++ptr);
                                msgPointer[1] = *(++ptr);
                                msgPointer[0] = *(++ptr);

                                cout << floats[j] << " ";
                            }
                        }
                        msgPointer += 4;
                    } cout << endl;
                }
                break;
        } // switch

    } // for
    cout << "--------marshaling end---------" << endl;
}

int unmarshallData(char * msgPointer, int * argTypes, void * args[], int argslen, bool allocateMemory) {

    std::vector< argT* > argTypeVector;

    generateArgTvector(argTypes, argTypeVector);

    cout << "========START UNMARSHALLING=========" << endl;

    cout << "argslen=" << argslen << endl;

    for (int i = 0; i < argslen; i++) {

        argT * argType = argTypeVector[i];

        char * chars;
        short * shorts;
        int * ints;
        long * longs;
        double * doubles;
        float * floats;

        cout << "arg" << i << " ";
        if (allocateMemory) {
            switch (argType->type) {
                case ARG_CHAR:
                    cout << "type=char : ";
                    if (argType->arraysize == 0) {
                        chars = new char();
                        *chars = msgPointer[0];
                        msgPointer += 1;

                        cout << *chars << " " << endl;

                        args[i] = chars;
                    } else {
                        chars = new char[argType->arraysize];
                        args[i] = chars;
                        for (int j = 0; j < argType->arraysize; j++) {

                            *chars = msgPointer[0];
                            msgPointer += 1;

                            cout << chars[j] << " ";

                            chars += 1;
                        } cout << endl;
                    }
                    break;

                case ARG_SHORT: // 2 byte
                    cout << "type=short : ";
                    if (argType->arraysize == 0) {
                        shorts = new short();
                        char *ptr = (char*)(shorts);

                        ptr[0] = *msgPointer;
                        ptr[1] = *(++msgPointer);
                        msgPointer += 1;

                        *shorts = *((short*)ptr);
                        cout << *shorts << " " << endl;

                        args[i] = shorts;
                    } else {
                        shorts = new short[argType->arraysize];
                        for (int j = 0; j < argType->arraysize; j++) {

                            char *ptr = (char*)(shorts);

                            ptr[0] = *msgPointer;
                            ptr[1] = *(++msgPointer);
                            msgPointer += 1;

                            *shorts = *((short*)ptr);
                            cout << *shorts << " ";

                            if (j == 0) {
                                args[i] = shorts;
                            }
                            shorts += 1;

                        } cout << endl;
                    }
                    break;

                case ARG_INT: // 4byte
                    cout << "type=int : ";
                    if (argType->arraysize == 0) {
                        ints = new int();

                        get4byteFromCharArray(ints, msgPointer);
                        msgPointer += 4;

                        cout << *ints << ", address = " << ints << endl;
                        args[i] = (void*)ints;

                    } else {
                        ints = new int[argType->arraysize];
                        for (int j = 0; j < argType->arraysize; j++) {
                            get4byteFromCharArray(ints, msgPointer);
                            msgPointer += 4;

                            cout << *ints << " ";
                            if (j == 0) {
                                args[i] = ints;
                            }

                            ints += 1;

                        } cout << endl;
                    }
                    break;
                case ARG_LONG: // 4 byte
                    if (argType->arraysize == 0) {
                        longs = new long();
                        char *ptr = (char *)(longs);

                        ptr[0] = *msgPointer;
                        ptr[1] = *(++msgPointer);
                        ptr[2] = *(++msgPointer);
                        ptr[3] = *(++msgPointer);
                        msgPointer += 1;

                        *longs = *((long*)ptr);
                        cout << *longs << " " << endl;

                        args[i] = longs;
                    } else {
                        longs = new long[argType->arraysize];
                        for (int j = 0; j < argType->arraysize; j++) {

                            char *ptr = (char*)(longs);

                            ptr[0] = *msgPointer;
                            ptr[1] = *(++msgPointer);
                            ptr[2] = *(++msgPointer);
                            ptr[3] = *(++msgPointer);
                            msgPointer += 1;

                            *longs = *((long*)ptr);
                            cout << *longs << " ";

                            if (j == 0) {
                                args[i] = longs;
                            }

                            longs += 1;
                        } cout << endl;
                    }
                    break;
                case ARG_DOUBLE: // 8byte
                    cout << "type=double : ";
                    if (argType->arraysize == 0) {
                        doubles = new double();

                        char *ptr = (char *)(doubles);

                        ptr[7] = *msgPointer;
                        ptr[6] = *(++msgPointer);
                        ptr[5] = *(++msgPointer);
                        ptr[4] = *(++msgPointer);
                        ptr[3] = *(++msgPointer);
                        ptr[2] = *(++msgPointer);
                        ptr[1] = *(++msgPointer);
                        ptr[0] = *(++msgPointer);
                        msgPointer += 1;

                        *doubles = *((double*)ptr);
                        cout << *doubles << " " << endl;

                        args[i] = doubles;

                    } else {
                        doubles = new double[argType->arraysize];
                        for (int j = 0; j < argType->arraysize; j++) {

                            char *ptr = (char*)(&doubles[j]);

                            ptr[7] = *msgPointer;
                            ptr[6] = *(++msgPointer);
                            ptr[5] = *(++msgPointer);
                            ptr[4] = *(++msgPointer);
                            ptr[3] = *(++msgPointer);
                            ptr[2] = *(++msgPointer);
                            ptr[1] = *(++msgPointer);
                            ptr[0] = *(++msgPointer);
                            msgPointer += 1;

                            *doubles = *((double*)ptr);
                            cout << *doubles << " ";

                            if (j == 0) {
                                args[i] = doubles;
                            }

                            doubles += 1;
                        } cout << endl;
                    }
                    break;
                case ARG_FLOAT: // 4 byte
                    cout << "type=float : ";
                    if (argType->arraysize == 0) {
                        floats = new float();

                        char *ptr = (char *)(floats);

                        ptr[3] = *msgPointer;
                        ptr[2] = *(++msgPointer);
                        ptr[1] = *(++msgPointer);
                        ptr[0] = *(++msgPointer);
                        msgPointer += 1;

                        *floats = *((float*)ptr);
                        cout << *floats << " " << endl;

                        args[i] = floats;

                    } else {
                        floats = new float[argType->arraysize];
                        for (int j = 0; j < argType->arraysize; j++) {

                            char *ptr = (char*)(&floats[j]);

                            ptr[3] = *msgPointer;
                            ptr[2] = *(++msgPointer);
                            ptr[1] = *(++msgPointer);
                            ptr[0] = *(++msgPointer);
                            msgPointer += 1;

                            *floats = *((float*)ptr);
                            cout << *floats << " ";

                            if (j == 0) {
                                args[i] = floats;
                            }

                            floats += 1;

                        } cout << endl;
                    }
                    break;
            }
        } else {
            switch (argType->type) {
                case ARG_CHAR:
                    cout << "type=char : ";
                    if (argType->arraysize == 0) {
                        *((char *)args[i]) = msgPointer[0];
                        cout << *msgPointer << " " << endl;

                        msgPointer += 1;

                    } else {
                        for (int j = 0; j < argType->arraysize; j++) {
                            ((char *)args[i])[j] = msgPointer[0];
                            cout << *msgPointer << " ";

                            msgPointer += 1;

                        } cout << endl;
                    }
                    break;

                case ARG_SHORT: // 2 byte
                    cout << "type=short : ";
                    if (argType->arraysize == 0) {
                        shorts = (short *)args[i];

                        char *ptr = (char*)(shorts);
                        ptr[0] = *msgPointer;
                        ptr[1] = *(++msgPointer);
                        msgPointer += 1;

                        // *shorts = *((short*)ptr);
                        cout << *shorts << " " << endl;

                    } else {

                        shorts = (short *) args[i];

                        for (int j = 0; j < argType->arraysize; j++) {

                            char *ptr = (char*)(shorts);
                            ptr[0] = *msgPointer;
                            ptr[1] = *(++msgPointer);
                            msgPointer += 1;

                            cout << *shorts << " ";

                            shorts += 1;

                        } cout << endl;
                    }
                    break;

                case ARG_INT: // 4byte
                    cout << "type=int : ";
                    if (argType->arraysize == 0) {
                        ints = (int *)args[i];
                        get4byteFromCharArray(ints, msgPointer);
                        msgPointer += 4;

                        cout << *ints << " " << endl;

                    } else {
                        ints = (int *) args[i];
                        for (int j = 0; j < argType->arraysize; j++) {
                            get4byteFromCharArray(ints, msgPointer);
                            msgPointer += 4;

                            cout << *ints << " ";

                            ints += 1;
                        } cout << endl;
                    }
                    break;
                case ARG_LONG: // 4 byte
                    cout << "type=long : ";
                    if (argType->arraysize == 0) {
                        longs = (long *)args[i];

                        char *ptr = (char *)(longs);

                        ptr[0] = *msgPointer;
                        ptr[1] = *(++msgPointer);
                        ptr[2] = *(++msgPointer);
                        ptr[3] = *(++msgPointer);
                        msgPointer += 1;

                        cout << *longs << " " << endl;

                    } else {
                        longs = (long *) args[i];
                        for (int j = 0; j < argType->arraysize; j++) {
                            char *ptr = (char*)(longs);

                            ptr[0] = *msgPointer;
                            ptr[1] = *(++msgPointer);
                            ptr[2] = *(++msgPointer);
                            ptr[3] = *(++msgPointer);
                            msgPointer += 1;

                            cout << *longs << " ";

                            longs += 1;
                        } cout << endl;
                    }
                    break;
                case ARG_DOUBLE: // 8byte
                    cout << "type=double : ";
                    if (argType->arraysize == 0) {
                        doubles = (double *)args[i];

                        char *ptr = (char *)(doubles);

                        ptr[7] = *msgPointer;
                        ptr[6] = *(++msgPointer);
                        ptr[5] = *(++msgPointer);
                        ptr[4] = *(++msgPointer);
                        ptr[3] = *(++msgPointer);
                        ptr[2] = *(++msgPointer);
                        ptr[1] = *(++msgPointer);
                        ptr[0] = *(++msgPointer);
                        msgPointer += 1;

                        cout << *doubles << " " << endl;

                    } else {
                        doubles = (double *) args[i];
                        for (int j = 0; j < argType->arraysize; j++) {

                            char *ptr = (char*)(doubles);

                            ptr[7] = *msgPointer;
                            ptr[6] = *(++msgPointer);
                            ptr[5] = *(++msgPointer);
                            ptr[4] = *(++msgPointer);
                            ptr[3] = *(++msgPointer);
                            ptr[2] = *(++msgPointer);
                            ptr[1] = *(++msgPointer);
                            ptr[0] = *(++msgPointer);
                            msgPointer += 1;

                            cout << *doubles << " ";

                            doubles += 1;
                        } cout << endl;
                    }
                    break;
                case ARG_FLOAT: // 4 byte
                    cout << "type=float : ";
                    if (argType->arraysize == 0) {
                        floats = (float *) args[i];

                        char *ptr = (char *)(floats);
                        ptr[3] = *msgPointer;
                        ptr[2] = *(++msgPointer);
                        ptr[1] = *(++msgPointer);
                        ptr[0] = *(++msgPointer);
                        msgPointer += 1;

                        cout << *floats << " " << endl;

                    } else {
                        floats = (float *) args[i];
                        for (int j = 0; j < argType->arraysize; j++) {

                            char *ptr = (char*)(floats);
                            ptr[3] = *msgPointer;
                            ptr[2] = *(++msgPointer);
                            ptr[1] = *(++msgPointer);
                            ptr[0] = *(++msgPointer);
                            msgPointer += 1;

                            cout << *floats << " ";

                            floats += 1;

                        } cout << endl;
                    }
                    break;
            } // switch
        } // else
    } // for
    cout << "=========END UNMARSHALLING==========" << endl;
    return 0;
} // end of function