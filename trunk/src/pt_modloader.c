#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h> // for toupper()/tolower()
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pt_palette.h"
#include "pt_header.h"
#include "pt_sampler.h"
#include "pt_textout.h"
#include "pt_audio.h"
#include "pt_helpers.h"
#include "pt_terminal.h"
#include "pt_visuals.h"

typedef struct mem_t
{
    int8_t _eof;
    uint8_t *_ptr, *_base;
    uint32_t _cnt, _bufsiz;
} mem_t;

static mem_t *mopen(const uint8_t *src, uint32_t length);
static void mclose(mem_t **buf);
static int32_t mgetc(mem_t *buf);
static size_t mread(void *buffer, size_t size, size_t count, mem_t *buf);
static void mseek(mem_t *buf, int32_t offset, int32_t whence);

uint8_t ppdecrunch(uint8_t *src, uint8_t *dst, uint8_t *offsetLens, uint32_t srcLen, uint32_t dstLen, uint8_t skipBits);

module_t *createNewMod(void)
{
    uint8_t i;
    module_t *newMod;

    newMod = (module_t *)(calloc(1, sizeof (module_t)));
    if (newMod == NULL)
    {
        showErrorMsgBox("Out of memory!");
        return (false);
    }

    for (i = 0; i < MAX_PATTERNS; ++i)
    {
        newMod->patterns[i] = (note_t *)(calloc(MOD_ROWS * AMIGA_VOICES, sizeof (note_t)));
        if (newMod->patterns[i] == NULL)
        {
            showErrorMsgBox("Out of memory!");
            return (false);
        }
    }

    newMod->sampleData = (int8_t *)(calloc(MOD_SAMPLES, MAX_SAMPLE_LEN));
    if (newMod->sampleData == NULL)
    {
        showErrorMsgBox("Out of memory!");
        return (false);
    }

    newMod->head.orderCount   = 1;
    newMod->head.patternCount = 1;

    for (i = 0; i < MOD_SAMPLES; ++i)
    {
        newMod->samples[i].offset = MAX_SAMPLE_LEN * i;
        newMod->samples[i].loopLength = 2;

        // setup GUI text pointers
        newMod->samples[i].volumeDisp     = &newMod->samples[i].volume;
        newMod->samples[i].lengthDisp     = &newMod->samples[i].length;
        newMod->samples[i].loopStartDisp  = &newMod->samples[i].loopStart;
        newMod->samples[i].loopLengthDisp = &newMod->samples[i].loopLength;
    }

    // setup GUI text pointers
    editor.currEditPatternDisp = &newMod->currPattern;
    editor.currPosDisp         = &newMod->currOrder;
    editor.currPatternDisp     = &newMod->head.order[0];
    editor.currPosEdPattDisp   = &newMod->head.order[0];
    editor.currLengthDisp      = &newMod->head.orderCount;

    editor.ui.updateSongSize = true;

    return (newMod);
}

