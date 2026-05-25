#pragma once

#include "jnLytRes.h"
#include "../main2d.h"

typedef struct
{
    jnui_bncl_res_element_t* element;
    bool visible;
}cellLyt_element_t;

//Renders a cell layout file
void cellLyt_render(jnui_bncl_res_t* bncl, cellLyt_element_t* lyt, CELL_BANK* cell, int screen);
cellLyt_element_t* cellLyt_init(jnui_bncl_res_t* bncl);

inline cellLyt_element_t* cellLyt_getLytElement(cellLyt_element_t* lyt, int idx)
{
    return &lyt[idx];
}

inline void cellLyt_setElementPosition(cellLyt_element_t* element, int x, int y)
{
    element->element->x.coord = x;
    element->element->y.coord = y;
}