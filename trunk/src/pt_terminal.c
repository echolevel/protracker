#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h> // for powf()
#include "pt_terminal.h"
#include "pt_textout.h"
#include "pt_tables.h"
#include "pt_palette.h"
#include "pt_helpers.h"
#include "pt_visuals.h"

#define SCROLL_AREA_TOP    24
#define SCROLL_AREA_HEIGHT 206

#define TOPAZ_UNPACKED_LEN (760 * 8)

static char *charBuffer, *textFormatBuffer;
static uint8_t *topazFont, row, col, overflowFlag;
static int32_t lastDragY, lastMouseY, numLines;
static int32_t scrollBufferPos, scrollBarEnd, scrollBarPage;
static int32_t scrollBarThumbTop, scrollBarThumbBottom;
static uint32_t textFgColor, dateFgColor, dateBgColor;

static const char *monthDaysText[31] =
{
    "1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th",
    "9th", "10th", "11th", "12th", "13th", "14th", "15th",
    "16th", "17th", "18th", "19th", "20th", "21st", "22nd",
    "23rd", "24th", "25th", "26th", "27th", "28th", "29th",
    "30th", "31st"
};

static const char *monthText[12] =
{
    "January", "February", "March",
    "April", "May", "June", "July",
    "August", "September", "October",
    "November", "December"
};

static void renderCharacter(uint32_t *buffer, uint32_t x, uint32_t y, char ch, uint32_t fontColor)
{
    const uint8_t *fontPointer;
    uint8_t line;
    uint32_t *bufferPointer;

    if ((ch < ' ') || (ch > '~'))
        ch = ' ';

    ch -= ' ';

    bufferPointer = buffer    + ((y * SCREEN_W) + x);
    fontPointer   = topazFont + (ch * (TERMINAL_FONT_W * TERMINAL_FONT_H));

    line = TERMINAL_FONT_H;
    while (line--)
    {
        if (*(fontPointer + 0)) *(bufferPointer + 0) = fontColor;
        if (*(fontPointer + 1)) *(bufferPointer + 1) = fontColor;
        if (*(fontPointer + 2)) *(bufferPointer + 2) = fontColor;
        if (*(fontPointer + 3)) *(bufferPointer + 3) = fontColor;
        if (*(fontPointer + 4)) *(bufferPointer + 4) = fontColor;
        if (*(fontPointer + 5)) *(bufferPointer + 5) = fontColor;
        if (*(fontPointer + 6)) *(bufferPointer + 6) = fontColor;
        if (*(fontPointer + 7)) *(bufferPointer + 7) = fontColor;

        fontPointer   += TERMINAL_FONT_W;
        bufferPointer += SCREEN_W;
    }
}

static void updateScrollBar(void)
{
    int32_t thumbTop, thumbHeight;

    thumbHeight = CLAMP((SCROLL_AREA_HEIGHT * scrollBarPage) / (scrollBarEnd + 1), 1, SCROLL_AREA_HEIGHT);
    thumbTop    = ((SCROLL_AREA_HEIGHT - thumbHeight) * scrollBufferPos) / (scrollBarEnd - scrollBarPage);

    scrollBarThumbTop    = SCROLL_AREA_TOP +  thumbTop;
    scrollBarThumbBottom = SCROLL_AREA_TOP + (thumbTop + thumbHeight);
}

void terminalScrollToStart(void)
{
    if (scrollBarEnd > scrollBarPage)
    {
        scrollBufferPos = 0;
        updateScrollBar();
    }
}

void terminalScrollToEnd(void)
{
    if (scrollBarEnd > scrollBarPage)
    {
        scrollBufferPos = scrollBarEnd - scrollBarPage;
        updateScrollBar();
    }
}

void terminalScrollPageUp(void)
{
    if (scrollBarEnd > scrollBarPage)
    {
        scrollBufferPos -= scrollBarPage;
        if (scrollBufferPos < 0)
            scrollBufferPos = 0;

        updateScrollBar();
    }
}

void terminalScrollPageDown(void)
{
    if (scrollBarEnd > scrollBarPage)
    {
        scrollBufferPos += scrollBarPage;
        if (scrollBufferPos > (scrollBarEnd - scrollBarPage))
            scrollBufferPos =  scrollBarEnd - scrollBarPage;

        updateScrollBar();
    }
}

void terminalScrollUp(void)
{
    if (scrollBarEnd > scrollBarPage)
    {
        if (--scrollBufferPos < 0)
              scrollBufferPos = 0;

        updateScrollBar();
    }
}

void terminalScrollDown(void)
{
    if (scrollBarEnd > scrollBarPage)
    {
        if (++scrollBufferPos > (scrollBarEnd - scrollBarPage))
              scrollBufferPos =  scrollBarEnd - scrollBarPage;

        updateScrollBar();
    }
}

