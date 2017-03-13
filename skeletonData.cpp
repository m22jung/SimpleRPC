#include "skeletonData.h"
#include "message_lib.h"
#include <string.h>
#include <stdio.h>
#include <vector>

SkeletonData::SkeletonData(char *n, int *argTypes, skeleton f) : f(f) {
    memcpy(name, n, 64);
    name[64] = '\0';
    printf("SkeletonData::name = %s\n", name);

    generateArgTvector(argTypes, argTv);
    num_argTv = argTv.size();
}

SkeletonData::~SkeletonData() {
    for (int i = 0; i < num_argTv; ++i) {
        delete argTv[i];
    }
}