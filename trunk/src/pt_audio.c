#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <io.h>
#else
#include <unistd.h>
#endif
#include <math.h> // sqrt(),tanf(),M_PI
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include "pt_audio.h"
#include "pt_header.h"
#include "pt_helpers.h"
#include "pt_blep.h"
#include "pt_config.h"
#include "pt_tables.h"
#include "pt_palette.h"
#include "pt_textout.h"
#include "pt_terminal.h"
#include "pt_visuals.h"

// rounded constants to fit in floats
#define M_PI_F  3.1415927f
#define M_2PI_F 6.2831855f

typedef struct ledFilter_t
{
    float led[4];
} ledFilter_t;

typedef struct ledFilterCoeff_t
{
    float led, ledFb;
} ledFilterCoeff_t;

typedef struct voice_t
{
    const int8_t *newData;
    const int8_t *data;
    int8_t newSample, swapSampleFlag, loopFlag, newLoopFlag;
    int32_t length, loopBegin, loopEnd, newLength, newLoopBegin;
    int32_t newLoopEnd, newSampleOffset, index, loopQuirk;
    float vol, panL, panR, fraction, delta, lastFraction, lastDelta;
} voice_t;

static volatile int8_t filterFlags = FILTER_LP_ENABLED;
static int8_t amigaPanFlag, defStereoSep = 25;
int8_t forceMixerOff = false;
static uint16_t ch1Pan, ch2Pan, ch3Pan, ch4Pan;
int32_t samplesPerTick;
static int32_t sampleCounter, renderSampleCounter;
static float *mixBufferL, *mixBufferR;
static blep_t blep[AMIGA_VOICES], blepVol[AMIGA_VOICES];
static lossyIntegrator_t filterLo, filterHi;
static ledFilterCoeff_t filterLEDC;
static ledFilter_t filterLED;
static voice_t voice[AMIGA_VOICES];
static SDL_AudioDeviceID dev;

int8_t processTick(void);      // defined in pt_modplayer.c
void storeTempVariables(void); // defined in pt_modplayer.c

void calcMod2WavTotalRows(void);

void setLEDFilter(uint8_t state)
{
    editor.useLEDFilter = state;

    if (editor.useLEDFilter)
        filterFlags |=  FILTER_LED_ENABLED;
    else
        filterFlags &= ~FILTER_LED_ENABLED;
}

void toggleLEDFilter(void)
{
    editor.useLEDFilter ^= 1;

    if (editor.useLEDFilter)
        filterFlags |=  FILTER_LED_ENABLED;
    else
        filterFlags &= ~FILTER_LED_ENABLED;
}

static void calcCoeffLED(float sr, float hz, ledFilterCoeff_t *filter)
{
    if (hz < (sr / 2.0f))
        filter->led = (M_2PI_F * hz) / sr;
    else
        filter->led = 1.0f;

    filter->ledFb = 0.125f + (0.125f / (1.0f - filter->led)); // Fb = 0.125 : Q ~= 1/sqrt(2) (Butterworth)
}

static void calcCoeffLossyIntegrator(float sr, float hz, lossyIntegrator_t *filter)
{
    filter->coeff[0] = tanf((M_PI_F * hz) / sr);
    filter->coeff[1] = 1.0f / (1.0f + filter->coeff[0]);
}

static void clearLEDFilter(ledFilter_t *filter)
{
    filter->led[0] = 0.0f;
    filter->led[1] = 0.0f;
    filter->led[2] = 0.0f;
    filter->led[3] = 0.0f;
}

static void clearLossyIntegrator(lossyIntegrator_t *filter)
{
    filter->buffer[0] = 0.0f;
    filter->buffer[1] = 0.0f;
}

static inline void lossyIntegratorLED(ledFilterCoeff_t filterC, ledFilter_t *filter, float *in, float *out)
{
    // left channel LED filter
    filter->led[0] += (filterC.led * (in[0] - filter->led[0])
        + filterC.ledFb * (filter->led[0] - filter->led[1]) + 1e-10f);
    filter->led[1] += (filterC.led * (filter->led[0] - filter->led[1]) + 1e-10f);
    out[0] = filter->led[1];

    // right channel LED filter
    filter->led[2] += (filterC.led * (in[1] - filter->led[2])
        + filterC.ledFb * (filter->led[2] - filter->led[3]) + 1e-10f);
    filter->led[3] += (filterC.led * (filter->led[2] - filter->led[3]) + 1e-10f);
    out[1] = filter->led[3];
}

inline void lossyIntegrator(lossyIntegrator_t *filter, float *in, float *out)
{
    float output;

    // left channel low-pass
    output = (filter->coeff[0] * in[0] + filter->buffer[0]) * filter->coeff[1];
    filter->buffer[0] = filter->coeff[0] * (in[0] - output) + output + 1e-10f;
    out[0] = output;

    // right channel low-pass
    output = (filter->coeff[0] * in[1] + filter->buffer[1]) * filter->coeff[1];
    filter->buffer[1] = filter->coeff[0] * (in[1] - output) + output + 1e-10f;
    out[1] = output;
}

inline void lossyIntegratorHighPass(lossyIntegrator_t *filter, float *in, float *out)
{
    float low[2];

    lossyIntegrator(filter, in, low);

    out[0] = in[0] - low[0];
    out[1] = in[1] - low[1];
}

static inline void updateScope(moduleChannel_t *ch, voice_t *v)
{
    ch->scopeLoopFlag = v->loopFlag;

    ch->scopeLoopQuirk   = false;
    ch->scopeLoopQuirk_f = ch->scopeLoopQuirk;

    ch->scopePos_f = v->newSampleOffset + v->index;

    ch->scopeLoopBegin = v->newSampleOffset + v->loopBegin;
    ch->scopeLoopEnd   = v->newSampleOffset + v->loopEnd;
    ch->scopeEnd       = v->newSampleOffset + v->length;

    ch->scopeLoopBegin_f = ch->scopeLoopBegin;
    ch->scopeLoopEnd_f   = ch->scopeLoopEnd;
    ch->scopeEnd_f       = ch->scopeEnd;
}

static inline void updateSampleData(voice_t *v)
{
    v->loopFlag  = v->newLoopFlag;
    v->loopBegin = v->newLoopBegin;
    v->loopEnd   = v->newLoopEnd;
    v->data      = v->newData;
    v->length    = v->newLength;
}

