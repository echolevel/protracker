#include <stdint.h>
#include <string.h>
#include "pt_header.h"
#include "pt_helpers.h"
#include "pt_tables.h"
#include "pt_palette.h"
#include "pt_visuals.h"

void charOut(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint8_t ch, uint32_t fontColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer;

    if (ch < ' ')
        return;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    fontPointer = fontBMP + (ch * (FONT_CHAR_W * FONT_CHAR_H));

    line = FONT_CHAR_H;
    while (line--)
    {
        if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor;
        if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor;
        if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor;
        if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor;
        if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor;
        if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor;
        if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor;
        if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor;

        fontPointer += FONT_CHAR_W;
        frameBufferPointer += SCREEN_W;
    }
}

void charOutBig(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint8_t ch, uint32_t fontColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer;

    if (ch < ' ')
        return;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    fontPointer = fontBMP + (ch * (FONT_CHAR_W * FONT_CHAR_H));

    line = FONT_CHAR_H;
    while (line--)
    {
        if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor;
        if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor;
        if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor;
        if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor;
        if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor;
        if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor;
        if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor;
        if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor;

        frameBufferPointer += (SCREEN_W);

        if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor;
        if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor;
        if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor;
        if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor;
        if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor;
        if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor;
        if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor;
        if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor;

        fontPointer += FONT_CHAR_W;
        frameBufferPointer += (SCREEN_W);
    }
}

void textOutNoSpace(uint32_t *frameBuffer, uint32_t x, uint32_t y, const char *text, uint32_t fontColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer, i;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    for (i = 0; text[i]; ++i)
    {
        if (text[i] <= ' ')
        {
            frameBufferPointer += FONT_CHAR_W;
            continue;
        }

        fontPointer = fontBMP + (text[i] * (FONT_CHAR_W * FONT_CHAR_H));

        line = FONT_CHAR_H;
        while (line--)
        {
            if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor;
            if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor;
            if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor;
            if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor;
            if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor;
            if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor;
            if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor;
            if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor;

            fontPointer += FONT_CHAR_W;
            frameBufferPointer += SCREEN_W;
        }

        frameBufferPointer -= ((FONT_CHAR_H * SCREEN_W) - FONT_CHAR_W);
    }
}

void textOut(uint32_t *frameBuffer, uint32_t x, uint32_t y, const char *text, uint32_t fontColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer, i;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    for (i = 0; text[i]; ++i)
    {
        if (text[i] < ' ')
        {
            frameBufferPointer += FONT_CHAR_W;
            continue;
        }

        fontPointer = fontBMP + (text[i] * (FONT_CHAR_W * FONT_CHAR_H));

        line = FONT_CHAR_H;
        while (line--)
        {
            if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor;
            if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor;
            if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor;
            if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor;
            if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor;
            if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor;
            if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor;
            if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor;

            fontPointer += FONT_CHAR_W;
            frameBufferPointer += SCREEN_W;
        }

        frameBufferPointer -= ((FONT_CHAR_H * SCREEN_W) - FONT_CHAR_W);
    }
}

void textOutBig(uint32_t *frameBuffer, uint32_t x, uint32_t y, const char *text, uint32_t fontColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer, i;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    for (i = 0; text[i]; ++i)
    {
        if (text[i] < ' ')
        {
            frameBufferPointer += FONT_CHAR_W;
            continue;
        }

        fontPointer = fontBMP  + (text[i] * (FONT_CHAR_W * FONT_CHAR_H));

        line = FONT_CHAR_H;
        while (line--)
        {
            if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor;
            if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor;
            if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor;
            if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor;
            if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor;
            if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor;
            if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor;
            if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor;

            frameBufferPointer += (SCREEN_W);

            if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor;
            if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor;
            if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor;
            if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor;
            if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor;
            if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor;
            if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor;
            if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor;

            fontPointer += FONT_CHAR_W;
            frameBufferPointer += (SCREEN_W);
        }

        frameBufferPointer -= (((FONT_CHAR_H * 2) * SCREEN_W) - FONT_CHAR_W);
    }
}

void printTwoDecimals(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOut(frameBuffer, x, y, "00", fontColor);
    }
    else
    {
        if (value > 99)
            value = 99;

        charOut(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor);
    }
}

