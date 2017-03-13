#ifndef __ARGT_H__
#define __ARGT_H__ 1

struct argT {
    bool input;
    bool output;
    int type;
    bool array;

    argT(bool input, bool output, int type, bool array);
};

#endif