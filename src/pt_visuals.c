#include <stdint.h>
#include <ctype.h> // tolower()
#include "pt_header.h"
#include "pt_keyboard.h"
#include "pt_mouse.h"
#include "pt_audio.h"
#include "pt_palette.h"
#include "pt_helpers.h"
#include "pt_textout.h"
#include "pt_tables.h"
#include "pt_modloader.h"
#include "pt_sampleloader.h"
#include "pt_patternviewer.h"
#include "pt_sampler.h"
#include "pt_diskop.h"
#include "pt_visuals.h"
#include "pt_helpers.h"
#include "pt_terminal.h"

typedef struct sprite_t
{
    int8_t visible, pixelType;
    uint16_t newX, newY, x, y, w, h;
    uint32_t colorKey, *refreshBuffer;
    const void *data;
} sprite_t;

extern int8_t forceMixerOff;     // pt_audio.c
extern uint32_t *pixelBuffer;    // pt_main.c
extern SDL_Window *window;       // pt_main.c
extern SDL_Renderer *renderer;   // pt_main.c
extern SDL_Texture *texture;     // pt_main.c
extern uint8_t vsync60HzPresent; // pt_main.c
extern uint8_t fullscreen;       // pt_main.c

sprite_t sprites[SPRITE_NUM];

// sprite background refresh buffers
static uint32_t vuMetersBg[4 * (10 * 48)];
// -------------------------

int8_t processTick(void);                     // pt_modplayer.c
void outputAudioToSample(int32_t numSamples); // pt_audio.c
extern int32_t samplesPerTick;                // pt_audio.c
void storeTempVariables(void);                // pt_modplayer.c
uint8_t getSongProgressInPercentage(void);    // pt_modplayer.c
void updateSongInfo1(void);
void updateSongInfo2(void);
void updateSampler(void);
void updatePatternData(void);
void updateMOD2WAVDialog(void);

void renderFrame(void)
{
    updateMOD2WAVDialog(); // must be first to avoid flickering issues

    updateSongInfo1(); // top left side of screen, when "disk op"/"pos ed" is hidden
    updateSongInfo2(); // two middle rows of screen, always visible
    updateEditOp();
    updatePatternData();
    updateDiskOp();
    updateSampler();
    updatePosEd();
    updateVisualizer();
    updateDragBars();

    if (editor.ui.terminalShown) // FIXME: needs optimizations... (copy framebuffer to a temp buffer and restore?)
        terminalRender(pixelBuffer);
}

void removeAskDialog(void)
{
    if (!editor.ui.askScreenShown && !editor.isWAVRendering)
        displayMainScreen();

    editor.ui.disablePosEd      = false;
    editor.ui.disableVisualizer = false;
}

void renderAskDialog(void)
{
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;
    int32_t y;

    editor.ui.disablePosEd      = true;
    editor.ui.disableVisualizer = true;

    // render ask dialog
    ptr32Src = editor.ui.pat2SmpDialogShown ? pat2SmpDialogBMP : yesNoDialogBMP;
    ptr32Dst = pixelBuffer + ((51 * SCREEN_W) + 160);

    y = 39;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 104 * sizeof (int32_t));

        ptr32Src += 104;
        ptr32Dst += SCREEN_W;
    }
}

void fillFromVuMetersBgBuffer(void)
{
    uint8_t i, y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    if (!editor.ui.samplerScreenShown && !editor.ui.terminalShown)
    {
        for (i = 0; i < AMIGA_VOICES; ++i)
        {
            ptr32Src = vuMetersBg + (i * (10 * 48));
            ptr32Dst = pixelBuffer + ((187 * SCREEN_W) + (55 + (i * 72)));

            y = 48;
            while (y--)
            {
                *(ptr32Dst + 0) = *(ptr32Src + 0);
                *(ptr32Dst + 1) = *(ptr32Src + 1);
                *(ptr32Dst + 2) = *(ptr32Src + 2);
                *(ptr32Dst + 3) = *(ptr32Src + 3);
                *(ptr32Dst + 4) = *(ptr32Src + 4);
                *(ptr32Dst + 5) = *(ptr32Src + 5);
                *(ptr32Dst + 6) = *(ptr32Src + 6);
                *(ptr32Dst + 7) = *(ptr32Src + 7);
                *(ptr32Dst + 8) = *(ptr32Src + 8);
                *(ptr32Dst + 9) = *(ptr32Src + 9);

                ptr32Src += 10;
                ptr32Dst -= SCREEN_W;
            }
        }
    }
}

void fillToVuMetersBgBuffer(void)
{
    uint8_t i, y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    if (!editor.ui.samplerScreenShown && !editor.ui.terminalShown)
    {
        for (i = 0; i < AMIGA_VOICES; ++i)
        {
            ptr32Src = pixelBuffer + ((187 * SCREEN_W) + (55 + (i * 72)));
            ptr32Dst = vuMetersBg + (i * (10 * 48));

            y = 48;
            while (y--)
            {
                *(ptr32Dst + 0) = *(ptr32Src + 0);
                *(ptr32Dst + 1) = *(ptr32Src + 1);
                *(ptr32Dst + 2) = *(ptr32Src + 2);
                *(ptr32Dst + 3) = *(ptr32Src + 3);
                *(ptr32Dst + 4) = *(ptr32Src + 4);
                *(ptr32Dst + 5) = *(ptr32Src + 5);
                *(ptr32Dst + 6) = *(ptr32Src + 6);
                *(ptr32Dst + 7) = *(ptr32Src + 7);
                *(ptr32Dst + 8) = *(ptr32Src + 8);
                *(ptr32Dst + 9) = *(ptr32Src + 9);

                ptr32Src -= SCREEN_W;
                ptr32Dst += 10;
            }
        }
    }
}

void renderVuMeters(void)
{
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;
    uint8_t i, y;

    fillToVuMetersBgBuffer();

    if (!editor.ui.samplerScreenShown && !editor.ui.terminalShown)
    {
        for (i = 0; i < AMIGA_VOICES; ++i)
        {
            ptr32Src = vuMeterBMP;
            ptr32Dst = pixelBuffer + ((187 * SCREEN_W) + (55 + (i * 72)));

            if (editor.ui.realVuMeters)
            {
                y = (uint8_t)(editor.realVuMeterVolumes[i] + 0.5f);
                if (y > 48) y = 48;
            }
            else
            {
                y = editor.vuMeterVolumes[i];
            }

            while (y--)
            {
                *(ptr32Dst + 0) = *(ptr32Src + 0);
                *(ptr32Dst + 1) = *(ptr32Src + 1);
                *(ptr32Dst + 2) = *(ptr32Src + 2);
                *(ptr32Dst + 3) = *(ptr32Src + 3);
                *(ptr32Dst + 4) = *(ptr32Src + 4);
                *(ptr32Dst + 5) = *(ptr32Src + 5);
                *(ptr32Dst + 6) = *(ptr32Src + 6);
                *(ptr32Dst + 7) = *(ptr32Src + 7);
                *(ptr32Dst + 8) = *(ptr32Src + 8);
                *(ptr32Dst + 9) = *(ptr32Src + 9);

                ptr32Src += 10;
                ptr32Dst -= SCREEN_W;
            }
        }
    }
}