int8_t modSave(char *fileName)
{
    int16_t tempPatternCount;
    int32_t i, nameTextLen;
    uint32_t tempLoopLength, tempLoopStart, j, k;
    note_t tmp;
    FILE *fmodule;

    tempPatternCount = 0;

    fmodule = fopen(fileName, "wb");
    if (fmodule == NULL)
    {
        displayErrorMsg("FILE I/O ERROR !");
        terminalPrintf("Module saving failed: file input/output error\n");

        return (false);
    }

    for (i = 0; i < 20; ++i)
        fputc(tolower(modEntry->head.moduleTitle[i]), fmodule);

    for (i = 0; i < MOD_SAMPLES; ++i)
    {
        for (j = 0; j < 22; ++j)
            fputc(tolower(modEntry->samples[i].text[j]), fmodule);

        fputc(modEntry->samples[i].length >> 9, fmodule);
        fputc(modEntry->samples[i].length >> 1, fmodule);
        fputc(modEntry->samples[i].fineTune & 0x0F, fmodule);
        fputc((modEntry->samples[i].volume > 64) ? 64 : modEntry->samples[i].volume, fmodule);

        tempLoopLength = modEntry->samples[i].loopLength;
        if (tempLoopLength < 2)
            tempLoopLength = 2;

        tempLoopStart = modEntry->samples[i].loopStart;
        if (tempLoopLength == 2)
            tempLoopStart = 0;

        fputc(tempLoopStart  >> 9, fmodule);
        fputc(tempLoopStart  >> 1, fmodule);
        fputc(tempLoopLength >> 9, fmodule);
        fputc(tempLoopLength >> 1, fmodule);
    }

    fputc(modEntry->head.orderCount & 0x00FF, fmodule);
    fputc(0x7F, fmodule); // ProTracker puts 0x7F at this place (restart pos)

    for (i = 0; i < MOD_ORDERS; ++i)
        fputc(modEntry->head.order[i] & 0x00FF, fmodule);

    tempPatternCount = 0;
    for (i = 0; i < MOD_ORDERS; ++i)
    {
        if (tempPatternCount < modEntry->head.order[i])
            tempPatternCount = modEntry->head.order[i];
    }

    if (++tempPatternCount > MAX_PATTERNS)
          tempPatternCount = MAX_PATTERNS;

    fwrite((tempPatternCount <= 64) ? "M.K." : "M!K!", 1, 4, fmodule);

    for (i = 0; i < tempPatternCount; ++i)
    {
        for (j = 0; j < MOD_ROWS; ++j)
        {
            for (k = 0; k < AMIGA_VOICES; ++k)
            {
                tmp = modEntry->patterns[i][(j * AMIGA_VOICES) + k];

                fputc((tmp.sample & 0xF0) | ((tmp.period >> 8) & 0x0F), fmodule);
                fputc(tmp.period & 0x00FF, fmodule);
                fputc(((tmp.sample << 4) & 0xF0) | (tmp.command & 0x0F), fmodule);
                fputc(tmp.param, fmodule);
            }
        }
    }

    for (i = 0; i < MOD_SAMPLES; ++i)
    {
        // Amiga ProTracker "BEEEEEEEEP" sample fix
        if ((modEntry->samples[i].length >= 2) && (modEntry->samples[i].loopLength == 2))
        {
            fputc(0, fmodule);
            fputc(0, fmodule);

            k = modEntry->samples[i].length;
            for (j = 2; j < k; ++j)
                fputc(modEntry->sampleData[modEntry->samples[i].offset + j], fmodule);
        }
        else
        {
            fwrite(&modEntry->sampleData[MAX_SAMPLE_LEN * i], 1, modEntry->samples[i].length, fmodule);
        }
    }

    fclose(fmodule);

    displayMsg("MODULE SAVED !");
    setMsgPointer();

    editor.diskop.cached = false;
    if (editor.ui.diskOpScreenShown)
        editor.ui.updateDiskOpFileList = true;

    updateWindowTitle(MOD_NOT_MODIFIED);

    if (moduleNameIsEmpty(modEntry->head.moduleTitle))
    {
        terminalPrintf("Module \"untitled\" saved\n");
    }
    else
    {
        terminalPrintf("Module \"");

        nameTextLen = 20;
        for (i = 19; i >= 0; --i)
        {
            if (modEntry->head.moduleTitle[i] == '\0')
                nameTextLen--;
            else
                break;
        }

        for (i = 0; i < nameTextLen; ++i)
        {
            if (modEntry->head.moduleTitle[i] != '\0')
                teriminalPutChar(tolower(modEntry->head.moduleTitle[i]));
            else
                teriminalPutChar(' ');
        }

        terminalPrintf("\" saved\n");
    }

    return (true);
}

static int8_t checkModType(const char *buf)
{
         if (!strncmp(buf, "M.K.", 4)) return (FORMAT_MK);   // ProTracker v1.x, handled as ProTracker v2.x
    else if (!strncmp(buf, "M!K!", 4)) return (FORMAT_MK2);  // ProTracker v2.x (if >64 patterns)
    else if (!strncmp(buf, "FLT4", 4)) return (FORMAT_FLT4); // StarTrekker (4ch), handled as ProTracker v2.x
    else if (!strncmp(buf, "4CHN", 4)) return (FORMAT_4CHN); // FastTracker II (4ch), handled as ProTracker v2.x
    else if (!strncmp(buf, "N.T.", 4)) return (FORMAT_MK);   // NoiseTracker 1.0, handled as ProTracker v2.x
    else if (!strncmp(buf, "M&K!", 4)) return (FORMAT_FEST); // Special NoiseTracker format (used in music disks?)
    else if (!strncmp(buf, "FEST", 4)) return (FORMAT_FEST); // Special NoiseTracker format (used in music disks?)

    return (FORMAT_UNKNOWN); // may be The Ultimate SoundTracker, 15 samples
}