void printTwoDecimalsBig(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOutBig(frameBuffer, x, y, "00", fontColor);
    }
    else
    {
        if (value > 99)
            value = 99;

        charOutBig(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor); value /= 10;
        charOutBig(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor);
    }
}

void printThreeDecimals(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOut(frameBuffer, x, y, "000", fontColor);
    }
    else
    {
        if (value > 999)
            value = 999;

        charOut(frameBuffer, x + (FONT_CHAR_W * 2), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor);
    }
}

void printFourDecimals(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOut(frameBuffer, x, y, "0000", fontColor);
    }
    else
    {
        if (value > 9999)
            value = 9999;

        charOut(frameBuffer, x + (FONT_CHAR_W * 3), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 2), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor);
    }
}

void printFiveDecimals(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOut(frameBuffer, x, y, "00000", fontColor);
    }
    else
    {
        if (value > 99999)
            value = 99999;

        charOut(frameBuffer, x + (FONT_CHAR_W * 4), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 3), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 2), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor); value /= 10;
        charOut(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor);
    }
}

// this one is used for module size and sampler screen display length (zeroes are padded with space)
void printSixDecimals(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    char NumberText[7];
    uint8_t i;

    if (value == 0)
    {
        textOut(frameBuffer, x, y, "     0", fontColor);
    }
    else
    {
        if (value > 999999)
            value = 999999;

        NumberText[6] = 0;
        NumberText[5] = '0' + (value % 10); value /= 10;
        NumberText[4] = '0' + (value % 10); value /= 10;
        NumberText[3] = '0' + (value % 10); value /= 10;
        NumberText[2] = '0' + (value % 10); value /= 10;
        NumberText[1] = '0' + (value % 10); value /= 10;
        NumberText[0] = '0' + (value % 10);

        i = 0;
        while (NumberText[i] == '0')
            NumberText[i++] = ' ';

        textOut(frameBuffer, x, y, NumberText, fontColor);
    }
}

void printOneHex(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    charOut(frameBuffer, x, y, hexTable[value & 15], fontColor);
}

void printOneHexBig(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    charOutBig(frameBuffer, x, y, hexTable[value & 15], fontColor);
}

void printTwoHex(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOut(frameBuffer, x, y, "00", fontColor);
    }
    else
    {
        value &= 0x000000FF;

        charOut(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[value >> 4], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[value & 15], fontColor);
    }
}

void printTwoHexBig(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOutBig(frameBuffer, x, y, "00", fontColor);
    }
    else
    {
        value &= 0x000000FF;

        charOutBig(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[value >> 4], fontColor);
        charOutBig(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[value & 15], fontColor);
    }
}

void printThreeHex(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOut(frameBuffer, x, y, "000", fontColor);
    }
    else
    {
        value &= 0x00000FFF;

        charOut(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[ value              >> 8], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[(value & (15 << 4)) >> 4], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 2), y, hexTable[ value              & 15], fontColor);
    }
}

void printFourHex(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOut(frameBuffer, x, y, "0000", fontColor);
    }
    else
    {
        value &= 0x0000FFFF;

        charOut(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[ value              >> 12], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[(value & (15 << 8)) >>  8], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 2), y, hexTable[(value & (15 << 4)) >>  4], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 3), y, hexTable[ value               & 15], fontColor);
    }
}

void printFiveHex(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor)
{
    if (value == 0)
    {
        textOut(frameBuffer, x, y, "00000", fontColor);
    }
    else
    {
        value &= 0x000FFFFF;

        charOut(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[ value               >> 16], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[(value & (15 << 12)) >> 12], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 2), y, hexTable[(value & (15 <<  8)) >>  8], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 3), y, hexTable[(value & (15 <<  4)) >>  4], fontColor);
        charOut(frameBuffer, x + (FONT_CHAR_W * 4), y, hexTable[ value                & 15], fontColor);
    }
}

void charOutBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint8_t ch, uint32_t fontColor, uint32_t backColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer;

    if (ch < ' ')
        return;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    fontPointer = fontBMP + (ch * (FONT_CHAR_W * FONT_CHAR_H));

    line = FONT_CHAR_H;
    while (line--)
    {
        if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor; else *(frameBufferPointer + 0) = backColor;
        if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor; else *(frameBufferPointer + 1) = backColor;
        if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor; else *(frameBufferPointer + 2) = backColor;
        if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor; else *(frameBufferPointer + 3) = backColor;
        if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor; else *(frameBufferPointer + 4) = backColor;
        if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor; else *(frameBufferPointer + 5) = backColor;
        if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor; else *(frameBufferPointer + 6) = backColor;
        if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor; else *(frameBufferPointer + 7) = backColor;

        fontPointer += FONT_CHAR_W;
        frameBufferPointer += SCREEN_W;
    }
}

void charOutBigBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint8_t ch, uint32_t fontColor, uint32_t backColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer;

    if (ch < ' ')
        return;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    fontPointer = fontBMP + (ch * (FONT_CHAR_W * FONT_CHAR_H));

    line = FONT_CHAR_H;
    while (line--)
    {
        if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor; else *(frameBufferPointer + 0) = backColor;
        if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor; else *(frameBufferPointer + 1) = backColor;
        if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor; else *(frameBufferPointer + 2) = backColor;
        if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor; else *(frameBufferPointer + 3) = backColor;
        if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor; else *(frameBufferPointer + 4) = backColor;
        if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor; else *(frameBufferPointer + 5) = backColor;
        if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor; else *(frameBufferPointer + 6) = backColor;
        if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor; else *(frameBufferPointer + 7) = backColor;

        frameBufferPointer += (SCREEN_W);

        if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor; else *(frameBufferPointer + 0) = backColor;
        if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor; else *(frameBufferPointer + 1) = backColor;
        if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor; else *(frameBufferPointer + 2) = backColor;
        if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor; else *(frameBufferPointer + 3) = backColor;
        if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor; else *(frameBufferPointer + 4) = backColor;
        if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor; else *(frameBufferPointer + 5) = backColor;
        if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor; else *(frameBufferPointer + 6) = backColor;
        if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor; else *(frameBufferPointer + 7) = backColor;

        fontPointer += FONT_CHAR_W;
        frameBufferPointer += (SCREEN_W);
    }
}

void textOutBgNoSpace(uint32_t *frameBuffer, uint32_t x, uint32_t y, const char *text, uint32_t fontColor, uint32_t backColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer, i;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    for (i = 0; text[i]; ++i)
    {
        if (text[i] <= ' ')
        {
            frameBufferPointer += FONT_CHAR_W;
            continue;
        }

        fontPointer = fontBMP + (text[i] * (FONT_CHAR_W * FONT_CHAR_H));

        line = FONT_CHAR_H;
        while (line--)
        {
            if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor; else *(frameBufferPointer + 0) = backColor;
            if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor; else *(frameBufferPointer + 1) = backColor;
            if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor; else *(frameBufferPointer + 2) = backColor;
            if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor; else *(frameBufferPointer + 3) = backColor;
            if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor; else *(frameBufferPointer + 4) = backColor;
            if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor; else *(frameBufferPointer + 5) = backColor;
            if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor; else *(frameBufferPointer + 6) = backColor;
            if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor; else *(frameBufferPointer + 7) = backColor;

            fontPointer += FONT_CHAR_W;
            frameBufferPointer += SCREEN_W;
        }

        frameBufferPointer -= ((FONT_CHAR_H * SCREEN_W) - FONT_CHAR_W);
    }
}

void textOutBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, const char *text, uint32_t fontColor, uint32_t backColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer, i;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    for (i = 0; text[i]; ++i)
    {
        if (text[i] < ' ')
        {
            frameBufferPointer += FONT_CHAR_W;
            continue;
        }

        fontPointer = fontBMP + (text[i] * (FONT_CHAR_W * FONT_CHAR_H));

        line = FONT_CHAR_H;
        while (line--)
        {
            if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor; else *(frameBufferPointer + 0) = backColor;
            if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor; else *(frameBufferPointer + 1) = backColor;
            if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor; else *(frameBufferPointer + 2) = backColor;
            if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor; else *(frameBufferPointer + 3) = backColor;
            if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor; else *(frameBufferPointer + 4) = backColor;
            if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor; else *(frameBufferPointer + 5) = backColor;
            if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor; else *(frameBufferPointer + 6) = backColor;
            if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor; else *(frameBufferPointer + 7) = backColor;

            fontPointer += FONT_CHAR_W;
            frameBufferPointer += SCREEN_W;
        }

        frameBufferPointer -= ((FONT_CHAR_H * SCREEN_W) - FONT_CHAR_W);
    }
}

void textOutBigBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, const char *text, uint32_t fontColor, uint32_t backColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *frameBufferPointer, i;

    frameBufferPointer = frameBuffer + ((y * SCREEN_W) + x);
    for (i = 0; text[i]; ++i)
    {
        if (text[i] < ' ')
        {
            frameBufferPointer += FONT_CHAR_W;
            continue;
        }

        fontPointer = fontBMP  + (text[i] * (FONT_CHAR_W * FONT_CHAR_H));

        line = FONT_CHAR_H;
        while (line--)
        {
            if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor; else *(frameBufferPointer + 0) = backColor;
            if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor; else *(frameBufferPointer + 1) = backColor;
            if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor; else *(frameBufferPointer + 2) = backColor;
            if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor; else *(frameBufferPointer + 3) = backColor;
            if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor; else *(frameBufferPointer + 4) = backColor;
            if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor; else *(frameBufferPointer + 5) = backColor;
            if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor; else *(frameBufferPointer + 6) = backColor;
            if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor; else *(frameBufferPointer + 7) = backColor;

            frameBufferPointer += (SCREEN_W);

            if (*(fontPointer + 0)) *(frameBufferPointer + 0) = fontColor; else *(frameBufferPointer + 0) = backColor;
            if (*(fontPointer + 1)) *(frameBufferPointer + 1) = fontColor; else *(frameBufferPointer + 1) = backColor;
            if (*(fontPointer + 2)) *(frameBufferPointer + 2) = fontColor; else *(frameBufferPointer + 2) = backColor;
            if (*(fontPointer + 3)) *(frameBufferPointer + 3) = fontColor; else *(frameBufferPointer + 3) = backColor;
            if (*(fontPointer + 4)) *(frameBufferPointer + 4) = fontColor; else *(frameBufferPointer + 4) = backColor;
            if (*(fontPointer + 5)) *(frameBufferPointer + 5) = fontColor; else *(frameBufferPointer + 5) = backColor;
            if (*(fontPointer + 6)) *(frameBufferPointer + 6) = fontColor; else *(frameBufferPointer + 6) = backColor;
            if (*(fontPointer + 7)) *(frameBufferPointer + 7) = fontColor; else *(frameBufferPointer + 7) = backColor;

            fontPointer += FONT_CHAR_W;
            frameBufferPointer += (SCREEN_W);
        }

        frameBufferPointer -= (((FONT_CHAR_H * 2) * SCREEN_W) - FONT_CHAR_W);
    }
}

void printTwoDecimalsBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBg(frameBuffer, x, y, "00", fontColor, backColor);
    }
    else
    {
        if (value > 99)
            value = 99;

        charOutBg(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor, backColor);
    }
}

void printTwoDecimalsBigBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBigBg(frameBuffer, x, y, "00", fontColor, backColor);
    }
    else
    {
        if (value > 99)
            value = 99;

        charOutBigBg(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBigBg(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor, backColor);
    }
}

void printThreeDecimalsBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBg(frameBuffer, x, y, "000", fontColor, backColor);
    }
    else
    {
        if (value > 999)
            value = 999;

        charOutBg(frameBuffer, x + (FONT_CHAR_W * 2), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor, backColor);
    }
}

void printFourDecimalsBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBg(frameBuffer, x, y, "0000", fontColor, backColor);
    }
    else
    {
        if (value > 9999)
            value = 9999;

        charOutBg(frameBuffer, x + (FONT_CHAR_W * 3), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 2), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor, backColor);
    }
}

void printFiveDecimalsBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBg(frameBuffer, x, y, "00000", fontColor, backColor);
    }
    else
    {
        if (value > 99999)
            value = 99999;

        charOutBg(frameBuffer, x + (FONT_CHAR_W * 4), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 3), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 2), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 1), y, '0' + (value % 10), fontColor, backColor); value /= 10;
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 0), y, '0' + (value % 10), fontColor, backColor);
    }
}

