#ifndef __PT_SAMPLER_H
#define __PT_SAMPLER_H

#include <stdint.h>

int32_t smpPos2Scr(int32_t pos); // sample pos   -> screen x pos
int32_t scr2SmpPos(int32_t x);   // screen x pos -> sample pos

void highPassSample(int32_t cutOff);
void lowPassSample(int32_t cutOff);
void samplerRemoveDcOffset(void);
void mixChordSample(void);
void samplerResample(void);
void doMix(void);
void boostSample(int8_t sample, int8_t ignoreMark);
void filterSample(int8_t sample, int8_t ignoreMark);
void toggleTuningTone(void);
void samplerSamDelete(uint8_t cut);
void samplerSamPaste(void);
void samplerSamCopy(void);
void samplerLoopToggle(void);
void samplerBarPressed(int8_t mouseButtonHeld);
void samplerEditSample(int8_t mouseButtonHeld);
void samplerSamplePressed(int8_t mouseButtonHeld);
void volBoxBarPressed(int8_t mouseButtonHeld);
void samplerZoomOut(void);
void sampleMarkerToBeg(void);
void sampleMarkerToCenter(void);
void sampleMarkerToEnd(void);
void samplerPlayWaveform(void);
void samplerPlayDisplay(void);
void samplerPlayRange(void);
void samplerRangeAll(void);
void samplerShowRange(void);
void samplerShowAll(void);
void redoSampleData(int8_t sample);
void fillSampleRedoBuffer(int8_t sample);
void removeTempLoopPoints(void);
void testTempLoopPoints(int32_t newSampleLen);
void updateSamplePos(void);
void fillSampleFilterUndoBuffer(void);
void exitFromSam(void);
void samplerScreen(void);
void displaySample(void);
void redrawSample(void);
int8_t allocSamplerVars(void);
void deAllocSamplerVars(void);
void setLoopSprites(void);

#endif
