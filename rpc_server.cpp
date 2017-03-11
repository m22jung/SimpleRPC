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
#include <utility>
using namespace std;

int sockfd_client, sockfd_binder, port;
char SERVER_ADDRESS[1024];

struct argT {
	bool input;
	bool output;
	int type;
	bool array;

	argT(bool input, bool output, int type, bool array);
};

argT::argT(bool input, bool output, int type, bool array)
: input(input), output(output), type(type), array(array) {}

struct SkeletonData {
	char name[64];
	skeleton f;
	vector< argT* > argTv;
	int argTypesSize;

	SkeletonData(char *n, int *argTypes, skeleton f);
	~SkeletonData();
};

SkeletonData::SkeletonData(char *n, int *argTypes, skeleton f) : f(f) {
	memcpy(name, n, 64);
	printf("\nSkeletonData::name = %s\n", name);

	for (int i = 0; ; ++i) {
		if (argTypes[i] == 0) { // end of argTypes
			argTypesSize = i - 1;
			break;
		}

		bool input, output, array;
		int type;

		int io = (argTypes[i] >> ARG_OUTPUT) & 0x00000003;
		cout << "int io=" << io << endl;
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
		cout << "type=" << type << endl;

		int arraysize = argTypes[i] & 0x0000FFFF;
		cout << "arraysize=" << arraysize << endl;
		if (arraysize != 0) array = true;

		argTv.push_back(new argT(input, output, type, array));
	}
}
// argTypes0[0] = (1 << ARG_OUTPUT) | (ARG_INT << 16);
// argTypes3[0] = (1 << ARG_OUTPUT) | (1 << ARG_INPUT) | (ARG_LONG << 16) | 11;
// message[0] = (messageLength >> 24) & 0xFF;
// message[1] = (messageLength >> 16) & 0xFF;
// message[2] = (messageLength >> 8) & 0xFF;
// message[3] = messageLength & 0xFF;

// len = (int)((unsigned char)(buffer[0]) << 24 |
// (unsigned char)(buffer[1]) << 16 |
// (unsigned char)(buffer[2]) << 8 |
// (unsigned char)(buffer[3]) );

SkeletonData::~SkeletonData() {
	for (int i = 0; i < argTypesSize; ++i) {
		delete argTv[i];
	}
}

vector<SkeletonData*> localDatabase;

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
	if (result < 0) {
		cout << "negative result" << endl;
		return result;
	}

	// makes an entry in a local database, associating the server skeleton with
	// name and list of argument types.
	localDatabase.push_back(new SkeletonData(name, argTypes, f));
	char c;
	cin >> c;
	// don't forget to delete on termination

	return result;
}