module_t *modLoad(const char *fileName)
{
    char modSig[4], tmpChar;
    int8_t mightBeSTK, numSamples, lateVerSTKFlag;
    uint8_t ppCrunchData[4], bytes[4], *ppBuffer;
    uint8_t *modBuffer, ch, row, pattern;
    int32_t i, tmp;
    uint32_t j, PP20, ppPackLen, ppUnpackLen;
    FILE *fmodule;
    module_t *newModule;
    moduleSample_t *s;
    note_t *note;
    mem_t *mod;

    lateVerSTKFlag = false;
    mightBeSTK     = false;

    newModule = (module_t *)(calloc(1, sizeof (module_t)));
    if (newModule == NULL)
    {
        displayErrorMsg(editor.outOfMemoryText);
        terminalPrintf(editor.modLoadOoMText);

        return (NULL);
    }

    fmodule = fopen(fileName, "rb");
    if (fmodule == NULL)
    {
        free(newModule);
        newModule = NULL;

        displayErrorMsg("FILE I/O ERROR !");
        terminalPrintf("Module loading failed: file input/output error!\n");

        return (NULL);
    }

    fseek(fmodule, 0, SEEK_END);
    newModule->head.moduleSize = ftell(fmodule);
    fseek(fmodule, 0, SEEK_SET);

    // check if mod is a powerpacker mod
    fread(&PP20, 4, 1, fmodule); if (bigEndian) PP20 = SWAP32(PP20);
    if (PP20 == 0x30325850) // "PX20"
    {
        free(newModule);
        fclose(fmodule);

        displayErrorMsg("ENCRYPTED PPACK !");
        terminalPrintf("Module loading failed: .MOD is PowerPacker encrypted!\n");

        return (NULL);
    }
    else if (PP20 == 0x30325050) // "PP20"
    {
        ppPackLen = newModule->head.moduleSize;
        if (ppPackLen & 3)
        {
            free(newModule);
            fclose(fmodule);

            displayErrorMsg("POWERPACKER ERROR");
            terminalPrintf("Module loading failed: unknown PowerPacker error\n");

            return (NULL);
        }

        fseek(fmodule, ppPackLen - 4, SEEK_SET);

        ppCrunchData[0] = (uint8_t)(fgetc(fmodule));
        ppCrunchData[1] = (uint8_t)(fgetc(fmodule));
        ppCrunchData[2] = (uint8_t)(fgetc(fmodule));
        ppCrunchData[3] = (uint8_t)(fgetc(fmodule));

        ppUnpackLen = (ppCrunchData[0] << 16) | (ppCrunchData[1] << 8) | ppCrunchData[2];

        // smallest and biggest possible .MOD
        if ((ppUnpackLen < 2108) || (ppUnpackLen > 4195326))
        {
            free(newModule);
            fclose(fmodule);

            displayErrorMsg("NOT A MOD FILE !");
            terminalPrintf("Module loading failed: not a valid .MOD file (incorrect unpacked file size)\n");

            return (NULL);
        }

        ppBuffer = (uint8_t *)(malloc(ppPackLen));
        if (ppBuffer == NULL)
        {
            free(newModule);
            fclose(fmodule);

            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf(editor.modLoadOoMText);

            return (NULL);
        }

        modBuffer = (uint8_t *)(malloc(ppUnpackLen));
        if (modBuffer == NULL)
        {
            free(newModule);
            free(ppBuffer);

            fclose(fmodule);

            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf(editor.modLoadOoMText);

            return (NULL);
        }

        fseek(fmodule, 0, SEEK_SET);
        fread(ppBuffer, 1, ppPackLen, fmodule);
        fclose(fmodule);

        ppdecrunch(ppBuffer + 8, modBuffer, ppBuffer + 4, ppPackLen - 12, ppUnpackLen, ppCrunchData[3]);
        free(ppBuffer);

        newModule->head.moduleSize = ppUnpackLen;
    }
    else
    {
        // smallest and biggest possible .MOD
        if ((newModule->head.moduleSize < 2108) || (newModule->head.moduleSize > 4195326))
        {
            free(newModule);
            fclose(fmodule);

            displayErrorMsg("NOT A MOD FILE !");
            terminalPrintf("Module loading failed: not a valid .MOD file (invalid file size)");

            return (NULL);
        }

        modBuffer = (uint8_t *)(malloc(newModule->head.moduleSize));
        if (modBuffer == NULL)
        {
            free(newModule);
            fclose(fmodule);

            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf(editor.modLoadOoMText);

            return (NULL);
        }

        fseek(fmodule, 0, SEEK_SET);
        fread(modBuffer, 1, newModule->head.moduleSize, fmodule);
        fclose(fmodule);
    }

    mod = mopen(modBuffer, newModule->head.moduleSize);
    if (mod == NULL)
    {
        free(modBuffer);
        free(newModule);

        displayErrorMsg("FILE I/O ERROR !");
        terminalPrintf("Module loading failed: file input/output error\n");

        return (NULL);
    }

    // check module tag
    mseek(mod, 0x0438, SEEK_SET);
    mread(modSig, 1, 4, mod);

    newModule->head.format = checkModType(modSig);
    if (newModule->head.format == FORMAT_UNKNOWN)
        mightBeSTK = true;

    mseek(mod, 0, SEEK_SET);

    mread(newModule->head.moduleTitle, 1, 20, mod);
    // index 21 of newModule->head.moduleTitle is already zeroed

    for (i = 0; i < 20; ++i)
    {
        tmpChar = newModule->head.moduleTitle[i];
        if (((tmpChar < ' ') || (tmpChar > '~')) && (tmpChar != '\0'))
            tmpChar = ' ';

        newModule->head.moduleTitle[i] = (char)(tolower(tmpChar));
    }

    // read sample information
    for (i = 0; i < MOD_SAMPLES; ++i)
    {
        s = &newModule->samples[i];

        if (mightBeSTK && (i > 14))
        {
            s->loopLength = 2;
        }
        else
        {
            mread(s->text, 1, 22, mod);
            // index 23 of s->text is already zeroed

            for (j = 0; j < 22; ++j)
            {
                tmpChar = s->text[j];
                if (((tmpChar < ' ') || (tmpChar > '~')) && (tmpChar != '\0'))
                    tmpChar = ' ';

                s->text[j] = (char)(tolower(tmpChar));
            }

            s->length = ((mgetc(mod) << 8) | mgetc(mod)) * 2;
            if (s->length > 9999)
                lateVerSTKFlag = true; // Only used if mightBeSTK is set

            if (newModule->head.format == FORMAT_FEST)
                s->fineTune = (uint8_t)((-mgetc(mod) & 0x1F) / 2); // One more bit of precision, + inverted
            else
                s->fineTune = (uint8_t)(mgetc(mod)) & 0x0F;

            s->volume = (uint8_t)(mgetc(mod));
            if (s->volume > 64)
                s->volume = 64;

            s->loopStart = ((mgetc(mod) << 8) | mgetc(mod)) * 2;
            if (mightBeSTK)
                s->loopStart /= 2;

            s->loopLength = ((mgetc(mod) << 8) | mgetc(mod)) * 2;
            if (s->loopLength < 2)
                s->loopLength = 2;

            // fix for poorly converted STK->PTMOD modules.
            if (!mightBeSTK && ((s->loopStart + s->loopLength) > s->length))
            {
                if (((s->loopStart / 2) + s->loopLength) <= s->length)
                    s->loopStart /= 2;
            }

            if (mightBeSTK)
            {
                if (s->loopLength > 2)
                {
                    tmp = s->loopStart;

                    s->length      -= s->loopStart;
                    s->loopStart    = 0;
                    s->tmpLoopStart = tmp;
                }

                // No finetune in STK/UST
                s->fineTune = 0;
            }
        }
    }

    // STK 2.5 had loopStart in words, not bytes. Convert if late version STK.
    for (i = 0; i < 15; ++i)
    {
        if (mightBeSTK && lateVerSTKFlag)
        {
            s = &newModule->samples[i];
            if (s->loopStart > 2)
            {
                s->length -= s->tmpLoopStart;
                s->tmpLoopStart *= 2;
            }
        }
    }

    newModule->head.orderCount = (uint8_t)(mgetc(mod));

    // fixes beatwave.mod (129 orders) and other weird MODs
    if (newModule->head.orderCount > 127)
    {
        if (newModule->head.orderCount > 129)
        {
            mclose(&mod);

            free(modBuffer);
            free(newModule);

            displayErrorMsg("NOT A MOD FILE !");
            terminalPrintf("Module loading failed: not a valid .MOD file (NumOrders > 127)\n");

            return (NULL);
        }

        newModule->head.orderCount = 127;
    }

    if (newModule->head.orderCount == 0)
    {
        mclose(&mod);

        free(modBuffer);
        free(newModule);

        displayErrorMsg("NOT A MOD FILE !");
        terminalPrintf("Module loading failed: not a valid .MOD file (NumOrders = 0)\n");

        return (NULL);
    }

    newModule->head.restartPos = (uint8_t)(mgetc(mod));
    if (mightBeSTK && ((newModule->head.restartPos == 0) || (newModule->head.restartPos > 220)))
    {
        mclose(&mod);

        free(modBuffer);
        free(newModule);

        displayErrorMsg("NOT A MOD FILE !");
        terminalPrintf("Module loading failed: not a valid .MOD file\n");

        return (NULL);
    }

    if (mightBeSTK)
    {
        // If we're still here at this point and the mightBeSTK flag is set,
        // then it's definitely a proper The Ultimate SoundTracker (STK) module.

        newModule->head.format = FORMAT_STK;

        if (newModule->head.restartPos != 120) // 120 = 125 (?)
        {
            if (newModule->head.restartPos > 239)
                newModule->head.restartPos = 239;

            // max BPM: 14536 (there was no clamping originally, sick!)
            newModule->head.initBPM = (uint16_t)(1773447 / ((240 - newModule->head.restartPos) * 122));
        }
    }

    for (i = 0; i < MOD_ORDERS; ++i)
    {
        newModule->head.order[i] = (int16_t)(mgetc(mod));
        if (newModule->head.order[i] > newModule->head.patternCount)
            newModule->head.patternCount = newModule->head.order[i];
    }

    if (++newModule->head.patternCount > MAX_PATTERNS)
    {
        mclose(&mod);

        free(modBuffer);
        free(newModule);

        displayErrorMsg("NOT A MOD FILE !");
        terminalPrintf("Module loading failed: not a valid .MOD file (NumPatterns > 100)\n");

        return (NULL);
    }

    if (newModule->head.format != FORMAT_STK) // The Ultimate SoundTracker MODs doesn't have this tag
        mseek(mod, 4, SEEK_CUR); // We already read/tested the tag earlier, skip it

    // init 100 patterns and load patternCount of patterns
    for (pattern = 0; pattern < MAX_PATTERNS; ++pattern)
    {
        newModule->patterns[pattern] = (note_t *)(calloc(MOD_ROWS * AMIGA_VOICES, sizeof (note_t)));
        if (newModule->patterns[pattern] == NULL)
        {
            mclose(&mod);

            free(modBuffer);

            for (i = 0; i < pattern; ++i)
                free(newModule->patterns[i]);

            free(newModule);

            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf(editor.modLoadOoMText);

            return (NULL);
        }
    }

    for (pattern = 0; pattern < newModule->head.patternCount; ++pattern)
    {
        note = newModule->patterns[pattern];
        for (row = 0; row < MOD_ROWS; ++row)
        {
            for (ch = 0; ch < AMIGA_VOICES; ++ch)
            {
                bytes[0] = (uint8_t)(mgetc(mod));
                bytes[1] = (uint8_t)(mgetc(mod));
                bytes[2] = (uint8_t)(mgetc(mod));
                bytes[3] = (uint8_t)(mgetc(mod));

                note->period  = ((bytes[0] & 0x0F) << 8) | bytes[1];
                note->sample  =  (bytes[0] & 0xF0) | (bytes[2] >> 4); // Don't (!) clamp, the player checks for invalid samples
                note->command = bytes[2] & 0x0F;
                note->param   = bytes[3];

                if ((newModule->head.format == FORMAT_NT) || (newModule->head.format == FORMAT_FEST))
                {
                    // Any Dxx == D00 in N.T./FEST modules
                    if (note->command == 0x0D)
                        note->param = 0x00;
                }
                else if (mightBeSTK)
                {
                    // Convert STK effects to PT effects
                    if (!lateVerSTKFlag)
                    {
                        if (note->command == 0x01)
                        {
                            // Arpeggio
                            note->command = 0x00;
                        }
                        else if (note->command == 0x02)
                        {
                            // Pitch slide
                            if (note->param & 0xF0)
                            {
                                note->command = 0x02;
                                note->param >>= 4;
                            }
                            else if (note->param & 0x0F)
                            {
                                note->command = 0x01;
                            }
                        }
                    }

                    // Volume slide/pattern break
                    if (note->command == 0x0D)
                    {
                        if (note->param == 0)
                            note->command = 0x0D;
                        else
                            note->command = 0x0A;
                    }
                }
                else if (newModule->head.format == FORMAT_4CHN) // 4CHN != PT MOD
                {
                    // Remove FastTracker II 8xx/E8x panning commands if present
                    if (note->command == 0x08)
                    {
                        // 8xx
                        note->command = 0;
                        note->param   = 0;
                    }
                    else if ((note->command == 0x0E) && ((note->param >> 4) == 0x08))
                    {
                        // E8x
                        note->command = 0;
                        note->param   = 0;
                    }

                    // Remove F00, FastTracker II didn't use F00 as STOP in .MOD
                    if ((note->command == 0x0F) && (note->param == 0x00))
                    {
                        note->command = 0;
                        note->param   = 0;
                    }
                }

                note++;
            }
        }
    }

    numSamples = (newModule->head.format == FORMAT_STK) ? 15 : 31;
    for (i = 0; i < numSamples; ++i)
        newModule->samples[i].offset = MAX_SAMPLE_LEN * i;

    // init 31*MAX_SAMPLE_LEN worth of sample data so that you can
    // easily load bigger samples if you're editing the song.
    // That's ~4MB. We have loads of RAM nowadays... No problem!
    newModule->sampleData = (int8_t *)(calloc(MOD_SAMPLES, MAX_SAMPLE_LEN));
    if (newModule->sampleData == NULL)
    {
        mclose(&mod);

        free(modBuffer);

        for (i = 0; i < MAX_PATTERNS; ++i)
        {
            if (newModule->patterns[i] != NULL)
                free(newModule->patterns[i]);
        }

        free(newModule);

        displayErrorMsg(editor.outOfMemoryText);
        terminalPrintf(editor.modLoadOoMText);

        return (NULL);
    }

    // load sample data
    numSamples = (newModule->head.format == FORMAT_STK) ? 15 : 31;
    for (i = 0; i < numSamples; ++i)
    {
        s = &newModule->samples[i];

        if (mightBeSTK && (s->loopLength > 2))
        {
            for (j = 0; j < (uint32_t)(s->tmpLoopStart); ++j)
                mgetc(mod); // skip

            mread(&newModule->sampleData[s->offset], 1, s->length - s->loopStart, mod);
        }
        else
        {
            mread(&newModule->sampleData[s->offset], 1, s->length, mod);
        }

        if ((s->loopLength > 2) && ((s->loopStart + s->loopLength) > s->length)) // fix for overflowing loops (f.ex. MOD.shorttune2)
        {
            if ((s->loopStart + s->loopLength) > MAX_SAMPLE_LEN) // not the best solution, but can't be bothered
            {
                s->loopStart  = 0;
                s->loopLength = 2;
            }
            else
            {
                s->length += ((s->loopStart + s->loopLength) - s->length);
            }
        }
    }

    mclose(&mod);
    free(modBuffer);

    return (newModule);
}

