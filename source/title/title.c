#include <nds.h>
#include <nf_lib.h>
#include <stdlib.h>

#include "../archive.h"
#include "../globalData.h"
#include "../ui/btnLyt.h"
#include "../ui/jnLytRes.h"
#include "../scene.h"
#include "title.h"

static title_state_t* sTitleSceneState;


void title_init(void)
{
    sTitleSceneState = malloc(sizeof(title_state_t));
    NF_Set2D(1,0);
    NF_ResetTiledBgBuffers();

    consoleInit(&sTitleSceneState->console, 3, BgType_Text4bpp, BgSize_T_256x256, 2, 0, false, false);
    consoleSetFont(&sTitleSceneState->console, &font);
    NF_ShowBg(1,3);


    sTitleSceneState->bnbl = (jnui_bnbl_res_t*)loadArchive("/scene/Title/test.bnbl");

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
}

void title_vblank(void)
{

}