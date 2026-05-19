#include <nds.h>

#include "../scene.h"
#include "logo.h"
#include "logoScene.h"

static void init(scene_manager_t* sceneManager)
{
    logo_init();
}

static void finalize(scene_manager_t* sceneManager)
{
    logo_finalize();
}

static void vblank(scene_manager_t* sceneManager, u32 frameCounter)
{
}

static void update(scene_manager_t* sceneManager, u32 frameCounter)
{
    logo_render(sceneManager, frameCounter);
}

void logosc_run(void)
{
    scene_def_t sceneDef =
    {
        init,
        update,
        finalize,
        vblank,
        60,
        30,
        true,
        false
    };
    scene_runScene(&sceneDef);
}