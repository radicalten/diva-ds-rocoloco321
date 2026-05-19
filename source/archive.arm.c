#include <nds.h>
#include <filesystem.h>
#include <stdlib.h>

#include "archive.h"

file_t loadArchiveEx(const char* path)
{
    FILE* fp;
    file_t file = {NULL, 0};
    fp = fopen(path, "rb");
    if(fp != NULL)
    {
        if(fseek(fp,0,SEEK_END)!=0) libndsCrash("FseekError");
        file.size = ftell(fp);
        rewind(fp);
        file.data = malloc(file.size);
        if(file.data != NULL)
        {
            fread(file.data,file.size,1,fp);
            DC_InvalidateRange(file.data, file.size);
        }
        fclose(fp);
    }
    return file;
}

void unloadArchiveEx(file_t* file)
{
    if(file->data != NULL)
    {
        free(file->data);
        file->size = 0;
    }
}

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
    if(data != NULL)
    {
        free(data);
    }
}