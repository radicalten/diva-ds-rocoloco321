#pragma once

typedef enum
{
    SCENE_LOGO = 0,
    SCENE_TITLE,
    SCENE_MENU,
    SCENE_SETTINGS,
    SCENE_PLAY,
    SCENE_EXIT,
    SCENE_INVALID = 255
} DivaScene;

typedef struct
{
    u8 currScene;
    u8 prevScene;
    u8 nextScene;
} scene_manager_t;

typedef void (*scene_main_func_t)(void);
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

void scene_main();
void scene_runScene(const scene_def_t* sceneDef);