void mixerSwapVoiceSource(uint8_t ch, const int8_t *src, int32_t length, int32_t loopStart, int32_t loopLength, int32_t newSampleOffset)
{
    voice_t *v;

    // If you swap sample in-realtime in PT
    // without putting a period, the current
    // sample will play through first (either
    // >len or >loop_len), then when that is
    // reached you change to the new sample you
    // put earlier (if new sample is looped)

    if ((loopStart + loopLength) > 2)
    {
        if (((loopStart + loopLength) > length) || (loopStart >= length))
            return;
    }

    v = &voice[ch];

    v->loopQuirk       = false;
    v->newSampleOffset = newSampleOffset;
    v->swapSampleFlag  = true;
    v->newData         = src;
    v->newLoopFlag     = (loopStart + loopLength) > 2;
    v->newLength       = length;
    v->newLoopBegin    = loopStart;
    v->newLoopEnd      = loopStart + loopLength;

    // if the mixer was already shut down earlier after a non-loop swap,
    // force swap again, but only if the new sample has loop enabled (ONLY!)
    if ((v->data == NULL) && v->newLoopFlag)
    {
        updateSampleData(v);

        // we need to wrap here for safety reasons
        while (v->index >= v->loopEnd)
               v->index  = v->loopBegin + (v->index - v->loopEnd);

        updateScope(&modEntry->channels[ch], v);
        modEntry->channels[ch].scopeTrigger = true;
    }
}

void mixerSetVoiceSource(uint8_t ch, const int8_t *src, int32_t length, int32_t loopStart, int32_t loopLength, int32_t offset)
{
    voice_t *v;

    v = &voice[ch];

    // PT quirk: LENGTH >65534 + effect 9xx (any number) = shut down voice
    if ((length > 65534) && (offset > 0))
    {
        v->data = NULL;
        return;
    }

    if ((loopStart + loopLength) > 2)
    {
        if ((loopStart >= length) || ((loopStart + loopLength) > length))
            return;
    }

    v->loopQuirk      = false;
    v->swapSampleFlag = false;
    v->data           = src;
    v->index          = offset;
    v->fraction       = 0.0f;
    v->length         = length;
    v->loopFlag       = (loopStart + loopLength) > 2;
    v->loopBegin      = loopStart;
    v->loopEnd        = loopStart + loopLength;

    // Check external 9xx usage (Set Sample Offset)
    if (v->loopFlag)
    {
        if (offset >= v->loopEnd)
            v->index = v->loopBegin;
    }
    else
    {
        if (offset >= v->length)
            v->data = NULL;
    }

    if ((loopLength > 2) && (loopStart == 0))
    {
        v->loopQuirk = v->loopEnd;
        v->loopEnd   = v->length;
    }
}

void mixerSetVoiceOffset(uint8_t ch, int32_t offset) // used for PlayRange/PlayDisplay only (sampler screen)
{
    voice_t *v;

    v = &voice[ch];

    if (offset >= v->length)
        v->data = NULL;
    else
        v->index = offset;
}

// adejr: these sin/cos approximations both use a 0..1
// parameter range and have 'normalized' (1/2 = 0db) coeffs
//
// the coeffs are for LERP(x, x * x, 0.224) * sqrt(2)
// max_error is minimized with 0.224 = 0.0013012886

static float sinApx(float x)
{
    x = x * (2.0f - x);
    return (x * 1.09742972f + x * x * 0.31678383f);
}

static float cosApx(float x)
{
    x = (1.0f - x) * (1.0f + x);
    return (x * 1.09742972f + x * x * 0.31678383f);
}

static void mixerSetVoicePan(uint8_t ch, uint16_t pan) // pan = 0..256
{
    float p;

    // proper 'normalized' equal-power panning is (assuming pan left to right):
    // L = cos(p * pi * 1/2) * sqrt(2);
    // R = sin(p * pi * 1/2) * sqrt(2);

    p = pan * (1.0f / 256.0f);

    voice[ch].panL = cosApx(p);
    voice[ch].panR = sinApx(p);
}

void mixerKillVoice(uint8_t ch)
{
    float tmpDelta, tmpVol, tmpPanL, tmpPanR;

    SDL_LockAudio();

    tmpVol   = voice[ch].vol;
    tmpPanL  = voice[ch].panL;
    tmpPanR  = voice[ch].panR;
    tmpDelta = voice[ch].delta;

    memset(&voice[ch],   0, sizeof (voice_t));
    memset(&blep[ch],    0, sizeof (blep_t));
    memset(&blepVol[ch], 0, sizeof (blep_t));

    voice[ch].data  = NULL;
    voice[ch].vol   = tmpVol;
    voice[ch].panL  = tmpPanL;
    voice[ch].panR  = tmpPanR;
    voice[ch].delta = tmpDelta;

    SDL_UnlockAudio();
}

void mixerKillAllVoices(void)
{
    int8_t i;

    SDL_LockAudio();

    memset(voice,   0, sizeof (voice));
    memset(blep,    0, sizeof (blep));
    memset(blepVol, 0, sizeof (blepVol));

    clearLossyIntegrator(&filterLo);
    clearLossyIntegrator(&filterHi);
    clearLEDFilter(&filterLED);

    editor.tuningFlag = false;

    mixerSetVoicePan(0, ch1Pan);
    mixerSetVoicePan(1, ch2Pan);
    mixerSetVoicePan(2, ch3Pan);
    mixerSetVoicePan(3, ch4Pan);

    for (i = 0; i < AMIGA_VOICES; ++i)
        voice[i].data = NULL;

    SDL_UnlockAudio();
}

void mixerKillVoiceIfReadingSample(uint8_t sample)
{
    uint8_t i;
    moduleChannel_t *ch;

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        // voice[x].data *always* points to sampleData[s->offset], so this method is safe.
        // Sample offsets (9xx, etc) are added to voice[x].index instead.

        if (voice[i].data == &modEntry->sampleData[modEntry->samples[sample].offset])
        {
            mixerKillVoice(i);

            ch = &modEntry->channels[i];

            ch->scopeEnabled    = false;
            ch->scopeTrigger    = false;
            ch->scopeLoopFlag   = false;
            ch->scopeKeepDelta  = true;
            ch->scopeKeepVolume = true;
        }
    }
}

void mixerSetVoiceVol(uint8_t ch, int8_t vol)
{
    voice[ch].vol = vol * (1.0f / 64.0f);
}

void mixerSetVoiceDelta(uint8_t ch, uint16_t period)
{
    voice_t *v;

    v = &voice[ch];

    // this is what really happens on Paula on a real Amiga
    // on normal video modes. Tested and confirmed!

    if (period == 0)
    {
        v->delta = 0.0f;
        return;
    }

    if (period < 113)
        period = 113;

    if (!editor.isSMPRendering)
    {
        v->delta = ((float)(PAULA_PAL_CLK) / period) / editor.outputFreq_f;
    }
    else
    {
        // for pattern-to-sample rendering
        if (editor.pat2SmpHQ)
            v->delta = ((float)(PAULA_PAL_CLK) / period) / 28836.0f; // high quality mode
        else
            v->delta = ((float)(PAULA_PAL_CLK) / period) / 22168.0f;
    }

    if (v->lastDelta == 0.0f)
        v->lastDelta = v->delta;
}

static inline void insertEndingBlep(blep_t *b, voice_t *v)
{
    if (b->lastValue != 0.0f)
    {
        if ((v->lastDelta > 0.0f) && (v->lastDelta > v->lastFraction))
            blepAdd(b, v->lastFraction / v->lastDelta, b->lastValue);

        b->lastValue = 0.0f;
    }
}

