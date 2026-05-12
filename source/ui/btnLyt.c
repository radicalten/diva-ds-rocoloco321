#include <nds.h>

#include "btnLyt.h"

bool isCoordInsideBounds(const jnui_bnbl_res_element_t* element, int tpX, int tpY)
{
    int minX = element->x.coord;
    int maxX = minX + element->width;
    int minY = element->y.coord;
    int maxY = element->height + minY;
    return (tpX >= minX && tpX <=maxX) && (tpY >= minY && tpY <=maxY);
}

int btnLyt_checkTouch(const jnui_bnbl_res_t* bnbl, int x, int y)
{
    int res = -1;
    int nrElements = getBnblElementCount(bnbl);
    int i;
    for(i=0;i<nrElements;i++)
    {
        if(isCoordInsideBounds(&bnbl->elements[i],x,y))
        {
            res = i;
            break;
        }
    }
    return res;
}