int8_t saveModule(int8_t checkIfFileExist, int8_t giveNewFreeFilename)
{
    char fileName[48];
    uint16_t i, j;
    struct stat statBuffer;

    memset(fileName, 0, sizeof (fileName));

    if (editor.diskop.modDot)
    {
        // extension.filename

        if (*modEntry->head.moduleTitle == '\0')
        {
            strcat(fileName, "mod.untitled");
        }
        else
        {
            strcat(fileName, "mod.");

            for (i = 4; i < (20 + 4); ++i)
            {
                fileName[i] = (char)(tolower(modEntry->head.moduleTitle[i - 4]));
                if (fileName[i] == '\0')
                    break;

                // convert illegal file name characters to spaces
                     if (fileName[i] ==  '<') fileName[i] = ' ';
                else if (fileName[i] ==  '>') fileName[i] = ' ';
                else if (fileName[i] ==  ':') fileName[i] = ' ';
                else if (fileName[i] ==  '"') fileName[i] = ' ';
                else if (fileName[i] ==  '/') fileName[i] = ' ';
                else if (fileName[i] == '\\') fileName[i] = ' ';
                else if (fileName[i] ==  '|') fileName[i] = ' ';
                else if (fileName[i] ==  '?') fileName[i] = ' ';
                else if (fileName[i] ==  '*') fileName[i] = ' ';
            }
        }
    }
    else
    {
        // filename.extension

        if (*modEntry->head.moduleTitle == '\0')
        {
            strcat(fileName, "untitled.mod");
        }
        else
        {
            for (i = 0; i < 20; ++i)
            {
                fileName[i] = (char)(tolower(modEntry->head.moduleTitle[i]));
                if (fileName[i] == '\0')
                    break;

                // convert illegal file name characters to spaces
                     if (fileName[i] ==  '<') fileName[i] = ' ';
                else if (fileName[i] ==  '>') fileName[i] = ' ';
                else if (fileName[i] ==  ':') fileName[i] = ' ';
                else if (fileName[i] ==  '"') fileName[i] = ' ';
                else if (fileName[i] ==  '/') fileName[i] = ' ';
                else if (fileName[i] == '\\') fileName[i] = ' ';
                else if (fileName[i] ==  '|') fileName[i] = ' ';
                else if (fileName[i] ==  '?') fileName[i] = ' ';
                else if (fileName[i] ==  '*') fileName[i] = ' ';
            }

            strcat(fileName, ".mod");
        }
    }

    if (giveNewFreeFilename)
    {
        if (stat(fileName, &statBuffer) == 0)
        {
            for (j = 1; j <= 9999; ++j) // This number should satisfy all! ;)
            {
                memset(fileName, 0, sizeof (fileName));

                if (editor.diskop.modDot)
                {
                    // extension.filename

                    if (*modEntry->head.moduleTitle == '\0')
                    {
                        sprintf(fileName, "mod.untitled-%d", j);
                    }
                    else
                    {
                        strcat(fileName, "mod.");

                        for (i = 4; i < 20 + 4; ++i)
                        {
                            fileName[i] = (char)(tolower(modEntry->head.moduleTitle[i - 4]));
                            if (fileName[i] == '\0')
                                break;

                            // convert illegal file name characters to spaces
                                 if (fileName[i] ==  '<') fileName[i] = ' ';
                            else if (fileName[i] ==  '>') fileName[i] = ' ';
                            else if (fileName[i] ==  ':') fileName[i] = ' ';
                            else if (fileName[i] ==  '"') fileName[i] = ' ';
                            else if (fileName[i] ==  '/') fileName[i] = ' ';
                            else if (fileName[i] == '\\') fileName[i] = ' ';
                            else if (fileName[i] ==  '|') fileName[i] = ' ';
                            else if (fileName[i] ==  '?') fileName[i] = ' ';
                            else if (fileName[i] ==  '*') fileName[i] = ' ';
                        }

                        sprintf(fileName, "%s-%d", fileName, j);
                    }
                }
                else
                {
                    // filename.extension

                    if (*modEntry->head.moduleTitle == '\0')
                    {
                        sprintf(fileName, "untitled-%d.mod", j);
                    }
                    else
                    {
                        for (i = 0; i < 20; ++i)
                        {
                            fileName[i] = (char)(tolower(modEntry->head.moduleTitle[i]));
                            if (fileName[i] == '\0')
                                break;

                            // convert illegal file name characters to spaces
                                 if (fileName[i] ==  '<') fileName[i] = ' ';
                            else if (fileName[i] ==  '>') fileName[i] = ' ';
                            else if (fileName[i] ==  ':') fileName[i] = ' ';
                            else if (fileName[i] ==  '"') fileName[i] = ' ';
                            else if (fileName[i] ==  '/') fileName[i] = ' ';
                            else if (fileName[i] == '\\') fileName[i] = ' ';
                            else if (fileName[i] ==  '|') fileName[i] = ' ';
                            else if (fileName[i] ==  '?') fileName[i] = ' ';
                            else if (fileName[i] ==  '*') fileName[i] = ' ';
                        }

                        sprintf(fileName, "%s-%d", fileName, j);
                        strcat(fileName, ".mod");
                    }
                }

                if (stat(fileName, &statBuffer) != 0)
                    break;
            }
        }
    }

    if (checkIfFileExist)
    {
        if (stat(fileName, &statBuffer) == 0)
        {
            editor.ui.askScreenShown = true;
            editor.ui.askScreenType  = ASK_SAVEMOD_OVERWRITE;

            pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
            setStatusMessage("OVERWRITE FILE ?", NO_CARRY);
            renderAskDialog();

            return (-1);
        }
    }

    if (editor.ui.askScreenShown)
    {
        editor.ui.answerNo       = false;
        editor.ui.answerYes      = false;
        editor.ui.askScreenShown = false;
    }

    return (modSave(fileName));
}

