#include <nds.h>
#include <nf_lib.h>

#include "../scene.h"
#include "logo.h"

void logo_init(void)
{

    NF_Set2D(0, 0);
    NF_Set2D(1, 0);
    NF_ResetTiledBgBuffers();
    NF_InitTiledBgSys(0);       // Top screen
    NF_InitTiledBgSys(1);       // Bottom screen

    NF_LoadTiledBg("/scene/Logo/logo_m_b", "mainBG0", 256, 256);
    NF_LoadTiledBg("/scene/Logo/logo_s_b", "subBG0", 256, 256);

    NF_CreateTiledBg(0, 0, "mainBG0");
    NF_CreateTiledBg(1, 0, "subBG0");

}

void logo_finalize(void)
{
    NF_DeleteTiledBg(0,0);
    NF_DeleteTiledBg(1,0);
}

void logo_render(scene_manager_t* arg, int frameCounter)
{
    if(frameCounter == 120)
    {
        arg->nextScene = SCENE_TITLE;
    }
}
