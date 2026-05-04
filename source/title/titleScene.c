#include <nds.h>

#include "../scene.h"
#include "titleScene.h"

static void init(scene_manager_t* sceneManager)
{
}

static void finalize(scene_manager_t* sceneManager)
{
}

static void vblank(scene_manager_t* sceneManager, u32 frameCounter)
{
}

static void update(scene_manager_t* sceneManager, u32 frameCounter)
{
}

int titlesc_run(scene_manager_t* arg)
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
    scene_runScene(sceneManager, &sceneDef);
    return 0;
}