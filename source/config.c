#include <nds.h>
#include <fat.h>

#include "math/math.h"
#include "config.h"
#include "globalData.h"

settings_t gSettings;

void config_load()
{
    if(gFatEnabled)
    {
        chdir("fat:/");
        
        FILE* file = fopen(CONFIGPATH, "rb");
        if(file)
        {
            printf("Reading settings file.\n");
            fread(&gSettings.speed, 4, 1, file);
            fread(&gSettings.opacity, 4, 1, file);
            fread(&gSettings.cache, 1, 1, file);
            fread(&gSettings.cache_bg, 1, 1, file);
            fread(&gSettings.debug, 1, 1, file);
            fread(&gSettings.mines, 1, 1, file);
            fclose(file);
            config_validate();
        }
        else
        {
            printf("Couldn't find settings file\nCreating a new one\n");
            config_init();
            config_save();
        }
        chdir("nitro:/");
    }
    else
    {
        config_init();
    }
}

void config_save()
{
    if(gFatEnabled)
    {
        chdir("fat:/");
        config_validate();
        FILE* file = fopen(CONFIGPATH, "wb");
	    if (file) 
        {
            printf("Saving config\n");
            fwrite(&gSettings.speed, 4, 1, file);
            fwrite(&gSettings.opacity, 4, 1, file);
            fwrite(&gSettings.cache, 1, 1, file);
            fwrite(&gSettings.cache_bg, 1, 1, file);
            fwrite(&gSettings.debug, 1, 1, file);
            fwrite(&gSettings.mines, 1, 1, file);
            fclose(file);
    	}
        chdir("nitro:/");
    }
    
}

void config_init()
{
    gSettings.speed = 4;
    gSettings.opacity = 6;
    gSettings.cache = true;
    gSettings.cache_bg = false;
    gSettings.mines = false;
    gSettings.debug = false;
}

void config_validate()
{

}