static void putNewLine(void)
{
    uint32_t i;

    if (numLines < TERMINAL_HISTORY_LINES)
    {
        numLines++;
    }
    else
    {
        // we reached the maximum backlog lines,
        // shift the buffer up so we can put our new line
        // at the bottom without using more memory.
        for (i = 0; i < (TERMINAL_HISTORY_LINES - 1); ++i)
            memcpy(&charBuffer[(i * TERMINAL_WIDTH)], &charBuffer[((i + 1) * TERMINAL_WIDTH)], TERMINAL_WIDTH);
    }

    scrollBarEnd = numLines - 1;
    terminalScrollDown();

    if (row < (TERMINAL_HEIGHT - 1))
        row++;

    col = 0;
}

static void putChar(char chr)
{
    charBuffer[((scrollBufferPos + row) * TERMINAL_WIDTH) + col] = chr;

    if (++col == TERMINAL_WIDTH)
    {
        col = 0;
        overflowFlag = true;

        putNewLine();
    }
}

static void putTab(void)
{
    uint8_t i;

    for (i = 0; i < TERMINAL_TAB_SIZE; ++i)
        putChar(' ');
}

static void printBuffer(const char *bufferPtr)
{
    // scroll back to bottom when putting new text
    if (scrollBarEnd > scrollBarPage)
    {
        scrollBufferPos = scrollBarEnd - scrollBarPage;
        updateScrollBar();
    }

    while (*bufferPtr != '\0')
    {
        // line feed
        if (*bufferPtr == '\n')
        {
            if (!overflowFlag)
                putNewLine();

            overflowFlag = false;

            bufferPtr++;
            continue;
        }

        // return carriage
        if (*bufferPtr == '\r')
        {
            col = 0;

            bufferPtr++;
            continue;
        }

        // horizontal tab
        if (*bufferPtr == '\t')
        {
            putTab();

            bufferPtr++;
            continue;
        }

        overflowFlag = false;

        putChar(*bufferPtr++);
    }
}

void terminalPrintf(const char *format, ...)
{
    va_list args;

    if (format[0] == '\0')
        return;

    va_start(args, format);
    vsnprintf(textFormatBuffer, 1024, format, args);
    va_end(args);

    printBuffer(textFormatBuffer);
}

void teriminalPutChar(const char chr)
{
    terminalPrintf("%c", chr);
}

void terminalClear(void)
{
    row = 0;
    col = 0;
    numLines = 0;
    overflowFlag = false;
    scrollBufferPos = 0;
    scrollBarEnd = 0;
    scrollBarThumbTop = 0;
    scrollBarThumbBottom = 0;

    memset(charBuffer, ' ', TERMINAL_BUFFER_SIZE);
}

void terminalHandleScrollBar(int8_t mouseButtonHeld)
{
    int32_t scrollThumbPos;

    if (!mouseButtonHeld)
    {
        if (scrollBarEnd > scrollBarPage)
        {
            if (input.mouse.y < scrollBarThumbTop)
            {
                terminalScrollPageUp();
                return;
            }

            if (input.mouse.y > (scrollBarThumbBottom - 1))
            {
                terminalScrollPageDown();
                return;
            }

            lastDragY  = input.mouse.y;
            lastMouseY = lastDragY - scrollBarThumbTop;
        }

        editor.ui.forceTermBarDrag = true;
    }

    if (input.mouse.y != lastDragY)
    {
        if (scrollBarEnd > scrollBarPage)
        {
            scrollThumbPos  = CLAMP((input.mouse.y - lastMouseY) - SCROLL_AREA_TOP, 0, SCROLL_AREA_HEIGHT);
            scrollBufferPos = CLAMP((scrollThumbPos * scrollBarEnd) / SCROLL_AREA_HEIGHT, 0, scrollBarEnd - scrollBarPage);

            updateScrollBar();

            lastDragY = input.mouse.y;
        }
    }
}

static void renderDate(uint32_t *frameBuffer)
{
    char dateBuffer[35]; // 35 fits the largest possible date string + null terminator
    uint8_t i, AM, mon, day, hour, min, sec, dateTextLen, dateTextMarginLeft;
    uint16_t year;
    time_t rawtime;
    struct tm *ptm;

    time(&rawtime);
    ptm = localtime(&rawtime);

    AM = true; // prevent compiler warning

    year = CLAMP(1900 + ptm->tm_year, 1900, 9999); // wow
    day  = CLAMP(ptm->tm_mday, 1, 31) - 1;
    mon  = CLAMP(ptm->tm_mon,  0, 11);
    hour = CLAMP(ptm->tm_hour, 0, 23);
    min  = CLAMP(ptm->tm_min,  0, 59);
    sec  = CLAMP(ptm->tm_sec,  0, 59);

    // 24hr to AM/PM conversion
    if (hour == 0)
    {
        hour = 12;
        AM = true;
    }
    else if ((hour >= 1) && (hour <= 11))
    {
        AM = true;
    }
    else if (hour == 12)
    {
        AM = false;
    }
    else if ((hour >= 13) && (hour <= 23))
    {
        hour -= 12;
        AM = false;
    }

    sprintf(dateBuffer, "%s of %s %d %d:%02d:%02d%s\n", monthDaysText[day], monthText[mon],
        year, hour, min, sec, AM ? "AM" : "PM");

    dateTextLen = (uint8_t)(strlen(dateBuffer));
    dateTextMarginLeft = ((SCREEN_W / 2) - ((dateTextLen * TERMINAL_FONT_W) / 2)) + (TERMINAL_FONT_W / 2);

    for (i = 0; i < dateTextLen; ++i)
    {
        // shadow
        renderCharacter(frameBuffer, dateTextMarginLeft + 1 + (i * TERMINAL_FONT_W),
            (SCREEN_H - 9) + 1, dateBuffer[i], dateBgColor);

        // foreground
        renderCharacter(frameBuffer, dateTextMarginLeft + (i * TERMINAL_FONT_W),
            SCREEN_H - 9, dateBuffer[i], dateFgColor);
    }
}

