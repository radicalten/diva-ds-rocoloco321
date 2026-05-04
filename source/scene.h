#pragma once

typedef enum
{
    SCENE_LOGO,
    SCENE_TITLE,
    SCENE_MENU,
    SCENE_SETTINGS,
    SCENE_PLAY
} DivaScene;

typedef struct
{
    s8 currScene;
    s8 prevScene;
    s8 nextScene;
} scene_manager_t;

typedef int (*process_main_func_t)(void* arg);
typedef void (*scene_init_func_t)(scene_manager_t* sceneManager);
typedef void (*scene_update_func_t)(scene_manager_t* sceneManager, u32 frameCounter);
typedef void (*scene_finalize_func_t)(scene_manager_t* sceneManager);
typedef void (*scene_vblank_func_t)(scene_manager_t* sceneManager, u32 frameCounter);

typedef struct
{
    scene_init_func_t initFunc;
    scene_update_func_t updateFunc;
    scene_finalize_func_t finalizeFunc;
    scene_vblank_func_t vblankFunc;
    s16 fadeInLength;
    s16 fadeOutLength;
    bool fadeInWhite;
    bool fadeOutWhite;
} scene_def_t;

extern scene_manager_t* sceneManager;

void scene_init();
void scene_main();
void scene_runScene(scene_manager_t* sceneManager, const scene_def_t* sceneDef);