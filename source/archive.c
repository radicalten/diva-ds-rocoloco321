#include <nds.h>
#include <filesystem.h>
#include <stdlib.h>

#include "archive.h"

void* loadArchive(const char* path)
{
    FILE* fp;
    size_t size;
    void* data = NULL;
    fp = fopen(path, "rb");
    if(fp != NULL)
    {
        fseek(fp,0,SEEK_END);
        size = ftell(fp);
        rewind(fp);
        data = malloc(size);
        if(data != NULL)
        {
            fread(data,size,1,fp);
        }
        fclose(fp);
    }
    return data;
}

void unloadArchive(void* data)
{
    free(data);
}