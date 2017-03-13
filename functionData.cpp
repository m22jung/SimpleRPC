#include "functionData.h"
#include "message_lib.h"
#include <string.h>
#include <stdio.h>
#include <vector>

FunctionData::FunctionData(char *n, int *argTypes) {
    memcpy(name, n, 64);
    name[64] = '\0';
    printf("FunctionData::name = %s\n", name);

    generateArgTvector(argTypes, argTv);
    num_argTv = argTv.size();
}

FunctionData::~FunctionData() {
    for (int i = 0; i < num_argTv; ++i) {
        delete argTv[i];
    }
}