static mem_t *mopen(const uint8_t *src, uint32_t length)
{
    mem_t *b;

    if ((src == NULL) || (length == 0))
        return (NULL);

    b = (mem_t *)(malloc(sizeof (mem_t)));
    if (b == NULL)
        return (NULL);

    b->_base   = (uint8_t *)(src);
    b->_ptr    = (uint8_t *)(src);
    b->_cnt    = length;
    b->_bufsiz = length;
    b->_eof    = false;

    return (b);
}

static void mclose(mem_t **buf)
{
    if (*buf != NULL)
    {
        free(*buf);
        *buf = NULL;
    }
}

static int32_t mgetc(mem_t *buf)
{
    int32_t b;

    if ((buf == NULL) || (buf->_ptr == NULL) || (buf->_cnt <= 0))
        return (0);

    b = *buf->_ptr;

    buf->_cnt--;
    buf->_ptr++;

    if (buf->_cnt <= 0)
    {
        buf->_ptr = buf->_base + buf->_bufsiz;
        buf->_cnt = 0;
        buf->_eof = true;
    }

    return (int32_t)(b);
}

static size_t mread(void *buffer, size_t size, size_t count, mem_t *buf)
{
    int32_t pcnt;
    size_t wrcnt;

    if ((buf == NULL) || (buf->_ptr == NULL))
        return (0);

    wrcnt = size * count;
    if ((size == 0) || buf->_eof)
        return (0);

    pcnt = (buf->_cnt > wrcnt) ? wrcnt : buf->_cnt;
    memcpy(buffer, buf->_ptr, pcnt);

    buf->_cnt -= pcnt;
    buf->_ptr += pcnt;

    if (buf->_cnt <= 0)
    {
        buf->_ptr = buf->_base + buf->_bufsiz;
        buf->_cnt = 0;
        buf->_eof = true;
    }

    return (pcnt / size);
}

