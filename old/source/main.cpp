#include <nds.h>
#include <time.h>

#include <fat.h>
#include <sys/dir.h>
#include <unistd.h>

//#include <mad.h>
#//include <tremor/ivorbiscodec.h>
//#include <tremor/ivorbisfile.h>
#include <zlib.h>
#include <png.h>
#include <gba-jpeg-decode.h>

#include <iostream>
#include <string>
#include <vector>

#include "main.h"
#include "globals.h"
#include "notice.h"
#include "pause.h"
#include <font.h>

using namespace std;

void vblank_interrupt() {
	if (shared_play) {
		shared_play->frame();
	} else if (shared_menu) {
		shared_menu->frame();
	}
}

int main(){
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
	consoleid = console->bgId;

	if (!fatInitDefault()) {
		printf("Failed to load libfat\n");
		printf("If you're using a flashcard, try to patch dds.nds with a DLDI patch\n");
		error();
	}

	if (!ddsCheck()) {
		printf("Try placing dds.nds at root of your sdcard and create a dds folder\n");
		printf("Also check if your sd card is write protected\n");
		error();
	}

	//library versions
	//printf("libmad " << MAD_VERSION_MAJOR << "." << MAD_VERSION_MINOR << "." << MAD_VERSION_PATC\nH);
	//printf("libogg 1.3.4\n");
	printf("libtremor lowmem 1.0.2\n");
	printf("zlib " << ZLIB_VERSIO\nN);
	printf("libpng " << PNG_LIBPNG_VER_STRIN\nG);
	//printf("libjpeg-turbo " << JPEG_LIB_VERSIO\nN);

	setBackdropColor(ARGB16(1, 29, 29, 29));
	setBackdropColorSub(ARGB16(1, 29, 29, 29));
	bgid = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 16, 0);
	bgSetPriority(bgid, 2);

	ConfigLoad();

	if (settings.debug) {
		bgShow(consoleid);
	} else {
		bgHide(consoleid);
	}

	//check if running on no$gba
	if (strncmp((char*)0x4FFFA00, "no$gba", 6) == 0) {
		nocash = true;
		printf("Running on no$gba\n");
	}

	//play frame
	irqSet(IRQ_VBLANK, vblank_interrupt);


	while (1) {
		switch (state) {
			case (0): {
				Menu menu;
				menu.loop();
			}
			break;
			case (1): {
				Play play;
				play.loop();
			}
			break;
			case (2): {
				Notice notice;
				notice.loop();
			}
			break;
			case (3): {
				Pause pause;
				pause.loop();
			}
			break;
		}
	}
	return 0;
}

void error() {
	while (1) {
		swiWaitForVBlank();
	}
}

bool ddsCheck() {
	DIR* dir = opendir("/dds");
	if (!dir) {
		printf("Can't find dds folder\n");
		printf("Trying to create it\n");
		//mkdir("/dds",777);
		dir = opendir("/dds"); 
		if (!dir) {
			printf("Couldn't create dds folder\n");
			return false;
		} 
	}
	if (dir) {
		closedir(dir);
	}
	return true;
}