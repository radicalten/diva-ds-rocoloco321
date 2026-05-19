#pragma once
#include "../archive.h"
#include "../main2d.h"
#include "../scene.h"

typedef struct
{
    m2d_bg_res_t bg0;
    m2d_bg_res_t bg0Sub;
    m2d_bgPal_res_t bgPal;
    m2d_bgPal_res_t bgPalSub;
    //PrintConsole console;
}__attribute__((aligned(4))) logo_state_t;


void logo_init(void);
void logo_render(scene_manager_t* arg, int frameCounter);
void logo_finalize(void);