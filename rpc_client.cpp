//
// Created by J on 2017. 3. 7..
//

#include <iostream>
#include <zconf.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <tclDecls.h>
#include "rpc.h"
#include "message_lib.h"
#include "type_lib.h"

using namespace std;

static int binderSocket = -1;

// LENGTH, TYPE, MESSAGE

int createSocket(char * addr, int port) {
    int socket;
    struct sockaddr_in address;

    socket = socket(PF_INET, SOCK_STREAM, 0);

    if (socket == -1) {
        return SOCKET_NOT_SETUP;
    }

    address.sin_addr.s_addr = inet_addr(addr);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (connect(socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        return SOCKET_CONNECTION_FAILED;
    }

    return socket;
}

int sendLocationRequestMessage(char * name, int argTypes[]) {

    if (binderSocket < 0) { // // set up binder socket if not defined
        char * binderAddressString = getenv ("BINDER_ADDRESS");
        if(binderAddressString == NULL) {
            return BINDER_ADDR_NOT_FOUND;
        }
        char * binderPortString = getenv("BINDER_PORT");
        if (binderPortString == NULL) {
            return BINDER_PORT_NOT_FOUND;
        }

        binderSocket = createSocket(binderAddressString, atoi(binderPortString));

        if (binderSocket < 0) {
            return BINDER_NOT_SETUP;
        }
    }

    return sendLocRequestAfterFormatting(binderSocket, name, argTypes);
}

int sendExecuteRequestMessage(int serverSocket, char* name, int* argTypes, void** args) {

    int result = sendExecRequestAfterFormatting(serverSocket, name, argTypes, args);

    if (result < 0) {
        return result;
    } else {
        // handle response
    }

}

int sendTerminationMessage() {
    // connect to binder

    if (binderSocket < 0) { // if binder socket is undefined
        char * binderAddressString = getenv ("BINDER_ADDRESS");
        if(binderAddressString == NULL) {
            return BINDER_ADDR_NOT_FOUND;
        }
        char * binderPortString = getenv("BINDER_PORT");
        if(binderPortString == NULL) {
            return BINDER_PORT_NOT_FOUND;
        }

        binderSocket = createSocket(binderAddressString, atoi(binderPortString));

        if (binderSocket < 0) {
            return BINDER_NOT_SETUP;
        }
    }

    return sendTerminateAfterFormatting(binderSocket);
}



int rpcCall(char* name, int* argTypes, void** args) {
    // send a loc req msg to the binder to locate the server for the procedure

    int locationRequestResult = sendLocationRequestMessage(name, argTypes);

    if (locationRequestResult < 0) {
        // Error
        return locationRequestResult;
    }

    // TODO: handle response from the binder

    //then send execute-req msg to the server
    int serverSocket = createSocket(addr, port);

    if (serverSocket < 0) {
        return SERVER_SOCKET_NOT_SETUP;
    }

    int responseFromServer = sendExecuteRequestMessage(serverSocket, name, argTypes, args);

    close(serverSocket);

    return responseFromServer;

}

int rpcTerminate() {
    int terminationRequestResult = sendTerminationMessage();

    return terminationRequestResult;
}