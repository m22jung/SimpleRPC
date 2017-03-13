#ifndef __FUNCTIONDATA_H__
#define __FUNCTIONDATA_H__ 1

#include "argT.h"
#include <vector>

struct argT;

struct FunctionData {
    char name[64];
    std::vector< argT* > argTv;
    int num_argTv;

    FunctionData(char *n, int *argTypes);
    ~FunctionData();
};

#endif