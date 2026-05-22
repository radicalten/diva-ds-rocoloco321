#pragma once
#include <nds.h>

//NitroSDK like synonyms for the built in libnds fx functions
//I'm sorry but I'm very used to these names

typedef s32 fx32; 

typedef struct
{
    fx32 x,y,z;
} VecFx32;


#define FX32_TO_FX32_8(n)       ((n)>>4)  //BlocksDS uses 24.8 fixed point in some functions

#define FX_MUL(a, b)            mulf32(a, b)
#define FX_DIV(a, b)            divf32(a, b)
#define FX32_CONST(a)           floattof32(a)
