#include <nds.h>
#include <stdio.h>
#include <time.h>
#include <font.h>
#include <fat.h>
#include <filesystem.h>
#include <sys/stat.h>
#include <sys/dir.h>

#include "config.h"
#include "globalData.h"
#include "saveData.h"
#include "scene.h"
#include "main.h"


void vblank_interrupt() 
{
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    
    initFont();
    PrintConsole* c = malloc(sizeof(PrintConsole));
    consoleInit(c, 3, BgType_Text4bpp, BgSize_T_256x256, 2, 0, false, false);
    consoleSetFont(c, &font);

    swiWaitForVBlank();

    if(!nitroFSInit(NULL))
    {
        printf("Unable to init NitroFS!\n");
        perror("nitroFSInit()");
        error();
    }

    if (!ddsCheck()) {
		printf("Try placing divaDS.nds at root of your sdcard and create a divaDS folder\n");
		printf("Also check if your sd card is write protected\n");
	}

    config_load();

    irqSet(IRQ_VBLANK, vblank_interrupt);
    free(c);

    setBrightness(3, 16);

    saveData_init();

    scene_init();
    scene_main();

}

void error() {
	while (1) {
		swiWaitForVBlank();
	}
}

bool ddsCheck() {
    if(!chdir("fat:/"))
    {
        gFatEnabled = true;
        DIR* dir = opendir("/divaDS");
        if (!dir) {
            printf("Can't find divaDS folder\n");
            printf("Trying to create it\n");
            mkdir("/divaDS",777);
            dir = opendir("/divaDS"); 
            if (!dir) {
                printf("Couldn't create divaDS folder\n");
                return false;
            } 
        }
        if (dir) {
            closedir(dir);
        }
    }
    else
    {
        printf("Couldn't access SD card\n Saving is not supported!\n");
        perror("chdir() ");
        gFatEnabled = false;
    }
    chdir("nitro:/");
	return true;
}