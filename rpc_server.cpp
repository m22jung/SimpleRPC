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
#include <map>
using namespace std;

vector<SkeletonData*> localDatabase;
vector<skeleton> matchingFs;
int sockfd_client, sockfd_binder, port;
struct sockaddr_in binder_addr, address;
socklen_t addrlen;
char SERVER_ADDRESS[1024];

map<int, pthread_t*> socket_connected;

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
//        cerr << "ERROR on binding" << endl;
        close(sockfd_client);
        return BINDER_NOT_SETUP;
    }

    getsockname(sockfd_client, (struct sockaddr *) &address, &addrlen);
    port = ntohs(address.sin_port);
    string SERVER_PORT = to_string(port);

    gethostname(SERVER_ADDRESS, 1024);
    
//    printf("SERVER_ADDRESS %s\n", SERVER_ADDRESS);
//    cout << "SERVER_PORT " << SERVER_PORT << endl;

    // open connection to binder
    char *BINDER_PORT = getenv("BINDER_PORT");
    if (BINDER_PORT == NULL) {
//        cerr << "ERROR, BINDER_PORT does not exist" << endl;
        return BINDER_PORT_NOT_FOUND;
    }
    binder_port = atoi(BINDER_PORT);
    
    char *BINDER_ADDRESS = getenv("BINDER_ADDRESS");
    if (BINDER_ADDRESS == NULL) {
//        cerr << "ERROR, BINDER_ADDRESS does not exist" << endl;
        return BINDER_ADDR_NOT_FOUND;
    }
    binder = gethostbyname( BINDER_ADDRESS );
    if (binder == NULL) {
//        cerr << "ERROR, no such host" << endl;
        return NO_HOST_FOUND;
    }
    
    sockfd_binder = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_binder < 0) {
//        cerr << "ERROR opening socket" << endl;
        return SOCKET_OPENING_FAILED;
    }

    bzero((char *) &binder_addr, sizeof(binder_addr));
    binder_addr.sin_family = AF_INET;

    bcopy((char *)binder->h_addr, (char *)&binder_addr.sin_addr.s_addr, binder->h_length);
    binder_addr.sin_port = htons(binder_port);

    if (connect(sockfd_binder,(struct sockaddr *) &binder_addr,sizeof(binder_addr)) < 0) {
//        cerr << "ERROR connecting" << endl;
        return SOCKET_CONNECTION_FAILED;
    }

    return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
	// informing binder that a server procedure with the indicated name and
	// list of argument type is avaliable at the server.

	int result = sendRegRequestAfterFormatting(sockfd_binder, SERVER_ADDRESS, port, name, argTypes);
//	cout << "rpcRegister Request result = " << result << endl;
	if (result < 0) {
//		cout << "negative result on resgister request" << endl;
		return result;
	}

	int msgType, returnCode;
    result = receiveRegisterResult(sockfd_binder, msgType, returnCode);
    if (result < 0) {
//        cout << "negative result on receive register result" << endl;
        return result;
    }

    if (msgType == REGISTER_SUCCESS) {
//    	cout << "REGISTER_SUCCESS received, returnCode=" << returnCode << endl;

    	int sameDataIndex = matchingArgT(name, argTypes, &localDatabase);
		
		if (sameDataIndex == -1) { // add new SkeletonData
//			cout << "\nFunction skeleton added:" << endl;
			localDatabase.push_back(new SkeletonData(name, argTypes, f));
            matchingFs.push_back(f);
		} else { // replace function skeleton
//			cout << "\nSame function added (replace function skeleton)" << endl;
			localDatabase[sameDataIndex]->f = f;
            matchingFs[sameDataIndex] = f;
		}
//		cout << endl;

    } else {
//    	cout << "####REGISTER_FAILURE#### returnCode=" << returnCode << endl;
    }

	return result;
}

