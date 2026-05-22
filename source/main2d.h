#pragma once
#include <nds.h>
#include <stdio.h>
#include "archive.h"


typedef enum
{
    FONT_CONSOLE
} Fonts;

typedef struct
{
    ConsoleFont fonts[1];
}m2d_fonts_t;

typedef struct
{
    file_t chr;
    file_t scr;
}m2d_bg_res_t;

typedef struct 
{
    file_t pal;
}m2d_bgPal_res_t;


typedef struct 
{
    int videoMode;
    int videoModeSub;
    int vramA;
    int vramB;
    int vramC;
    int vramD;
    int vramE;
    int vramF;
    int vramG;
    int vramH;
    int vramI;
}display_config_t;


extern m2d_fonts_t gFonts;

void m2d_loadFonts();
void m2d_loadDisplayConfig(display_config_t* config);
void m2d_initBackground(m2d_bg_res_t* bg,int bgId, const char* path);
void m2d_initBgPalette(m2d_bgPal_res_t* file, int screen, const char* path);
void m2d_destroyBackground(m2d_bg_res_t* bg);
void m2d_destroyBgPal(m2d_bgPal_res_t* pal);

inline void m2d_setBlendAlpha(u32 src, u32 dst, u32 evA, u32 evB)
{
    REG_BLDCNT = BLEND_ALPHA | src | dst;
    REG_BLDALPHA = BLDALPHA_EVA(evA) | BLDALPHA_EVB(evB);
}

inline void m2d_setBlendAlphaSub(u32 src, u32 dst, u32 evA, u32 evB)
{
    REG_BLDCNT_SUB = BLEND_ALPHA | src | dst;
    REG_BLDALPHA_SUB = BLDALPHA_EVA(evA) | BLDALPHA_EVB(evB);
}

inline void m2d_disableBlendAlpha()
{
    REG_BLDCNT = 0;
}

inline int m2d_getBgSize(int id)
{
    return bgState[id].size;
}
