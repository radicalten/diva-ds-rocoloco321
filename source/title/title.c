#include <nds.h>
#include <stdlib.h>

#include "../archive.h"
#include "../globalData.h"
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
    VRAM_C_SUB_BG_0x06200000,
    VRAM_D_LCD,
    VRAM_E_LCD,
    VRAM_F_LCD,
    VRAM_G_LCD,
    VRAM_H_LCD,
    VRAM_I_LCD,
};


void title_init(void)
{
    sTitleSceneState = malloc(sizeof(title_state_t));
    m2d_loadDisplayConfig(&dispConfig);
    consoleInit(&sTitleSceneState->console, 3, BgType_Text4bpp, BgSize_T_256x256, 2, 0, false, false);
    consoleSetFont(&sTitleSceneState->console, &gFonts.fonts[FONT_CONSOLE]);

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
    unloadArchive(sTitleSceneState->bnbl);
    free(sTitleSceneState);
    sTitleSceneState = NULL;
}

void title_render(scene_manager_t* arg, int frameCounter)
{
    int touchRes = -1;
    consoleClear();
    scanKeys();
    u32 keys_held =  keysHeld();
    if(keys_held & KEY_TOUCH)
    {
        touchPosition tp;
        touchRead(&tp);
        touchRes = btnLyt_checkTouch(sTitleSceneState->bnbl, tp.px, tp.py);
    }
    if(keys_held & KEY_SELECT)
    {
        arg->nextScene = SCENE_LOGO;
    }
    printf("Touch Res: %d", touchRes);
   // NF_SpriteOamSet(1);
}

void title_vblank(void)
{
    oamUpdate(&oamSub);
}