void *execute(void *arg) {
    int newsockfd = *(int*)arg;
    int msgLength, msgType;

    // read client call
    int result = receiveLengthAndType(newsockfd, msgLength, msgType);
    if (result < 0) {
//        cout << "ERROR on read, returnCode=" << result << endl;
        pthread_exit(NULL);
        //return; // result;
    }

    char message[msgLength];
//    cout << "rpcCall message length = " << msgLength << endl;

    if (read(newsockfd, message + 8, msgLength - 8) < 0) {
//        cerr << "ERROR reading from socket" << endl;
        pthread_exit(NULL   );
        //return; // READING_SOCKET_ERROR;
    }
//    cout << "Second read of the message" << endl;

    char name[64];
    int argTypeslen = 72;
    for (;;) {
        int temp;
        get4byteFromCharArray(&temp, message + argTypeslen);
//        cout << "pointer=" << argTypeslen << " temp=" << temp << endl;
        argTypeslen += 4;
        if (temp == 0) break;
    }
    argTypeslen -= 72;

    int argTypes[argTypeslen];
    receiveNameAndArgTypeForRPCCall(message, name, argTypes, argTypeslen);

//    cout << "DEBUG:::: Length is " << msgLength << endl;

    int sameDataIndex = matchingArgT(name, argTypes, &localDatabase); // search database

    if (sameDataIndex == -1) { // skeleton doesn't exist in this server. return error
//        cout << "could not find called function" << endl;
        sendExecFailureAfterFormatting(newsockfd, FUNCTION_SKELETON_DOES_NOT_EXIST_IN_THIS_SERVER);
    } else {
        // run function skeleton
//        cout << "execute function skeleton. fn_name=" << localDatabase[sameDataIndex]->name << endl;

        int argslen = (argTypeslen / 4) - 1;
//        cout << "ARG LEN IS " << argslen << endl;
        //void ** receivedArgs = (void**)malloc(argslen * sizeof(void*));
        void *receivedArgs[argslen];
        int unmarshallResult = unmarshallData(message + 8 + 64 + argTypeslen, argTypes, receivedArgs, argslen, true);
        
//        cout << "In receivedArgs:" << endl;
        for (int k=0; k < argslen; ++k) {
//            cout << "receivedArgs" << k << " = " << (int*)receivedArgs[k] << endl;
        }

        if (unmarshallResult != 0) {
//            cout << "UNMARSHALL FAILED" << endl;
        }
        
        int skelResult = matchingFs[sameDataIndex](argTypes, receivedArgs);

        if (skelResult == 0) {
//            cout << "Returned result from f_skel: " << *(int *)receivedArgs[0] << endl;
            sendExecSuccessAfterFormatting(newsockfd, name, argTypes, receivedArgs);
//            cout << "Sent Exec Success Msg" << endl;
        } else {
//            cout << "called function skelResult = " << skelResult << endl;
            sendExecFailureAfterFormatting(newsockfd, skelResult);
        }
    }
    close( newsockfd );
}

int rpcExecute() {
//    cout << "***rpcExecute***" << endl;
    int maxfd, newsockfd;
    fd_set readfds;

    if ( listen(sockfd_client, SOMAXCONN) < 0 ) {
//        cerr << "ERROR on listen" << endl;
        return ERROR_ON_LEASON;
    }

    while (true) { // for as many protocols
        FD_ZERO(&readfds); // reset readfds
        FD_SET(sockfd_binder, &readfds);
        FD_SET(sockfd_client, &readfds);
        maxfd = sockfd_client > sockfd_binder ? sockfd_client : sockfd_binder;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
//            cerr << "ERROR on select" << endl;
            continue;
        }

        if (FD_ISSET(sockfd_binder, &readfds)) { // termination?
//            cout << "Termination received" << endl;

            // wait untill all threads are finished
            for (map<int, pthread_t*>::iterator it = socket_connected.begin();
                it != socket_connected.end(); ++it) {
                delete it->second;
            }

            // delete all localdata
            int ldb_size = localDatabase.size();
            for (int i = 0; i < ldb_size; ++i) {
                delete localDatabase[i];
            }
            break; // terminate server
        }

        // new rpcCall from a client
        if ((newsockfd = accept(sockfd_client, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
//            cerr << "ERROR on accept" << endl;
            continue;
        }
//        cout << "Client accepted" << endl;

        // Create a new thread
        // newsockfd is closed in execute function
        socket_connected[newsockfd] = new pthread_t;
        int rc = pthread_create(socket_connected.find(newsockfd)->second, NULL, execute, 
                                (void*) &(socket_connected.find(newsockfd)->first));
//        if (rc) cerr << "ERROR pthread" << endl;
        
    } // while

   return 0;
} // rpcExecute