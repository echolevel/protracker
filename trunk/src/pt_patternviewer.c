#include <stdint.h>
#include "pt_header.h"
#include "pt_palette.h"
#include "pt_tables.h"
#include "pt_textout.h"
#include "pt_helpers.h"

static uint8_t periodToNote(int16_t period)
{
    uint8_t l, m, h;

    l = 0;
    h = 35;

    while (h >= l)
    {
        m = (h + l) / 2;
        if (m >= 36) break; // should never happen, but let's stay on the safe side

             if (periodTable[m] == period) return (m);
        else if (periodTable[m]  > period) l = m + 1;
        else                               h = m - 1;
    }

    return (255); // illegal period
}

void drawPatternNormal(uint32_t *frameBuffer);
void drawPatternDotted(uint32_t *frameBuffer);

void redrawPattern(uint32_t *frameBuffer)
{
    if (editor.ui.pattDots)
        drawPatternDotted(frameBuffer);
    else
        drawPatternNormal(frameBuffer);
}

void drawPatternNormal(uint32_t *frameBuffer)
{
    int8_t rowMiddlePos;
    uint8_t i, j, tempNote, rowDispCheck;
    uint16_t y, y2, putXOffset, putYOffset, rowData;
    note_t note;

    for (i = 0; i < 15; ++i)
    {
        rowMiddlePos = i - 7;
        rowDispCheck = modEntry->currRow + rowMiddlePos;

        if (rowDispCheck < MOD_ROWS)
        {
            rowData    = rowDispCheck * 4;
            putYOffset = 140 + (i * 7);

            if (i == 7) // are we on the play row (middle)?
            {
                putYOffset++; // align font to play row (middle)

                // put current row number
                printTwoDecimalsBigBg(frameBuffer, 8, putYOffset, rowMiddlePos + modEntry->currRow, palette[PAL_GENTXT], palette[PAL_GENBKG]);

                // pattern data
                for (j = 0; j < AMIGA_VOICES; ++j)
                {
                    note = modEntry->patterns[modEntry->currPattern][rowData + j];
                    putXOffset = 26 + (j * 72);

                    if (note.period == 0)
                    {
                        textOutBigBg(frameBuffer, putXOffset + 6, putYOffset, "---", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    }
                    else
                    {
                        tempNote = periodToNote(note.period);
                        if (tempNote == 255)
                            textOutBigBg(frameBuffer, putXOffset + 6, putYOffset, "???", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        else
                            textOutBigBg(frameBuffer, putXOffset + 6, putYOffset, editor.accidental ? noteNames2[tempNote] : noteNames1[tempNote], palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    }

                    if (editor.ui.blankZeroFlag)
                    {
                        if (note.sample & 0xF0)
                            printOneHexBigBg(frameBuffer, putXOffset + 30, putYOffset, note.sample >> 4, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        else
                            printOneHexBigBg(frameBuffer, putXOffset + 30, putYOffset, ' ', palette[PAL_GENBKG], palette[PAL_GENBKG]);
                    }
                    else
                    {
                        printOneHexBigBg(frameBuffer, putXOffset + 30, putYOffset, note.sample >> 4, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    }

                    printOneHexBigBg(frameBuffer, putXOffset + 38, putYOffset, note.sample & 0x0F, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    printOneHexBigBg(frameBuffer, putXOffset + 46, putYOffset, note.command, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    printTwoHexBigBg(frameBuffer, putXOffset + 54, putYOffset, note.param,  palette[PAL_GENTXT], palette[PAL_GENBKG]);
                }
            }
            else
            {
                if (i > 7)
                    putYOffset += 7; // beyond play row, jump some pixels out of the row (middle)

                // put current row number
                printTwoDecimalsBg(frameBuffer, 8, putYOffset, rowMiddlePos + modEntry->currRow, palette[PAL_PATTXT], palette[PAL_BACKGRD]);

                // pattern data
                for (j = 0; j < AMIGA_VOICES; ++j)
                {
                    note = modEntry->patterns[modEntry->currPattern][rowData + j];
                    putXOffset = 26 + (j * 72);

                    if (note.period == 0)
                    {
                        textOutBg(frameBuffer, putXOffset + 6, putYOffset, "---", palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    }
                    else
                    {
                        tempNote = periodToNote(note.period);
                        if (tempNote == 255)
                            textOutBg(frameBuffer, putXOffset + 6, putYOffset, "???", palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        else
                            textOutBg(frameBuffer, putXOffset + 6, putYOffset, editor.accidental ? noteNames2[tempNote] : noteNames1[tempNote], palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    }

                    if (editor.ui.blankZeroFlag)
                    {
                        if (note.sample & 0xF0)
                            printOneHexBg(frameBuffer, putXOffset + 30, putYOffset, note.sample >> 4, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        else
                            printOneHexBg(frameBuffer, putXOffset + 30, putYOffset, ' ', palette[PAL_BACKGRD], palette[PAL_BACKGRD]);
                    }
                    else
                    {
                        printOneHexBg(frameBuffer, putXOffset + 30, putYOffset, note.sample >> 4, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    }

                    printOneHexBg(frameBuffer, putXOffset + 38, putYOffset, note.sample & 0x0F, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    printOneHexBg(frameBuffer, putXOffset + 46, putYOffset, note.command, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    printTwoHexBg(frameBuffer, putXOffset + 54, putYOffset, note.param, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                }
            }
        }
    }

    // fill margin
    if (modEntry->currRow <= 6)
    {
        y2 = 140 + ((7 -  modEntry->currRow) * 7);
        for (y = 140; y < y2; y += 7)
            textOutBgNoSpace(frameBuffer, 9, y, "00 --000000 --000000 --000000 --000000", palette[PAL_BACKGRD], palette[PAL_BACKGRD]);
    }
    else if (modEntry->currRow >= 57)
    {
        y2 = 245 - ((modEntry->currRow - 56) * 7);
        for (y = 245; y > y2; y -= 7)
            textOutBgNoSpace(frameBuffer, 9, y, "00 --000000 --000000 --000000 --000000", palette[PAL_BACKGRD], palette[PAL_BACKGRD]);
    }
}

void drawPatternDotted(uint32_t *frameBuffer)
{
    int8_t rowMiddlePos;
    uint8_t i, j, tempNote, rowDispCheck;
    uint16_t y, y2, putXOffset, putYOffset, rowData;
    note_t note;

    for (i = 0; i < 15; ++i)
    {
        rowMiddlePos = i - 7;
        rowDispCheck = modEntry->currRow + rowMiddlePos;

        if (rowDispCheck < MOD_ROWS)
        {
            rowData    = rowDispCheck * 4;
            putYOffset = 140 + (i * 7);

            if (i == 7) // are we on the play row (middle)?
            {
                putYOffset++; // align font to play row (middle)

                // put current row number
                printTwoDecimalsBigBg(frameBuffer, 8, putYOffset, rowMiddlePos + modEntry->currRow, palette[PAL_GENTXT], palette[PAL_GENBKG]);

                // pattern data
                for (j = 0; j < AMIGA_VOICES; ++j)
                {
                    note = modEntry->patterns[modEntry->currPattern][rowData + j];
                    putXOffset = 26 + (j * 72);

                    if (note.period == 0)
                    {
                        charOutBigBg(frameBuffer, putXOffset +  6, putYOffset, 128, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        charOutBigBg(frameBuffer, putXOffset + 14, putYOffset, 128, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        charOutBigBg(frameBuffer, putXOffset + 22, putYOffset, 128, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    }
                    else
                    {
                        tempNote = periodToNote(note.period);
                        if (tempNote == 255)
                            textOutBigBg(frameBuffer, putXOffset + 6, putYOffset, "???", palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        else
                            textOutBigBg(frameBuffer, putXOffset + 6, putYOffset, editor.accidental ? noteNames2[tempNote] : noteNames1[tempNote], palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    }

                    if (note.sample)
                    {
                        printOneHexBigBg(frameBuffer, putXOffset + 30, putYOffset, note.sample >> 4, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        printOneHexBigBg(frameBuffer, putXOffset + 38, putYOffset, note.sample & 0x0F, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    }
                    else
                    {
                        charOutBigBg(frameBuffer, putXOffset + 30, putYOffset, 128, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        charOutBigBg(frameBuffer, putXOffset + 38, putYOffset, 128, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    }

                    if ((note.command | note.param) == 0)
                    {
                        charOutBigBg(frameBuffer, putXOffset + 46, putYOffset, 128, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        charOutBigBg(frameBuffer, putXOffset + 54, putYOffset, 128, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        charOutBigBg(frameBuffer, putXOffset + 62, putYOffset, 128, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    }
                    else
                    {
                        printOneHexBigBg(frameBuffer, putXOffset + 46, putYOffset, note.command, palette[PAL_GENTXT], palette[PAL_GENBKG]);
                        printTwoHexBigBg(frameBuffer, putXOffset + 54, putYOffset, note.param,   palette[PAL_GENTXT], palette[PAL_GENBKG]);
                    }
                }
            }
            else
            {
                if (i > 7)
                    putYOffset += 7; // beyond play row, jump some pixels out of the row (middle)

                // put current row number
                printTwoDecimalsBg(frameBuffer, 8, putYOffset, rowMiddlePos + modEntry->currRow, palette[PAL_PATTXT], palette[PAL_BACKGRD]);

                // pattern data
                for (j = 0; j < AMIGA_VOICES; ++j)
                {
                    note = modEntry->patterns[modEntry->currPattern][rowData + j];
                    putXOffset = 26 + (j * 72);

                    if (note.period == 0)
                    {
                        charOutBg(frameBuffer, putXOffset + 6,  putYOffset, 128, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        charOutBg(frameBuffer, putXOffset + 14, putYOffset, 128, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        charOutBg(frameBuffer, putXOffset + 22, putYOffset, 128, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    }
                    else
                    {
                        tempNote = periodToNote(note.period);
                        if (tempNote == 255)
                            textOutBg(frameBuffer, putXOffset + 6, putYOffset, "???", palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        else
                            textOutBg(frameBuffer, putXOffset + 6, putYOffset, editor.accidental ? noteNames2[tempNote] : noteNames1[tempNote], palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    }

                    if (note.sample)
                    {
                        printOneHexBg(frameBuffer, putXOffset + 30, putYOffset, note.sample >> 4,   palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        printOneHexBg(frameBuffer, putXOffset + 38, putYOffset, note.sample & 0x0F, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    }
                    else
                    {
                        charOutBg(frameBuffer, putXOffset + 30, putYOffset, 128, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        charOutBg(frameBuffer, putXOffset + 38, putYOffset, 128, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    }

                    if ((note.command | note.param) == 0)
                    {
                        charOutBg(frameBuffer, putXOffset + 46, putYOffset, 128, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        charOutBg(frameBuffer, putXOffset + 54, putYOffset, 128, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        charOutBg(frameBuffer, putXOffset + 62, putYOffset, 128, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    }
                    else
                    {
                        printOneHexBg(frameBuffer, putXOffset + 46, putYOffset, note.command, palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                        printTwoHexBg(frameBuffer, putXOffset + 54, putYOffset, note.param,   palette[PAL_PATTXT], palette[PAL_BACKGRD]);
                    }
                }
            }
        }
    }

    // fill margin
    if (modEntry->currRow <= 6)
    {
        y2 = 140 + ((7 -  modEntry->currRow) * 7);
        for (y = 140; y < y2; y += 7)
            textOutBgNoSpace(frameBuffer, 9, y, "00 --000000 --000000 --000000 --000000", palette[PAL_BACKGRD], palette[PAL_BACKGRD]);
    }
    else if (modEntry->currRow >= 57)
    {
        y2 = 245 - ((modEntry->currRow - 56) * 7);
        for (y = 245; y > y2; y -= 7)
            textOutBgNoSpace(frameBuffer, 9, y, "00 --000000 --000000 --000000 --000000", palette[PAL_BACKGRD], palette[PAL_BACKGRD]);
    }
}