void terminalRender(uint32_t *frameBuffer)
{
    int32_t y, x;
    const uint32_t *ptr32Src;
    uint32_t *ptr32Dst, pixel;

    // hide some sprites...
    hideSprite(SPRITE_PATTERN_CURSOR);
    hideSprite(SPRITE_LOOP_PIN_LEFT);
    hideSprite(SPRITE_LOOP_PIN_RIGHT);
    hideSprite(SPRITE_SAMPLING_POS_LINE);

    // clear background with non-palette black
    memset(frameBuffer, 0, SCREEN_W * SCREEN_H * sizeof (int32_t));

    // render window title graphics
    memcpy(frameBuffer, termTopBMP, 320 * 11 * sizeof (uint32_t));

    // render scrollbar graphics
    ptr32Src = termScrollBarBMP;
    ptr32Dst = frameBuffer + ((11 * SCREEN_W) + 309);

    y = 232;
    while (y--)
    {
        memcpy(ptr32Dst, ptr32Src, 11 * sizeof (uint32_t));

        ptr32Dst += SCREEN_W;
        ptr32Src += 11;
    }

    // render scrollbar thumb
    if (scrollBarEnd > scrollBarPage)
    {
        ptr32Dst = frameBuffer + ((scrollBarThumbTop * SCREEN_W) + 311);
        pixel    = palette[PAL_GENBKG2];

        y = scrollBarThumbBottom - scrollBarThumbTop;
        while (y--)
        {
            *(ptr32Dst + 0) = pixel;
            *(ptr32Dst + 1) = pixel;
            *(ptr32Dst + 2) = pixel;
            *(ptr32Dst + 3) = pixel;
            *(ptr32Dst + 4) = pixel;
            *(ptr32Dst + 5) = pixel;
            *(ptr32Dst + 6) = pixel;

            ptr32Dst += SCREEN_W;
        }
    }

    // render text
    if (numLines > 0)
    {
        for (y = 0; y < TERMINAL_HEIGHT; ++y)
        {
            for (x = 0; x < TERMINAL_WIDTH; ++x)
            {
                renderCharacter(frameBuffer, 3 + (x * TERMINAL_FONT_W), 12 + (y * TERMINAL_FONT_H),
                    charBuffer[((scrollBufferPos + y) * TERMINAL_WIDTH) + x], textFgColor);
            }
        }
    }

    renderDate(frameBuffer);
}

int8_t terminalInit(void)
{
    uint8_t *writePtr, readByte, bitMask;
    const uint8_t *readPtr;
    uint32_t i, bit;

    charBuffer = (char *)(malloc(TERMINAL_BUFFER_SIZE));
    if (charBuffer == NULL)
    {
        showErrorMsgBox("Out of memory!");
        return (false);
    }

    textFormatBuffer = (char *)(malloc(1024));
    if (textFormatBuffer == NULL)
    {
        showErrorMsgBox("Out of memory!");
        return (false);
    }

    topazFont = (uint8_t *)(malloc(TOPAZ_UNPACKED_LEN));
    if (topazFont == NULL)
    {
        showErrorMsgBox("Out of memory!");
        return (false);
    }

    // unpack font

    writePtr  = topazFont;
    readPtr   = topazFontPacked;

    for (i = 0; i < TOPAZ_UNPACKED_LEN / 8; ++i)
    {
        readByte = *readPtr++;
        for (bit = 0; bit < 8; ++bit)
        {
            bitMask = (bit == 0) ? 0x01 : (1 << bit);
            *writePtr++ = (readByte & bitMask) >> bit;
        }
    }

    textFgColor = 0x00C0C0C0;
    dateFgColor = 0x00909090;
    dateBgColor = 0x00282828;

    scrollBarPage = TERMINAL_HEIGHT - 1;
    terminalClear();

    return (true);
}

void terminalFree(void)
{
    free(charBuffer);
    free(textFormatBuffer);
    free(topazFont);
}
