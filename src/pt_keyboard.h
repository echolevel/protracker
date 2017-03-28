#ifndef __PT_KEYBOARD_H
#define __PT_KEYBOARD_H

#include <SDL2/SDL.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdint.h>

char scanCodeToUSKey(SDL_Scancode key);

void textMarkerMoveLeft(void);
void textMarkerMoveRight(void);
void textCharPrevious(void);
void textCharNext(void);

#ifdef _WIN32
LRESULT CALLBACK lowLevelKeyboardProc(int32_t nCode, WPARAM wParam, LPARAM lParam);
#endif

void updateKeyModifiers(void);
void handleKeyRepeat(SDL_Scancode keyEntry);
void keyUpHandler(SDL_Scancode keyEntry);
void keyDownHandler(SDL_Scancode keyEntry);

#endif
