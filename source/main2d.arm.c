#include <nds.h>
#include "archive.h"
#include <fontConsole.h>

#include "main2d.h"

m2d_fonts_t gFonts;

int objCount = 0;
int objCountSub = 0;
int mapMode;
int mapModeSub;
OAM_BUFFER_OBJ mainOam[128] = {0};
OAM_BUFFER_OBJ subOam[128] = {0};

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

void m2d_getNceExt(char* dst, const char* path)
{
    sprintf(dst, "%s.nce.bin", path);
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

//TODO:Split this into a load + init functions to allow backgrounds with shared tiles 
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
    if(m2d_getBgSize(bgId) == BgSize_T_512x256) //512x256 requires special handling
    {
        int nrLines = div32(bg->scr.size, 0x80);
        for(int i=0;i<nrLines;i++)
        {
            dmaCopy(bg->scr.data + (0x80*i), bgGetMapPtr(bgId) + (0x20*i), 0x40); //map ptr should add 0x40 instead of 0x20 but idk why isn't it doing that
            dmaCopy(bg->scr.data + 0x40 + (0x80*i), bgGetMapPtr(bgId) + (0x20*i) + 0x400, 0x40);
        }
    }
    else
    {
        dmaCopy(bg->scr.data,bgGetMapPtr(bgId), bg->scr.size);
    }
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

void m2d_loadInitCell(m2d_obj_res_t* res, int screen, const char* path)
{
    char chrPath[255];
    char palPath[255];
    char objPath[255];
    m2d_getNcgExt(chrPath, path);
    m2d_getNclExt(palPath, path);
    m2d_getNceExt(objPath, path);
    res->chr = loadArchiveEx(chrPath);
    if(res->chr.data == NULL)
    {
        libndsCrash("There was an error loading the character file!");
    }
    res->pal = loadArchiveEx(palPath);
    if(res->pal.data == NULL)
    {
        libndsCrash("There was an error loading the palette file!");
    }
    res->obj = loadArchive(objPath);
    if(res->obj == NULL)
    {
        libndsCrash("There was an error loading the cell file!");
    }
    
    DC_FlushRange(res->chr.data, res->chr.size);
    DC_FlushRange(res->pal.data, res->pal.size);
    if (!screen)
    {
        dmaCopy(res->chr.data, SPRITE_GFX, res->chr.size);
        dmaCopy(res->pal.data, SPRITE_PALETTE, res->pal.size);
    }
    else
    {
        dmaCopy(res->chr.data, SPRITE_GFX_SUB, res->chr.size);
        dmaCopy(res->pal.data, SPRITE_PALETTE_SUB, res->pal.size);       
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

void m2d_destroyObj(m2d_obj_res_t* res)
{
    unloadArchiveEx(&res->chr);
    unloadArchiveEx(&res->pal);
    unloadArchive(&res->obj);
}

void m2d_disableOam(int scr)
{
    if (!scr)
        REG_DISPCNT &= ~DISPLAY_SPR_ACTIVE;
    else
        REG_DISPCNT_SUB &= ~DISPLAY_SPR_ACTIVE;
        
}

void m2d_enableOam(int scr, int mappingMode)
{
    if (!scr)
    {
        REG_DISPCNT |= DISPLAY_SPR_ACTIVE | (mappingMode & 0xffffff0);
        mapMode = mappingMode;
    }
    else
    {
        REG_DISPCNT_SUB |= DISPLAY_SPR_ACTIVE | (mappingMode & 0xffffff0);
        mapModeSub = mappingMode;
    }
}


void m2d_clearOam()
{
    objCount = 0;
    objCountSub = 0;
    memset(&mainOam, 0, sizeof(mainOam));
    memset(&subOam, 0, sizeof(subOam));
}

void m2d_renderCell(CELL* cell, int screen, int x, int y)
{
    int i;
    int nrOam = cell->nObj;
    if (!screen)
    {
        if(objCount + nrOam >= 128)
        {
            libndsCrash("The cell no longer fits in the Main OAM Buffer!");
        }
        for(i=0;i<nrOam;i++)
        {
            memcpy(&mainOam[objCount+i].obj, &cell->obj[i], sizeof(OAM_OBJ));
            mainOam[objCount+i].obj.x += x + cell->obj[i].x;
            mainOam[objCount+i].obj.y += y + cell->obj[i].y;
        }
        objCount++;
    }
    else
    {
        if(objCountSub + nrOam >= 128)
        {
            libndsCrash("The cell no longer fits in the Sub OAM Buffer!");
        }
        for(i=0;i<nrOam;i++)
        {
            //subOam[objCountSub+i].obj = cell->obj[i].obj;
            memcpy(&subOam[objCountSub+i].obj, &cell->obj[i], sizeof(OAM_OBJ));
            subOam[objCountSub+i].obj.x += x + cell->obj[i].x;
            subOam[objCountSub+i].obj.y += y + cell->obj[i].y;
        }
        objCountSub++;
    } 
}

void m2d_prepareBuffers()
{
    int i; 
    DC_FlushRange(&mainOam, sizeof(mainOam));
    DC_FlushRange(&subOam, sizeof(subOam));
    for(i=0;i<128-objCount;i++)
    {
        mainOam[i+objCount].obj.x = -128;
        mainOam[i+objCount].obj.y = -32;
    }
    for(i=0;i<128-objCountSub;i++)
    {
        subOam[i+objCountSub].obj.x = -128;
        subOam[i+objCountSub].obj.y = -32;
    }
}

void m2d_applyBuffers()
{
    DC_FlushRange(&mainOam, sizeof(mainOam));
    DC_FlushRange(&subOam, sizeof(subOam));
    dmaCopy(&mainOam, OAM, sizeof(mainOam));
    dmaCopy(&subOam, OAM_SUB, sizeof(subOam));
}
