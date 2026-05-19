#include <nds.h>

#include "../scene.h"
#include "title.h"
#include "titleScene.h"

static void init(scene_manager_t* sceneManager)
{
    title_init();
}

static void finalize(scene_manager_t* sceneManager)
{
    title_finalize();
}

static void vblank(scene_manager_t* sceneManager, u32 frameCounter)
{
    title_vblank();
}

static void update(scene_manager_t* sceneManager, u32 frameCounter)
{
    title_render(sceneManager, frameCounter);
}

void titlesc_run(void)
{
    scene_def_t sceneDef =
    {
        init,
        update,
        finalize,
        vblank,
        10,
        10,
        false,
        false
    };
    scene_runScene(&sceneDef);
}