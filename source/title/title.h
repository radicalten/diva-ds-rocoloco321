#pragma once
#include "../math/fx.h"
#include "../ui/jnLytRes.h"
#include "../main2d.h"
typedef struct
{
    m2d_bg_res_t bg3;
    m2d_bg_res_t bg2;
    m2d_bg_res_t bg3Sub;
    m2d_bg_res_t bg2Sub;
    m2d_bgPal_res_t bgPal;
    m2d_bgPal_res_t bgPalSub;
    jnui_bnbl_res_t* bnbl;
    int state;
    int bg2Ptr; //Returned by InitBG
    int bg2PtrSub;
} title_state_t;


void title_init(void);
void title_finalize(void);
void title_render(scene_manager_t* arg, int frameCounter);
void title_vblank(void);