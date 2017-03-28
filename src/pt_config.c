#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifndef _WIN32
#include <dlfcn.h>
#include <unistd.h>
#endif
#include "pt_helpers.h"
#include "pt_header.h"
#include "pt_config.h"
#include "pt_tables.h"
#include "pt_audio.h"
#include "pt_diskop.h"
#include "pt_config.h"
#include "pt_textout.h"

int8_t loadConfig(void)
{
    char cfgString[19], *configBuffer;
    uint8_t r, g, b, tmp8, iniConfigFound, ptConfigFound;
    uint16_t tmp16;
    int32_t lineLen;
    uint32_t configFileSize, i;
    FILE *configFile;

    iniConfigFound = true;
    ptConfigFound  = true;

    // set standard config values first
    ptConfig.pattDots          = false;
    ptConfig.dottedCenterFlag  = true;
    ptConfig.a500LowPassFilter = false;
    ptConfig.soundFrequency    = 48000;
    ptConfig.stereoSeparation  = 15;
    ptConfig.videoScaleFactor  = 2;
    ptConfig.blepSynthesis     = true;
    ptConfig.realVuMeters      = false;
    ptConfig.modDot            = false;
    ptConfig.accidental        = 0; // sharp
    ptConfig.quantizeValue     = 1;
    ptConfig.transDel          = false;
    ptConfig.blankZeroFlag     = false;

    memset(ptConfig.defaultDiskOpDir, 0, PATH_MAX_LEN + 1);

#ifndef _WIN32
    // first check for $HOME/.protracker
    sprintf(editor.tempPath, "%s/.protracker", getenv("HOME"));
    chdir(editor.tempPath);

    configFile = fopen("protracker.ini", "r");
    if (configFile == NULL)
    {
        changePathToExecutablePath();

        configFile = fopen("protracker.ini", "r");
        if (configFile == NULL)
            iniConfigFound = false;
    }
#else
    configFile = fopen("protracker.ini", "r");
    if (configFile == NULL)
        iniConfigFound = false;
#endif

    if (iniConfigFound)
    {
        fseek(configFile, 0, SEEK_END);
        configFileSize = ftell(configFile);
        rewind(configFile);

        configBuffer = (char *)(malloc(configFileSize));
        if (configBuffer == NULL)
        {
            fclose(configFile);
            showErrorMsgBox("Out of memory!");

            return (false);
        }

        fread(configBuffer, 1, configFileSize, configFile);
        fclose(configFile);

        configBuffer = strtok(configBuffer, "\n");
        while (configBuffer != NULL)
        {
            lineLen = strlen(configBuffer);

            // remove CR in CRLF linefeed (if present)
            if (configBuffer[lineLen - 1] == '\r')
            {
                configBuffer[lineLen - 1] = '\0';
                lineLen--;
            }

            // COMMENT OR CATEGORY
            if ((*configBuffer == ';') || (*configBuffer == '['))
            {
                configBuffer = strtok(NULL, "\n");
                continue;
            }

            // PATTDOTS
            else if (strncmp(configBuffer, "PATTDOTS=", 9) == 0)
            {
                     if (strncmp(&configBuffer[9], "TRUE",  4) == 0) ptConfig.pattDots = true;
                else if (strncmp(&configBuffer[9], "FALSE", 5) == 0) ptConfig.pattDots = false;
            }

            // BLANKZERO
            else if (strncmp(configBuffer, "BLANKZERO=", 10) == 0)
            {
                     if (strncmp(&configBuffer[10], "TRUE",  4) == 0) ptConfig.blankZeroFlag = true;
                else if (strncmp(&configBuffer[10], "FALSE", 5) == 0) ptConfig.blankZeroFlag = false;
            }

            // REALVUMETERS
            else if (strncmp(configBuffer, "REALVUMETERS=", 13) == 0)
            {
                     if (strncmp(&configBuffer[13], "TRUE",  4) == 0) ptConfig.realVuMeters = true;
                else if (strncmp(&configBuffer[13], "FALSE", 5) == 0) ptConfig.realVuMeters = false;
            }

            // ACCIDENTAL
            else if (strncmp(configBuffer, "ACCIDENTAL=", 11) == 0)
            {
                     if (strncmp(&configBuffer[11], "SHARP",  4) == 0) ptConfig.accidental = 0;
                else if (strncmp(&configBuffer[11], "FLAT",   5) == 0) ptConfig.accidental = 1;
            }

            // QUANTIZE
            else if (strncmp(configBuffer, "QUANTIZE=", 9) == 0)
            {
                if (configBuffer[9] != '\0')
                    ptConfig.quantizeValue = (int16_t)(CLAMP(atoi(&configBuffer[9]), 0, 63));
            }

            // TRANSDEL
            else if (strncmp(configBuffer, "TRANSDEL=", 9) == 0)
            {
                     if (strncmp(&configBuffer[9], "TRUE",  4) == 0) ptConfig.transDel = true;
                else if (strncmp(&configBuffer[9], "FALSE", 5) == 0) ptConfig.transDel = false;
            }

            // DOTTEDCENTER
            else if (strncmp(configBuffer, "DOTTEDCENTER=", 13) == 0)
            {
                     if (strncmp(&configBuffer[13], "TRUE",  4) == 0) ptConfig.dottedCenterFlag = true;
                else if (strncmp(&configBuffer[13], "FALSE", 5) == 0) ptConfig.dottedCenterFlag = false;
            }

            // MODDOT
            else if (strncmp(configBuffer, "MODDOT=", 7) == 0)
            {
                     if (strncmp(&configBuffer[7], "TRUE",  4) == 0) ptConfig.modDot = true;
                else if (strncmp(&configBuffer[7], "FALSE", 5) == 0) ptConfig.modDot = false;
            }

            // SCALE3X (deprecated)
            else if (strncmp(configBuffer, "SCALE3X=", 8) == 0)
            {
                     if (strncmp(&configBuffer[8], "TRUE",  4) == 0) ptConfig.videoScaleFactor = 3;
                else if (strncmp(&configBuffer[8], "FALSE", 5) == 0) ptConfig.videoScaleFactor = 2;
            }

            // VIDEOSCALE
            else if (strncmp(configBuffer, "VIDEOSCALE=", 11) == 0)
            {
                     if (strncmp(&configBuffer[11], "1X", 2) == 0) ptConfig.videoScaleFactor = 1;
                else if (strncmp(&configBuffer[11], "2X", 2) == 0) ptConfig.videoScaleFactor = 2;
                else if (strncmp(&configBuffer[11], "3X", 2) == 0) ptConfig.videoScaleFactor = 3;
                else if (strncmp(&configBuffer[11], "4X", 2) == 0) ptConfig.videoScaleFactor = 4;
            }

            // BLEP
            else if (strncmp(configBuffer, "BLEP=", 5) == 0)
            {
                     if (strncmp(&configBuffer[5], "TRUE",  4) == 0) ptConfig.blepSynthesis = true;
                else if (strncmp(&configBuffer[5], "FALSE", 5) == 0) ptConfig.blepSynthesis = false;
            }

            // DEFAULTDIR
            else if (strncmp(configBuffer, "DEFAULTDIR=", 11) == 0)
            {
                i = 11;
                while (configBuffer[i] == ' ') i++;                 // remove spaces before string (if present)
                while (configBuffer[lineLen - 1] == ' ') lineLen--; // remove spaces after string  (if present)

                lineLen -= i;

                if (lineLen > 0)
                    strncpy(ptConfig.defaultDiskOpDir, &configBuffer[i], (lineLen > PATH_MAX_LEN) ? PATH_MAX_LEN : lineLen);
            }

            // A500LOWPASSFILTER
            else if (strncmp(configBuffer, "A500LOWPASSFILTER=", 18) == 0)
            {
                     if (strncmp(&configBuffer[18], "TRUE",  4) == 0) ptConfig.a500LowPassFilter = true;
                else if (strncmp(&configBuffer[18], "FALSE", 5) == 0) ptConfig.a500LowPassFilter = false;
            }

            // A4000LOWPASSFILTER (deprecated, same as A500LOWPASSFILTER)
            else if (strncmp(configBuffer, "A4000LOWPASSFILTER=", 19) == 0)
            {
                     if (strncmp(&configBuffer[19], "TRUE",  4) == 0) ptConfig.a500LowPassFilter = true;
                else if (strncmp(&configBuffer[19], "FALSE", 5) == 0) ptConfig.a500LowPassFilter = false;
            }

            // FREQUENCY
            else if (strncmp(configBuffer, "FREQUENCY=", 10) == 0)
            {
                if (configBuffer[10] != '\0')
                    ptConfig.soundFrequency = (uint32_t)(CLAMP(atoi(&configBuffer[10]), 44100, 48000));
            }

            // STEREOSEPARATION
            else if (strncmp(configBuffer, "STEREOSEPARATION=", 17) == 0)
            {
                if (configBuffer[17] != '\0')
                    ptConfig.stereoSeparation = (int8_t)(CLAMP(atoi(&configBuffer[17]), 0, 100));
            }

            configBuffer = strtok(NULL, "\n");
        }

        free(configBuffer);
    }

    editor.ui.pattDots         = ptConfig.pattDots;
    editor.ui.dottedCenterFlag = ptConfig.dottedCenterFlag;
    editor.ui.videoScaleFactor = ptConfig.videoScaleFactor;
    editor.ui.realVuMeters     = ptConfig.realVuMeters;
    editor.diskop.modDot       = ptConfig.modDot;
    editor.blepSynthesis       = ptConfig.blepSynthesis;
    editor.ui.blankZeroFlag    = ptConfig.blankZeroFlag;
    editor.accidental          = ptConfig.accidental;
    editor.quantizeValue       = ptConfig.quantizeValue;
    editor.transDelFlag        = ptConfig.transDel;
    editor.oldTempo            = editor.initialTempo;

#ifndef _WIN32
    changePathToExecutablePath();
#endif

    // Load PT.Config if exists...
    configFile = fopen("PT.Config", "rb"); // PT didn't read PT.Config with no number, but let's support it
    if (configFile == NULL)
    {
        for (i = 0; i < 99; ++i)
        {
            sprintf(cfgString, "PT.Config-%02d", i);

            configFile = fopen(cfgString, "rb");
            if (configFile != NULL)
                break;
        }

        if (i == 99)
            ptConfigFound = false;
    }

    // change path to home for UNIX/BSD systems
#ifndef _WIN32
    changePathToHome();
#endif

    if (ptConfigFound)
    {
        fseek(configFile, 0, SEEK_END);
        configFileSize = ftell(configFile);

        if (configFileSize != 1024)
        {
            fclose(configFile);
            return (true);
        }

        fseek(configFile, 5, SEEK_SET);
        fread(cfgString, 1, 19, configFile);

        if (strncmp(cfgString, " Configuration File", 19))
        {
            fclose(configFile);
            return (true);
        }

        // most likely a valid PT.Config, let's load settings

        // Palette
        fseek(configFile, 154, SEEK_SET);
        for (i = 0; i < 8; ++i)
        {
            fread(&tmp16, 2, 1, configFile); // stored as Big-Endian
            if (!bigEndian) tmp16 = SWAP16(tmp16);

            r = ((tmp16 & 0x0F00) >> 8) * 17;
            g = ((tmp16 & 0x00F0) >> 4) * 17;
            b = ((tmp16 & 0x000F) >> 0) * 17;

            palette[i] = (r << 16) | (g << 8) | b;
        }

        // Transpose Delete (delete out of range notes on transposing)
        fseek(configFile, 174, SEEK_SET);
        fread(&tmp8, 1, 1, configFile);
        ptConfig.transDel = tmp8 ? true : false;
        editor.transDelFlag = ptConfig.transDel;

        // Note style (sharps/flats)
        fseek(configFile, 200, SEEK_SET);
        fread(&tmp8, 1, 1, configFile);
        ptConfig.accidental = tmp8 ? 1 : 0;
        editor.accidental = ptConfig.accidental;

        // Multi Mode Next
        fseek(configFile, 462, SEEK_SET);
        fread(&editor.multiModeNext[0], 1, 1, configFile);
        fread(&editor.multiModeNext[1], 1, 1, configFile);
        fread(&editor.multiModeNext[2], 1, 1, configFile);
        fread(&editor.multiModeNext[3], 1, 1, configFile);

        // Effect Macros
        fseek(configFile, 466, SEEK_SET);
        for (i = 0; i < 10; ++i)
        {
            fread(&tmp16, 2, 1, configFile); // stored as Big-Endian
            if (!bigEndian) tmp16 = SWAP16(tmp16);

            editor.effectMacros[i] = tmp16;
        }

        // Timing Mode (CIA/VBLANK)
        fseek(configFile, 487, SEEK_SET);
        fread(&tmp8, 1, 1, configFile);
        editor.timingMode = tmp8 ? TEMPO_MODE_CIA : TEMPO_MODE_VBLANK;

        // Blank Zeroes
        fseek(configFile, 490, SEEK_SET);
        fread(&tmp8, 1, 1, configFile);
        ptConfig.blankZeroFlag = tmp8 ? true : false;
        editor.ui.blankZeroFlag = ptConfig.blankZeroFlag;

        // Initial Tempo (don't load if timing is set to VBLANK)
        if (editor.timingMode == TEMPO_MODE_CIA)
        {
            fseek(configFile, 497, SEEK_SET);
            fread(&tmp8, 1, 1, configFile);
            if (tmp8 < 32) tmp8 = 32;
            editor.initialTempo = tmp8;
            editor.oldTempo = tmp8;
        }

        // Tuning Tone Note
        fseek(configFile, 501, SEEK_SET);
        fread(&tmp8, 1, 1, configFile);
        if (tmp8 > 35) tmp8 = 35;
        editor.tuningNote = tmp8;

        // Tuning Tone Volume
        fseek(configFile, 503, SEEK_SET);
        fread(&tmp8, 1, 1, configFile);
        if (tmp8 > 64) tmp8 = 64;
        editor.tuningVol = tmp8;

        // Initial Speed
        fseek(configFile, 545, SEEK_SET);
        fread(&tmp8, 1, 1, configFile);
        if (editor.timingMode == TEMPO_MODE_VBLANK)
        {
            editor.initialSpeed = tmp8;
        }
        else
        {
            if (tmp8 > 0x20) tmp8 = 0x20;
            editor.initialSpeed = tmp8;
        }

        // VU-Meter Colors
        fseek(configFile, 546, SEEK_SET);
        for (i = 0; i < 48; ++i)
        {
            fread(&vuMeterColors[i], 2, 1, configFile); // stored as Big-Endian
            if (!bigEndian) vuMeterColors[i] = SWAP16(vuMeterColors[i]);
        }

        // Spectrum Analyzer Colors
        fseek(configFile, 642, SEEK_SET);
        for (i = 0; i < 36; ++i)
        {
            fread(&analyzerColors[i], 2, 1, configFile); // stored as Big-Endian
            if (!bigEndian) analyzerColors[i] = SWAP16(analyzerColors[i]);
        }

        fclose(configFile);
    }

    if (iniConfigFound || ptConfigFound)
        editor.configFound = true;

    return (true);
}