void updateVoiceParams(void)
{
    uint8_t i;
    moduleSample_t *s;
    moduleChannel_t *ch;
    voice_t *v;

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        // voice[x].data *always* points to sampleData[s->offset], so this method is safe.
        // Sample offsets (9xx, etc) are added to voice[x].index instead.

        if (voice[i].data == &modEntry->sampleData[modEntry->samples[editor.currSample].offset])
        {
            s  = &modEntry->samples[editor.currSample];
            ch = &modEntry->channels[i];
            v  = &voice[i];

            v->loopBegin = s->loopStart;
            v->loopEnd   = s->loopStart + s->loopLength;
            v->length    = s->length;

            ch->scopeLoopQuirk   = false;
            ch->scopeLoopQuirk_f = ch->scopeLoopQuirk;

            ch->scopeLoopBegin = s->offset + v->loopBegin;
            ch->scopeLoopEnd   = s->offset + v->loopEnd;
            ch->scopeEnd       = s->offset + s->length;

            ch->scopeLoopBegin_f = ch->scopeLoopBegin;
            ch->scopeLoopEnd_f   = ch->scopeLoopEnd;
            ch->scopeEnd_f       = ch->scopeEnd;
        }
    }
}

void toggleLowPassFilter(void)
{
    if (filterFlags & FILTER_LP_ENABLED)
    {
        filterFlags &= ~FILTER_LP_ENABLED;

        displayMsg("FILTER MOD: A1200");
    }
    else
    {
        filterFlags |= FILTER_LP_ENABLED;
        clearLossyIntegrator(&filterLo);

        editor.errorMsgActive  = true;
        editor.errorMsgBlock   = false;
        editor.errorMsgCounter = 24; // medium short flash

        displayMsg("FILTER MOD: A500");
    }
}

