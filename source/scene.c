#include <nds.h>
#include <nds/cothread.h>
#include <stdlib.h>

#include "logo/logoScene.h"
#include "title/titleScene.h"
#include "scene.h"
/*
Simple scene manager heavily inspired by the scene manager present in Mario Kart DS, obtained via reverse engineering.
(Credits to the MKDS Decomp team)

General flow:
1. Alloc a "manager" which holds the current, next and previous scene ID, as well as setting the initial scene ID
2. Enter an infinite loop
3. Set the current and previous scene IDs based on the contents of the scene manager, and clear the next scene field
4. Run the scene "main" function from the scene table
5. Repeat from step 2 

Scene "main" function:
Inside this function we set the scene function pointers (init, render, vblank and finalize/destroy functions)
as well as the fade in and fade out parameters (see scene.h)
after that, it calls scene_runScene with those parameters.

scene_runScene:
This is where the scene logic is executed:
1. Run the scene def init function
2. Apply the fade in
3. Enter a loop where the scene render and vblank functions are executed
4. The loop will keep executing until the scene sets the nextScene field to anything other than -1
5. Apply the fade out (Keep in mind that the render and vblank scene functions are still being executed here,
   this is to be able to have a smoother transition between scenes)
6. Run the scene finalize/destroy function

After scene_runScene is done executing, the scene manager will go back to the start of it's infinite loop.
*/

scene_manager_t* sceneManager;

static const scene_main_func_t sSceneTable[] =
{
    (scene_main_func_t)logosc_run,
    (scene_main_func_t)titlesc_run
};

int fadeFrame = 0;
int fadeLenght; 
int fadeDirection;

static void updateFadeIn()
{
    int brightness;
    fadeFrame++;
    brightness = (fadeLenght - fadeFrame) * 16 / fadeLenght;
    brightness *= fadeDirection;
    setBrightness(3, brightness);
}

static void updateFadeOut()
{
    int brightness;
    fadeFrame++;
    brightness = (fadeFrame * 16) / fadeLenght;
    brightness *= fadeDirection;
    setBrightness(3, brightness);
}

static void setFadeLenght(int len, int direction)
{
    fadeLenght = len;
    fadeDirection = direction ? 1 : -1;
    fadeFrame = 0;
}

void scene_init(void)
{
    sceneManager = malloc(sizeof(scene_manager_t));
    sceneManager->currScene = -1;
    sceneManager->nextScene = SCENE_LOGO;
    sceneManager->prevScene = -1;
}

void scene_main(void)
{
    while(true)
    {
        sceneManager->prevScene = sceneManager->currScene;
        sceneManager->currScene = sceneManager->nextScene;
        sceneManager->nextScene = -1;
        sSceneTable[sceneManager->currScene](sceneManager);
    }
}

void scene_runScene(scene_manager_t* sceneManager, const scene_def_t* sceneDef)
{
    u32 frameCounter = 0;
    sceneDef->initFunc(sceneManager);
    setFadeLenght(sceneDef->fadeInLength, sceneDef->fadeInWhite);
    do
    {
        updateFadeIn();
        cothread_yield_irq(IRQ_VBLANK);
    }
    while(fadeFrame <= fadeLenght);
    do
    {
        frameCounter++;
        sceneDef->updateFunc(sceneManager, frameCounter);
        cothread_yield_irq(IRQ_VBLANK);
        sceneDef->vblankFunc(sceneManager, frameCounter);
    }
    while(sceneManager->nextScene == -1);
    setFadeLenght(sceneDef->fadeOutLength, sceneDef->fadeOutWhite);
    do
    {
        frameCounter++;
        sceneDef->updateFunc(sceneManager, frameCounter);
        updateFadeOut();
        cothread_yield_irq(IRQ_VBLANK);
        sceneDef->vblankFunc(sceneManager, frameCounter);
    }
    while(fadeFrame <= fadeLenght);
    sceneDef->finalizeFunc(sceneManager);
    cothread_yield_irq(IRQ_VBLANK);
}