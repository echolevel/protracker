#define _GNU_SOURCE
// Dl_info and dladdr for GNU/Linux needs this define before header including

#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif
#include "pt_helpers.h"
#include "pt_header.h"
#include "pt_tables.h"
#include "pt_palette.h"

extern SDL_Window *window; // pt_main.c

void showErrorMsgBox(const char *fmt, ...)
{
    char strBuf[1024];
    va_list args;

    // format the text string
    va_start(args, fmt);
    vsnprintf(strBuf, sizeof (strBuf), fmt, args);
    va_end(args);

    // window can be NULL here, no problem...
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Critical Error", strBuf, window);
}

void periodToScopeDelta(moduleChannel_t *ch, uint16_t period)
{
    double rate;

    if (period == 0)
    {
        ch->scopeReadDelta_f = 0.0;
        ch->scopeDrawDelta_f = 1.0;
    }
    else
    {
        if (period < 113)
            period = 113;

        rate = (double)(PAULA_PAL_CLK) / period;

        ch->scopeReadDelta_f = rate / (VBLANK_HZ     / 1.001); // "/ 1.001" to get real vblank hz (google it, verified to fix sync)
        ch->scopeDrawDelta_f = rate / (PAULA_PAL_CLK / 428.0); // ProTracker middle-C Hz (period 428)
    }
}

int8_t volumeToScopeVolume(uint8_t vol)
{
    return (0 - vol);
}

void changePathToHome(void)
{
#ifndef _WIN32
    sprintf(editor.tempPath, "%s", getenv("HOME"));
    chdir(editor.tempPath);
#endif
}

// this should ONLY be called AFTER initializeVars()
void changePathToExecutablePath(void)
{
#ifndef _WIN32
    uint32_t i, pathLen;
    Dl_info info;

    dladdr((void *)(&changePathToExecutablePath), &info);
    memset(editor.tempPath, 0, PATH_MAX_LEN + 1);
    realpath(info.dli_fname, editor.tempPath);

    // truncate file name
    pathLen = strlen(editor.tempPath);
    for (i = pathLen; i != 0; --i)
    {
        if (editor.tempPath[i] == '/')
        {
            editor.tempPath[i] = '\0';
            break;
        }
    }

    chdir(editor.tempPath);
    #ifdef __APPLE__
        chdir("../../../"); // package.app/Contents/MacOS
    #endif
#endif
}

int8_t sampleNameIsEmpty(char *name)
{
    uint8_t i, n;

    if (name == NULL)
        return (true);

    n = 0;
    for (i = 0; i < 22; ++i)
    {
        if (name[i] == '\0')
            n++;
    }

    return ((n == 22) ? true : false);
}

int8_t moduleNameIsEmpty(char *name)
{
    uint8_t i, n;

    if (name == NULL)
        return (true);

    n = 0;
    for (i = 0; i < 20; ++i)
    {
        if (name[i] == '\0')
            n++;
    }

    return ((n == 20) ? true : false);
}

void updateWindowTitle(int8_t modified)
{
    char titleTemp[64];

    if (modified)
        modEntry->modified = true;

    if (modEntry->head.moduleTitle[0] != '\0')
    {
        if (modified)
        {
            if (editor.diskop.modDot)
                sprintf(titleTemp, "ProTracker v2.3D clone *(mod.%s)", modEntry->head.moduleTitle);
            else
                sprintf(titleTemp, "ProTracker v2.3D clone *(%s.mod)", modEntry->head.moduleTitle);
        }
        else
        {
            if (editor.diskop.modDot)
                sprintf(titleTemp, "ProTracker v2.3D clone (mod.%s)", modEntry->head.moduleTitle);
            else
                sprintf(titleTemp, "ProTracker v2.3D clone (%s.mod)", modEntry->head.moduleTitle);
        }

        SDL_SetWindowTitle(window, titleTemp);
    }
    else
    {
        if (modified)
        {
            if (editor.diskop.modDot)
                SDL_SetWindowTitle(window, "ProTracker v2.3D clone *(mod.untitled)");
            else
                SDL_SetWindowTitle(window, "ProTracker v2.3D clone *(untitled.mod)");
        }
        else
        {
            if (editor.diskop.modDot)
                SDL_SetWindowTitle(window, "ProTracker v2.3D clone (mod.untitled)");
            else
                SDL_SetWindowTitle(window, "ProTracker v2.3D clone (untitled.mod)");
        }
    }
}

void recalcChordLength(void)
{
    int8_t note;
    moduleSample_t *s;

    s = &modEntry->samples[editor.currSample];

    if (editor.chordLengthMin)
    {
        note = MAX(MAX((editor.note1 == 36) ? -1 : editor.note1,
                       (editor.note2 == 36) ? -1 : editor.note2),
                   MAX((editor.note3 == 36) ? -1 : editor.note3,
                       (editor.note4 == 36) ? -1 : editor.note4));
    }
    else
    {
        note = MIN(MIN(editor.note1, editor.note2), MIN(editor.note3, editor.note4));
    }

    if ((note < 0) || (note > 35))
    {
        editor.chordLength = 0;
    }
    else
    {
        PT_ASSERT(editor.tuningNote < 36);

        if (editor.tuningNote < 36)
        {
            editor.chordLength = ((s->length * periodTable[(37 * s->fineTune) + note]) / periodTable[editor.tuningNote]) & 0xFFFFFFFE;
            if (editor.chordLength > MAX_SAMPLE_LEN)
                editor.chordLength = MAX_SAMPLE_LEN;
        }
    }

    if (editor.ui.editOpScreenShown && (editor.ui.editOpScreen == 3))
        editor.ui.updateLengthText = true;
}

uint8_t hexToInteger2(char *ptr)
{
    char lo, hi;

    // This routine must ONLY be used on an address
    // where two bytes can be read. It will mess up
    // if the ASCII values are not '0 .. 'F'

    hi = ptr[0];
    lo = ptr[1];

    // high nybble
    if (hi >= 'a')
        hi -= ' ';

    hi -= '0';
    if (hi > 9)
        hi -= 7;

    // low nybble
    if (lo >= 'a')
        lo -= ' ';

    lo -= '0';
    if (lo > 9)
        lo -= 7;

    return ((hi << 4) | lo);
}