// this one is used for module size and sampler screen display length (zeroes are padded with space)
void printSixDecimalsBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    char NumberText[7];
    uint8_t i;

    if (value == 0)
    {
        textOutBg(frameBuffer, x, y, "     0", fontColor, backColor);
    }
    else
    {
        if (value > 999999)
            value = 999999;

        NumberText[6] = 0;
        NumberText[5] = '0' + (value % 10); value /= 10;
        NumberText[4] = '0' + (value % 10); value /= 10;
        NumberText[3] = '0' + (value % 10); value /= 10;
        NumberText[2] = '0' + (value % 10); value /= 10;
        NumberText[1] = '0' + (value % 10); value /= 10;
        NumberText[0] = '0' + (value % 10);

        i = 0;
        while (NumberText[i] == '0')
            NumberText[i++] = ' ';

        textOutBg(frameBuffer, x, y, NumberText, fontColor, backColor);
    }
}

void printOneHexBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    charOutBg(frameBuffer, x, y, hexTable[value & 15], fontColor, backColor);
}

void printOneHexBigBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    charOutBigBg(frameBuffer, x, y, hexTable[value & 15], fontColor, backColor);
}

void printTwoHexBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBg(frameBuffer, x, y, "00", fontColor, backColor);
    }
    else
    {
        value &= 0x000000FF;

        charOutBg(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[value >> 4], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[value & 15], fontColor, backColor);
    }
}

void printTwoHexBigBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBigBg(frameBuffer, x, y, "00", fontColor, backColor);
    }
    else
    {
        value &= 0x000000FF;

        charOutBigBg(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[value >> 4], fontColor, backColor);
        charOutBigBg(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[value & 15], fontColor, backColor);
    }
}

void printThreeHexBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBg(frameBuffer, x, y, "000", fontColor, backColor);
    }
    else
    {
        value &= 0x00000FFF;

        charOutBg(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[ value              >> 8], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[(value & (15 << 4)) >> 4], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 2), y, hexTable[ value              & 15], fontColor, backColor);
    }
}

void printFourHexBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBg(frameBuffer, x, y, "0000", fontColor, backColor);
    }
    else
    {
        value &= 0x0000FFFF;

        charOutBg(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[ value              >> 12], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[(value & (15 << 8)) >>  8], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 2), y, hexTable[(value & (15 << 4)) >>  4], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 3), y, hexTable[ value               & 15], fontColor, backColor);
    }
}

void printFiveHexBg(uint32_t *frameBuffer, uint32_t x, uint32_t y, uint32_t value, uint32_t fontColor, uint32_t backColor)
{
    if (value == 0)
    {
        textOutBg(frameBuffer, x, y, "00000", fontColor, backColor);
    }
    else
    {
        value &= 0x000FFFFF;

        charOutBg(frameBuffer, x + (FONT_CHAR_W * 0), y, hexTable[ value               >> 16], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 1), y, hexTable[(value & (15 << 12)) >> 12], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 2), y, hexTable[(value & (15 <<  8)) >>  8], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 3), y, hexTable[(value & (15 <<  4)) >>  4], fontColor, backColor);
        charOutBg(frameBuffer, x + (FONT_CHAR_W * 4), y, hexTable[ value                & 15], fontColor, backColor);
    }
}

void setPrevStatusMessage(void)
{
    strcpy(editor.ui.statusMessage, editor.ui.prevStatusMessage);
    editor.ui.updateStatusText = true;
}

void setStatusMessage(const char *message, uint8_t carry)
{
    if (carry)
        strcpy(editor.ui.prevStatusMessage, message);

   strcpy(editor.ui.statusMessage, message);
   editor.ui.updateStatusText = true;
}

void displayMsg(const char *msg)
{
    editor.errorMsgActive  = true;
    editor.errorMsgBlock   = false;
    editor.errorMsgCounter = 0;

    if (*msg != '\0')
        setStatusMessage(msg, NO_CARRY);
}

void displayErrorMsg(const char *msg)
{
    editor.errorMsgActive  = true;
    editor.errorMsgBlock   = true;
    editor.errorMsgCounter = 0;

    if (*msg != '\0')
        setStatusMessage(msg, NO_CARRY);

    pointerErrorMode();
}
