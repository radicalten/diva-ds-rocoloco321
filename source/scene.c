#include <nds.h>
#include <nds/cothread.h>
#include <stdlib.h>

#include "logo/logoScene.h"
#include "title/titleScene.h"
#include "scene.h"

//Hyper simplified version of the MKDS scene manager

scene_manager_t* sceneManager;

static const process_main_func_t sSceneTable[] =
{
    (process_main_func_t)logosc_run,
    (process_main_func_t)titlesc_run
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