static void mseek(mem_t *buf, int32_t offset, int32_t whence)
{
    if (buf == NULL)
        return;

    if (buf->_base)
    {
        switch (whence)
        {
            case SEEK_SET: buf->_ptr  = buf->_base + offset;                break;
            case SEEK_CUR: buf->_ptr += offset;                             break;
            case SEEK_END: buf->_ptr  = buf->_base + buf->_bufsiz + offset; break;
            default: break;
        }

        buf->_eof = false;
        if (buf->_ptr >= (buf->_base + buf->_bufsiz))
        {
            buf->_ptr = buf->_base + buf->_bufsiz;
            buf->_eof = true;
        }

        buf->_cnt = (buf->_base + buf->_bufsiz) - buf->_ptr;
    }
}

/*
** Code taken from Heikki Orsila's amigadepack. Seems to have no license,
** so I'll assume it fits into wtfpl (wtfpl.net). Heikki should contact me
** if it shall not.
** Modified by 8bitbubsy
**/

#define PP_READ_BITS(nbits, var) do {       \
  bitCnt = (nbits);                         \
  while (bitsLeft < bitCnt) {               \
    if (bufSrc < src) return (false);       \
    bitBuffer |= (*--bufSrc << bitsLeft);   \
    bitsLeft += 8;                          \
  }                                         \
  (var) = 0;                                \
  bitsLeft -= bitCnt;                       \
  while (bitCnt--) {                        \
    (var) = ((var) << 1) | (bitBuffer & 1); \
    bitBuffer >>= 1;                        \
  }                                         \
} while (0);

