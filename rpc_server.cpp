#include "rpc.h"
#include "type_lib.h"
#include "message_lib.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> // FD_SET, FD_ISSET, FD_ZERO macros
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h> 
#include <stdio.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <vector>
using namespace std;

vector<SkeletonData*> localDatabase;
int sockfd_client, sockfd_binder, port;
char SERVER_ADDRESS[1024];

int rpcInit() {
	int binder_port;
	struct sockaddr_in binder_addr, address;
	struct hostent *binder;

	// connection socket for clients
    socklen_t addrlen = sizeof(address);

    sockfd_client = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_client < 0) {
        cerr << "ERROR opening socket" << endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = INADDR_ANY;

    if ( bind(sockfd_client, (struct sockaddr *) &address, addrlen) < 0) {
        cerr << "ERROR on binding" << endl;
        close(sockfd_client);
        return -1;
    }

    getsockname(sockfd_client, (struct sockaddr *) &address, &addrlen);
    port = ntohs(address.sin_port);
    string SERVER_PORT = to_string(port);

    gethostname(SERVER_ADDRESS, 1024);
    
    printf("SERVER_ADDRESS %s\n", SERVER_ADDRESS);
    cout << "SERVER_PORT " << SERVER_PORT << endl;

    if ( listen(sockfd_client, 5) < 0 ) {
        cerr << "ERROR on listen" << endl;
        return -1;
    }

    // open connection to binder
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
    
    sockfd_binder = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_binder < 0) {
        cerr << "ERROR opening socket" << endl;
        return -1;
    }

    bzero((char *) &binder_addr, sizeof(binder_addr));
    binder_addr.sin_family = AF_INET;

    bcopy((char *)binder->h_addr, (char *)&binder_addr.sin_addr.s_addr, binder->h_length);
    binder_addr.sin_port = htons(binder_port);

    if (connect(sockfd_binder,(struct sockaddr *) &binder_addr,sizeof(binder_addr)) < 0) {
        cerr << "ERROR connecting" << endl;
        return -1;
    }

    return 0;
}


int rpcRegister(char* name, int* argTypes, skeleton f) {
	// informing binder that a server procedure with the indicated name and
	// list of argument type is avaliable at the server.
	// ...(send protocol to binder)...
	int result = sendRegRequestAfterFormatting(sockfd_binder, SERVER_ADDRESS, port, name, argTypes);
	cout << "rpcRegister Request result = " << result << endl;
	if (result < 0) {
		cout << "negative result on resgister request" << endl;
		return result;
	}

	int msgType, returnCode;
    result = receiveRegisterResult(sockfd_binder, msgType, returnCode);
    if (result < 0) {
        cout << "negative result on receive register result" << endl;
        return result;
    }

    if (msgType == REGISTER_SUCCESS) {
    	cout << "REGISTER_SUCCESS received, returnCode=" << returnCode << endl;

    	int sameDataIndex = matchingArgT(name, argTypes, &localDatabase);
		
		if (sameDataIndex == -1) { // add new SkeletonData
			cout << "\nFunction skeleton added:" << endl;
			localDatabase.push_back(new SkeletonData(name, argTypes, f));
		} else { // replace function skeleton
			cout << "\nSame function added (replace function skeleton)" << endl;
			localDatabase[sameDataIndex]->f = f;
		}
		cout << endl;
		// don't forget to delete local database on termination

    } else {
    	cout << "####REGISTER_FAILURE#### returnCode=" << returnCode << endl; 
    }

	return result;
}
//int rpcExecute() {
//
//    bool running = true;
//    // wait for the client call
//    int msgLength, msgType;
//
//    if (sockfd_client < 0) {
//        return SOCKET_NOT_SETUP;
//    }
//
//    while (running) {
//        // TODO: Also listen to the binder socket
//        receiveLengthAndType(sockfd_client, msgLength, msgType);
//
//        char *message = new char[msgLength];
//
//        if (read(sockfd_client, message + 8, msgLength - 8) < 0) {
//            cerr << "ERROR reading from socket" << endl;
//            continue;
//        }
//
//        char *name = new char[64];
//        int argTypeslen;
//        int *argTypes;
//        void ** args;
//
//        switch(msgType) {
//            case EXECUTE:
//                // TODO: Create a new thread if new exec message received
//                receiveNameAndArgTypeAndArgs(msgLength, message, name, argTypes, args);
//
//                int sameDataIndex = matchingArgT<vector<SkeletonData*>>(name, argTypes, &localDatabase);
//
//                if (sameDataIndex == -1) { // skeleton doesn't exist in this server. return error
//                    sendExecFailureAfterFormatting(sockfd_client, FUNCTION_SKELETON_DOES_NOT_EXIST_IN_THIS_SERVER);
//                } else {
//                    localDatabase[sameDataIndex]->f(argTypes, args);
//                    sendExecSuccessAfterFormatting(sockfd_client, name, argTypes, args);
//                }
//
//                break;
//            case TERMINATE:
//                // TODO: In multithreading environment, close all running threads
//                // TODO: verify that it came from the correct binder's ip addr / hostname
//                running = false;
//                break;
//        }
//    }
//
//
//
//    return 0;
//}