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
	if (result < 0) {
		cout << "negative result" << endl;
		return result;
	}

	// generate argument information for this request
	vector< argT* > v;
	cout << "Registering argTypes:" << endl;
	printf("SkeletonData::name = %s\n", name);
	generateArgTvector(argTypes, v);
	int vsize = v.size();

	// makes an entry in a local database, associating the server skeleton with
	// name and list of argument types.
	int localDatabaseSize = localDatabase.size();
	int sameDataIndex = -1;

	for (int i = 0; i < localDatabaseSize; ++i) {
		bool flag_samename = true;

		// check if function name is same
		for (int j = 0; j < 64; ++j) {
			if (name[j] == '\0' && localDatabase[i]->name[j] == '\0') break; // has same name
			if (name[j] != localDatabase[i]->name[j]) {
				flag_samename = false;
				break;
			}
		}
		if (!flag_samename) continue; // move to next data

		// check if number of arguments is same
		if (vsize != localDatabase[i]->num_argTv) continue; // move to next data

		bool flag_sameArg = true;

		// check if argument types are same
		for (int j = 0; j < vsize; ++j) {
			if (v[j]->type != (localDatabase[i]->argTv)[j]->type ||
				v[j]->array != (localDatabase[i]->argTv)[j]->array) { // has different arg type
				flag_sameArg = false;
				break;
			}
		}
		if (!flag_sameArg) continue; // move to next data

		sameDataIndex = i;
		break;
	}
	
	if (sameDataIndex == -1) { // add new SkeletonData
		cout << "\nFunction skeleton added:" << endl;
		localDatabase.push_back(new SkeletonData(name, argTypes, f));
	} else { // replace function skeleton
		cout << "\nSame function added (replace function skeleton)" << endl;
		localDatabase[sameDataIndex]->f = f;
	}
	cout << endl;
	// don't forget to delete local database on termination

	return result;
}