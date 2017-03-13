#ifndef __SERVERDATA_H__
#define __SERVERDATA_H__ 1

#include "functionData.h"
#include <vector>

struct ServerData {
    char hostname[1024];
    int port;
    std::vector<FunctionData*> fns;
    int num_fns;

    ServerData(char* hn, int port);
    ~ServerData();
    bool functionInList(FunctionData* fdata);
    void addFunctionToList(FunctionData* fdata);
};

#endif