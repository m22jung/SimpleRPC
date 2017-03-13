#ifndef __SKELETONDATA_H__
#define __SKELETONDATA_H__ 1

#include "argT.h"
#include "rpc.h"
#include <vector>

struct SkeletonData {
    char name[64];
    skeleton f;
    std::vector< argT* > argTv;
    int num_argTv;

    SkeletonData(char *n, int *argTypes, skeleton f);
    ~SkeletonData();
};

#endif