void updateSongInfo1(void) // left side of screen, when Disk Op. is hidden
{
    moduleSample_t *currSample;

    if (!editor.ui.diskOpScreenShown)
    {
        currSample = &modEntry->samples[editor.currSample];

        if (editor.ui.updateSongPos)
        {
            editor.ui.updateSongPos = false;
            printThreeDecimalsBg(pixelBuffer, 72, 3, *editor.currPosDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }

        if (editor.ui.updateSongPattern)
        {
            editor.ui.updateSongPattern = false;
            printTwoDecimalsBg(pixelBuffer, 80, 14, *editor.currPatternDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }

        if (editor.ui.updateSongLength)
        {
            editor.ui.updateSongLength = false;
            if (!editor.isWAVRendering)
                printThreeDecimalsBg(pixelBuffer, 72, 25, *editor.currLengthDisp,palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }

        if (editor.ui.updateCurrSampleFineTune)
        {
            editor.ui.updateCurrSampleFineTune = false;

            if (!editor.isWAVRendering)
            {
                if (currSample->fineTune >= 8)
                {
                    charOutBg(pixelBuffer, 80, 36, '-', palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    charOutBg(pixelBuffer, 88, 36, '0' + (0x10 - (currSample->fineTune & 0x0F)), palette[PAL_GENTXT], palette[PAL_GENBKG]);
                }
                else if (currSample->fineTune > 0)
                {
                    charOutBg(pixelBuffer, 80, 36, '+', palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    charOutBg(pixelBuffer, 88, 36, '0' + (currSample->fineTune & 0x0F), palette[PAL_GENTXT], palette[PAL_GENBKG]);
                }
                else
                {
                    charOutBg(pixelBuffer, 80, 36, ' ', palette[PAL_GENBKG], palette[PAL_GENBKG]);
                    charOutBg(pixelBuffer, 88, 36, '0', palette[PAL_GENTXT], palette[PAL_GENBKG]);
                }
            }
        }

        if (editor.ui.updateCurrSampleNum)
        {
            editor.ui.updateCurrSampleNum = false;
            if (!editor.isWAVRendering)
            {
                printTwoHexBg(pixelBuffer, 80, 47,
                    editor.sampleZero ? 0 : ((*editor.currSampleDisp) + 1), palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
        }

        if (editor.ui.updateCurrSampleVolume)
        {
            editor.ui.updateCurrSampleVolume = false;
            if (!editor.isWAVRendering)
                printTwoHexBg(pixelBuffer, 80, 58, *currSample->volumeDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }

        if (editor.ui.updateCurrSampleLength)
        {
            editor.ui.updateCurrSampleLength = false;
            if (!editor.isWAVRendering)
                printFiveHexBg(pixelBuffer, 56, 69, *currSample->lengthDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }

        if (editor.ui.updateCurrSampleRepeat)
        {
            editor.ui.updateCurrSampleRepeat = false;
            printFiveHexBg(pixelBuffer, 56, 80, *currSample->loopStartDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }

        if (editor.ui.updateCurrSampleReplen)
        {
            editor.ui.updateCurrSampleReplen = false;
            printFiveHexBg(pixelBuffer, 56, 91, *currSample->loopLengthDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }
    }
}

void updateSongInfo2(void) // two middle rows of screen, always present
{
    int32_t x, i;
    moduleSample_t *currSample;

    if (editor.ui.updateStatusText)
    {
        editor.ui.updateStatusText = false;

        // clear background
        textOutBg(pixelBuffer, 88, 127, "                 ", palette[PAL_GENBKG], palette[PAL_GENBKG]);

        // render status text
        if (!editor.errorMsgActive && editor.blockMarkFlag && !editor.ui.askScreenShown
            && !editor.ui.clearScreenShown && !editor.swapChannelFlag)
        {
            textOut(pixelBuffer,  88, 127, "MARK BLOCK", palette[PAL_GENTXT]);
            charOut(pixelBuffer, 192, 127, '-', palette[PAL_GENTXT]);

            editor.blockToPos = modEntry->currRow;
            if (editor.blockFromPos >= editor.blockToPos)
            {
                printTwoDecimals(pixelBuffer, 176, 127, editor.blockToPos,   palette[PAL_GENTXT]);
                printTwoDecimals(pixelBuffer, 200, 127, editor.blockFromPos, palette[PAL_GENTXT]);
            }
            else
            {
                printTwoDecimals(pixelBuffer, 176, 127, editor.blockFromPos, palette[PAL_GENTXT]);
                printTwoDecimals(pixelBuffer, 200, 127, editor.blockToPos,   palette[PAL_GENTXT]);
            }
        }
        else
        {
            textOut(pixelBuffer, 88, 127, editor.ui.statusMessage, palette[PAL_GENTXT]);
        }
    }

    if (editor.ui.updateSongBPM)
    {
        editor.ui.updateSongBPM = false;
        if (!editor.ui.samplerScreenShown)
            printThreeDecimalsBg(pixelBuffer, 32, 123, modEntry->currBPM, palette[PAL_GENTXT], palette[PAL_GENBKG]);
    }

    if (editor.ui.updateCurrPattText)
    {
        editor.ui.updateCurrPattText = false;
        if (!editor.ui.samplerScreenShown)
            printTwoDecimalsBg(pixelBuffer, 8, 127, *editor.currEditPatternDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
    }

    if (editor.ui.updateTrackerFlags)
    {
        editor.ui.updateTrackerFlags = false;

        charOutBg(pixelBuffer, 1, 113, ' ', palette[PAL_GENTXT], palette[PAL_GENBKG]);
        charOutBg(pixelBuffer, 8, 113, ' ', palette[PAL_GENTXT], palette[PAL_GENBKG]);
        if (editor.autoInsFlag)
        {
            charOut(pixelBuffer, 0, 113, 'I', palette[PAL_GENTXT]);

            // in Amiga PT, "auto insert" 9 means 0
            if (editor.autoInsSlot == 9)
                charOut(pixelBuffer, 8, 113, '0', palette[PAL_GENTXT]);
            else
                charOut(pixelBuffer, 8, 113, '1' + editor.autoInsSlot, palette[PAL_GENTXT]);
        }

        charOutBg(pixelBuffer, 1, 102, ' ', palette[PAL_GENTXT], palette[PAL_GENBKG]);
        if (editor.metroFlag)
            charOut(pixelBuffer, 0, 102, 'M', palette[PAL_GENTXT]);

        charOutBg(pixelBuffer, 16, 102, ' ', palette[PAL_GENTXT], palette[PAL_GENBKG]);
        if (editor.multiFlag)
            charOut(pixelBuffer, 16, 102, 'M', palette[PAL_GENTXT]);

        charOutBg(pixelBuffer, 24, 102, '0' + editor.editMoveAdd,palette[PAL_GENTXT], palette[PAL_GENBKG]);

        charOutBg(pixelBuffer, 311, 128, ' ', palette[PAL_GENBKG], palette[PAL_GENBKG]);
        if (editor.pNoteFlag == 1)
        {
            pixelBuffer[(129 * SCREEN_W) + 314] = palette[PAL_GENTXT];
            pixelBuffer[(129 * SCREEN_W) + 315] = palette[PAL_GENTXT];
        }
        else if (editor.pNoteFlag == 2)
        {
            pixelBuffer[(128 * SCREEN_W) + 314] = palette[PAL_GENTXT];
            pixelBuffer[(128 * SCREEN_W) + 315] = palette[PAL_GENTXT];
            pixelBuffer[(130 * SCREEN_W) + 314] = palette[PAL_GENTXT];
            pixelBuffer[(130 * SCREEN_W) + 315] = palette[PAL_GENTXT];
        }
    }

    if (editor.ui.updateSongTime)
    {
        editor.ui.updateSongTime = false;
        printTwoDecimalsBg(pixelBuffer, 272, 102, editor.playTime / 60, palette[PAL_GENTXT], palette[PAL_GENBKG]);
        printTwoDecimalsBg(pixelBuffer, 296, 102, editor.playTime % 60, palette[PAL_GENTXT], palette[PAL_GENBKG]);
    }

    if (editor.ui.updateSongName)
    {
        editor.ui.updateSongName = false;
        for (x = 0; x < 20; ++x)
        {
            charOutBg(pixelBuffer, 104 + (x * FONT_CHAR_W), 102,
                    (modEntry->head.moduleTitle[x] == '\0') ? '_' : modEntry->head.moduleTitle[x],
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }
    }

    if (editor.ui.updateCurrSampleName)
    {
        editor.ui.updateCurrSampleName = false;
        currSample = &modEntry->samples[editor.currSample];

        for (x = 0; x < 22; ++x)
        {
            charOutBg(pixelBuffer, 104 + (x * FONT_CHAR_W), 113,
                    (currSample->text[x] == '\0') ? '_' : currSample->text[x],
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }
    }

    if (editor.ui.updateSongSize)
    {
        editor.ui.updateSongSize = false;

        // clear background
        textOutBg(pixelBuffer, 264, 123, "      ", palette[PAL_GENBKG], palette[PAL_GENBKG]);

        // calculate module length
        modEntry->head.totalSampleSize = 0;
        for (i = 0; i < MOD_SAMPLES; ++i)
            modEntry->head.totalSampleSize += modEntry->samples[i].length;

        modEntry->head.patternCount = 0;
        for (i = 0; i < MOD_ORDERS; ++i)
        {
            if (modEntry->head.order[i] > modEntry->head.patternCount)
                modEntry->head.patternCount = modEntry->head.order[i];
        }

        modEntry->head.moduleSize = 2108 + modEntry->head.totalSampleSize + (1024 * modEntry->head.patternCount);

        if (modEntry->head.moduleSize > 999999)
        {
            charOut(pixelBuffer, 304, 123, 'K', palette[PAL_GENTXT]);
            printFourDecimals(pixelBuffer, 272, 123, modEntry->head.moduleSize / 1000, palette[PAL_GENTXT]);
        }
        else
        {
            printSixDecimals(pixelBuffer, 264, 123, modEntry->head.moduleSize, palette[PAL_GENTXT]);
        }
    }

    if (editor.ui.updateSongTiming)
    {
        editor.ui.updateSongTiming = false;
        textOutBg(pixelBuffer, 288, 130, (editor.timingMode == TEMPO_MODE_CIA) ? "CIA" : "VBL", palette[PAL_GENTXT], palette[PAL_GENBKG]);
    }
}

void updateCursorPos(void)
{
    if (!editor.ui.samplerScreenShown)
        setSpritePos(SPRITE_PATTERN_CURSOR, cursorPosTable[editor.cursor.pos], 188);
}

void updateSampler(void)
{
    int32_t tmpSampleOffset;
    moduleSample_t *s;

    if (editor.ui.samplerScreenShown)
    {
        PT_ASSERT((editor.currSample >= 0) && (editor.currSample <= 30));
        if ((editor.currSample >= 0) && (editor.currSample <= 30))
        {
            s = &modEntry->samples[editor.currSample];

            // update 9xx offset
            if (!editor.ui.samplerVolBoxShown && !editor.ui.samplerFiltersBoxShown && (s->length > 0))
            {
                if ((input.mouse.x >= 3) && (input.mouse.x <= 316) && (input.mouse.y >= 138) && (input.mouse.y <= 201))
                {
                    tmpSampleOffset = (int32_t)((scr2SmpPos(input.mouse.x - 3) / 256.0f) + 0.5f);
                    tmpSampleOffset = 0x0900 + CLAMP(tmpSampleOffset, 0x00, 0xFF);

                    if (tmpSampleOffset != editor.ui.lastSampleOffset)
                    {
                        editor.ui.lastSampleOffset = tmpSampleOffset;
                        editor.ui.update9xxPos = true;
                    }
                }
            }

            // display 9xx offset
            if (editor.ui.update9xxPos)
            {
                editor.ui.update9xxPos = false;

                textOutBg(pixelBuffer, 288, 247, "---", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                if ((s->length <= 0x0000FFFE) && (editor.ui.lastSampleOffset > 0x0900) && (editor.ui.lastSampleOffset <= 0x09FF))
                    printThreeHexBg(pixelBuffer, 288, 247, editor.ui.lastSampleOffset, palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
        }

        if (editor.ui.updateResampleNote)
        {
            editor.ui.updateResampleNote = false;

            // show resample note
            if (editor.ui.changingSmpResample)
            {
                textOutBg(pixelBuffer, 288, 236, "---", palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
            else
            {
                PT_ASSERT(editor.resampleNote < 36);
                if (editor.resampleNote < 36)
                {
                    textOutBg(pixelBuffer, 288, 236,
                        editor.accidental ? noteNames2[editor.resampleNote] : noteNames1[editor.resampleNote],
                        palette[PAL_GENTXT], palette[PAL_GENBKG]);
                }
            }
        }

        if (editor.ui.samplerVolBoxShown)
        {
            if (editor.ui.updateVolFromText)
            {
                editor.ui.updateVolFromText = false;
                printThreeDecimalsBg(pixelBuffer, 176, 157, *editor.vol1Disp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateVolToText)
            {
                editor.ui.updateVolToText = false;
                printThreeDecimalsBg(pixelBuffer, 176, 168, *editor.vol2Disp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
        }
        else if (editor.ui.samplerFiltersBoxShown)
        {
            if (editor.ui.updateLPText)
            {
                editor.ui.updateLPText = false;
                printFiveDecimalsBg(pixelBuffer, 160, 157, *editor.lpCutOffDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateHPText)
            {
                editor.ui.updateHPText = false;
                printFiveDecimalsBg(pixelBuffer, 160, 168, *editor.hpCutOffDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateNormFlag)
            {
                editor.ui.updateNormFlag = false;

                if (editor.normalizeFiltersFlag)
                    textOutBg(pixelBuffer, 207, 179, "YES", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                else
                    textOutBg(pixelBuffer, 207, 179, "NO ", palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
        }
    }
}

void showVolFromSlider(void)
{
    uint8_t x;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst, pixel;

    // clear background
    ptr32Src = samplerVolumeBMP + ((4 * 136) + 33);
    ptr32Dst = pixelBuffer + ((158 * SCREEN_W) + 105);
    memcpy(ptr32Dst, ptr32Src, 65 * sizeof (int32_t));
    memcpy(ptr32Dst + (1 * SCREEN_W), ptr32Src, 65 * sizeof (int32_t));
    memcpy(ptr32Dst + (2 * SCREEN_W), ptr32Src, 65 * sizeof (int32_t));

    // render slider
    ptr32Dst = pixelBuffer + ((158 * SCREEN_W) + (105 + ((editor.vol1 * 3) / 10)));
    pixel = palette[PAL_QADSCP];
    x = 5;
    while (x--)
    {
        *ptr32Dst = pixel;
        *(ptr32Dst + (1 * SCREEN_W)) = pixel;
        *(ptr32Dst + (2 * SCREEN_W)) = pixel;

        ptr32Dst++;
    }
}

void showVolToSlider(void)
{
    uint8_t x;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst, pixel;

    // clear background
    ptr32Src = samplerVolumeBMP + ((4 * 136) + 33);
    ptr32Dst = pixelBuffer + ((169 * SCREEN_W) + 105);
    memcpy(ptr32Dst, ptr32Src, 65 * sizeof (int32_t));
    memcpy(ptr32Dst + (1 * SCREEN_W), ptr32Src, 65 * sizeof (int32_t));
    memcpy(ptr32Dst + (2 * SCREEN_W), ptr32Src, 65 * sizeof (int32_t));

    // render slider
    ptr32Dst = pixelBuffer + ((169 * SCREEN_W) + (105 + ((editor.vol2 * 3) / 10)));
    pixel = palette[PAL_QADSCP];
    x = 5;
    while (x--)
    {
        *ptr32Dst = pixel;
        *(ptr32Dst + (1 * SCREEN_W)) = pixel;
        *(ptr32Dst + (2 * SCREEN_W)) = pixel;

        ptr32Dst++;
    }
}

void renderSamplerVolBox(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    ptr32Src = samplerVolumeBMP;
    ptr32Dst = pixelBuffer + ((154 * SCREEN_W) + 72);

    y = 33;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 136 * sizeof (int32_t));

        ptr32Src += 136;
        ptr32Dst += SCREEN_W;
    }

    editor.ui.updateVolFromText = true;
    editor.ui.updateVolToText   = true;
    showVolFromSlider();
    showVolToSlider();

    // hide loop sprites
    hideSprite(SPRITE_LOOP_PIN_LEFT);
    hideSprite(SPRITE_LOOP_PIN_RIGHT);
}

void removeSamplerVolBox(void)
{
    displaySample();
}

void renderSamplerFiltersBox(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    ptr32Src = samplerFiltersBMP;
    ptr32Dst = pixelBuffer + ((154 * SCREEN_W) + 65);

    y = 33;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 186 * sizeof (int32_t));

        ptr32Src += 186;
        ptr32Dst += SCREEN_W;
    }

    textOut(pixelBuffer, 200, 157, "HZ", palette[PAL_GENTXT]);
    textOut(pixelBuffer, 200, 168, "HZ", palette[PAL_GENTXT]);

    editor.ui.updateLPText = true;
    editor.ui.updateHPText = true;
    editor.ui.updateNormFlag = true;

    // hide loop sprites
    hideSprite(SPRITE_LOOP_PIN_LEFT);
    hideSprite(SPRITE_LOOP_PIN_RIGHT);
}

void removeSamplerFiltersBox(void)
{
    displaySample();
}

void renderDiskOpScreen(void)
{
    memcpy(pixelBuffer, diskOpScreenBMP, 99 * 320 * sizeof (int32_t));

    editor.ui.updateDiskOpPathText = true;
    editor.ui.updatePackText = true;
    editor.ui.updateSaveFormatText = true;
    editor.ui.updateLoadMode = true;
    editor.ui.updateDiskOpFileList = true;
}

void updateDiskOp(void)
{
    char tmpChar;
    uint8_t i, y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    if (editor.ui.diskOpScreenShown && !editor.ui.posEdScreenShown)
    {
        if (editor.ui.updateDiskOpFileList)
        {
            editor.ui.updateDiskOpFileList = false;
            diskOpRenderFileList(pixelBuffer);
        }

        if (editor.ui.updateLoadMode)
        {
            editor.ui.updateLoadMode = false;

            // clear backgrounds
            charOutBg(pixelBuffer, 147,  3, ' ', palette[PAL_GENBKG], palette[PAL_GENBKG]);
            charOutBg(pixelBuffer, 147, 14, ' ', palette[PAL_GENBKG], palette[PAL_GENBKG]);

            ptr32Src = arrowBMP;
            ptr32Dst = pixelBuffer + ((((11 * editor.diskop.mode) + 3) * SCREEN_W) + 148);

            y = 5;
            while (y--)
            {
                *(ptr32Dst + 0) = *(ptr32Src + 0);
                *(ptr32Dst + 1) = *(ptr32Src + 1);
                *(ptr32Dst + 2) = *(ptr32Src + 2);
                *(ptr32Dst + 3) = *(ptr32Src + 3);
                *(ptr32Dst + 4) = *(ptr32Src + 4);
                *(ptr32Dst + 5) = *(ptr32Src + 5);

                ptr32Src += 6;
                ptr32Dst += SCREEN_W;
            }
        }

        if (editor.ui.updatePackText)
        {
            editor.ui.updatePackText = false;
            textOutBg(pixelBuffer, 120, 3, editor.diskop.modPackFlg ? "ON " : "OFF", palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }

        if (editor.ui.updateSaveFormatText)
        {
            editor.ui.updateSaveFormatText = false;

                 if (editor.diskop.smpSaveType == DISKOP_SMP_WAV) textOutBg(pixelBuffer, 120, 14, "WAV", palette[PAL_GENTXT], palette[PAL_GENBKG]);
            else if (editor.diskop.smpSaveType == DISKOP_SMP_IFF) textOutBg(pixelBuffer, 120, 14, "IFF", palette[PAL_GENTXT], palette[PAL_GENBKG]);
            else if (editor.diskop.smpSaveType == DISKOP_SMP_RAW) textOutBg(pixelBuffer, 120, 14, "RAW", palette[PAL_GENTXT], palette[PAL_GENBKG]);
        }

        if (editor.ui.updateDiskOpPathText)
        {
            editor.ui.updateDiskOpPathText = false;

            // print disk op. path
            for (i = 0; i < 26; ++i)
            {
                tmpChar = editor.currPath[editor.textofs.diskOpPath + i];
                if (((tmpChar < ' ') || (tmpChar > '~')) && (tmpChar != '\0'))
                    tmpChar = ' '; // was illegal character

                charOutBg(pixelBuffer, 24 + (i * FONT_CHAR_W), 25, (tmpChar == '\0') ? '_' : tmpChar,
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
        }
    }
}

void updatePosEd(void)
{
    char posEdChar;
    uint8_t x, y, i;
    int16_t posEdPosition;

    if (editor.ui.posEdScreenShown)
    {
        if (editor.ui.updatePosEd)
        {
            editor.ui.updatePosEd = false;

            if (!editor.ui.disablePosEd)
            {
                posEdPosition = modEntry->currOrder;
                if (posEdPosition > (modEntry->head.orderCount - 1))
                    posEdPosition =  modEntry->head.orderCount - 1;

                // top five
                for (y = 0; y < 5; ++y)
                {
                    if ((posEdPosition - (5 - y)) >= 0)
                    {
                        printThreeDecimalsBg(pixelBuffer, 128, 23 + (y * 6), posEdPosition - (5 - y), palette[PAL_QADSCP], palette[PAL_BACKGRD]);
                        printTwoDecimalsBg(pixelBuffer,   160, 23 + (y * 6), modEntry->head.order[posEdPosition - (5 - y)], palette[PAL_QADSCP], palette[PAL_BACKGRD]);
                        for (i = 0; i < 15; ++i)
                        {
                            posEdChar = editor.ui.pattNames[((modEntry->head.order[posEdPosition - (5 - y)] * 16) + editor.textofs.posEdPattName) + i];
                            if (posEdChar == '\0')
                                posEdChar = ' ';

                            charOutBg(pixelBuffer, 184 + (i * FONT_CHAR_W), 23 + (y * 6), posEdChar, palette[PAL_QADSCP], palette[PAL_BACKGRD]);
                        }
                    }
                    else
                    {
                        x = FONT_CHAR_W * 22;
                        while (x--)
                        {
                            pixelBuffer[((23 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                            pixelBuffer[((24 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                            pixelBuffer[((25 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                            pixelBuffer[((26 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                            pixelBuffer[((27 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                        }
                    }
                }

                // middle
                printThreeDecimalsBg(pixelBuffer, 128, 53, posEdPosition, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                printTwoDecimalsBg(pixelBuffer,   160, 53, *editor.currPosEdPattDisp, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                for (i = 0; i < 15; ++i)
                {
                    posEdChar = editor.ui.pattNames[((modEntry->head.order[posEdPosition] * 16) + editor.textofs.posEdPattName) + i];
                    if (posEdChar == '\0')
                        posEdChar = ' ';

                    charOutBg(pixelBuffer, 184 + (i * FONT_CHAR_W), 53, posEdChar, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                }

                // bottom six
                for (y = 0; y < 6; ++y)
                {
                    if ((posEdPosition + y) < (modEntry->head.orderCount - 1))
                    {
                        printThreeDecimalsBg(pixelBuffer, 128, 59 + (y * 6), posEdPosition + (y + 1), palette[PAL_QADSCP], palette[PAL_BACKGRD]);
                        printTwoDecimalsBg(pixelBuffer  , 160, 59 + (y * 6), modEntry->head.order[posEdPosition + (y + 1)], palette[PAL_QADSCP], palette[PAL_BACKGRD]);
                        for (i = 0; i < 15; ++i)
                        {
                            posEdChar = editor.ui.pattNames[((modEntry->head.order[posEdPosition + (y + 1)] * 16) + editor.textofs.posEdPattName) + i];
                            if (posEdChar == '\0')
                                posEdChar = ' ';

                            charOutBg(pixelBuffer, 184 + (i * FONT_CHAR_W), 59 + (y * 6), posEdChar, palette[PAL_QADSCP], palette[PAL_BACKGRD]);
                        }
                    }
                    else
                    {
                        x = FONT_CHAR_W * 22;
                        while (x--)
                        {
                            pixelBuffer[((59 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                            pixelBuffer[((60 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                            pixelBuffer[((61 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                            pixelBuffer[((62 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                            pixelBuffer[((63 + (y * 6)) * SCREEN_W) + (128 + x)] = palette[PAL_BACKGRD];
                        }
                    }
                }

                // hack to fix bottom part of text edit marker in pos ed
                if (editor.ui.getLineFlag)
                {
                    if ((editor.ui.editObject == PTB_PE_PATT) || (editor.ui.editObject == PTB_PE_PATTNAME))
                        renderTextEditMarker();
                }

                // hack to fix broken pixels after editing text...
                pixelBuffer[(53 * SCREEN_W) + 303] = palette[PAL_GENBKG2];
                pixelBuffer[(54 * SCREEN_W) + 303] = palette[PAL_GENBKG2];
                pixelBuffer[(55 * SCREEN_W) + 303] = palette[PAL_GENBKG2];
                pixelBuffer[(56 * SCREEN_W) + 303] = palette[PAL_GENBKG2];
                pixelBuffer[(57 * SCREEN_W) + 303] = palette[PAL_GENBKG2];
            }
        }
    }
}

void renderPosEdScreen(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    ptr32Src = posEdBMP;
    ptr32Dst = pixelBuffer + 120;

    y = 99;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 200 * sizeof (int32_t));

        ptr32Src += 200;
        ptr32Dst += SCREEN_W;
    }
}

void renderMuteButtons(void)
{
    uint8_t i, y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst, lineW;

    if (!editor.ui.diskOpScreenShown && !editor.ui.posEdScreenShown)
    {
        for (i = 0; i < AMIGA_VOICES; ++i)
        {
            if (editor.muted[i])
            {
                ptr32Src = muteButtonsBMP + (i * (6 * 7));
                lineW = 7;
            }
            else
            {
                ptr32Src = trackerFrameBMP + ((3 + (i * 11)) * SCREEN_W) + 310;
                lineW = SCREEN_W;
            }

            ptr32Dst = pixelBuffer + ((3 + (i * 11)) * SCREEN_W) + 310;

            y = 6;
            while (y--)
            {
                *(ptr32Dst + 0) = *(ptr32Src + 0);
                *(ptr32Dst + 1) = *(ptr32Src + 1);
                *(ptr32Dst + 2) = *(ptr32Src + 2);
                *(ptr32Dst + 3) = *(ptr32Src + 3);
                *(ptr32Dst + 4) = *(ptr32Src + 4);
                *(ptr32Dst + 5) = *(ptr32Src + 5);
                *(ptr32Dst + 6) = *(ptr32Src + 6);


                ptr32Src += lineW;
                ptr32Dst += SCREEN_W;
            }
        }
    }
}

void renderClearScreen(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    editor.ui.disablePosEd      = true;
    editor.ui.disableVisualizer = true;

    ptr32Src = clearDialogBMP;
    ptr32Dst = pixelBuffer + ((51 * SCREEN_W) + 160);

    y = 39;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 104 * sizeof (int32_t));

        ptr32Src += 104;
        ptr32Dst += SCREEN_W;
    }
}

void removeClearScreen(void)
{
    displayMainScreen();

    editor.ui.disablePosEd      = false;
    editor.ui.disableVisualizer = false;
}

void updateCurrSample(void)
{
    editor.ui.updateCurrSampleName = true;
    editor.ui.updateSongSize = true;

    if (!editor.ui.diskOpScreenShown)
    {
        editor.ui.updateCurrSampleFineTune = true;
        editor.ui.updateCurrSampleNum      = true;
        editor.ui.updateCurrSampleVolume   = true;
        editor.ui.updateCurrSampleLength   = true;
        editor.ui.updateCurrSampleRepeat   = true;
        editor.ui.updateCurrSampleReplen   = true;
    }

    removeTempLoopPoints();
    if (editor.ui.samplerScreenShown)
        redrawSample();

    updateSamplePos();
    recalcChordLength();
}

void updatePatternData(void)
{
    if (editor.ui.updatePatternData)
    {
        editor.ui.updatePatternData = false;

        if (!editor.ui.samplerScreenShown)
            redrawPattern(pixelBuffer);
    }
}

void removeTextEditMarker(void)
{
    uint32_t *ptr32Dst, *ptr32Dst_2, pixel;

    if (editor.ui.getLineFlag)
    {
        ptr32Dst   = pixelBuffer + ((editor.ui.lineCurY * SCREEN_W) + (editor.ui.lineCurX - 4));
        ptr32Dst_2 = ptr32Dst - SCREEN_W;

        if ((editor.ui.editObject == PTB_PE_PATT) || (editor.ui.editObject == PTB_PE_PATTNAME))
        {
            pixel = palette[PAL_BACKGRD];
            *(ptr32Dst + 0) = pixel;
            *(ptr32Dst + 1) = pixel;
            *(ptr32Dst + 2) = pixel;
            *(ptr32Dst + 3) = pixel;
            *(ptr32Dst + 4) = pixel;
            *(ptr32Dst + 5) = pixel;
            *(ptr32Dst + 6) = pixel;

            pixel = palette[PAL_GENBKG2];
            *(ptr32Dst_2 + 0) = pixel;
            *(ptr32Dst_2 + 1) = pixel;
            *(ptr32Dst_2 + 2) = pixel;
            *(ptr32Dst_2 + 3) = pixel;
            *(ptr32Dst_2 + 4) = pixel;
            *(ptr32Dst_2 + 5) = pixel;
            *(ptr32Dst_2 + 6) = pixel;

            editor.ui.updatePosEd = true;
        }
        else
        {
            pixel = palette[PAL_GENBKG];
            *(ptr32Dst   + 0) = pixel;
            *(ptr32Dst   + 1) = pixel;
            *(ptr32Dst   + 2) = pixel;
            *(ptr32Dst   + 3) = pixel;
            *(ptr32Dst   + 4) = pixel;
            *(ptr32Dst   + 5) = pixel;
            *(ptr32Dst   + 6) = pixel;
            *(ptr32Dst_2 + 0) = pixel;
            *(ptr32Dst_2 + 1) = pixel;
            *(ptr32Dst_2 + 2) = pixel;
            *(ptr32Dst_2 + 3) = pixel;
            *(ptr32Dst_2 + 4) = pixel;
            *(ptr32Dst_2 + 5) = pixel;
            *(ptr32Dst_2 + 6) = pixel;
        }
    }
}

void renderTextEditMarker(void)
{
    uint32_t *ptr32Dst, *ptr32Dst_2, pixel;

    if (editor.ui.getLineFlag)
    {
        ptr32Dst   = pixelBuffer + ((editor.ui.lineCurY * SCREEN_W) + (editor.ui.lineCurX - 4));
        ptr32Dst_2 = ptr32Dst - SCREEN_W;
        pixel      = palette[PAL_TEXTMARK];

        *(ptr32Dst   + 0) = pixel;
        *(ptr32Dst   + 1) = pixel;
        *(ptr32Dst   + 2) = pixel;
        *(ptr32Dst   + 3) = pixel;
        *(ptr32Dst   + 4) = pixel;
        *(ptr32Dst   + 5) = pixel;
        *(ptr32Dst   + 6) = pixel;
        *(ptr32Dst_2 + 0) = pixel;
        *(ptr32Dst_2 + 1) = pixel;
        *(ptr32Dst_2 + 2) = pixel;
        *(ptr32Dst_2 + 3) = pixel;
        *(ptr32Dst_2 + 4) = pixel;
        *(ptr32Dst_2 + 5) = pixel;
        *(ptr32Dst_2 + 6) = pixel;
    }
}

void updateDragBars(void)
{
    if (editor.ui.sampleMarkingPos >= 0) samplerSamplePressed(MOUSE_BUTTON_HELD);
    if (editor.ui.forceTermBarDrag)      terminalHandleScrollBar(MOUSE_BUTTON_HELD);
    if (editor.ui.forceSampleDrag)       samplerBarPressed(MOUSE_BUTTON_HELD);
    if (editor.ui.forceSampleEdit)       samplerEditSample(MOUSE_BUTTON_HELD);
    if (editor.ui.forceVolDrag)          volBoxBarPressed(MOUSE_BUTTON_HELD);
}

void updateVisualizer(void)
{
    int8_t tmpSmp8;
    uint8_t i,  y, totalVoicesActive;
    int16_t scopeData;
    const int16_t mixScaleTable[AMIGA_VOICES] = { 388, 570, 595, 585 };
    uint16_t x;
    int32_t newPos, tmpVol, chNum, scopeBuffer[200 - 3], scopeTemp;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst, *scopePtr, pixel;
    float scopePos_f;
    moduleChannel_t *ch;

    if (!editor.ui.diskOpScreenShown && !editor.ui.posEdScreenShown &&
        !editor.ui.editOpScreenShown && !editor.ui.aboutScreenShown &&
        !editor.ui.disableVisualizer && !editor.ui.askScreenShown   &&
        !editor.isWAVRendering       && !editor.ui.terminalShown)
    {
        if (editor.ui.visualizerMode == VISUAL_QUADRASCOPE)
        {
            // quadrascope

            scopePtr = pixelBuffer + ((71 * SCREEN_W) + 128);

            i = AMIGA_VOICES;
            while (i--)
            {
                chNum = (AMIGA_VOICES - 1) - i;
                ch = &modEntry->channels[chNum];

                // clear background
                ptr32Src = trackerFrameBMP  + ((71 * SCREEN_W) + 128);
                ptr32Dst = pixelBuffer + ((55 * SCREEN_W) + (128 + (chNum * (SCOPE_WIDTH + 8))));
                y = 33;
                while (y--)
                {
                    memcpy(ptr32Dst, ptr32Src, SCOPE_WIDTH * sizeof (int32_t));
                    ptr32Dst += SCREEN_W;
                }

                // render scopes

                scopePos_f = ch->scopePos_f;

                x = SCOPE_WIDTH;
                while (x--)
                {
                    if (editor.muted[chNum] || !ch->scopeEnabled || !ch->period)
                    {
                        scopePtr[x] = palette[PAL_QADSCP];
                    }
                    else
                    {
                        newPos = (int32_t)(scopePos_f); // don't round here, truncate instead!

                        if (ch->scopeLoopFlag)
                        {
                            if (newPos >= ch->scopeLoopEnd)
                                 newPos = ch->scopeLoopBegin + ((newPos - ch->scopeLoopBegin) % (ch->scopeLoopEnd - ch->scopeLoopBegin));
                        }
                        else
                        {
                            if (newPos >= ch->scopeEnd)
                            {
                                scopePtr[x] = palette[PAL_QADSCP];
                                continue;
                            }
                        }

                        newPos = CLAMP(newPos, 0, (MAX_SAMPLE_LEN * 31) - 1);

                        if (modEntry->sampleData != NULL)
                            tmpSmp8 = modEntry->sampleData[newPos];
                        else
                            tmpSmp8 = 0;

                        scopeData = tmpSmp8 * ch->scopeVolume;

                        // "arithmetic shift right" on signed number simulation
                        if (scopeData < 0)
                            scopeData = 0xFF80 | ((uint16_t)(scopeData) >> 9); // 0xFF80 = 2^16 - 2^(16-9)
                        else
                            scopeData /= (1 << 9);

                        scopeData = CLAMP(scopeData, -32, 32);
                        scopePtr[(scopeData * SCREEN_W) + x] = palette[PAL_QADSCP];

                        scopePos_f += ch->scopeDrawDelta_f;
                    }
                }

                scopePtr += (SCOPE_WIDTH + 8);
            }
        }
        else if (editor.ui.visualizerMode == VISUAL_SPECTRUM)
        {
            // spectrum analyzer

            for (i = 0; i < SPECTRUM_BAR_NUM; ++i)
            {
                ptr32Src = spectrumAnaBMP + (SPECTRUM_BAR_HEIGHT - 1);
                ptr32Dst = pixelBuffer    + ((59 * SCREEN_W) + (129 + (i * (SPECTRUM_BAR_WIDTH + 2))));
                pixel    = palette[PAL_GENBKG];
                tmpVol   = editor.spectrumVolumes[i];

                y = SPECTRUM_BAR_HEIGHT;
                while (y--)
                {
                    if (y < tmpVol)
                    {
                        *(ptr32Dst + 0) = *ptr32Src;
                        *(ptr32Dst + 1) = *ptr32Src;
                        *(ptr32Dst + 2) = *ptr32Src;
                        *(ptr32Dst + 3) = *ptr32Src;
                        *(ptr32Dst + 4) = *ptr32Src;
                        *(ptr32Dst + 5) = *ptr32Src;
                    }
                    else
                    {
                        *(ptr32Dst + 0) = pixel;
                        *(ptr32Dst + 1) = pixel;
                        *(ptr32Dst + 2) = pixel;
                        *(ptr32Dst + 3) = pixel;
                        *(ptr32Dst + 4) = pixel;
                        *(ptr32Dst + 5) = pixel;
                    }

                    ptr32Src--;
                    ptr32Dst += SCREEN_W;
                }
            }
        }
        else
        {
            // monoscope

            // clear background (should be optimized somehow...)
            ptr32Src = monoScopeBMP + (11 * 200);
            ptr32Dst = pixelBuffer  + (55 * SCREEN_W) + 120;
            y = 44;
            while (y--)
            {
                memcpy(ptr32Dst, ptr32Src, 197 * sizeof (int32_t));
                ptr32Dst += SCREEN_W;
            }

            // mix channels

            memset(scopeBuffer, 0, sizeof (scopeBuffer));

            totalVoicesActive = 0;
            for (i = 0; i < AMIGA_VOICES; ++i)
            {
                ch = &modEntry->channels[i];

                if (!editor.muted[ch->chanIndex] && ch->scopeEnabled && ch->period)
                {
                    scopePos_f = ch->scopePos_f;
                    for (x = 0; x < (200 - 3); ++x)
                    {
                        newPos = (int32_t)(scopePos_f); // don't round here, truncate!

                        if (ch->scopeLoopFlag)
                        {
                            if (newPos >= ch->scopeLoopEnd)
                            {
                                newPos = ch->scopeLoopBegin + ((newPos - ch->scopeLoopBegin) % (ch->scopeLoopEnd - ch->scopeLoopBegin));

                                     if (newPos < ch->scopeLoopBegin) newPos = ch->scopeLoopBegin;
                                else if (newPos > ch->scopeLoopEnd)   newPos = ch->scopeLoopBegin;
                            }
                        }
                        else
                        {
                            if (newPos >= ch->scopeEnd)
                                break;
                        }

                        if (newPos >= ((MAX_SAMPLE_LEN * 31) - 1))
                            break;

                        if (modEntry->sampleData != NULL)
                            tmpSmp8 = modEntry->sampleData[newPos];
                        else
                            tmpSmp8 = 0;

                        scopeBuffer[x] += (tmpSmp8 * ch->scopeVolume);

                        scopePos_f += (ch->scopeDrawDelta_f);
                    }

                    totalVoicesActive++;
                }
            }

            // render buffer

            for (x = 0; x < (200 - 3); ++x)
            {
                scopeTemp = scopeBuffer[x];
                if ((scopeTemp != 0) && (totalVoicesActive > 0))
                    scopeTemp = (55 + 21) + CLAMP(scopeTemp / mixScaleTable[totalVoicesActive - 1], -21, 22);
                else
                    scopeTemp =  55 + 21;

                pixelBuffer[(scopeTemp * SCREEN_W) + (120 + x)] = palette[PAL_QADSCP];
            }
        }
    }
}

void renderQuadrascopeBg(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    ptr32Src = trackerFrameBMP  + (44 * SCREEN_W) + 120;
    ptr32Dst = pixelBuffer + (44 * SCREEN_W) + 120;

    y = 55;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 200 * sizeof (int32_t));

        ptr32Src += SCREEN_W;
        ptr32Dst += SCREEN_W;
    }

    // fix two pixels in the tracker GUI
    pixelBuffer[(99 * SCREEN_W) + 318] = palette[PAL_BORDER];
    pixelBuffer[(99 * SCREEN_W) + 319] = palette[PAL_GENBKG];
}

void renderSpectrumAnalyzerBg(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    ptr32Src = spectrumVisualsBMP;
    ptr32Dst = pixelBuffer + (44 * SCREEN_W) + 120;

    y = 55;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 200 * sizeof (int32_t));

        ptr32Src += 200;
        ptr32Dst += SCREEN_W;
    }

    // fix two pixels in the tracker GUI
    pixelBuffer[(99 * SCREEN_W) + 318] = palette[PAL_BORDER];
    pixelBuffer[(99 * SCREEN_W) + 319] = palette[PAL_GENBKG];
}

void renderMonoscopeBg(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    ptr32Src = monoScopeBMP;
    ptr32Dst = pixelBuffer + (44 * SCREEN_W) + 120;

    y = 55;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 200 * sizeof (int32_t));

        ptr32Src += 200;
        ptr32Dst += SCREEN_W;
    }

    // change two pixels in the tracker GUI
    pixelBuffer[(99 * SCREEN_W) + 318] = palette[PAL_GENBKG];
    pixelBuffer[(99 * SCREEN_W) + 319] = palette[PAL_GENBKG2];
}

void renderAboutScreen(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    if (!editor.ui.diskOpScreenShown && !editor.ui.posEdScreenShown &&
        !editor.ui.editOpScreenShown &&  editor.ui.aboutScreenShown)
    {
        ptr32Src = aboutScreenBMP;
        ptr32Dst = pixelBuffer + (44 * SCREEN_W) + 120;

        y = 55;
        while (y--)
        {
            memcpy(ptr32Dst, ptr32Src, 200 * sizeof (int32_t));

            ptr32Src += 200;
            ptr32Dst += SCREEN_W;
        }
    }
}

void renderEditOpMode(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    // select what character box to render
    switch (editor.ui.editOpScreen)
    {
        default:
        case 0:
            ptr32Src = &editOpModeCharsBMP[editor.sampleAllFlag ? EDOP_MODE_BMP_A_OFS : EDOP_MODE_BMP_S_OFS];
        break;

        case 1:
        {
                 if (editor.trackPattFlag == 0) ptr32Src = &editOpModeCharsBMP[EDOP_MODE_BMP_T_OFS];
            else if (editor.trackPattFlag == 1) ptr32Src = &editOpModeCharsBMP[EDOP_MODE_BMP_P_OFS];
            else                                ptr32Src = &editOpModeCharsBMP[EDOP_MODE_BMP_S_OFS];
        }
        break;

        case 2:
            ptr32Src = &editOpModeCharsBMP[editor.halfClipFlag ? EDOP_MODE_BMP_C_OFS : EDOP_MODE_BMP_H_OFS];
        break;

        case 3:
            ptr32Src = (editor.newOldFlag == 0) ? &editOpModeCharsBMP[EDOP_MODE_BMP_N_OFS] : &editOpModeCharsBMP[EDOP_MODE_BMP_O_OFS];
        break;
    }

    // render it...
    ptr32Dst = pixelBuffer + (47 * SCREEN_W) + 310;
    y = 6;
    while (y--)
    {
        *(ptr32Dst + 0) = *(ptr32Src + 0);
        *(ptr32Dst + 1) = *(ptr32Src + 1);
        *(ptr32Dst + 2) = *(ptr32Src + 2);
        *(ptr32Dst + 3) = *(ptr32Src + 3);
        *(ptr32Dst + 4) = *(ptr32Src + 4);
        *(ptr32Dst + 5) = *(ptr32Src + 5);
        *(ptr32Dst + 6) = *(ptr32Src + 6);

        ptr32Src += 7;
        ptr32Dst += SCREEN_W;
    }
}

void renderEditOpScreen(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    // select which background to render
    switch (editor.ui.editOpScreen)
    {
        default:
        case 0: ptr32Src = editOpScreen1BMP; break;
        case 1: ptr32Src = editOpScreen2BMP; break;
        case 2: ptr32Src = editOpScreen3BMP; break;
        case 3: ptr32Src = editOpScreen4BMP; break;
    }

    // render background
    ptr32Dst = pixelBuffer + (44 * SCREEN_W) + 120;
    y = 55;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 200 * sizeof (int32_t));

        ptr32Src += 200;
        ptr32Dst += SCREEN_W;
    }

    renderEditOpMode();

    // render content
    if (editor.ui.editOpScreen == 0)
    {
        textOut(pixelBuffer, 128, 47, "  TRACK      PATTERN  ", palette[PAL_GENTXT]);
    }
    else if (editor.ui.editOpScreen == 1)
    {
        textOut(pixelBuffer, 128, 47, "  RECORD     SAMPLES  ", palette[PAL_GENTXT]);

        editor.ui.updateRecordText   = true;
        editor.ui.updateQuantizeText = true;
        editor.ui.updateMetro1Text   = true;
        editor.ui.updateMetro2Text   = true;
        editor.ui.updateFromText     = true;
        editor.ui.updateKeysText     = true;
        editor.ui.updateToText       = true;
    }
    else if (editor.ui.editOpScreen == 2)
    {
        textOut(pixelBuffer, 128, 47, "    SAMPLE EDITOR     ", palette[PAL_GENTXT]);
        charOut(pixelBuffer, 272, 91, '%', palette[PAL_GENTXT]); // for Volume text

        editor.ui.updatePosText = true;
        editor.ui.updateModText = true;
        editor.ui.updateVolText = true;
    }
    else if (editor.ui.editOpScreen == 3)
    {
        textOut(pixelBuffer, 128, 47, " SAMPLE CHORD EDITOR  ", palette[PAL_GENTXT]);

        editor.ui.updateLengthText = true;
        editor.ui.updateNote1Text  = true;
        editor.ui.updateNote2Text  = true;
        editor.ui.updateNote3Text  = true;
        editor.ui.updateNote4Text  = true;
    }
}

void removeTerminalScreen(void)
{
    displayMainScreen();

    // re-render middle-screen background
    memcpy(&pixelBuffer[99 * SCREEN_W], &trackerFrameBMP[99 * SCREEN_W], 320 * 22 * sizeof (int32_t));

    if (editor.ui.samplerScreenShown)
    {
        memcpy(&pixelBuffer[(121 * SCREEN_W)], samplerScreenBMP, 320 * 134 * sizeof (int32_t));

        editor.ui.updateStatusText = true;
        editor.ui.updateSongSize = true;
        editor.ui.updateSongTiming = true;
        editor.ui.updateResampleNote = true;
        editor.ui.update9xxPos = true;

        displaySample();

        if (editor.ui.samplerVolBoxShown)
            renderSamplerVolBox();
        else if (editor.ui.samplerFiltersBoxShown)
            renderSamplerFiltersBox();
    }
    else
    {
        updateCursorPos();
    }
}

void renderMOD2WAVDialog(void)
{
    uint8_t y;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst;

    ptr32Src = mod2wavBMP;
    ptr32Dst = pixelBuffer + ((27 * SCREEN_W) + 64);

    y = 48;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 192 * sizeof (int32_t));

        ptr32Src += 192;
        ptr32Dst += SCREEN_W;
    }
}

void updateMOD2WAVDialog(void)
{
    uint8_t x, y, barLength, percent;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst, pixel;

    if (editor.ui.updateMod2WavDialog)
    {
        editor.ui.updateMod2WavDialog = false;

        if (editor.isWAVRendering)
        {
            if (editor.ui.mod2WavFinished)
            {
                editor.ui.mod2WavFinished = false;

                resetSong();
                pointerSetMode(POINTER_MODE_IDLE, DO_CARRY);

                if (editor.abortMod2Wav)
                {
                    displayErrorMsg("MOD2WAV ABORTED !");
                    terminalPrintf("MOD2WAV aborted\n");
                }
                else
                {
                    displayMsg("MOD RENDERED !");
                    setMsgPointer();

                    if (modEntry->head.moduleTitle[0] == '\0')
                        terminalPrintf("Module \"untitled\" rendered to WAV\n");
                    else
                        terminalPrintf("Module \"%s\" rendered to WAV\n", modEntry->head.moduleTitle);
                }

                editor.isWAVRendering = false;
                SDL_WaitThread(editor.mod2WavThread, NULL);
                displayMainScreen();
            }
            else
            {
                // clear progress bar background
                ptr32Src = mod2wavBMP + ((15 * 192) + 6);
                ptr32Dst = pixelBuffer + ((42 * SCREEN_W) + 70);
                y = 11;
                while (y--)
                {
                    memcpy(ptr32Dst, ptr32Src, 180 * sizeof (int32_t));
                    ptr32Dst += SCREEN_W;
                }

                // render progress bar
                percent = getSongProgressInPercentage();
                if (percent > 100)
                    percent = 100;

                barLength = (uint8_t)(((float)(percent) * (180.0f / 100.0f)) + 0.5f);

                ptr32Dst = pixelBuffer + ((42 * SCREEN_W) + 70);
                pixel    = palette[PAL_GENBKG2];

                y = 11;
                while (y--)
                {
                    x = barLength;
                    while (x--)
                        *ptr32Dst++ = pixel;

                    ptr32Dst += (SCREEN_W - barLength);
                }

                // render percentage
                pixel = palette[PAL_GENTXT];
                if (percent > 99)
                    printThreeDecimals(pixelBuffer, 144, 45, percent, pixel);
                else
                    printTwoDecimals(pixelBuffer,   152, 45, percent, pixel);

                charOut(pixelBuffer, 168, 45, '%', pixel);
            }
        }
    }
}

void updateEditOp(void)
{
    if (!editor.ui.posEdScreenShown && !editor.ui.diskOpScreenShown && editor.ui.editOpScreenShown)
    {
        if (editor.ui.editOpScreen == 1)
        {
            if (editor.ui.updateRecordText)
            {
                editor.ui.updateRecordText = false;
                textOutBg(pixelBuffer, 176, 58, (editor.recordMode == RECORD_PATT) ? "PATT" : "SONG",
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateQuantizeText)
            {
                editor.ui.updateQuantizeText = false;
                printTwoDecimalsBg(pixelBuffer, 192, 69, *editor.quantizeValueDisp,
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateMetro1Text)
            {
                editor.ui.updateMetro1Text = false;
                printTwoDecimalsBg(pixelBuffer, 168, 80, *editor.metroSpeedDisp,
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateMetro2Text)
            {
                editor.ui.updateMetro2Text = false;
                printTwoDecimalsBg(pixelBuffer, 192, 80, *editor.metroChannelDisp,
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateFromText)
            {
                editor.ui.updateFromText = false;
                printTwoHexBg(pixelBuffer, 264, 80, *editor.sampleFromDisp,
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateKeysText)
            {
                editor.ui.updateKeysText = false;
                textOutBg(pixelBuffer, 160, 91, editor.multiFlag ? "MULTI " : "SINGLE",
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateToText)
            {
                editor.ui.updateToText = false;
                printTwoHexBg(pixelBuffer, 264, 91, *editor.sampleToDisp,
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
        }
        else if (editor.ui.editOpScreen == 2)
        {
            if (editor.ui.updateMixText)
            {
                editor.ui.updateMixText = false;
                if (editor.mixFlag)
                {
                    textOutBg(pixelBuffer, 128, 47, editor.mixText, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    textOutBg(pixelBuffer, 248, 47, "  ", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                }
                else
                {
                    textOutBg(pixelBuffer, 128, 47, "    SAMPLE EDITOR     ",
                        palette[PAL_GENTXT], palette[PAL_GENBKG]);
                }
            }

            if (editor.ui.updatePosText)
            {
                editor.ui.updatePosText = false;
                printFiveHexBg(pixelBuffer, 240, 58, *editor.samplePosDisp,
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateModText)
            {
                editor.ui.updateModText = false;
                printThreeDecimalsBg(pixelBuffer, 256, 69,
                    (editor.modulateSpeed < 0) ? (0 - editor.modulateSpeed) : editor.modulateSpeed,
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);

                if (editor.modulateSpeed < 0)
                    charOutBg(pixelBuffer, 248, 69, '-', palette[PAL_GENTXT], palette[PAL_GENBKG]);
                else
                    charOutBg(pixelBuffer, 248, 69, ' ', palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateVolText)
            {
                editor.ui.updateVolText = false;
                printThreeDecimalsBg(pixelBuffer, 248, 91, *editor.sampleVolDisp,
                    palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
        }
        else if (editor.ui.editOpScreen == 3)
        {
            if (editor.ui.updateLengthText)
            {
                editor.ui.updateLengthText = false;

                // clear background
                textOutBg(pixelBuffer, 160, 91, "     ", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                charOut(pixelBuffer,   198, 91,     ':', palette[PAL_GENBKG]);

                if ((modEntry->samples[editor.currSample].loopLength > 2) || (modEntry->samples[editor.currSample].loopStart >= 2))
                {
                    textOut(pixelBuffer, 165, 91, "LOOP", palette[PAL_GENTXT]);
                }
                else
                {
                    printFiveHex(pixelBuffer, 160, 91, *editor.chordLengthDisp, palette[PAL_GENTXT]); // CHORD MAX LENGTH
                    charOut(pixelBuffer, 198, 91, (editor.chordLengthMin) ? '.' : ':', palette[PAL_GENTXT]); // MIN/MAX FLAG
                }
            }

            if (editor.ui.updateNote1Text)
            {
                editor.ui.updateNote1Text = false;
                if (editor.note1 > 35)
                    textOutBg(pixelBuffer, 256, 58, "---", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                else
                    textOutBg(pixelBuffer, 256, 58, editor.accidental ? noteNames2[editor.note1] : noteNames1[editor.note1],
                        palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateNote2Text)
            {
                editor.ui.updateNote2Text = false;
                if (editor.note2 > 35)
                    textOutBg(pixelBuffer, 256, 69, "---", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                else
                    textOutBg(pixelBuffer, 256, 69, editor.accidental ? noteNames2[editor.note2] : noteNames1[editor.note2],
                        palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }

            if (editor.ui.updateNote3Text)
            {
                editor.ui.updateNote3Text = false;
                if (editor.note3 > 35)
                    textOutBg(pixelBuffer, 256, 80, "---", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                else
                    textOutBg(pixelBuffer, 256, 80, editor.accidental ? noteNames2[editor.note3] : noteNames1[editor.note3],
                        palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
            
            if (editor.ui.updateNote4Text)
            {
                editor.ui.updateNote4Text = false;
                if (editor.note4 > 35)
                    textOutBg(pixelBuffer, 256, 91, "---", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                else
                    textOutBg(pixelBuffer, 256, 91, editor.accidental ? noteNames2[editor.note4] : noteNames1[editor.note4],
                        palette[PAL_GENTXT], palette[PAL_GENBKG]);
            }
        }
    }
}

void displayMainScreen(void)
{
    editor.blockMarkFlag = false;

    editor.ui.updateSongTime     = true;
    editor.ui.updateSongName     = true;
    editor.ui.updateSongSize     = true;
    editor.ui.updateSongTiming   = true;
    editor.ui.updateTrackerFlags = true;
    editor.ui.updateStatusText   = true;

    editor.ui.updateCurrSampleName = true;

    if (!editor.ui.diskOpScreenShown)
    {
        editor.ui.updateCurrSampleFineTune = true;
        editor.ui.updateCurrSampleNum      = true;
        editor.ui.updateCurrSampleVolume   = true;
        editor.ui.updateCurrSampleLength   = true;
        editor.ui.updateCurrSampleRepeat   = true;
        editor.ui.updateCurrSampleReplen   = true;
    }

    if (editor.ui.samplerScreenShown)
    {
        if (!editor.ui.diskOpScreenShown)
            memcpy(pixelBuffer, trackerFrameBMP, 320 * 121 * sizeof (int32_t));
    }
    else
    {
        if (!editor.ui.diskOpScreenShown)
            memcpy(pixelBuffer, trackerFrameBMP, 320 * 255 * sizeof (int32_t));
        else
            memcpy(&pixelBuffer[121 * SCREEN_W], &trackerFrameBMP[121 * SCREEN_W], 320 * 134 * sizeof (int32_t));

        editor.ui.updateSongBPM      = true;
        editor.ui.updateCurrPattText = true;
        editor.ui.updatePatternData  = true;
    }

    if (editor.ui.diskOpScreenShown)
    {
        renderDiskOpScreen();
    }
    else
    {
        editor.ui.updateSongPos     = true;
        editor.ui.updateSongPattern = true;
        editor.ui.updateSongLength  = true;

        // zeroes (can't integrate zeroes in the graphics, the palette entry is above the 2-bit range)
        charOut(pixelBuffer, 64,  3,  '0', palette[PAL_GENTXT]);
        textOut(pixelBuffer, 64, 14, "00", palette[PAL_GENTXT]);

        if (!editor.isWAVRendering)
        {
            charOut(pixelBuffer, 64, 25,  '0', palette[PAL_GENTXT]);
            textOut(pixelBuffer, 64, 47, "00", palette[PAL_GENTXT]);
            textOut(pixelBuffer, 64, 58, "00", palette[PAL_GENTXT]);
        }

        if (editor.ui.posEdScreenShown)
        {
            renderPosEdScreen();
            editor.ui.updatePosEd = true;
        }
        else
        {
            if (editor.ui.editOpScreenShown)
            {
                renderEditOpScreen();
            }
            else
            {
                if (editor.ui.aboutScreenShown)
                {
                    renderAboutScreen();
                }
                else
                {
                         if (editor.ui.visualizerMode == VISUAL_QUADRASCOPE) renderQuadrascopeBg();
                    else if (editor.ui.visualizerMode == VISUAL_SPECTRUM   ) renderSpectrumAnalyzerBg();
                    else                                                     renderMonoscopeBg();
                }
            }

            renderMuteButtons();
        }
    }
}

void handleAskNo(void)
{
    editor.ui.pat2SmpDialogShown = false;

    switch (editor.ui.askScreenType)
    {
        case ASK_SAVEMOD_OVERWRITE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            saveModule(DONT_CHECK_IF_FILE_EXIST, GIVE_NEW_FILENAME);
        }
        break;

        case ASK_SAVESMP_OVERWRITE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            saveSample(DONT_CHECK_IF_FILE_EXIST, GIVE_NEW_FILENAME);
        }
        break;

        case ASK_DOWNSAMPLING:
        {
            extLoadWAVSampleCallback(DONT_DOWNSAMPLE);

            pointerSetPreviousMode();
            setPrevStatusMessage();
        }
        break;

        default:
        {
            pointerSetPreviousMode();
            setPrevStatusMessage();

            editor.errorMsgActive  = true;
            editor.errorMsgBlock   = true;
            editor.errorMsgCounter = 0;

            pointerErrorMode();
        }
        break;
    }

    removeAskDialog();
}

void handleAskYes(void)
{
    char fileName[20 + 4 + 1];
    int8_t *tmpSmpBuffer, oldSample, oldRow;
    int32_t j, newLength, oldSamplesPerTick;
    uint32_t i;
    float tmpFloat;
    moduleSample_t *s;

    switch (editor.ui.askScreenType)
    {
        case ASK_RESTORE_SAMPLE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            redoSampleData(editor.currSample);
        }
        break;

        case ASK_PAT2SMP:
        {
            editor.ui.pat2SmpDialogShown = false;

            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            editor.pat2SmpBuf = (int16_t *)(malloc(MAX_SAMPLE_LEN * sizeof (int16_t)));
            if (editor.pat2SmpBuf == NULL)
            {
                displayErrorMsg(editor.outOfMemoryText);
                terminalPrintf("PAT2SMP failed: out of memory!\n");

                return;
            }

            oldRow = editor.songPlaying ? 0 : modEntry->currRow;
            oldSamplesPerTick = samplesPerTick;

            editor.isSMPRendering = true; // this must be set before restartSong()
            storeTempVariables();
            restartSong();

            modEntry->row = oldRow;
            modEntry->currRow = modEntry->row;

            editor.blockMarkFlag = false;

            pointerSetMode(POINTER_MODE_READ_DIR, NO_CARRY);
            setStatusMessage("RENDERING...", NO_CARRY);

            // set temporary samplesPerTick
            tmpFloat = (editor.pat2SmpHQ ? 28836.0f : 22168.0f) * (125.0f / 50.0f);
            mixerSetSamplesPerTick((int32_t)((tmpFloat / (float)(modEntry->currBPM)) + 0.5f));

            editor.pat2SmpPos = 0;
            while (editor.isSMPRendering)
            {
                // process tick
                if (editor.songPlaying)
                {
                    if (!processTick())
                        editor.songPlaying = false;
                }
                else
                {
                    resetSong();
                    editor.isSMPRendering = false;

                    break; // rendering is done
                }

                // output tick to sample
                outputAudioToSample(samplesPerTick);
            }

            // set back old row and samplesPerTick
            modEntry->row = oldRow;
            modEntry->currRow = modEntry->row;
            mixerSetSamplesPerTick(oldSamplesPerTick);

            // normalize 16-bit buffer
            normalize16bitSigned(editor.pat2SmpBuf, MIN(editor.pat2SmpPos, MAX_SAMPLE_LEN));

            // clear all of the old sample
            memset(&modEntry->sampleData[modEntry->samples[editor.currSample].offset], 0, MAX_SAMPLE_LEN);

            // quantize to 8-bit
            for (i = 0; i < editor.pat2SmpPos; ++i)
                modEntry->sampleData[modEntry->samples[editor.currSample].offset + i] = quantize16bitTo8bit(editor.pat2SmpBuf[i]);

            // free temp mixing buffer
            free(editor.pat2SmpBuf);

            // zero out sample text
            memset(modEntry->samples[editor.currSample].text, 0, sizeof (modEntry->samples[editor.currSample].text));

            // set new sample text
            if (editor.pat2SmpHQ)
            {
                strcpy(modEntry->samples[editor.currSample].text, "pat2smp (a-3 tune:+5)");
                modEntry->samples[editor.currSample].fineTune = 5;
            }
            else
            {
                strcpy(modEntry->samples[editor.currSample].text, "pat2smp (f-3 tune:+1)");
                modEntry->samples[editor.currSample].fineTune = 1;
            }

            // new sample attributes
            modEntry->samples[editor.currSample].length     = editor.pat2SmpPos;
            modEntry->samples[editor.currSample].volume     = 64;
            modEntry->samples[editor.currSample].loopStart  = 0;
            modEntry->samples[editor.currSample].loopLength = 2;

            pointerSetMode(POINTER_MODE_IDLE, DO_CARRY);

            displayMsg("ROWS RENDERED!");
            setMsgPointer();
            terminalPrintf("Pattern rendered from row %d to sample slot %02X\n", oldRow, editor.currSample + 1);

            editor.samplePos = 0;
            updateCurrSample();
        }
        break;

        case ASK_SAVE_ALL_SAMPLES:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            oldSample = editor.currSample;

            for (i = 0; i < MOD_SAMPLES; ++i)
            {
                editor.currSample = (int8_t)(i);
                if (modEntry->samples[i].length > 2)
                    saveSample(DONT_CHECK_IF_FILE_EXIST, GIVE_NEW_FILENAME);
            }

            editor.currSample = oldSample;

            displayMsg("SAMPLES SAVED !");
            terminalPrintf("Saved all samples to the current directory\n");
            setMsgPointer();
        }
        break;

        case ASK_MAKE_CHORD:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            mixChordSample();
        }
        break;

        case ASK_BOOST_ALL_SAMPLES: // for insane minds
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            for (i = 0; i < MOD_SAMPLES; ++i)
                boostSample(i, true);

            if (editor.ui.samplerScreenShown)
                redrawSample();

            updateWindowTitle(MOD_IS_MODIFIED);
        }
        break;

        case ASK_FILTER_ALL_SAMPLES: // for insane minds
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            for (i = 0; i < MOD_SAMPLES; ++i)
                filterSample(i, true);

            if (editor.ui.samplerScreenShown)
                redrawSample();

            updateWindowTitle(MOD_IS_MODIFIED);
        }
        break;

        case ASK_UPSAMPLE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            s = &modEntry->samples[editor.currSample];

            tmpSmpBuffer = (int8_t *)(malloc(s->length));
            if (tmpSmpBuffer == NULL)
            {
                displayErrorMsg(editor.outOfMemoryText);
                terminalPrintf("Sample upsample failed: out of memory!\n");

                return;
            }

            newLength = (s->length / 2) & 0xFFFFFFFE;
            if (newLength < 2)
                return;

            mixerKillVoiceIfReadingSample(editor.currSample);

            memcpy(tmpSmpBuffer, &modEntry->sampleData[s->offset], s->length);
            memset(&modEntry->sampleData[s->offset], 0, MAX_SAMPLE_LEN);

            // upsample
            for (j = 0; j < newLength; ++j)
                modEntry->sampleData[s->offset + j] = tmpSmpBuffer[j * 2];

            free(tmpSmpBuffer);

            s->length     = newLength;
            s->loopStart  = (s->loopStart  / 2) & 0xFFFFFFFE;
            s->loopLength = (s->loopLength / 2) & 0xFFFFFFFE;

            if (s->loopLength < 2)
            {
                s->loopStart  = 0;
                s->loopLength = 2;
            }

            updateCurrSample();

            editor.ui.updateSongSize = true;
            updateWindowTitle(MOD_IS_MODIFIED);
        }
        break;

        case ASK_DOWNSAMPLE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            s = &modEntry->samples[editor.currSample];

            tmpSmpBuffer = (int8_t *)(malloc(s->length));
            if (tmpSmpBuffer == NULL)
            {
                displayErrorMsg(editor.outOfMemoryText);
                terminalPrintf("Sample downsample failed: out of memory!\n");

                return;
            }

            newLength = s->length * 2;
            if (newLength > MAX_SAMPLE_LEN)
                newLength = MAX_SAMPLE_LEN;

            mixerKillVoiceIfReadingSample(editor.currSample);

            memcpy(tmpSmpBuffer, &modEntry->sampleData[s->offset], s->length);
            memset(&modEntry->sampleData[s->offset], 0, MAX_SAMPLE_LEN);

            // downsample
            for (j = 0; j < newLength; ++j)
                modEntry->sampleData[s->offset + j] = tmpSmpBuffer[j / 2];

            free(tmpSmpBuffer);

            s->length = newLength;

            if (s->loopLength > 2)
            {
                s->loopStart  *= 2;
                s->loopLength *= 2;

                if ((s->loopStart + s->loopLength) > s->length)
                {
                    s->loopStart  = 0;
                    s->loopLength = 2;
                }
            }

            updateCurrSample();

            editor.ui.updateSongSize = true;
            updateWindowTitle(MOD_IS_MODIFIED);
        }
        break;

        case ASK_KILL_SAMPLE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            mixerKillVoiceIfReadingSample(editor.currSample);

            modEntry->samples[editor.currSample].fineTune   = 0;
            modEntry->samples[editor.currSample].volume     = 0;
            modEntry->samples[editor.currSample].length     = 0;
            modEntry->samples[editor.currSample].loopStart  = 0;
            modEntry->samples[editor.currSample].loopLength = 2;

            memset(&modEntry->samples[editor.currSample].text, 0, sizeof (modEntry->samples[editor.currSample].text));
            memset(&modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN)], 0, MAX_SAMPLE_LEN);

            editor.samplePos = 0;
            updateCurrSample();

            editor.ui.updateSongSize = true;
            updateWindowTitle(MOD_IS_MODIFIED);
        }
        break;

        case ASK_RESAMPLE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            samplerResample();
        }
        break;

        case ASK_DOWNSAMPLING:
        {
            // for WAV sample loader

            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            extLoadWAVSampleCallback(DO_DOWNSAMPLE);
        }
        break;

        case ASK_MOD2WAV_OVERWRITE:
        {
            memset(fileName, 0, sizeof (fileName));

            if (modEntry->head.moduleTitle[0] != '\0')
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

                strcat(fileName, ".wav");
            }
            else
            {
                strcpy(fileName, "untitled.wav");
            }

            renderToWav(fileName, DONT_CHECK_IF_FILE_EXIST);
        }
        break;

        case ASK_MOD2WAV:
        {
            memset(fileName, 0, sizeof (fileName));

            if (modEntry->head.moduleTitle[0] != '\0')
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

                strcat(fileName, ".wav");
            }
            else
            {
                strcpy(fileName, "untitled.wav");
            }

            renderToWav(fileName, CHECK_IF_FILE_EXIST);
        }
        break;

        case ASK_QUIT:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            editor.ui.throwExit = true;
        }
        break;

        case ASK_SAVE_SAMPLE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            saveSample(CHECK_IF_FILE_EXIST, DONT_GIVE_NEW_FILENAME);
        }
        break;

        case ASK_SAVESMP_OVERWRITE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            saveSample(DONT_CHECK_IF_FILE_EXIST, DONT_GIVE_NEW_FILENAME);
        }
        break;

        case ASK_SAVE_MODULE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            saveModule(CHECK_IF_FILE_EXIST, DONT_GIVE_NEW_FILENAME);
        }
        break;

        case ASK_SAVEMOD_OVERWRITE:
        {
            editor.errorMsgActive  = false;
            editor.errorMsgBlock   = false;
            editor.errorMsgCounter = 0;

            pointerSetPreviousMode();
            setPrevStatusMessage();

            saveModule(DONT_CHECK_IF_FILE_EXIST, DONT_GIVE_NEW_FILENAME);
        }
        break;

        default: break;
    }

    removeAskDialog();
}

void createGraphics(void)
{
    uint8_t j, x, y, r8, g8, b8;
    int16_t r, g, b, r2, g2, b2;
    uint16_t i;

    // create pattern cursor graphics
    for (y = 0; y < 14; ++y)
    {
        // top two rows have a lighter color
        if (y < 2)
        {
            r = RGB_R(palette[PAL_PATCURSOR]);
            g = RGB_G(palette[PAL_PATCURSOR]);
            b = RGB_B(palette[PAL_PATCURSOR]);

            r += 0x33;
            g += 0x33;
            b += 0x33;

            if (r > 0xFF) r = 0xFF;
            if (g > 0xFF) g = 0xFF;
            if (b > 0xFF) b = 0xFF;

            r8 = r & 0x00FF;
            g8 = g & 0x00FF;
            b8 = b & 0x00FF;

            for (x = 0; x < 11; ++x)
                patternCursorBMP[(y * 11) + x] = TO_RGB(r8, g8, b8);
        }

        // sides (same color)
        if ((y >= 2) && y <= 12)
        {
            patternCursorBMP[(y * 11) + 0] = palette[PAL_PATCURSOR];

            for (x = 1; x < 10; ++x)
                patternCursorBMP[(y * 11) + x] = palette[PAL_COLORKEY];

            patternCursorBMP[(y * 11) + 10] = palette[PAL_PATCURSOR];
        }

        // bottom two rows have a darker color
        if (y > 11)
        {
            r = RGB_R(palette[PAL_PATCURSOR]);
            g = RGB_G(palette[PAL_PATCURSOR]);
            b = RGB_B(palette[PAL_PATCURSOR]);

            r -= 0x33;
            g -= 0x33;
            b -= 0x33;

            if (r < 0x00) r = 0x00;
            if (g < 0x00) g = 0x00;
            if (b < 0x00) b = 0x00;

            r8 = r & 0x00FF;
            g8 = g & 0x00FF;
            b8 = b & 0x00FF;

            for (x = 0; x < 11; ++x)
                patternCursorBMP[(y * 11) + x] = TO_RGB(r8, g8, b8);
        }
    }

    // create spectrum analyzer bar graphics
    for (i = 0; i < 36; ++i)
    {
        r8 = ((analyzerColors[35 - i] & 0x0F00) >> 8) * 17;
        g8 = ((analyzerColors[35 - i] & 0x00F0) >> 4) * 17;
        b8 = ((analyzerColors[35 - i] & 0x000F) >> 0) * 17;

        spectrumAnaBMP[i] = TO_RGB(r8, g8, b8);
    }

    // create VU-Meter bar graphics
    for (i = 0; i < 48; ++i)
    {
        r = ((vuMeterColors[47 - i] & 0x0F00) >> 8) * 17;
        g = ((vuMeterColors[47 - i] & 0x00F0) >> 4) * 17;
        b = ((vuMeterColors[47 - i] & 0x000F) >> 0) * 17;

        // brighter pixels on the left side
        r2 = r + 0x33;
        g2 = g + 0x33;
        b2 = b + 0x33;

        if (r2 > 0xFF) r2 = 0xFF;
        if (g2 > 0xFF) g2 = 0xFF;
        if (b2 > 0xFF) b2 = 0xFF;

        vuMeterBMP[(i * 10) + 0] = TO_RGB(r2, g2, b2);
        vuMeterBMP[(i * 10) + 1] = vuMeterBMP[(i * 10) + 0];

        // main pixels
        for (j = 2; j < 8; ++j)
            vuMeterBMP[(i * 10) + j] = TO_RGB(r, g, b);

        // darker pixels on the right side
        r2 = r - 0x33;
        g2 = g - 0x33;
        b2 = b - 0x33;

        if (r2 < 0x00) r2 = 0x00;
        if (g2 < 0x00) g2 = 0x00;
        if (b2 < 0x00) b2 = 0x00;

        vuMeterBMP[(i * 10) + 8] = TO_RGB(r2, g2, b2);
        vuMeterBMP[(i * 10) + 9] = vuMeterBMP[(i * 10) + 8];
    }

    for (i = 0; i < 64;  ++i) samplingPosBMP[i] = samplingPosBMP[i];
    for (i = 0; i < 512; ++i) loopPinsBMP[i]    = loopPinsBMP[i];
}

void freeBMPs(void)
{
    if (trackerFrameBMP    != NULL) free(trackerFrameBMP);
    if (monoScopeBMP       != NULL) free(monoScopeBMP);
    if (samplerScreenBMP   != NULL) free(samplerScreenBMP);
    if (samplerVolumeBMP   != NULL) free(samplerVolumeBMP);
    if (samplerFiltersBMP  != NULL) free(samplerFiltersBMP);
    if (clearDialogBMP     != NULL) free(clearDialogBMP);
    if (diskOpScreenBMP    != NULL) free(diskOpScreenBMP);
    if (mod2wavBMP         != NULL) free(mod2wavBMP);
    if (posEdBMP           != NULL) free(posEdBMP);
    if (spectrumVisualsBMP != NULL) free(spectrumVisualsBMP);
    if (yesNoDialogBMP     != NULL) free(yesNoDialogBMP);
    if (pat2SmpDialogBMP   != NULL) free(pat2SmpDialogBMP);
    if (editOpScreen1BMP   != NULL) free(editOpScreen1BMP);
    if (editOpScreen2BMP   != NULL) free(editOpScreen2BMP);
    if (editOpScreen3BMP   != NULL) free(editOpScreen3BMP);
    if (editOpScreen4BMP   != NULL) free(editOpScreen4BMP);
    if (aboutScreenBMP     != NULL) free(aboutScreenBMP);
    if (muteButtonsBMP     != NULL) free(muteButtonsBMP);
    if (editOpModeCharsBMP != NULL) free(editOpModeCharsBMP);
    if (arrowBMP           != NULL) free(arrowBMP);
    if (termTopBMP         != NULL) free(termTopBMP);
    if (termScrollBarBMP   != NULL) free(termScrollBarBMP);
}

uint32_t *unpackBMP(const uint8_t *src, uint32_t packedLen)
{
    const uint8_t *packSrc;
    uint8_t *tmpBuffer, *packDst, byteIn;
    int16_t count;
    uint32_t *dst, decodedLength, i;

    // RLE decode
    decodedLength = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

    tmpBuffer = (uint8_t *)(malloc(decodedLength + 512)); // some margin is needed, the packer is buggy
    if (tmpBuffer == NULL)
        return (NULL);

    packSrc = src + 4;
    packDst = tmpBuffer;

    i = packedLen - 4;
    while (i > 0)
    {
        byteIn = *packSrc++;
        if (byteIn == 0xCC) // compactor code
        {
            count  = *packSrc++;
            byteIn = *packSrc++;

            while (count >= 0)
            {
                *packDst++ = byteIn;
                count--;
            }

            i -= 2;
        }
        else
        {
            *packDst++ = byteIn;
        }

        i--;
    }

    // 2-bit to 8-bit conversion
    dst = (uint32_t *)(malloc((decodedLength * 4) * sizeof (int32_t)));
    if (dst == NULL)
    {
        free(tmpBuffer);
        return (NULL);
    }

    for (i = 0; i < decodedLength; ++i)
    {
        dst[(i * 4) + 0] = palette[(tmpBuffer[i] & 0xC0) >> 6];
        dst[(i * 4) + 1] = palette[(tmpBuffer[i] & 0x30) >> 4];
        dst[(i * 4) + 2] = palette[(tmpBuffer[i] & 0x0C) >> 2];
        dst[(i * 4) + 3] = palette[(tmpBuffer[i] & 0x03) >> 0];
    }

    free(tmpBuffer);

    return (dst);
}

int8_t unpackBMPs(void)
{
    uint8_t i;

    trackerFrameBMP    = unpackBMP(trackerFramePackedBMP,    sizeof (trackerFramePackedBMP));
    monoScopeBMP       = unpackBMP(monoScopePackedBMP,       sizeof (monoScopePackedBMP));
    samplerScreenBMP   = unpackBMP(samplerScreenPackedBMP,   sizeof (samplerScreenPackedBMP));
    samplerVolumeBMP   = unpackBMP(samplerVolumePackedBMP,   sizeof (samplerVolumePackedBMP));
    samplerFiltersBMP  = unpackBMP(samplerFiltersPackedBMP,  sizeof (samplerFiltersPackedBMP));
    clearDialogBMP     = unpackBMP(clearDialogPackedBMP,     sizeof (clearDialogPackedBMP));
    diskOpScreenBMP    = unpackBMP(diskOpScreenPackedBMP,    sizeof (diskOpScreenPackedBMP));
    mod2wavBMP         = unpackBMP(mod2wavPackedBMP,         sizeof (mod2wavPackedBMP));
    posEdBMP           = unpackBMP(posEdPackedBMP,           sizeof (posEdPackedBMP));
    spectrumVisualsBMP = unpackBMP(spectrumVisualsPackedBMP, sizeof (spectrumVisualsPackedBMP));
    yesNoDialogBMP     = unpackBMP(yesNoDialogPackedBMP,     sizeof (yesNoDialogPackedBMP));
    pat2SmpDialogBMP   = unpackBMP(pat2SmpDialogPackedBMP,   sizeof (pat2SmpDialogPackedBMP));
    editOpScreen1BMP   = unpackBMP(editOpScreen1PackedBMP,   sizeof (editOpScreen1PackedBMP));
    editOpScreen2BMP   = unpackBMP(editOpScreen2PackedBMP,   sizeof (editOpScreen2PackedBMP));
    editOpScreen3BMP   = unpackBMP(editOpScreen3PackedBMP,   sizeof (editOpScreen3PackedBMP));
    editOpScreen4BMP   = unpackBMP(editOpScreen4PackedBMP,   sizeof (editOpScreen4PackedBMP));
    aboutScreenBMP     = unpackBMP(aboutScreenPackedBMP,     sizeof (aboutScreenPackedBMP));
    muteButtonsBMP     = unpackBMP(muteButtonsPackedBMP,     sizeof (muteButtonsPackedBMP));
    editOpModeCharsBMP = unpackBMP(editOpModeCharsPackedBMP, sizeof (editOpModeCharsPackedBMP));
    termTopBMP         = unpackBMP(termTopPackedBMP,         sizeof (termTopPackedBMP));
    termScrollBarBMP   = unpackBMP(termScrollBarPackedBMP,   sizeof (termScrollBarPackedBMP));

    arrowBMP = (uint32_t *)(malloc(30 * sizeof (int32_t))); // different format

    if (
        (trackerFrameBMP   == NULL) || (samplerScreenBMP   == NULL) || (samplerVolumeBMP == NULL) ||
        (clearDialogBMP    == NULL) || (diskOpScreenBMP    == NULL) || (mod2wavBMP       == NULL) ||
        (posEdBMP          == NULL) || (spectrumVisualsBMP == NULL) || (yesNoDialogBMP   == NULL) ||
        (editOpScreen1BMP  == NULL) || (editOpScreen2BMP   == NULL) || (editOpScreen3BMP == NULL) ||
        (editOpScreen4BMP  == NULL) || (aboutScreenBMP     == NULL) || (monoScopeBMP     == NULL) ||
        (muteButtonsBMP    == NULL) || (editOpModeCharsBMP == NULL) || (arrowBMP         == NULL) ||
        (samplerFiltersBMP == NULL) || (termTopBMP         == NULL) || (termScrollBarBMP == NULL) ||
        (yesNoDialogBMP    == NULL)
       )
    {
        showErrorMsgBox("Out of memory!");
        return (false); // BMPs are free'd in cleanUp()
    }

    for (i = 0; i < 30; ++i)
        arrowBMP[i] = palette[arrowPaletteBMP[i]];

    createGraphics();

    return (true);
}

void videoClose(void)
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    free(pixelBuffer);
}

void setupSprites(void)
{
    uint8_t i;

    memset(sprites, 0, sizeof (sprites));

    sprites[SPRITE_MOUSE_POINTER].data = mousePointerBMP;
    sprites[SPRITE_MOUSE_POINTER].pixelType = SPRITE_TYPE_PALETTE;
    sprites[SPRITE_MOUSE_POINTER].colorKey = PAL_COLORKEY;
    sprites[SPRITE_MOUSE_POINTER].w = 16;
    sprites[SPRITE_MOUSE_POINTER].h = 16;
    sprites[SPRITE_MOUSE_POINTER].x = SCREEN_W; // initial state (hidden)
    sprites[SPRITE_MOUSE_POINTER].y = SCREEN_H; // initial state (hidden)
    hideSprite(SPRITE_MOUSE_POINTER);

    sprites[SPRITE_PATTERN_CURSOR].data = patternCursorBMP;
    sprites[SPRITE_PATTERN_CURSOR].pixelType = SPRITE_TYPE_RGB;
    sprites[SPRITE_PATTERN_CURSOR].colorKey = palette[PAL_COLORKEY];
    sprites[SPRITE_PATTERN_CURSOR].w = 11;
    sprites[SPRITE_PATTERN_CURSOR].h = 14;
    sprites[SPRITE_PATTERN_CURSOR].x = SCREEN_W; // initial state (hidden)
    sprites[SPRITE_PATTERN_CURSOR].y = SCREEN_H; // initial state (hidden)
    hideSprite(SPRITE_PATTERN_CURSOR);

    sprites[SPRITE_LOOP_PIN_LEFT].data = loopPinsBMP;
    sprites[SPRITE_LOOP_PIN_LEFT].pixelType = SPRITE_TYPE_RGB;
    sprites[SPRITE_LOOP_PIN_LEFT].colorKey = palette[PAL_COLORKEY];
    sprites[SPRITE_LOOP_PIN_LEFT].w = 4;
    sprites[SPRITE_LOOP_PIN_LEFT].h = 64;
    sprites[SPRITE_LOOP_PIN_LEFT].x = SCREEN_W; // initial state (hidden)
    sprites[SPRITE_LOOP_PIN_LEFT].y = SCREEN_H; // initial state (hidden)
    hideSprite(SPRITE_LOOP_PIN_LEFT);

    sprites[SPRITE_LOOP_PIN_RIGHT].data = loopPinsBMP + (4 * 64);
    sprites[SPRITE_LOOP_PIN_RIGHT].pixelType = SPRITE_TYPE_RGB;
    sprites[SPRITE_LOOP_PIN_RIGHT].colorKey = palette[PAL_COLORKEY];
    sprites[SPRITE_LOOP_PIN_RIGHT].w = 4;
    sprites[SPRITE_LOOP_PIN_RIGHT].h = 64;
    sprites[SPRITE_LOOP_PIN_RIGHT].x = SCREEN_W; // initial state (hidden)
    sprites[SPRITE_LOOP_PIN_RIGHT].y = SCREEN_H; // initial state (hidden)
    hideSprite(SPRITE_LOOP_PIN_RIGHT);

    sprites[SPRITE_SAMPLING_POS_LINE].data = samplingPosBMP;
    sprites[SPRITE_SAMPLING_POS_LINE].pixelType = SPRITE_TYPE_RGB;
    sprites[SPRITE_SAMPLING_POS_LINE].colorKey = palette[PAL_COLORKEY];
    sprites[SPRITE_SAMPLING_POS_LINE].w = 1;
    sprites[SPRITE_SAMPLING_POS_LINE].h = 64;
    sprites[SPRITE_SAMPLING_POS_LINE].x = SCREEN_W; // initial state (hidden)
    sprites[SPRITE_SAMPLING_POS_LINE].y = SCREEN_H; // initial state (hidden)
    hideSprite(SPRITE_SAMPLING_POS_LINE);

    // setup refresh buffer (used to clear sprites after each frame)
    for (i = 0; i < SPRITE_NUM; ++i)
        sprites[i].refreshBuffer = (uint32_t *)(malloc(sprites[i].w * sprites[i].h * sizeof (int32_t)));
}

void freeSprites(void)
{
    uint8_t i;

    for (i = 0; i < SPRITE_NUM; ++i)
        free(sprites[i].refreshBuffer);
}

void setSpritePos(uint8_t sprite, uint16_t x, uint16_t y)
{
    sprites[sprite].newX = x;
    sprites[sprite].newY = y;
}

void hideSprite(uint8_t sprite)
{
    sprites[sprite].newX = SCREEN_W;
    sprites[sprite].newY = SCREEN_H;
}

void eraseSprites(void)
{
    int8_t i;
    uint16_t x, y, sx, sy, sw, sh, dstAdd;
    const uint32_t *src32;
    uint32_t *dst32;

    for (i = (SPRITE_NUM - 1); i >= 0; --i) // reverse order, or else it will mess up
    {
        if ((sprites[i].y >= SCREEN_H) || (sprites[i].x >= SCREEN_W)) continue;

        sw = sprites[i].w;
        sh = sprites[i].h;
        sx = sprites[i].x;
        sy = sprites[i].y;

        src32  = sprites[i].refreshBuffer;
        if (src32 == NULL)
            return;

        dst32  = pixelBuffer + ((sy * SCREEN_W) + sx);
        dstAdd = SCREEN_W - sw;

        for (y = 0; y < sh; ++y)
        {
            if ((y + sy) >= SCREEN_H) break;

            for (x = 0; x < sw; ++x)
            {
                if ((x + sx) >= SCREEN_W)
                {
                    x = sw - x;
                    dst32 += x;
                    src32 += x;
                    break;
                }

                *dst32++ = *src32++;
            }

            dst32 += dstAdd;
        }
    }

    fillFromVuMetersBgBuffer(); // works differently, but let's put it here
}

void renderSprites(void)
{
    uint8_t i;
    const uint8_t *src8;
    uint16_t x, y, sx, sy, sw, sh, pitch;
    const uint32_t *src32;
    uint32_t *dst32, *clr32, colorKey;

    renderVuMeters(); // works differently, but let's put it here

    for (i = 0; i < SPRITE_NUM; ++i)
    {
        sprites[i].x = sprites[i].newX;
        sprites[i].y = sprites[i].newY;
        if ((sprites[i].y >= SCREEN_H) || (sprites[i].x >= SCREEN_W)) continue;

        sw = sprites[i].w;
        sh = sprites[i].h;
        sx = sprites[i].x;
        sy = sprites[i].y;

        dst32 = pixelBuffer + ((sy * SCREEN_W) + sx);
        clr32 = sprites[i].refreshBuffer;
        if (clr32 == NULL)
            return;

        pitch = SCREEN_W - sw;

        colorKey = sprites[i].colorKey;
        if (sprites[i].pixelType == SPRITE_TYPE_RGB)
        {
            src32 = (uint32_t *)(sprites[i].data);
            if (src32 == NULL)
                return;

            for (y = 0; y < sh; ++y)
            {
                if ((y + sy) >= SCREEN_H) break;

                for (x = 0; x < sw; ++x)
                {
                    if ((x + sx) >= SCREEN_W)
                    {
                        x = sw - x;
                        clr32 += x;
                        dst32 += x;
                        src32 += x;
                        break;
                    }

                    *clr32++ = *dst32; // fill refresh buffer
                    if (*src32 != colorKey)
                        *dst32  = *src32;

                    dst32++;
                    src32++;
                }

                dst32 += pitch;
            }
        }
        else
        {
            src8 = (uint8_t *)(sprites[i].data);
            if (src8 == NULL)
                return;

            for (y = 0; y < sh; ++y)
            {
                if ((y + sy) >= SCREEN_H) break;

                for (x = 0; x < sw; ++x)
                {
                    if ((x + sx) >= SCREEN_W)
                    {
                        x = sw - x;
                        clr32 += x;
                        dst32 += x;
                        src8  += x;
                        break;
                    }

                    *clr32++ = *dst32; // fill refresh buffer
                    if (*src8 != colorKey)
                    {
                        if (*src8 < PALETTE_NUM)
                            *dst32 = palette[*src8];
                    }

                    dst32++;
                    src8++;
                }

                dst32 += pitch;
            }
        }
    }
}

void flipFrame(void)
{
    SDL_UpdateTexture(texture, NULL, pixelBuffer, SCREEN_W * sizeof (int32_t));

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void updateSpectrumAnalyzer(int16_t period, int8_t volume)
{
    int16_t scaledVol, scaledNote;

    if (volume > 0)
    {
        scaledVol  = (volume * 256) / 682;              //   682 = (64 * 256) / 24    (64 = max sample vol)
        scaledNote = 743 - (period - 113);              //   743 = 856 - 113          (856 = C-1 period, 113 = B-3 period)
        scaledNote = (scaledNote * scaledNote) / 25093; // 25093 = (743 * 743) / 22   (22 = num of spectrum bars-1)

        // scaledNote now ranges 0..22, no need to clamp

        // increment main spectrum bar
        editor.spectrumVolumes[scaledNote] += scaledVol;
        if (editor.spectrumVolumes[scaledNote] > SPECTRUM_BAR_HEIGHT)
            editor.spectrumVolumes[scaledNote] = SPECTRUM_BAR_HEIGHT;

        // increment left side of spectrum bar with half volume
        if (scaledNote > 0)
        {
            editor.spectrumVolumes[scaledNote - 1] += (scaledVol / 2);
            if (editor.spectrumVolumes[scaledNote - 1] > SPECTRUM_BAR_HEIGHT)
                editor.spectrumVolumes[scaledNote - 1] = SPECTRUM_BAR_HEIGHT;
        }

        // increment right side of spectrum bar with half volume
        if (scaledNote < (SPECTRUM_BAR_NUM - 1))
        {
            editor.spectrumVolumes[scaledNote + 1] += (scaledVol / 2);
            if (editor.spectrumVolumes[scaledNote + 1] > SPECTRUM_BAR_HEIGHT)
                editor.spectrumVolumes[scaledNote + 1] = SPECTRUM_BAR_HEIGHT;
        }
    }
}

static void triggerScope(moduleChannel_t *ch, moduleSample_t *s)
{
    ch->scopeEnabled   = true;
    ch->scopeLoopFlag  = (s->loopStart + s->loopLength) > 2;

    ch->scopeEnd       = s->offset + s->length;
    ch->scopeLoopBegin = s->offset + s->loopStart;
    ch->scopeLoopEnd   = s->offset + s->loopStart + s->loopLength;

    // one-shot loop simulation (real PT didn't show this in the scopes...)
    ch->scopeLoopQuirk = false;
    if ((s->loopLength > 2) && (s->loopStart == 0))
    {
        ch->scopeLoopQuirk = ch->scopeLoopEnd;
        ch->scopeLoopEnd   = ch->scopeEnd;
    }

    ch->scopeLoopQuirk_f = ch->scopeLoopQuirk;
    ch->scopeLoopBegin_f = ch->scopeLoopBegin;
    ch->scopeLoopEnd_f   = ch->scopeLoopEnd;
    ch->scopeEnd_f       = ch->scopeEnd;

    if (ch->scopeChangePos)
    {
        ch->scopeChangePos = false;

        // ch->scopePos was externally modified

        if (ch->scopeLoopFlag)
        {
            if (ch->scopePos_f >= ch->scopeLoopEnd_f)
                ch->scopePos_f  = ch->scopeLoopBegin_f + fmod(ch->scopePos_f - ch->scopeLoopBegin_f, ch->scopeLoopEnd_f - ch->scopeLoopBegin_f);
        }
        else
        {
            if (ch->scopePos_f >= ch->scopeEnd_f)
                ch->scopeEnabled = false;
        }
    }
    else
    {
        ch->scopePos_f = s->offset;
    }
}

void sinkVisualizerBars(void)
{
    uint8_t i;

    // decrease VU-Meters
    if (editor.ui.realVuMeters)
    {
        for (i = 0; i < AMIGA_VOICES; ++i)
        {
            editor.realVuMeterVolumes[i] -= 3.5f;
            if (editor.realVuMeterVolumes[i] < 0.0f)
                editor.realVuMeterVolumes[i] = 0.0f;
        }
    }
    else
    {
        for (i = 0; i < AMIGA_VOICES; ++i)
        {
            if (editor.vuMeterVolumes[i] > 0)
                editor.vuMeterVolumes[i]--;
        }
    }

    // decrease spectrumanalyzer bars
    for (i = 0; i < SPECTRUM_BAR_NUM; ++i)
    {
        if (editor.spectrumVolumes[i] > 0)
            editor.spectrumVolumes[i]--;
    }
}

void updateQuadrascope(void)
{
    uint8_t i;
    int32_t samplePlayPos, scopePos;
    moduleSample_t *s, *currSample;
    moduleChannel_t *ch;

    if (forceMixerOff || editor.isWAVRendering)
        return;

    currSample = &modEntry->samples[editor.currSample];
    hideSprite(SPRITE_SAMPLING_POS_LINE);

    for (i = 0; i < AMIGA_VOICES; i++)
    {
        ch = &modEntry->channels[i];
        if (ch->scopeTrigger)
        {
            ch->scopeTrigger = false;
            ch->scopeEnabled = false;

            s = &modEntry->samples[ch->sample - 1];
            if (s->length > 0)
                triggerScope(ch, s);
        }
        else
        {
            if (ch->scopeEnabled)
            {
                if (ch->scopeKeepVolume) ch->scopeVolume = volumeToScopeVolume(ch->volume);
                if (ch->scopeKeepDelta)  periodToScopeDelta(ch, ch->period);

                ch->scopePos_f += ch->scopeReadDelta_f;
                if (ch->scopeLoopFlag)
                {
                    if (ch->scopePos_f >= ch->scopeLoopEnd_f)
                    {
                        if (ch->scopePos_f >= ch->scopeLoopEnd_f)
                            ch->scopePos_f  = ch->scopeLoopBegin_f + fmod(ch->scopePos_f - ch->scopeLoopBegin_f, ch->scopeLoopEnd_f - ch->scopeLoopBegin_f);

                        // just in case... Shouldn't happen, and this will zero out the fractional part... Not good!
                             if (ch->scopePos_f < ch->scopeLoopBegin_f) ch->scopePos_f = ch->scopeLoopBegin_f;
                        else if (ch->scopePos_f > ch->scopeLoopEnd_f)   ch->scopePos_f = ch->scopeLoopEnd_f;

                        if (ch->scopeLoopQuirk_f > 0.0)
                        {
                            ch->scopeLoopEnd_f   = ch->scopeLoopQuirk_f;
                            ch->scopeLoopQuirk_f = 0.0;
                        }
                    }
                }
                else
                {
                    if (ch->scopePos_f >= ch->scopeEnd_f)
                        ch->scopeEnabled = false;
                }
            }
        }

        // update sample read position sprite
        if (editor.ui.samplerScreenShown && !editor.ui.terminalShown)
        {
            if ((currSample->length > 0) && !editor.muted[ch->chanIndex])
            {
                if (ch->scopeEnabled && !editor.ui.samplerVolBoxShown && !editor.ui.samplerFiltersBoxShown)
                {
                    scopePos = (int32_t)(ch->scopePos_f); // don't round here, truncate instead!
                    if ((scopePos >= currSample->offset) && (scopePos < (currSample->offset + currSample->length)))
                    {
                        samplePlayPos = 3 + smpPos2Scr(scopePos - currSample->offset);
                        if ((samplePlayPos >= 3) && (samplePlayPos <= 316))
                            setSpritePos(SPRITE_SAMPLING_POS_LINE, samplePlayPos, 138);
                    }
                }
            }
        }
    }
}

uint32_t _50HzCallBack(uint32_t interval, void *param)
{
    if ((editor.playMode != PLAY_MODE_PATTERN) ||
        ((editor.currMode == MODE_RECORD) && (editor.recordMode != RECORD_PATT)))
    {
        if (++editor.ticks50Hz == 50)
        {
            editor.ticks50Hz = 0;

            // exactly one second has passed, let's increment the play timer

            if (!editor.isWAVRendering && editor.songPlaying)
            {
                if (editor.playTime < 5999) // 5999 = 99:59
                    editor.playTime++;
            }

            editor.ui.updateSongTime = true;
        }
    }

    (void)(param); // make compiler happy

    return (interval);
}

uint32_t mouseCallback(uint32_t interval, void *param)
{
    int32_t mx, my;
    float mx_f, my_f;

    SDL_GetMouseState(&mx, &my);

    mx_f = mx / input.mouse.scaleX_f;
    my_f = my / input.mouse.scaleY_f;

    mx = (int32_t)(mx_f + 0.5f);
    my = (int32_t)(my_f + 0.5f);

    /* clamp to edges */
    mx = CLAMP(mx, 0, SCREEN_W - 1);
    my = CLAMP(my, 0, SCREEN_H - 1);

    input.mouse.newlyPolledX = mx;
    input.mouse.newlyPolledY = my;

    (void)(param); // make compiler happy

    return (interval);
}

void toggleFullscreen(void)
{
    int32_t w, h;

    fullscreen ^= 1;
    if (fullscreen)
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_SetWindowGrab(window, SDL_TRUE);
    }
    else
    {
        SDL_SetWindowFullscreen(window, 0);
        SDL_SetWindowSize(window, SCREEN_W * editor.ui.videoScaleFactor, SCREEN_H * editor.ui.videoScaleFactor);
        SDL_SetWindowGrab(window, SDL_FALSE);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    SDL_GetWindowSize(window, &w, &h);
    SDL_WarpMouseInWindow(window, w / 2, h / 2);

    updateMouseScaling();
}

int8_t setupVideo(void)
{
    int32_t screenW, screenH;
    uint32_t windowFlags, rendererFlags, textureFormat;
    SDL_DisplayMode dm;

    screenW = SCREEN_W * editor.ui.videoScaleFactor;
    screenH = SCREEN_H * editor.ui.videoScaleFactor;

    windowFlags   = 0;
    rendererFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
    textureFormat = SDL_PIXELFORMAT_RGB888;

#ifdef __APPLE__
    textureFormat = SDL_PIXELFORMAT_ARGB8888; // this seems to be slightly faster in OS X / macOS
#endif

#ifdef _WIN32
#if SDL_PATCHLEVEL >= 4
    SDL_SetHint(SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4, "1"); // this is for Windows only
#endif
#endif

#if SDL_PATCHLEVEL >= 1
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
#endif

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        showErrorMsgBox("Couldn't initialize SDL: %s", SDL_GetError());
        return (false);
    }

    vsync60HzPresent = false;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
    {
        if ((dm.refresh_rate == 59) || (dm.refresh_rate == 60)) // 59Hz is a wrong NTSC legacy value from EDID. It's 60Hz!
        {
            vsync60HzPresent = true;
            rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
        }
    }

    SDL_SetWindowTitle(window, "ProTracker v2.3D");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    window = SDL_CreateWindow("ProTracker v2.3D", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, screenW, screenH,
                              windowFlags);
    if (window == NULL)
    {
        showErrorMsgBox("Couldn't create SDL window:\n%s", SDL_GetError());
        return (false);
    }

    renderer = SDL_CreateRenderer(window, -1, rendererFlags);
    if (renderer == NULL)
    {
        if (vsync60HzPresent)
        {
            // try again without vsync flag
            vsync60HzPresent = false;

            rendererFlags &= ~SDL_RENDERER_PRESENTVSYNC;
            renderer = SDL_CreateRenderer(window, -1, rendererFlags);
        }

        if (renderer == NULL)
        {
            showErrorMsgBox("Couldn't create SDL renderer:\n" \
                           "%s\n\n" \
                           "Is your GPU (+ driver) too old?", SDL_GetError());

            return (false);
        }
    }

    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

#if SDL_PATCHLEVEL >= 5
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);
#endif

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    texture = SDL_CreateTexture(renderer, textureFormat, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    if (texture == NULL)
    {
        showErrorMsgBox("Couldn't create a %dx%d GPU texture:\n" \
                        "%s\n\n" \
                        "Is your GPU (+ driver) too old?", SCREEN_W, SCREEN_H, SDL_GetError());

        return (false);
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

    // frame buffer used by SDL (for texture)
    pixelBuffer = (uint32_t *)(malloc(SCREEN_W * SCREEN_H * sizeof (int32_t)));
    if (pixelBuffer == NULL)
    {
        showErrorMsgBox("Out of memory!");
        return (false);
    }

    SDL_ShowCursor(SDL_DISABLE);

    if (!vsync60HzPresent)
        SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    updateMouseScaling();

    return (true);
}
