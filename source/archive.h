#pragma once

typedef struct
{
    void* data;
    size_t size;
} file_t;

void* loadArchive(const char* path);
file_t loadArchiveEx(const char* path);
void unloadArchive(void* data);
void unloadArchiveEx(file_t* data);


