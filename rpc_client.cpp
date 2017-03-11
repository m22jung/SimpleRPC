//
// Created by J on 2017. 3. 7..
//

#include <iostream>
#include <zconf.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rpc.h"
#include "message_lib.h"
#include "type_lib.h"
#include <unistd.h>
#include <netdb.h>
#include <string.h>

using namespace std;

int binderSocket = -1;
int serverSocket = -1;

// LENGTH, TYPE, MESSAGE

int createServerSocket(char * addr, int port) {
    struct sockaddr_in server_addr;
    struct hostent *server;

    server = gethostbyname(addr);

    if (server == NULL) {
        cerr << "ERROR, no such host" << endl;
        return -1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "ERROR opening socket" << endl;
        return -1;
    }

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);

    if (connect(binderSocket,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
        cerr << "ERROR connecting" << endl;
        return -1;
    }

    return 0;
}

int createBinderSocket() {
    cout << "creating binder socket " << endl;

    int binder_port;
    struct sockaddr_in binder_addr;
    struct hostent *binder;
    char *BINDER_PORT = getenv("BINDER_PORT");
    if (BINDER_PORT == NULL) {
        cerr << "ERROR, BINDER_PORT does not exist" << endl;
        return -1;
    }
    binder_port = atoi(BINDER_PORT);

    char *BINDER_ADDRESS = getenv("BINDER_ADDRESS");
    if (BINDER_ADDRESS == NULL) {
        cerr << "ERROR, BINDER_ADDRESS does not exist" << endl;
        return -1;
    }
    binder = gethostbyname( BINDER_ADDRESS );
    if (binder == NULL) {
        cerr << "ERROR, no such host" << endl;
        return -1;
    }

    binderSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (binderSocket < 0) {
        cerr << "ERROR opening socket" << endl;
        return -1;
    }

    bzero((char *) &binder_addr, sizeof(binder_addr));
    binder_addr.sin_family = AF_INET;

    bcopy((char *)binder->h_addr, (char *)&binder_addr.sin_addr.s_addr, binder->h_length);
    binder_addr.sin_port = htons(binder_port);

    if (connect(binderSocket,(struct sockaddr *) &binder_addr,sizeof(binder_addr)) < 0) {
        cerr << "ERROR connecting" << endl;
        return -1;
    }

    return 0;

}

int sendLocationRequestMessage(char * name, int argTypes[]) {

    if (binderSocket < 0) { // // set up binder socket if not defined
//        char * binderAddressString = getenv ("BINDER_ADDRESS");
//        if(binderAddressString == NULL) {
//            return BINDER_ADDR_NOT_FOUND;
//        }
//        char * binderPortString = getenv("BINDER_PORT");
//        if (binderPortString == NULL) {
//            return BINDER_PORT_NOT_FOUND;
//        }

        int result = createBinderSocket();

        if (result < 0) {
            return BINDER_NOT_SETUP;
        }
    }

    return sendLocRequestAfterFormatting(binderSocket, name, argTypes);
}

int sendExecuteRequestMessage(int serverSocket, char* name, int* argTypes, void** args) {

    return sendExecRequestAfterFormatting(serverSocket, name, argTypes, args);

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

        binderSocket = createBinderSocket();

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

    int length, msgType;
    // Extract length and type
    int extractLengthAndTypeResult = receiveLengthAndType(binderSocket, length, msgType);

    if (extractLengthAndTypeResult < 0) {
        // error
        return extractLengthAndTypeResult;
    }

    // Depending on the type, handle differently

    char *message = new char[length];
    if (read(binderSocket, message + 8, length - 8) < 0) {
        return READING_SOCKET_ERROR;
    }

    char *server_identifier = new char[1024];
    int port;
    switch(msgType) {
        case LOC_SUCCESS:
            receiveServerIdentifierAndPort(length, message, server_identifier, port);
            // extract server_identifer, port
            break;
        case LOC_FAILURE:
            int reasonCode = 0;
            receiveReasonCode(length, message, reasonCode);
            return reasonCode;
            // extract reason code,return it
            break;
    }
    delete [] message;

    //then send execute-req msg to the server
    int serverSocket = createServerSocket(server_identifier, port);

    if (serverSocket < 0) {
        return SERVER_SOCKET_NOT_SETUP;
    }

    int responseFromServer = sendExecuteRequestMessage(serverSocket, name, argTypes, args);

    //TODO: Handle response from the server
    extractLengthAndTypeResult = receiveLengthAndType(binderSocket, length, msgType);

    if (extractLengthAndTypeResult < 0) {
        // error
        return extractLengthAndTypeResult;
    }

    message = new char[length];
    if (read(binderSocket, message + 8, length - 8) < 0) {
        return READING_SOCKET_ERROR;
    }
//    receiveNameAndArgTypeAndArgs(length, message, name, argTypes, args); // potential bug source

    close(serverSocket);

    return responseFromServer;

}

int rpcTerminate() {
    int terminationRequestResult = sendTerminationMessage();
    close(binderSocket);

    return terminationRequestResult;
}