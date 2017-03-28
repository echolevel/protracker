#ifndef __PT_EDIT_H
#define __PT_EDIT_H

#include <SDL2/SDL.h>
#include <stdint.h>

void saveUndo(void);
void undoLastChange(void);
void copySampleTrack(void);
void delSampleTrack(void);
void exchSampleTrack(void);
void trackNoteUp(uint8_t sampleFlag, uint8_t from, uint8_t to);
void trackNoteDown(uint8_t sampleFlag, uint8_t from, uint8_t to);
void trackOctaUp(uint8_t sampleFlag, uint8_t from, uint8_t to);
void trackOctaDown(uint8_t sampleFlag, uint8_t from, uint8_t to);
void pattNoteUp(uint8_t sampleFlag);
void pattNoteDown(uint8_t sampleFlag);
void pattOctaUp(uint8_t sampleFlag);
void pattOctaDown(uint8_t sampleFlag);
void exitGetTextLine(uint8_t updateValue);
void getTextLine(int16_t editObject);
void getNumLine(uint8_t type, int16_t editObject);
void handleEditKeys(SDL_Scancode keyEntry, int8_t normalMode);
uint8_t handleSpecialKeys(SDL_Scancode keyEntry);
int8_t keyToNote(SDL_Keycode keyEntry);
void updateTextObject(int16_t editObject);

#endif
