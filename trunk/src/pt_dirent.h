#ifndef __PT_DIRENT_H
#define __PT_DIRENT_H

#include <errno.h>
#include <sys/stat.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#include <stdint.h>

#ifndef FILENAME_MAX
#define FILENAME_MAX 260
#endif

#ifndef DT_DIR
#define DT_DIR 0x00
#define DT_REG 0x10
#endif

typedef struct dirent
{
    char d_name[FILENAME_MAX];
    uint16_t d_namlen;
    uint32_t d_type;
} dirent;

typedef struct DIR
{
    dirent fd;
    HANDLE fHandle;
    WIN32_FIND_DATAA fData;
} DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int32_t closedir(DIR *dirp);
#else
#include <dirent.h>
#endif

#endif
