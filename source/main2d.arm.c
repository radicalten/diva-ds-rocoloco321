#include <nds.h>
#include "archive.h"
#include <fontConsole.h>

#include "main2d.h"

m2d_fonts_t gFonts;

void m2d_getNscExt(char* dst, const char* path)
{
    sprintf(dst, "%s.nsc.bin", path);
}

void m2d_getNcgExt(char* dst, const char* path)
{
    sprintf(dst, "%s.ncg.bin", path);
}

void m2d_getNclExt(char* dst, const char* path)
{
    sprintf(dst, "%s.ncl.bin", path);
}

void m2d_loadFonts()
{
    gFonts.fonts[FONT_CONSOLE].gfx = (u16*)fontConsoleTiles;
	gFonts.fonts[FONT_CONSOLE].pal = (u16*)fontConsolePal;
	gFonts.fonts[FONT_CONSOLE].numChars = 95;
	gFonts.fonts[FONT_CONSOLE].numColors = fontConsolePalLen / 2;
	gFonts.fonts[FONT_CONSOLE].bpp = 4;
	gFonts.fonts[FONT_CONSOLE].asciiOffset = 32;
}

void m2d_loadDisplayConfig(display_config_t* config)
{
    videoSetMode(config->videoMode);
    videoSetModeSub(config->videoModeSub);
    vramSetBankA(config->vramA);
    vramSetBankB(config->vramB);
    vramSetBankC(config->vramC);
    vramSetBankD(config->vramD);
    vramSetBankE(config->vramE);
    vramSetBankF(config->vramF);
    vramSetBankG(config->vramG);
    vramSetBankH(config->vramH);
    vramSetBankI(config->vramI);
}

void m2d_initBackground(m2d_bg_res_t* bg,int bgId, const char* path)
{
    char chrPath[255];
    char scrPath[255];
    m2d_getNcgExt(chrPath, path);
    m2d_getNscExt(scrPath, path);
    bg->chr = loadArchiveEx(chrPath);
    if(bg->chr.data == NULL)
    {
        libndsCrash("There was an error loading the character file!");
    }
    DC_FlushRange(bg->chr.data, bg->chr.size);
    dmaCopy(bg->chr.data,bgGetGfxPtr(bgId), bg->chr.size);

    bg->scr = loadArchiveEx(scrPath);
    if(bg->scr.data == NULL)
    {
        libndsCrash("There was an error loading the screen file!");
    }
    DC_FlushRange(bg->scr.data, bg->scr.size);
    dmaCopy(bg->scr.data,bgGetMapPtr(bgId), bg->scr.size);
}

void m2d_initBgPalette(m2d_bgPal_res_t* file, int screen, const char* path)
{
    char palPath[255];
    m2d_getNclExt(palPath, path);
    file->pal = loadArchiveEx(palPath);
    if(file->pal.data == NULL)
    {
        libndsCrash("There was an error loading the palette file!");
    }
    DC_FlushRange(file->pal.data, file->pal.size);
    if(screen)
    {
        dmaCopy(file->pal.data, BG_PALETTE_SUB, file->pal.size);
    }
    else
    {
        dmaCopy(file->pal.data, BG_PALETTE, file->pal.size);
    }
}

void m2d_destroyBackground(m2d_bg_res_t* bg)
{
    unloadArchiveEx(&bg->chr);
    unloadArchiveEx(&bg->scr);
}

void m2d_destroyBgPal(m2d_bgPal_res_t* file)
{
    unloadArchiveEx(&file->pal);
}