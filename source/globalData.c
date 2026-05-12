#include <nds.h>
#include <font.h>
#include <stdio.h>
#include "globalData.h"

bool gFatEnabled; //Todo: move to archive.h
ConsoleFont font;

void initFont()
{
	font.gfx = (u16*)fontTiles;
	font.pal = (u16*)fontPal;
	font.numChars = 95;
	font.numColors = fontPalLen / 2;
	font.bpp = 4;
	font.asciiOffset = 32;
}