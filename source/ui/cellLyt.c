#include <nds.h>

#include "cellLyt.h"
#include "../main2d.h"

cellLyt_element_t* cellLyt_init(jnui_bncl_res_t* bncl)
{
    cellLyt_element_t* lyt;
    int nrElements = bncl->header.nrElements;
    lyt = malloc(sizeof(cellLyt_element_t) * nrElements);
    if(lyt == NULL) libndsCrash("Not enough memory to init cell lyt!");
    for (int i = 0; i < nrElements; i++)
    {
        lyt[i].visible = true;
        lyt[i].element = &bncl->elements[i];
    }
    return lyt;
}


void cellLyt_render(jnui_bncl_res_t* bncl, cellLyt_element_t* lyt, CELL_BANK* cell, int screen)
{
    int nrElements = bncl->header.nrElements;

    for(int i = 0; i<nrElements; i++)
    {
        cellLyt_element_t* element = &lyt[i];
        if(!element->visible) continue;
        CELL* cellData = m2d_getCell(cell, element->element->cellId);
        m2d_renderCell(cellData, screen, element->element->x.coord, element->element->y.coord);
    }
}