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
struct sockaddr_in binder_addr, address;
socklen_t addrlen;
char SERVER_ADDRESS[1024];
vector<int> socket_connected;

int rpcInit() {
	int binder_port;
	struct hostent *binder;

	// connection socket for clients
    addrlen = sizeof(address);

    sockfd_client = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_client < 0) {
        cerr << "ERROR opening socket" << endl;
        return SOCKET_NOT_SETUP;
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

int rpcExecute() {
    cout << "***rpcExecute***" << endl;
    bool running = true;
    int maxfd, newsockfd;
    fd_set readfds;

    if ( listen(sockfd_client, SOMAXCONN) < 0 ) {
        cerr << "ERROR on listen" << endl;
        return -1;
    }

    FD_ZERO(&readfds); // reset readfds
    FD_SET(sockfd_binder, &readfds);
    FD_SET(sockfd_client, &readfds);
    cout << "sockfd_binder=" << sockfd_binder << " sockfd_client=" << sockfd_client << endl;
    maxfd = sockfd_client > sockfd_binder ? sockfd_client : sockfd_binder;

    while (running) { // for as many protocols
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            cerr << "ERROR on select" << endl;
            continue;
        }

        if (FD_ISSET(sockfd_binder, &readfds)) { // termination?
            cout << "Termination received" << endl;
        }


        // new rpcCall from a client
        if ((newsockfd = accept(sockfd_client, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            cerr << "ERROR on accept" << endl;
            continue;
        }
        cout << "Client accepted" << endl;

        // TODO: Create a new thread if new exec message received
        // pthread(); // newsockfd closed in pthread

        int msgLength, msgType;
        // read client call
        int result = receiveLengthAndType(newsockfd, msgLength, msgType);
        if (result < 0) {
            cout << "ERROR on read, returnCode=" << result << endl;
            continue;
        }

        char message[msgLength];
        cout << "rpcCall message length = " << msgLength << endl;
 
        if (read(newsockfd, message + 8, msgLength - 8) < 0) {
            cerr << "ERROR reading from socket" << endl;
            continue;
        }
        cout << "Second read of the message" << endl;

        char name[64];
        int argTypeslen = 72;
        for (;;) {
            int temp;
            get4byteFromCharArray(&temp, message + argTypeslen);
            cout << "pointer=" << argTypeslen << " temp=" << temp << endl;
            argTypeslen += 4;
            if (temp == 0) break;
        }
        argTypeslen -= 72;

        int argTypes[argTypeslen];
        receiveNameAndArgTypeForRPCCall(message, name, argTypes, argTypeslen);

        cout << "DEBUG:::: Length is " << msgLength << endl;

        int sameDataIndex = matchingArgT(name, argTypes, &localDatabase); // search database

        if (sameDataIndex == -1) { // skeleton doesn't exist in this server. return error
            cout << "could not find called function" << endl;
            sendExecFailureAfterFormatting(newsockfd, FUNCTION_SKELETON_DOES_NOT_EXIST_IN_THIS_SERVER);
        } else {
            // run function skeleton
            cout << "execute function skeleton. fn_name=" << localDatabase[sameDataIndex]->name << endl;

            int argslen = msgLength - 8 - 64 - argTypeslen;
            void ** receivedArgs = (void**)malloc(argslen * sizeof(void*));
            int unmarshallResult = unmarshallData(message + 8 + 64 + argTypeslen, argTypes, receivedArgs, argTypeslen, true);
            if (unmarshallResult != 0) {
                cout << "UNMARSHALL FAILED" << endl;
            }

            localDatabase[sameDataIndex]->f(argTypes, receivedArgs);
            cout << "Returned result from f_skel: " << (int *)receivedArgs[0] << endl;
            sendExecSuccessAfterFormatting(newsockfd, name, argTypes, receivedArgs);
            cout << "Sent Exec Success Msg" << endl;
        }
        close( newsockfd );
    } // while

   return 0;
} // rpcExecute