#ifndef __PT_CONFIG_H
#define __PT_CONFIG_H

#include <stdint.h>

struct ptConfig_t
{
    char *defaultDiskOpDir;
    int8_t dottedCenterFlag, pattDots, a500LowPassFilter;
    int8_t stereoSeparation, videoScaleFactor, blepSynthesis;
    int8_t modDot, accidental, blankZeroFlag, realVuMeters;
    int8_t transDel;
    int16_t quantizeValue;
    uint32_t soundFrequency;
} ptConfig;

int8_t loadConfig();

#endif
