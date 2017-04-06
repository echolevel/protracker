#ifndef __PT_VISUALS_H
#define __PT_VISUALS_H

#include <stdint.h>

void freeBMPs(void);
void handleAskNo(void);
void handleAskYes(void);
int8_t setupVideo(void);
void renderFrame(void);
void flipFrame(void);
void updateSpectrumAnalyzer(int16_t period, int8_t volume);
void sinkVisualizerBars(void);
void updateQuadrascope(void);
void updatePosEd(void);
void updateVisualizer(void);
void updateEditOp(void);
void updateDiskOp(void);
void toggleFullscreen(void);
uint32_t _50HzCallBack(uint32_t interval, void *param);
uint32_t mouseCallback(uint32_t interval, void *param);
void videoClose(void);
int8_t unpackBMPs(void);
void createGraphics(void);
void displayMainScreen(void);
void renderAskDialog(void);
void renderPosEdScreen(void);
void renderDiskOpScreen(void);
void renderMuteButtons(void);
void renderClearScreen(void);
void renderAboutScreen(void);
void renderQuadrascopeBg(void);
void renderSpectrumAnalyzerBg(void);
void renderMonoscopeBg(void);
void renderMOD2WAVDialog(void);
void renderEditOpMode(void);
void renderTextEditMarker(void);
void renderEditOpScreen(void);
void renderSamplerVolBox(void);
void renderSamplerFiltersBox(void);

void removeTextEditMarker(void);
void removeClearScreen(void);
void removeSamplerVolBox(void);
void removeSamplerFiltersBox(void);
void removeAskDialog(void);
void removeTerminalScreen(void);

void fillToVuMetersBgBuffer(void);
void showVolFromSlider(void);
void showVolToSlider(void);
void updateCurrSample(void);
void eraseSprites(void);
void renderSprites(void);
void updateDragBars(void);
void invertRange(void);
void updateCursorPos(void);
void fillFromVuMetersBgBuffer(void);
void renderVuMeters(void);

void setupSprites(void);
void freeSprites(void);
void setSpritePos(uint8_t sprite, uint16_t x, uint16_t y);
void hideSprite(uint8_t sprite);

enum
{
    SPRITE_PATTERN_CURSOR    = 0,
    SPRITE_LOOP_PIN_LEFT     = 1,
    SPRITE_LOOP_PIN_RIGHT    = 2,
    SPRITE_SAMPLING_POS_LINE = 3,
    SPRITE_MOUSE_POINTER     = 4, // above all other sprites

    SPRITE_NUM,

    SPRITE_TYPE_PALETTE = 0,
    SPRITE_TYPE_RGB     = 1
};

#endif
