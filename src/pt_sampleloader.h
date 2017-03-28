#ifndef __PT_SAMPLELOADER_H
#define __PT_SAMPLELOADER_H

#include <stdint.h>

void extLoadWAVSampleCallback(int8_t downSample);
int8_t saveSample(int8_t checkIfFileExist, int8_t giveNewFreeFilename);
int8_t loadSample(const char *fileName, char *entryName);

#endif