uint8_t ppdecrunch(uint8_t *src, uint8_t *dst, uint8_t *offsetLens, uint32_t srcLen, uint32_t dstLen, uint8_t skipBits)
{
    uint8_t *bufSrc, *dstEnd, *out, bitsLeft, bitCnt;
    uint32_t x, todo, offBits, offset, written, bitBuffer;

    if ((src == NULL) || (dst == NULL) || (offsetLens == NULL))
        return (false);

    bitsLeft  = 0;
    bitBuffer = 0;
    written   = 0;
    bufSrc    = src + srcLen;
    out       = dst + dstLen;
    dstEnd    = out;

    PP_READ_BITS(skipBits, x);
    while (written < dstLen)
    {
        PP_READ_BITS(1, x);
        if (x == 0)
        {
            todo = 1;

            do
            {
                PP_READ_BITS(2, x);
                todo += x;
            }
            while (x == 3);

            while (todo--)
            {
                PP_READ_BITS(8, x);

                if (out <= dst)
                    return (false);

                *--out = (uint8_t)(x);
                written++;
            }

            if (written == dstLen)
                break;
        }

        PP_READ_BITS(2, x);

        offBits = offsetLens[x];
        todo    = x + 2;

        if (x == 3)
        {
            PP_READ_BITS(1, x);
            if (x == 0) offBits = 7;

            PP_READ_BITS((uint8_t)(offBits), offset);
            do
            {
                PP_READ_BITS(3, x);
                todo += x;
            }
            while (x == 7);
        }
        else
        {
            PP_READ_BITS((uint8_t)(offBits), offset);
        }

        if ((out + offset) >= dstEnd)
            return (false);

        while (todo--)
        {
            x = out[offset];

            if (out <= dst)
                return (false);

            *--out = (uint8_t)(x);
            written++;
        }
    }

    return (true);
}

