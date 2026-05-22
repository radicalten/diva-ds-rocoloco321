#include <nds.h>
#include <stdlib.h>

#include "../archive.h"
#include "../globalData.h"
#include "../math/fx.h"
#include "../ui/btnLyt.h"
#include "../ui/jnLytRes.h"
#include "../main2d.h"
#include "../scene.h"
#include "title.h"

static title_state_t* sTitleSceneState;

static display_config_t dispConfig = 
{
    MODE_0_2D,
    MODE_0_2D,
    VRAM_A_MAIN_BG_0x06000000,
    VRAM_B_LCD,
    VRAM_C_LCD,
    VRAM_D_LCD,
    VRAM_E_LCD,
    VRAM_F_LCD,
    VRAM_G_LCD,
    VRAM_H_SUB_BG, 
    VRAM_I_SUB_SPRITE
};


void title_init(void)
{
    sTitleSceneState = malloc(sizeof(title_state_t));
    m2d_loadDisplayConfig(&dispConfig);

    int bg3 = bgInitHidden(3, BgType_Text4bpp, BgSize_T_256x256, 3, 0);
    sTitleSceneState->bg2Ptr = bgInitHidden(2, BgType_Text4bpp, BgSize_T_256x256, 4, 1);

    m2d_initBackground(&sTitleSceneState->bg3, bg3, "/scene/Title/title3_m_b");
    m2d_initBackground(&sTitleSceneState->bg2, sTitleSceneState->bg2Ptr, "/scene/Title/title2_m_b");
    m2d_initBackground(&sTitleSceneState->bg2, sTitleSceneState->bg2Ptr, "/scene/Title/title2_m_b");
    m2d_initBgPalette(&sTitleSceneState->bgPal, false, "/scene/Title/title_m_b");
    bgSetPriority(bg3, 3);
    bgSetPriority(sTitleSceneState->bg2Ptr, 1);
    bgShow(bg3);
    bgShow(sTitleSceneState->bg2Ptr);
    m2d_setBlendAlpha(BLEND_SRC_BG2, BLEND_DST_BG3, 8,8);

   // int bg3Sub = bgInitHiddenSub() 


    

    sTitleSceneState->bnbl = (jnui_bnbl_res_t*)loadArchive("/scene/Title/test.bnbl");

/*
    //NF_LoadTiledBg("/scene/Logo/logo_m_b", "mainBG0", 256, 256);
    NF_LoadTiledBg("/scene/Title/bg", "subBG0", 256, 256);

    //NF_CreateTiledBg(0, 0, "mainBG0");
    NF_CreateTiledBg(1, 0, "subBG0");

    NF_LoadSpriteGfx("scene/Title/spr1", 0, 32, 32);
    NF_LoadSpritePal("scene/Title/spr1", 0);
    NF_LoadSpriteGfx("scene/Title/spr2", 1, 32, 32);
    NF_LoadSpritePal("scene/Title/spr2", 1);

    NF_VramSpriteGfx(1,0,0,true);
    NF_VramSpritePal(1,0,0);

    NF_VramSpriteGfx(1,1,1,true);
    NF_VramSpritePal(1,1,1);

    NF_CreateSprite(1,0,0,0,32,32);
    NF_CreateSprite(1,1,1,1,64,64);
    */
}

void title_finalize(void)
{
    m2d_destroyBackground(&sTitleSceneState->bg3);
    m2d_destroyBackground(&sTitleSceneState->bg2);
    m2d_destroyBgPal(&sTitleSceneState->bgPal);
    unloadArchive(sTitleSceneState->bnbl);
    free(sTitleSceneState);
    sTitleSceneState = NULL;
}

void title_render(scene_manager_t* arg, int frameCounter)
{
}

void title_vblank(void)
{
    fx32 scroll = FX32_TO_FX32_8(FX32_CONST(0.25));
    bgScrollf(sTitleSceneState->bg2Ptr, -scroll, scroll);
    bgUpdate();
    //oamUpdate(&oamSub);
}