#pragma once
#include "../ui/jnLytRes.h"

typedef struct
{
    int state;
    PrintConsole console;
    jnui_bnbl_res_t* bnbl;
} title_state_t;


void title_init(void);
void title_finalize(void);
void title_render(scene_manager_t* arg, int frameCounter);
void title_vblank(void);