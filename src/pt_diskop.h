#ifndef __PT_DISKOP_H
#define __PT_DISKOP_H

#include <stdint.h>

enum
{
    DISKOP_FILE     = 0,
    DISKOP_DIR      = 1,
    DISKOP_NO_CACHE = 0,
    DISKOP_CACHE    = 1
};

#define DISKOP_LIST_SIZE 10

void handleEntryJumping(char jumpToChar);
int8_t diskOpEntryIsEmpty(int32_t fileIndex);
int8_t diskOpEntryIsDir(int32_t fileIndex);
char *diskOpGetEntry(int32_t fileIndex);
int8_t diskOpSetPath(const char *path, uint8_t cache);
void diskOpSetInitPath(void);
void diskOpRenderFileList(uint32_t *frameBuffer);
int8_t allocDiskOpVars(void);
void deAllocDiskOpVars(void);
void freeDiskOpFileMem(void);

#endif
