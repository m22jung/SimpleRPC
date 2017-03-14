#include "serverData.h"
#include "message_lib.h"
#include <string.h>
#include <vector>

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
            printf("on server=%s, port=%d\n", hostname, port);
            return true;
        }
    }
    return false;
}

void ServerData::addFunctionToList(FunctionData* fdata) {
    fns.push_back(fdata);
    num_fns++;
}
