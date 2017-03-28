#ifndef __PT_MODLOADER_H
#define __PT_MODLOADER_H

#include <stdint.h>
#include "pt_header.h"

module_t *createNewMod(void);
int8_t saveModule(int8_t checkIfFileExist, int8_t giveNewFreeFilename);
int8_t modSave(char *fileName);
module_t *modLoad(const char *fileName);
void setupNewMod(void);

void diskOpLoadFile(uint32_t fileEntryRow); // pt_mouse.c

#endif