static void outputAudio(int16_t *target, int32_t numSamples)
{
    volatile float *vuMeter;
    int8_t tmpVol, tmpSmp8;
    uint8_t i;
    int16_t *outStream;
    int32_t j;
    float tempSample, tempVolume, out[2], mutedVol;
    blep_t *bSmp, *bVol;
    voice_t *v;
    moduleChannel_t *ch;

    memset(mixBufferL, 0, sizeof (float) * numSamples);
    memset(mixBufferR, 0, sizeof (float) * numSamples);

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        v = &voice[i];

        mutedVol = -1.0f;
        if (editor.muted[i])
        {
            mutedVol = v->vol;
            v->vol   = 0.0f;
        }

        if (voice[i].data != NULL)
        {
            bSmp    = &blep[i];
            bVol    = &blepVol[i];
            ch      = &modEntry->channels[i];
            vuMeter = &editor.realVuMeterVolumes[i];

            j = 0;
            for (; j < numSamples; ++j)
            {
                tmpSmp8 = (v->data != NULL) ? v->data[v->index] : 0;

                tempSample = tmpSmp8 * (1.0f / 128.0f);
                tempVolume = (v->data != NULL) ? v->vol : 0.0f;

                // if sample data changes, anti-alias the step
                // this code assumes index can't change >1 per sample
                if (editor.blepSynthesis)
                {
                    if (tempSample != bSmp->lastValue)
                    {
                        if ((v->lastDelta > 0.0f) && (v->lastDelta > v->lastFraction))
                            blepAdd(bSmp, v->lastFraction / v->lastDelta, bSmp->lastValue - tempSample);

                        bSmp->lastValue = tempSample;
                    }

                    // if volume data changes, anti-alias the step
                    if (tempVolume != bVol->lastValue)
                    {
                        PT_ASSERT(tempVolume <= 1.0f);

                        blepAdd(bVol, 0.0f, bVol->lastValue - tempVolume);
                        bVol->lastValue = tempVolume;
                    }

                    // add sample anti-alias data to sample value and scale by volume
                    if (bSmp->samplesLeft) tempSample += blepRun(bSmp);

                    // add volume anti-alias pulse data to volume value
                    if (bVol->samplesLeft) tempVolume += blepRun(bVol);
                }

                tempSample *= tempVolume;

                if (editor.ui.realVuMeters)
                {
                    tmpVol = tempSample * 48.0f;
                    tmpVol = ABS(tmpVol);
                    if (tmpVol > *vuMeter)
                        *vuMeter = tmpVol;
                }

                mixBufferL[j] += (tempSample * v->panL);
                mixBufferR[j] += (tempSample * v->panR);

                if (v->data != NULL)
                {
                    v->fraction += v->delta;

                    if (v->fraction >= 1.0f)
                    {
                        v->index++;
                        v->fraction -= 1.0f;

                        // store last updated state for blep fraction
                        v->lastFraction = v->fraction;
                        v->lastDelta    = v->delta;

                        PT_ASSERT(v->lastDelta > v->lastFraction);

                        if (v->loopFlag)
                        {
                            if (v->index >= v->loopEnd)
                            {
                                if (v->swapSampleFlag)
                                {
                                    v->swapSampleFlag = false;
 
                                    if (!v->newLoopFlag)
                                    {
                                        v->data = NULL;
                                        ch->scopeLoopFlag = false;

                                        if (editor.blepSynthesis)
                                        {
                                            insertEndingBlep(bSmp, v);
                                            j++; // we inserted an ending blep sample
                                        }

                                        break;
                                    }

                                    updateSampleData(v);

                                    v->index = v->loopBegin;

                                    ch->scopeLoopFlag = true;
                                    updateScope(ch, v);
                                }
                                else
                                {
                                    v->index = v->loopBegin;

                                    if (v->loopQuirk)
                                    {
                                        v->loopEnd   = v->loopQuirk;
                                        v->loopQuirk = false;
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (v->index >= v->length)
                            {
                                if (v->swapSampleFlag)
                                {
                                    v->swapSampleFlag = false;

                                    if (!v->newLoopFlag)
                                    {
                                        v->data = NULL;
                                        ch->scopeLoopFlag = false;

                                        if (editor.blepSynthesis)
                                        {
                                            insertEndingBlep(bSmp, v);
                                            j++; // we inserted an ending blep sample
                                        }

                                        break;
                                    }

                                    updateSampleData(v);

                                    v->index = v->loopBegin;

                                    ch->scopeLoopFlag = true;
                                    updateScope(ch, v);
                                }
                                else
                                {
                                    v->data = NULL;

                                    if (editor.blepSynthesis)
                                    {
                                        insertEndingBlep(bSmp, v);
                                        j++; // we inserted an ending blep sample
                                    }

                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // sometimes a channel might be empty,
            // but the blep-buffer still contains data.
            if (editor.blepSynthesis)
            {
                if ((j < numSamples) && (v->data == NULL) && (bSmp->samplesLeft || bVol->samplesLeft))
                {
                    for (; j < numSamples; ++j)
                    {
                        tempSample = bSmp->lastValue;
                        tempVolume = bVol->lastValue;

                        if (bSmp->samplesLeft) tempSample += blepRun(bSmp);
                        if (bVol->samplesLeft) tempVolume += blepRun(bVol);

                        tempSample *= tempVolume;

                        mixBufferL[j] += (tempSample * v->panL);
                        mixBufferR[j] += (tempSample * v->panR);
                    }
                }
            }
        }

        if (mutedVol != -1.0f)
            v->vol = mutedVol;
    }

    outStream = target;
    for (j = 0; j < numSamples; ++j)
    {
        out[0] = mixBufferL[j];
        out[1] = mixBufferR[j];

        if (filterFlags & FILTER_LP_ENABLED)
            lossyIntegrator(&filterLo, out, out);

        if (filterFlags & FILTER_LED_ENABLED)
            lossyIntegratorLED(filterLEDC, &filterLED, out, out);

        lossyIntegratorHighPass(&filterHi, out, out);

        // - Highpass doubles channel peak
        // - Panning adds sqrt(2) at 50% and 100% wide pans
        // - Minimal correlation means mixing channels can result in sqrt(2), not 2
        // - All this taken into account makes the best-case level of 2 okay,
        // although 8*sqrt(2)*little from LED filter overshoot is possible worst-case!
        // - Even in the best case without pan and without high-pass, the level for
        // chiptunes using amp 64 pulse waves will be 4. With pan, 2 * sqrt(2) (2.83).
        // - It is likely that the majority of modules will be peaking around 3.
        out[0] *= (-32767.0f / 3.2f); // negative because signal phase is flipped on A500/A1200 for some reason
        out[1] *= (-32767.0f / 3.2f); // ----

        // clamp in case of overflow
        out[0] = CLAMP(out[0], -32768.0f, 32767.0f);
        out[1] = CLAMP(out[1], -32768.0f, 32767.0f);

        // convert to int and send to output
        *outStream++ = (int16_t)(out[0]);
        *outStream++ = (int16_t)(out[1]);
    }
}

void audioCallback(void *userdata, uint8_t *stream, int32_t len)
{
    int16_t *out;
    int32_t sampleBlock, samplesTodo;

    (void)(userdata); // make compiler happy

    if (forceMixerOff) // for MOD2WAV
    {
        memset(stream, 0, len); // mute
        return;
    }

    out = (int16_t *)(stream);

    sampleBlock = len / 4;
    while (sampleBlock)
    {
        samplesTodo = (sampleBlock < sampleCounter) ? sampleBlock : sampleCounter;
        if (samplesTodo > 0)
        {
            outputAudio(out, samplesTodo);
            out += (2 * samplesTodo);

            sampleBlock   -= samplesTodo;
            sampleCounter -= samplesTodo;
        }
        else
        {
            if (editor.songPlaying)
                processTick();

            sampleCounter = samplesPerTick;
        }
    }
}

static void calculateFilterCoeffs(void)
{
    double lp_R, lp_C, lp_Hz;
    double led_R1, led_R2, led_C1, led_C2, led_Hz;
    double hp_R, hp_C, hp_Hz;

    // Amiga 500 filter emulation, by aciddose (Xhip author)
    // All Amiga computers have three (!) filters, not just the "LED" filter.
    //
    // First comes a static low-pass 6dB formed by the supply current
    // from the Paula's mixture of channels A+B / C+D into the opamp with
    // 0.1uF capacitor and 360 ohm resistor feedback in inverting mode biased by
    // dac vRef (used to center the output).
    //
    // R = 360 ohm
    // C = 0.1uF
    // Low Hz = 4420.97~ = 1 / (2pi * 360 * 0.0000001)
    //
    // Under spice simulation the circuit yields -3dB = 4400Hz.
    // In the Amiga 1200 and CD-32, the low-pass cutoff is 26kHz+, so the
    // static low-pass filter is disabled in the mixer in A1200 mode.
    //
    // Next comes a bog-standard Sallen-Key filter ("LED") with:
    // R1 = 10K ohm
    // R2 = 10K ohm
    // C1 = 6800pF
    // C2 = 3900pF
    // Q ~= 1/sqrt(2)
    //
    // This filter is optionally bypassed by an MPF-102 JFET chip when
    // the LED filter is turned off.
    //
    // Under spice simulation the circuit yields -3dB = 2800Hz.
    // 90 degrees phase = 3000Hz (so, should oscillate at 3kHz!)
    //
    // The buffered output of the Sallen-Key passes into an RC high-pass with:
    // R = 1.39K ohm (1K ohm + 390 ohm)
    // C = 22uF (also C = 330nF, for improved high-frequency)
    //
    // High Hz = 5.2~ = 1 / (2pi * 1390 * 0.000022)
    // Under spice simulation the circuit yields -3dB = 5.2Hz.

    // Amiga 500 RC low-pass filter:
    lp_R  = 360.0;     // 360 ohm resistor
    lp_C  = 0.0000001; // 0.1uF capacitor
    lp_Hz = 1.0 / (2.0 * M_PI * lp_R * lp_C);
    calcCoeffLossyIntegrator(editor.outputFreq_f, (float)(lp_Hz), &filterLo);

    // Amiga 500 Sallen-Key "LED" filter:
    led_R1 = 10000.0;      // 10K ohm resistor
    led_R2 = 10000.0;      // 10K ohm resistor
    led_C1 = 0.0000000068; // 6800pF capacitor
    led_C2 = 0.0000000039; // 3900pF capacitor
    led_Hz = 1.0 / (2.0 * M_PI * sqrt(led_R1 * led_R2 * led_C1 * led_C2));
    calcCoeffLED(editor.outputFreq_f, (float)(led_Hz), &filterLEDC);

    // Amiga 500 RC high-pass filter:
    hp_R  = 1390.0;   // 1K ohm resistor + 390 ohm resistor
    hp_C  = 0.000022; // 22uF capacitor
    hp_Hz = 1.0 / (2.0 * M_PI * hp_R * hp_C);
    calcCoeffLossyIntegrator(editor.outputFreq_f, (float)(hp_Hz), &filterHi);
}

void mixerCalcVoicePans(uint8_t stereoSeparation)
{
    uint8_t scaledPanPos;

    scaledPanPos = (stereoSeparation * 128) / 100;

    ch1Pan = 128 - scaledPanPos;
    ch2Pan = 128 + scaledPanPos;
    ch3Pan = 128 + scaledPanPos;
    ch4Pan = 128 - scaledPanPos;

    mixerSetVoicePan(0, ch1Pan);
    mixerSetVoicePan(1, ch2Pan);
    mixerSetVoicePan(2, ch3Pan);
    mixerSetVoicePan(3, ch4Pan);
}

int8_t setupAudio(void)
{
    int32_t maxSamplesToMix;
    SDL_AudioSpec want, have;

    want.freq     = ptConfig.soundFrequency;
    want.format   = AUDIO_S16;
    want.channels = 2;
    want.callback = audioCallback;
    want.userdata = NULL;
    want.samples  = 1024; // should be 2^n for compatibility with all sound cards

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0)
    {
        showErrorMsgBox("Unable to open audio device: %s", SDL_GetError());
        return (false);
    }

    if (have.freq < 44100) // lower than this is not safe for one-step mixer w/ BLEP
    {
        showErrorMsgBox("Unable to open audio: The audio output rate couldn't be used!");
        return (false);
    }

    if (have.format != want.format)
    {
        showErrorMsgBox("Unable to open audio: The sample format (signed 16-bit) couldn't be used!");
        return (false);
    }

    maxSamplesToMix = (int32_t)(((have.freq * 2.5) / 32.0) + 0.5);

    mixBufferL = (float *)(calloc(maxSamplesToMix, sizeof (float)));
    if (mixBufferL == NULL)
    {
        showErrorMsgBox("Out of memory!");
        return (false);
    }

    mixBufferR = (float *)(calloc(maxSamplesToMix, sizeof (float)));
    if (mixBufferR == NULL)
    {
        showErrorMsgBox("Out of memory!");
        return (false);
    }

    editor.mod2WavBuffer = (int16_t *)(malloc(sizeof (int16_t) * maxSamplesToMix));
    if (editor.mod2WavBuffer == NULL)
    {
        showErrorMsgBox("Out of memory!");
        return (false);
    }

    editor.audioBufferSize  = have.samples;
    ptConfig.soundFrequency = have.freq;
    editor.outputFreq       = ptConfig.soundFrequency;
    editor.outputFreq_f     = (float)(ptConfig.soundFrequency);

    mixerCalcVoicePans(ptConfig.stereoSeparation);
    defStereoSep = ptConfig.stereoSeparation;

    filterFlags = ptConfig.a500LowPassFilter ? FILTER_LP_ENABLED : 0;

    calculateFilterCoeffs();

    samplesPerTick = 0;
    sampleCounter  = 0;

    SDL_PauseAudioDevice(dev, false);

    return (true);
}

void audioClose(void)
{
    editor.songPlaying = false;

    mixerKillAllVoices();

    if (dev > 0)
    {
        SDL_PauseAudioDevice(dev, true);
        SDL_CloseAudioDevice(dev);
        dev = 0;
    }

    if (mixBufferL != NULL)
    {
        free(mixBufferL);
        mixBufferL = NULL;
    }

    if (mixBufferR != NULL)
    {
        free(mixBufferR);
        mixBufferR = NULL;
    }

    if (editor.mod2WavBuffer != NULL)
    {
        free(editor.mod2WavBuffer);
        editor.mod2WavBuffer = NULL;
    }
}

void mixerSetSamplesPerTick(int32_t val)
{
    samplesPerTick = val;
}

void toggleAmigaPanMode(void)
{
    amigaPanFlag ^= 1;

    if (!amigaPanFlag)
    {
        mixerCalcVoicePans(defStereoSep);

        editor.errorMsgActive  = true;
        editor.errorMsgBlock   = false;
        editor.errorMsgCounter = 24; // medium short flash

        setStatusMessage("AMIGA PANNING OFF", NO_CARRY);
    }
    else
    {
        mixerCalcVoicePans(100);

        editor.errorMsgActive  = true;
        editor.errorMsgBlock   = false;
        editor.errorMsgCounter = 24; // medium short flash

        setStatusMessage("AMIGA PANNING ON", NO_CARRY);
    }
}

// PAT2SMP RELATED STUFF

static inline void insertEndingBlep2(blep_t *b, voice_t *v)
{
    if (v->delta < 1.0f)
    {
        if (b->lastValue != 0.0f)
        {
            if ((v->lastDelta > 0.0f) && (v->lastDelta > v->lastFraction))
                blepAdd(b, v->lastFraction / v->lastDelta, b->lastValue);

            b->lastValue = 0.0f;
        }
    }
}

void outputAudioToSample(int32_t numSamples)
{
    int8_t tmpSmp8;
    uint8_t i;
    int32_t j, fracTrunc;
    float tempSample, tempVolume, out[2], mutedVol;
    blep_t *bSmp, *bVol;
    voice_t *v;
    moduleChannel_t *ch;

    memset(mixBufferL, 0, numSamples * sizeof (float));

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        v = &voice[i];

        mutedVol = -1.0f;
        if (editor.muted[i])
        {
            mutedVol = v->vol;
            v->vol   = 0.0f;
        }

        if (voice[i].data != NULL)
        {
            bSmp = &blep[i];
            bVol = &blepVol[i];
            ch   = &modEntry->channels[i];

            j = 0;
            for (; j < numSamples; ++j)
            {
                tmpSmp8 = (v->data != NULL) ? v->data[v->index] : 0;

                tempSample = tmpSmp8 * (1.0f / 128.0f);
                tempVolume = (v->data != NULL) ? v->vol : 0.0f;

                if (v->delta < 1.0f)
                {
                    if (tempSample != bSmp->lastValue)
                    {
                        if ((v->lastDelta > 0.0f) && (v->lastDelta > v->lastFraction))
                            blepAdd(bSmp, v->lastFraction / v->lastDelta, bSmp->lastValue - tempSample);

                        bSmp->lastValue = tempSample;
                    }

                    if (tempVolume != bVol->lastValue)
                    {
                        blepAdd(bVol, 0.0f, bVol->lastValue - tempVolume);
                        bVol->lastValue = tempVolume;
                    }
                }

                if (bSmp->samplesLeft) tempSample += blepRun(bSmp);
                if (bVol->samplesLeft) tempVolume += blepRun(bVol);

                tempSample *= tempVolume;

                mixBufferL[j] += tempSample;

                if (v->data != NULL)
                {
                    v->fraction += v->delta;

                    if (v->fraction >= 1.0f)
                    {
                        fracTrunc    = (int32_t)(v->fraction);
                        v->index    += fracTrunc;
                        v->fraction -= fracTrunc;

                        v->lastFraction = v->fraction;
                        v->lastDelta    = v->delta;

                        if (v->loopFlag)
                        {
                            if (v->index >= v->loopEnd)
                            {
                                if (v->swapSampleFlag)
                                {
                                    v->swapSampleFlag = false;

                                    if (!v->newLoopFlag)
                                    {
                                        v->data = NULL;
                                        ch->scopeLoopFlag = false;

                                        if (v->delta < 1.0f)
                                            insertEndingBlep2(bSmp, v);

                                        j++;
                                        break;
                                    }

                                    updateSampleData(v);

                                    v->index = v->loopBegin;

                                    ch->scopeLoopFlag = true;
                                    updateScope(ch, v);
                                }
                                else
                                {
                                    v->index = v->loopBegin;

                                    if (v->loopQuirk)
                                    {
                                        v->loopEnd   = v->loopQuirk;
                                        v->loopQuirk = false;
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (v->index >= v->length)
                            {
                                if (v->swapSampleFlag)
                                {
                                    v->swapSampleFlag = false;

                                    if (!v->newLoopFlag)
                                    {
                                        v->data = NULL;
                                        ch->scopeLoopFlag = false;

                                        if (v->delta < 1.0f)
                                            insertEndingBlep2(bSmp, v);

                                        j++;
                                        break;
                                    }

                                    updateSampleData(v);

                                    v->index = v->loopBegin;

                                    ch->scopeLoopFlag = true;
                                    updateScope(ch, v);
                                }
                                else
                                {
                                    v->data = NULL;

                                    if (v->delta < 1.0f)
                                        insertEndingBlep2(bSmp, v);

                                    j++;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if ((j < numSamples) && (v->data == NULL) && (bSmp->samplesLeft || bVol->samplesLeft))
            {
                for (; j < numSamples; ++j)
                {
                    tempSample = bSmp->lastValue;
                    tempVolume = bVol->lastValue;

                    if (bSmp->samplesLeft) tempSample += blepRun(bSmp);
                    if (bVol->samplesLeft) tempVolume += blepRun(bVol);

                    tempSample *= tempVolume;

                    mixBufferL[j] += tempSample;
                }
            }
        }

        if (mutedVol != -1.0f)
            v->vol = mutedVol;
    }

    for (j = 0; j < numSamples; ++j)
    {
        out[0] = mixBufferL[j];
        out[1] = mixBufferL[j];

        lossyIntegratorHighPass(&filterHi, out, out);

        out[0] *= (32767.0f / 3.2f);
        out[0]  = CLAMP(out[0], -32768.0f, 32767.0f);

        if (editor.pat2SmpPos < MAX_SAMPLE_LEN)
        {
            editor.pat2SmpBuf[editor.pat2SmpPos] = (int16_t)(out[0]);
            editor.pat2SmpPos++;
        }
    }
}

// MOD2WAV RELATED STUFF
static void outputAudioToFile(FILE *fOut, int32_t numSamples)
{
    int8_t tmpSmp8;
    uint8_t i;
    int16_t *ptr, renderL, renderR;
    int32_t j;
    float tempSample, tempVolume, out[2], mutedVol;
    blep_t *bSmp, *bVol;
    voice_t *v;
    moduleChannel_t *ch;

    memset(mixBufferL, 0, numSamples * sizeof (float));
    memset(mixBufferR, 0, numSamples * sizeof (float));

    ptr = editor.mod2WavBuffer;

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        v = &voice[i];

        mutedVol = -1.0f;
        if (editor.muted[i])
        {
            mutedVol = v->vol;
            v->vol   = 0.0f;
        }

        if (voice[i].data != NULL)
        {
            bSmp = &blep[i];
            bVol = &blepVol[i];
            ch   = &modEntry->channels[i];

            j = 0;
            for (; j < numSamples; ++j)
            {
                tmpSmp8 = (v->data != NULL) ? v->data[v->index] : 0;

                tempSample = tmpSmp8 * (1.0f / 128.0f);
                tempVolume = (v->data != NULL) ? v->vol : 0.0f;

                if (tempSample != bSmp->lastValue)
                {
                    PT_ASSERT(v->lastDelta > 0.0f);
                    PT_ASSERT(v->lastDelta > v->lastFraction);

                    if (v->lastDelta > 0.0f)
                        blepAdd(bSmp, v->lastFraction / v->lastDelta, bSmp->lastValue - tempSample);

                    bSmp->lastValue = tempSample;
                }

                if (tempVolume != bVol->lastValue)
                {
                    PT_ASSERT(tempVolume <= 1.0f);

                    blepAdd(bVol, 0.0f, bVol->lastValue - tempVolume);
                    bVol->lastValue = tempVolume;
                }

                if (bSmp->samplesLeft) tempSample += blepRun(bSmp);
                if (bVol->samplesLeft) tempVolume += blepRun(bVol);

                tempSample *= tempVolume;

                mixBufferL[j] += (tempSample * v->panL);
                mixBufferR[j] += (tempSample * v->panR);

                if (v->data != NULL)
                {
                    v->fraction += v->delta;

                    if (v->fraction >= 1.0f)
                    {
                        v->index++;
                        v->fraction -= 1.0f;

                        v->lastFraction = v->fraction;
                        v->lastDelta    = v->delta;

                        PT_ASSERT(v->lastDelta > v->lastFraction);

                        if (v->loopFlag)
                        {
                            if (v->index >= v->loopEnd)
                            {
                                if (v->swapSampleFlag)
                                {
                                    v->swapSampleFlag = false;

                                    if (!v->newLoopFlag)
                                    {
                                        v->data = NULL;
                                        ch->scopeLoopFlag = false;

                                        insertEndingBlep(bSmp, v);
                                        j++;

                                        break;
                                    }

                                    updateSampleData(v);

                                    v->index = v->loopBegin;

                                    ch->scopeLoopFlag = true;
                                    updateScope(ch, v);
                                }
                                else
                                {
                                    v->index = v->loopBegin;

                                    if (v->loopQuirk)
                                    {
                                        v->loopEnd   = v->loopQuirk;
                                        v->loopQuirk = false;
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (v->index >= v->length)
                            {
                                if (v->swapSampleFlag)
                                {
                                    v->swapSampleFlag = false;

                                    if (!v->newLoopFlag)
                                    {
                                        v->data = NULL;
                                        ch->scopeLoopFlag = false;

                                        insertEndingBlep(bSmp, v);
                                        j++;

                                        break;
                                    }

                                    updateSampleData(v);

                                    v->index = v->loopBegin;

                                    ch->scopeLoopFlag = true;
                                    updateScope(ch, v);
                                }
                                else
                                {
                                    v->data = NULL;

                                    insertEndingBlep(bSmp, v);
                                    j++;

                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if ((j < numSamples) && (v->data == NULL) && (bSmp->samplesLeft || bVol->samplesLeft))
            {
                for (; j < numSamples; ++j)
                {
                    tempSample = bSmp->lastValue;
                    tempVolume = bVol->lastValue;

                    if (bSmp->samplesLeft) tempSample += blepRun(bSmp);
                    if (bVol->samplesLeft) tempVolume += blepRun(bVol);

                    tempSample *= tempVolume;

                    mixBufferL[j] += (tempSample * v->panL);
                    mixBufferR[j] += (tempSample * v->panR);
                }
            }
        }

        if (mutedVol != -1.0f)
            v->vol = mutedVol;
    }

    for (j = 0; j < numSamples; ++j)
    {
        out[0] = mixBufferL[j];
        out[1] = mixBufferR[j];

        if (filterFlags & FILTER_LP_ENABLED)
            lossyIntegrator(&filterLo, out, out);

        if (filterFlags & FILTER_LED_ENABLED)
            lossyIntegratorLED(filterLEDC, &filterLED, out, out);

        lossyIntegratorHighPass(&filterHi, out, out);

        out[0] *= (-32767.0f / 3.2f); // negative because signal phase is flipped on A500/A1200 for some reason
        out[1] *= (-32767.0f / 3.2f); // ---
        out[0]  = CLAMP(out[0], -32768.0f, 32767.0f);
        out[1]  = CLAMP(out[1], -32768.0f, 32767.0f);

        if (bigEndian)
        {
            renderL = SWAP16((int16_t)(out[0]));
            renderR = SWAP16((int16_t)(out[1]));
        }
        else
        {
            renderL = (int16_t)(out[0]);
            renderR = (int16_t)(out[1]);
        }

        *ptr++ = renderL;
        *ptr++ = renderR;
    }

    fwrite(editor.mod2WavBuffer, sizeof (int16_t), 2 * numSamples, fOut);
}

int32_t mod2WavThreadFunc(void *ptr)
{
    int32_t renderSampleBlock, renderSamplesTodo;
    uint32_t totalSampleCounter, totalTickCounter, totalRiffChunkLen;
    FILE *fOut;
    wavHeader_t wavHeader;

    fOut = (FILE *)(ptr);
    if (fOut == NULL)
        return (1);

    // skip wav header place, render data first
    fseek(fOut, sizeof (wavHeader_t), SEEK_SET);

    totalTickCounter    = 0;
    totalSampleCounter  = 0;
    renderSampleCounter = 0;

    while (editor.songPlaying && editor.isWAVRendering && !editor.abortMod2Wav)
    {
        renderSampleBlock = editor.outputFreq / 50;
        while (renderSampleBlock)
        {
            renderSamplesTodo = (renderSampleBlock < renderSampleCounter) ? renderSampleBlock : renderSampleCounter;
            if (renderSamplesTodo > 0)
            {
                outputAudioToFile(fOut, renderSamplesTodo);

                renderSampleBlock   -= renderSamplesTodo;
                renderSampleCounter -= renderSamplesTodo;
                totalSampleCounter  += renderSamplesTodo;
            }
            else
            {
                if (!processTick())
                {
                    editor.songPlaying = false;
                    break;
                }

                totalTickCounter = (totalTickCounter + 1) % 128;
                if (totalTickCounter == 0)
                    editor.ui.updateMod2WavDialog = true;

                renderSampleCounter = samplesPerTick;
            }
        }
    }

    if (totalSampleCounter & 1)
        fputc(0, fOut); // pad align byte

    totalRiffChunkLen = ftell(fOut) - 8;

    editor.ui.mod2WavFinished     = true;
    editor.ui.updateMod2WavDialog = true;

    // go back and fill the missing WAV header
    fseek(fOut, 0, SEEK_SET);

    wavHeader.chunkID       = bigEndian ? SWAP32(0x46464952) : 0x46464952; // "RIFF"
    wavHeader.chunkSize     = bigEndian ? SWAP32(totalRiffChunkLen) : (totalRiffChunkLen);
    wavHeader.format        = bigEndian ? SWAP32(0x45564157) : 0x45564157; // "WAVE"
    wavHeader.subchunk1ID   = bigEndian ? SWAP32(0x20746D66) : 0x20746D66; // "fmt "
    wavHeader.subchunk1Size = bigEndian ? SWAP32(16) : 16;
    wavHeader.audioFormat   = bigEndian ? SWAP16(1) : 1;
    wavHeader.numChannels   = bigEndian ? SWAP16(2) : 2;
    wavHeader.sampleRate    = bigEndian ? SWAP32(editor.outputFreq) : editor.outputFreq;
    wavHeader.bitsPerSample = bigEndian ? SWAP16(16) : 16;
    wavHeader.byteRate      = bigEndian ? SWAP32(wavHeader.sampleRate * wavHeader.numChannels * wavHeader.bitsPerSample / 8)
                            : (wavHeader.sampleRate * wavHeader.numChannels * wavHeader.bitsPerSample / 8);
    wavHeader.blockAlign    = bigEndian ? SWAP16(wavHeader.numChannels * wavHeader.bitsPerSample / 8)
                            : (wavHeader.numChannels * wavHeader.bitsPerSample / 8);
    wavHeader.subchunk2ID   = bigEndian ? SWAP32(0x61746164) : 0x61746164; // "data"
    wavHeader.subchunk2Size = bigEndian ? SWAP32(totalSampleCounter *  4) : (totalSampleCounter *  4); // 16-bit stereo = * 4

    fwrite(&wavHeader, sizeof (wavHeader_t), 1, fOut);
    fclose(fOut);

    return (1);
}

int8_t renderToWav(char *fileName, int8_t checkIfFileExist)
{
    FILE *fOut;
    struct stat statBuffer;

    if (checkIfFileExist)
    {
        if (stat(fileName, &statBuffer) == 0)
        {
            editor.ui.askScreenShown = true;
            editor.ui.askScreenType  = ASK_MOD2WAV_OVERWRITE;

            pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
            setStatusMessage("OVERWRITE FILE?", NO_CARRY);

            renderAskDialog();

            return (false);
        }
    }

    if (editor.ui.askScreenShown)
    {
        editor.ui.askScreenShown = false;

        editor.ui.answerNo  = false;
        editor.ui.answerYes = false;
    }

    fOut = fopen(fileName, "wb");
    if (fOut == NULL)
    {
        displayErrorMsg("FILE I/O ERROR");
        terminalPrintf("MOD2WAV failed: file input/output error\n");

        return (false);
    }

    storeTempVariables();
    calcMod2WavTotalRows();
    restartSong();

    terminalPrintf("MOD2WAV started (rows to render: %d)\n", modEntry->rowsInTotal);

    editor.blockMarkFlag = false;

    pointerSetMode(POINTER_MODE_READ_DIR, NO_CARRY);
    setStatusMessage("RENDERING MOD...", NO_CARRY);

    editor.ui.disableVisualizer = true;
    editor.isWAVRendering = true;
    renderMOD2WAVDialog();

    editor.abortMod2Wav = false;
    editor.mod2WavThread = SDL_CreateThread(mod2WavThreadFunc, "mod2wav ProTracker thread", fOut);

    return (true);
}

// for MOD2WAV
void calcMod2WavTotalRows(void)
{
    int8_t pattLoopCounter[AMIGA_VOICES], patternLoopRow[AMIGA_VOICES];
    uint8_t modRow, pBreakFlag, posJumpAssert, pBreakPosition;
    uint8_t calcingRows, ch, pos, bxxSamePosFlag;
    int16_t modOrder;
    uint16_t modPattern;
    note_t *note;

    memset(pattLoopCounter, 0, sizeof (pattLoopCounter));
    memset(patternLoopRow,  0, sizeof (patternLoopRow));

    modEntry->rowsCounter = 0;
    modEntry->rowsInTotal = 0;

    modRow     = 0;
    modOrder   = 0;
    modPattern = modEntry->head.order[0];

    pBreakPosition = 0;
    posJumpAssert  = false;
    pBreakFlag     = false;
    calcingRows    = true;

    memset(editor.rowVisitTable, 0, MOD_ORDERS * MOD_ROWS);
    while (calcingRows)
    {
        if (modOrder < 0)
            editor.rowVisitTable[((modEntry->head.orderCount - 1) * MOD_ROWS) + modRow] = true;
        else
            editor.rowVisitTable[(modOrder * MOD_ROWS) + modRow] = true;

        bxxSamePosFlag = false;
        for (ch = 0; ch < AMIGA_VOICES; ++ch)
        {
            note = &modEntry->patterns[modPattern][(modRow * AMIGA_VOICES) + ch];

            // Bxx - Position Jump
            if (note->command == 0x0B)
            {
                if (note->param == modOrder)
                    bxxSamePosFlag = true;

                modOrder = note->param - 1;

                pBreakPosition = 0;
                posJumpAssert  = true;
                pBreakFlag     = true;
            }

            // Dxx - Pattern Break
            else if (note->command == 0x0D)
            {
                pos = (((note->param >> 4) * 10) + (note->param & 0x0F));

                if (pos > 63)
                    pBreakPosition = 0;
                else
                    pBreakPosition = pos;

                posJumpAssert = true;
                pBreakFlag    = true;
            }

            // Fxx - Set Speed
            else if (note->command == 0x0F)
            {
                if (note->param == 0)
                {
                    calcingRows = false;
                    break;
                }
            }

            // E6x - Pattern Loop
            else if ((note->command == 0x0E) && ((note->param >> 4) == 0x06))
            {
                pos = note->param & 0x0F;
                if (pos == 0)
                {
                    patternLoopRow[ch] = modRow;
                }
                else
                {
                    // this is so ugly
                    if (pattLoopCounter[ch] == 0)
                    {
                        pattLoopCounter[ch] = pos;

                        pBreakPosition = patternLoopRow[ch];
                        pBreakFlag     = true;

                        for (pos = pBreakPosition; pos <= modRow; ++pos)
                            editor.rowVisitTable[(modOrder * MOD_ROWS) + pos] = false;
                    }
                    else
                    {
                        if (--pattLoopCounter[ch] != 0)
                        {
                            pBreakPosition = patternLoopRow[ch];
                            pBreakFlag     = true;

                            for (pos = pBreakPosition; pos <= modRow; ++pos)
                                editor.rowVisitTable[(modOrder * MOD_ROWS) + pos] = false;
                        }
                    }
                }
            }
        }

        modRow++;
        modEntry->rowsInTotal++;

        if (posJumpAssert && pBreakFlag && bxxSamePosFlag && (pBreakPosition == (modRow - 1)))
        {
            // quirky way used to "stop" some MODs (f.ex. testlast.mod)
            calcingRows = false;
            break;
        }

        if (pBreakFlag)
        {
            pBreakFlag = false;

            modRow = pBreakPosition;
            pBreakPosition = 0;
        }

        if ((modRow >= MOD_ROWS) || posJumpAssert)
        {
            modRow = pBreakPosition;

            posJumpAssert  = false;
            pBreakPosition = 0;

            if (++modOrder >= modEntry->head.orderCount)
            {
                calcingRows = false;
                break;
            }

            modPattern = modEntry->head.order[modOrder];
            if (modPattern > (MAX_PATTERNS - 1))
                modPattern =  MAX_PATTERNS - 1;
        }

        if (editor.rowVisitTable[(modOrder * MOD_ROWS) + modRow])
        {
            // row has been visited before, we're now done!
            calcingRows = false;
            break;
        }
    }
}

int8_t quantizeFloatTo8bit(float smpFloat)
{
    smpFloat = CLAMP(smpFloat, -128.0f, 127.0f);
    return (int8_t)(smpFloat);
}

int8_t quantize32bitTo8bit(int32_t smp32)
{
    double smp_d;

    smp_d = smp32 / 16777216.0;
    smp_d = ROUND_SMP_D(smp_d);
    smp_d = CLAMP(smp_d, -128.0, 127.0);

    return (int8_t)(smp_d);
}

int8_t quantize24bitTo8bit(int32_t smp32)
{
    double smp_d;

    smp_d = smp32 / 65536.0;
    smp_d = ROUND_SMP_D(smp_d);
    smp_d = CLAMP(smp_d, -128.0, 127.0);

    return (int8_t)(smp_d);
}

int8_t quantize16bitTo8bit(int16_t smp16)
{
    double smp_d;

    smp_d = smp16 / 256.0;
    smp_d = ROUND_SMP_D(smp_d);
    smp_d = CLAMP(smp_d, -128.0, 127.0);

    return (int8_t)(smp_d);
}

void normalize32bitSigned(int32_t *sampleData, uint32_t sampleLength)
{
    uint32_t sample, sampleVolPeak, i;
    double gain;

    sampleVolPeak = 0;
    for (i = 0; i < sampleLength; ++i)
    {
        sample = ABS(sampleData[i]);
        if (sampleVolPeak < sample)
            sampleVolPeak = sample;
    }

    // prevent division by zero!
    if (sampleVolPeak <= 0)
        sampleVolPeak  = 1;

    gain = ((4294967296.0 / 2.0) - 1.0) / (double)(sampleVolPeak);
    for (i = 0; i < sampleLength; ++i)
        sampleData[i] = (int32_t)(sampleData[i] * gain);
}

void normalize24bitSigned(int32_t *sampleData, uint32_t sampleLength)
{
    uint32_t sample, sampleVolPeak, i;
    double gain;

    sampleVolPeak = 0;
    for (i = 0; i < sampleLength; ++i)
    {
        sample = ABS(sampleData[i]);
        if (sampleVolPeak < sample)
            sampleVolPeak = sample;
    }

    // prevent division by zero!
    if (sampleVolPeak <= 0)
        sampleVolPeak  = 1;

    gain = ((16777216.0 / 2.0) - 1.0) / (double)(sampleVolPeak);
    for (i = 0; i < sampleLength; ++i)
        sampleData[i] = (int32_t)(sampleData[i] * gain);
}

void normalize16bitSigned(int16_t *sampleData, uint32_t sampleLength)
{
    uint32_t sample, sampleVolPeak, i;
    float gain;

    sampleVolPeak = 0;
    for (i = 0; i < sampleLength; ++i)
    {
        sample = ABS(sampleData[i]);
        if (sampleVolPeak < sample)
            sampleVolPeak = sample;
    }

    // prevent division by zero!
    if (sampleVolPeak <= 0)
        sampleVolPeak  = 1;

    gain = ((65536.0f / 2.0f) - 1.0f) / (float)(sampleVolPeak);
    for (i = 0; i < sampleLength; ++i)
        sampleData[i] = (int16_t)(sampleData[i] * gain);
}

void normalize8bitFloatSigned(float *sampleData, uint32_t sampleLength)
{
    uint32_t i;
    float sample, sampleVolPeak, gain;

    sampleVolPeak = 0.0f;
    for (i = 0; i < sampleLength; ++i)
    {
        sample = fabsf(sampleData[i]);
        if (sampleVolPeak < sample)
            sampleVolPeak = sample;
    }

    if (sampleVolPeak > 0.0f)
    {
        gain = ((256.0f / 2.0f) - 1.0f) / sampleVolPeak;
        for (i = 0; i < sampleLength; ++i)
            sampleData[i] *= gain;
    }
}
