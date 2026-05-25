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

//Special thanks to Garhoohin for reverse engineering the Hudson cell format

typedef struct OAM_OBJ_ {
	//attr 0
	union {
		u16 attr0;
		struct {				
			u16 y         : 8;
			u16 affine    : 1;
			u16 disable   : 1; //or doubleSize
			u16 mode      : 2;
			u16 mosaic    : 1;
			u16 depth     : 1;
			u16 shape     : 2;
		};
	};
	
	//attr 1
	union {
		u16 attr1;
		struct {
			u16 x         : 9;
			u16 matrix    : 5; //or HV flip
			u16 size      : 2;
		};
	};
	
	//attr 2
	union {
		u16 attr2;
		struct {
			u16 character : 10;
			u16 priority  : 2;
			u16 palette   : 4;
		};
	};
} OAM_OBJ;

typedef struct OAM_BUFFER_OBJ_ {
	OAM_OBJ obj;
	u16 p;        //!
} OAM_BUFFER_OBJ;

typedef struct CELL_BANK_ {
	u32 nCells;
	u32 cellOffsets[0];
} CELL_BANK;

typedef struct CELL_OBJ_ {
	OAM_OBJ obj;
	s16 x;
	s16 y;
} CELL_OBJ;

typedef struct CELL_ {
	u16 nObj;
	CELL_OBJ obj[0];
} CELL;

typedef struct 
{
    file_t chr;
    file_t pal;
    CELL_BANK* obj;
}m2d_obj_res_t;


extern m2d_fonts_t gFonts;
extern int objCount;
extern int objCountSub;

void m2d_loadFonts();
void m2d_loadDisplayConfig(display_config_t* config);
void m2d_initBackground(m2d_bg_res_t* bg,int bgId, const char* path);
void m2d_initBgPalette(m2d_bgPal_res_t* file, int screen, const char* path);
void m2d_loadInitCell(m2d_obj_res_t* res, int screen, const char* path);
void m2d_destroyBackground(m2d_bg_res_t* bg);
void m2d_destroyBgPal(m2d_bgPal_res_t* pal);
void m2d_destroyObj(m2d_obj_res_t* res);
void m2d_disableOam(int scr);
void m2d_enableOam(int scr, int mappingMode);
void m2d_clearOam();
void m2d_renderCell(CELL* cell, int screen, int x, int y);
void m2d_prepareBuffers();
void m2d_applyBuffers();

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


inline CELL* m2d_getCell(CELL_BANK* bnk, int cellId)
{
    u32 offset = bnk->cellOffsets[cellId];
    u8* base = (u8*)bnk;
    return (CELL*)(base + offset + 4);
}