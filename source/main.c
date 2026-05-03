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
#include "main.h"


void vblank_interrupt() {
    /*
	if (shared_play) {
		shared_play->frame();
	} else if (shared_menu) {
		shared_menu->frame();
	}
        */
}

int main(int argc, char **argv)
{
    srand(time(NULL));
	videoSetMode(MODE_5_2D);
	videoSetModeSub(MODE_5_2D);
	bgExtPaletteEnable();
	bgExtPaletteEnableSub();
	vramSetBankA(VRAM_A_MAIN_BG_0x06040000);
	vramSetBankB(VRAM_B_MAIN_SPRITE);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	vramSetBankD(VRAM_D_SUB_SPRITE);
	vramSetBankE(VRAM_E_MAIN_BG);
	vramSetBankF(VRAM_F_LCD); //bg ext palette
	vramSetBankH(VRAM_H_LCD); //bg ext palette sub
	oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
	oamInit(&oamSub, SpriteMapping_Bmp_1D_128, false);

    //set up debug console
	PrintConsole *console = consoleInit(0, 0, BgType_Text4bpp, BgSize_T_256x256, 0, 1, false, false);
	ConsoleFont font;
	font.gfx = (u16*)fontTiles;
	font.pal = (u16*)fontPal;
	font.numChars = 95;
	font.numColors = fontPalLen / 2;
	font.bpp = 4;
	font.asciiOffset = 32;
	consoleSetFont(console, &font);
	bgSetPriority(console->bgId, 0);
	gConsoleBgId = console->bgId;


    if(!nitroFSInit(NULL))
    {
        printf("Unable to init NitroFS!\n");
        perror("nitroFSInit()");
        error();
    }

    if (!ddsCheck()) {
		printf("Try placing dds.nds at root of your sdcard and create a dds folder\n");
		printf("Also check if your sd card is write protected\n");
	}

    //LibPrint went here, I'm skipping it

    setBackdropColor(ARGB16(1, 29, 29, 29));
	setBackdropColorSub(ARGB16(1, 29, 29, 29));
	gBgId = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 16, 0);
	bgSetPriority(gBgId, 2);

    config_load();
    
    if (gSettings.debug) 
    {
		bgShow(gConsoleBgId);
	} else 
    {
		bgHide(gConsoleBgId);
	}

    //A check for nosgba was here

    irqSet(IRQ_VBLANK, vblank_interrupt);


    while(1)
    {
        
    }
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
        DIR* dir = opendir("/dds");
        if (!dir) {
            printf("Can't find dds folder\n");
            printf("Trying to create it\n");
            mkdir("/dds",777);
            dir = opendir("/dds"); 
            if (!dir) {
                printf("Couldn't create dds folder\n");
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