#pragma once

#include "jnLytRes.h"


//Returns the bnbl element ID if touch coordinates matches an element, returns -1 otherwise
int btnLyt_checkTouch(const jnui_bnbl_res_t* bnbl, int x, int y);

static inline int getBnblElementCount(const jnui_bnbl_res_t* bnbl)
{
    return bnbl->header.nrElements;
}
