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
    int bg3Sub = bgInitHiddenSub(3, BgType_Text4bpp, BgSize_T_256x256, 3, 0);
    sTitleSceneState->bg2PtrSub = bgInitHiddenSub(2, BgType_Text4bpp, BgSize_T_256x256, 4, 1);

    m2d_initBackground(&sTitleSceneState->bg3, bg3, "/scene/Title/title3_m_b");
    m2d_initBackground(&sTitleSceneState->bg2, sTitleSceneState->bg2Ptr, "/scene/Title/title2_m_b");
    m2d_initBgPalette(&sTitleSceneState->bgPal, false, "/scene/Title/title_m_b");

    m2d_initBackground(&sTitleSceneState->bg3Sub, bg3Sub, "/scene/Title/title3_s_b");
    m2d_initBackground(&sTitleSceneState->bg2Sub, sTitleSceneState->bg2PtrSub, "/scene/Title/title2_s_b");
    m2d_initBgPalette(&sTitleSceneState->bgPalSub, true, "/scene/Title/title_s_b");

    bgSetPriority(bg3, 3);
    bgSetPriority(sTitleSceneState->bg2Ptr, 1);
    bgShow(bg3);
    bgShow(sTitleSceneState->bg2Ptr);
    m2d_setBlendAlpha(BLEND_SRC_BG2, BLEND_DST_BG3, 8,8);
    bgSetPriority(bg3Sub, 3);
    bgSetPriority(sTitleSceneState->bg2PtrSub, 1);
    bgShow(bg3Sub);
    bgShow(sTitleSceneState->bg2PtrSub);
    m2d_setBlendAlphaSub(BLEND_SRC_BG2, BLEND_DST_BG3, 8,8);

    m2d_clearOam();
    m2d_prepareBuffers();
    m2d_enableOam(true, SpriteMapping_1D_32);
    m2d_loadInitCell(&sTitleSceneState->objSub, true, "/scene/Title/test_s_o");
    CELL* cell0 = m2d_getCell(sTitleSceneState->objSub.obj, 1);
    m2d_renderCell(cell0, true, 0,0);
    m2d_applyBuffers();
    

    sTitleSceneState->bnbl = (jnui_bnbl_res_t*)loadArchive("/scene/Title/test.bnbl");

}

void title_finalize(void)
{
    m2d_destroyBackground(&sTitleSceneState->bg3);
    m2d_destroyBackground(&sTitleSceneState->bg2);
    m2d_destroyBackground(&sTitleSceneState->bg3Sub);
    m2d_destroyBackground(&sTitleSceneState->bg2Sub);
    m2d_destroyBgPal(&sTitleSceneState->bgPal);
    m2d_destroyBgPal(&sTitleSceneState->bgPalSub);
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
    bgScrollf(sTitleSceneState->bg2PtrSub, -scroll, scroll);
    bgUpdate();
    //oamUpdate(&oamSub);
}