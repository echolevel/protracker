#ifndef __PT_TERMINAL_H
#define __PT_TERMINAL_H

#include <stdint.h>

// change this for a bigger backlog, but at the cost of more RAM
#define TERMINAL_HISTORY_LINES 1024
#define TERMINAL_TAB_SIZE 4

#define TERMINAL_WINDOW_WIDTH 304
#define TERMINAL_WINDOW_HEIGHT 232
#define TERMINAL_FONT_W 8
#define TERMINAL_FONT_H 8
#define TERMINAL_WIDTH (TERMINAL_WINDOW_WIDTH / TERMINAL_FONT_W)
#define TERMINAL_HEIGHT (TERMINAL_WINDOW_HEIGHT / TERMINAL_FONT_H)
#define TERMINAL_BUFFER_SIZE (TERMINAL_WIDTH * (TERMINAL_HISTORY_LINES + TERMINAL_HEIGHT))

void terminalPrintf(const char *format, ...);
void teriminalPutChar(const char chr);
void terminalClear(void);
void terminalScrollToStart(void);
void terminalScrollToEnd(void);
void terminalScrollPageUp(void);
void terminalScrollPageDown(void);
void terminalScrollUp(void);
void terminalScrollDown(void);
void terminalHandleScrollBar(int8_t mouseButtonHeld);
void terminalRender(uint32_t *frameBuffer);
int8_t terminalInit(void);
void terminalFree(void);

#endif
