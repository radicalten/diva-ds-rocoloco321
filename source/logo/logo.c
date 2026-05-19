#include <nds.h>

#include "../main2d.h"
#include "../scene.h"
#include "logo.h"

static logo_state_t* sLogoState;

static display_config_t dispConfig = 
{
    MODE_0_2D,
    MODE_0_2D,
    VRAM_A_MAIN_BG_0x06000000,
    VRAM_B_LCD,
    VRAM_C_SUB_BG_0x06200000,
    VRAM_D_LCD,
    VRAM_E_LCD,
    VRAM_F_LCD,
    VRAM_G_LCD,
    VRAM_H_LCD,
    VRAM_I_LCD,
};

void logo_init(void)
{
    sLogoState = malloc(sizeof(logo_state_t)); 
    m2d_loadDisplayConfig(&dispConfig);
    int bg0 = bgInitHidden(0, BgType_Text8bpp, BgSize_T_256x256, 4, 0);
    int bg0Sub = bgInitHiddenSub(0, BgType_Text8bpp, BgSize_T_256x256, 10, 0);

    m2d_initBackground(&sLogoState->bg0, bg0, "/scene/Logo/logo_m_b");
    m2d_initBgPalette(&sLogoState->bgPal, false, "/scene/Logo/logo_m_b");

    m2d_initBackground(&sLogoState->bg0Sub, bg0Sub, "/scene/Logo/logo_s_b");
    m2d_initBgPalette(&sLogoState->bgPalSub, true, "/scene/Logo/logo_s_b");

    bgShow(bg0);
    bgShow(bg0Sub);

    

/*

    NF_LoadTiledBg("/scene/Logo/logo_m_b", "mainBG0", 256, 256);
    NF_LoadTiledBg("/scene/Logo/logo_s_b", "subBG0", 256, 256);

    NF_CreateTiledBg(0, 0, "mainBG0");
    NF_CreateTiledBg(1, 0, "subBG0");
*/
}

void logo_finalize(void)
{
    m2d_destroyBackground(&sLogoState->bg0);
    m2d_destroyBackground(&sLogoState->bg0Sub);
    m2d_destroyBgPal(&sLogoState->bgPal);
    m2d_destroyBgPal(&sLogoState->bgPalSub);
    free(sLogoState);
    /*
    NF_DeleteTiledBg(0,0);
    NF_DeleteTiledBg(1,0);
    */
}

void logo_render(scene_manager_t* arg, int frameCounter)
{
    if(frameCounter == 120)
    {
        arg->nextScene = SCENE_TITLE;
    }
}