void setupNewMod(void)
{
    int8_t i;
    uint8_t nameTextLen;

    // setup GUI text pointers
    for (i = 0; i < MOD_SAMPLES; ++i)
    {
        modEntry->samples[i].volumeDisp     = &modEntry->samples[i].volume;
        modEntry->samples[i].lengthDisp     = &modEntry->samples[i].length;
        modEntry->samples[i].loopStartDisp  = &modEntry->samples[i].loopStart;
        modEntry->samples[i].loopLengthDisp = &modEntry->samples[i].loopLength;

        fillSampleRedoBuffer(i);
    }

    editor.currEditPatternDisp = &modEntry->currPattern;
    editor.currPosDisp         = &modEntry->currOrder;
    editor.currPatternDisp     = &modEntry->head.order[0];
    editor.currPosEdPattDisp   = &modEntry->head.order[0];
    editor.currLengthDisp      = &modEntry->head.orderCount;

    // calculate MOD size
    editor.ui.updateSongSize = true;

    editor.muted[0] = false;
    editor.muted[1] = false;
    editor.muted[2] = false;
    editor.muted[3] = false;

    editor.editMoveAdd        = 1;
    editor.currSample         = 0;
    editor.playTime           = 0;
    editor.modLoaded          = true;
    editor.blockMarkFlag      = false;
    editor.sampleZero         = false;
    editor.keypadSampleOffset = 0;

    setLEDFilter(false); // real PT doesn't do this there, but that's insane

    memset(editor.ui.pattNames, 0, MAX_PATTERNS * 16);

    modSetPos(0, 0);

    updateWindowTitle(MOD_NOT_MODIFIED);

    if (moduleNameIsEmpty(modEntry->head.moduleTitle))
    {
        terminalPrintf("Module \"untitled\" loaded\n");
    }
    else
    {
        terminalPrintf("Module \"");

        nameTextLen = 20;
        for (i = 19; i >= 0; --i)
        {
            if (modEntry->head.moduleTitle[i] == '\0')
                nameTextLen--;
            else
                break;
        }

        for (i = 0; i < nameTextLen; ++i)
        {
            if (modEntry->head.moduleTitle[i] != '\0')
                teriminalPutChar(tolower(modEntry->head.moduleTitle[i]));
            else
                teriminalPutChar(' ');
        }

        terminalPrintf("\" loaded\n");
    }

    modSetSpeed(6);

    if (modEntry->head.initBPM > 0)
        modSetTempo(modEntry->head.initBPM);
    else
        modSetTempo(125);

    modSetPos(0, 0);

    updateCurrSample();
    editor.samplePos = 0;
    updateSamplePos();
}
