#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "pt_header.h"
#include "pt_audio.h"
#include "pt_helpers.h"
#include "pt_palette.h"
#include "pt_tables.h"
#include "pt_modloader.h"
#include "pt_config.h"
#include "pt_sampler.h"
#include "pt_visuals.h"
#include "pt_textout.h"
#include "pt_terminal.h"

extern int8_t forceMixerOff; // pt_audio.c

static int8_t tempVolume, pattBreakFlag, pattDelayFlag, pattBreakBugPos;
static int8_t forceEffectsOff, oldRow;
static uint8_t tempFlags, pBreakFlag, posJumpAssert, pattDelayTime;
static uint8_t pattDelayTime2, pBreakPosition, modHasBeenPlayed, oldSpeed;
static int16_t modOrder, modPattern, oldPattern, oldOrder;
static uint16_t tempPeriod, modBPM, oldBPM;

// Normal effects
static void fxArpeggio(moduleChannel_t *ch);
static void fxPitchSlideUp(moduleChannel_t *ch);
static void fxPitchSlideDown(moduleChannel_t *ch);
static void fxGlissando(moduleChannel_t *ch);
static void fxVibrato(moduleChannel_t *ch);
static void fxGlissandoVolumeSlide(moduleChannel_t *ch);
static void fxVibratoVolumeSlide(moduleChannel_t *ch);
static void fxTremolo(moduleChannel_t *ch);
static void fxNotInUse(moduleChannel_t *ch);
static void fxSampleOffset(moduleChannel_t *ch);
static void fxVolumeSlide(moduleChannel_t *ch);
static void fxPositionJump(moduleChannel_t *ch);
static void fxSetVolume(moduleChannel_t *ch);
static void fxPatternBreak(moduleChannel_t *ch);
static void fxExtended(moduleChannel_t *ch);
static void fxSetTempo(moduleChannel_t *ch);

// Extended effects
static void efxSetLEDFilter(moduleChannel_t *ch);
static void efxFinePortamentoSlideUp(moduleChannel_t *ch);
static void efxFinePortamentoSlideDown(moduleChannel_t *ch);
static void efxGlissandoControl(moduleChannel_t *ch);
static void efxVibratoControl(moduleChannel_t *ch);
static void efxSetFineTune(moduleChannel_t *ch);
static void efxPatternLoop(moduleChannel_t *ch);
static void efxTremoloControl(moduleChannel_t *ch);
static void efxKarplusStrong(moduleChannel_t *ch);
static void efxRetrigNote(moduleChannel_t *ch);
static void efxFineVolumeSlideUp(moduleChannel_t *ch);
static void efxFineVolumeSlideDown(moduleChannel_t *ch);
static void efxNoteCut(moduleChannel_t *ch);
static void efxNoteDelay(moduleChannel_t *ch);
static void efxPatternDelay(moduleChannel_t *ch);
static void efxInvertLoop(moduleChannel_t *ch);

typedef void (*effectRoutine_t)(moduleChannel_t *);

// Normal effects
static effectRoutine_t fxRoutines[16] =
{
    fxArpeggio,
    fxPitchSlideUp,
    fxPitchSlideDown,
    fxGlissando,
    fxVibrato,
    fxGlissandoVolumeSlide,
    fxVibratoVolumeSlide,
    fxTremolo,
    fxNotInUse,
    fxSampleOffset,
    fxVolumeSlide,
    fxPositionJump,
    fxSetVolume,
    fxPatternBreak,
    fxExtended,
    fxSetTempo
};

// Extended effects
static effectRoutine_t efxRoutines[16] =
{
    efxSetLEDFilter,
    efxFinePortamentoSlideUp,
    efxFinePortamentoSlideDown,
    efxGlissandoControl,
    efxVibratoControl,
    efxSetFineTune,
    efxPatternLoop,
    efxTremoloControl,
    efxKarplusStrong,
    efxRetrigNote,
    efxFineVolumeSlideUp,
    efxFineVolumeSlideDown,
    efxNoteCut,
    efxNoteDelay,
    efxPatternDelay,
    efxInvertLoop
};

void storeTempVariables(void) // this one is accessed in other files, so non-static
{
    oldBPM     = modEntry->currBPM;
    oldRow     = modEntry->currRow;
    oldOrder   = modEntry->currOrder;
    oldSpeed   = modEntry->currSpeed;
    oldPattern = modEntry->currPattern;
}

static void processInvertLoop(moduleChannel_t *ch)
{
    if (ch->invertLoopSpeed > 0)
    {
        ch->invertLoopDelay += funkTable[ch->invertLoopSpeed];
        if (ch->invertLoopDelay >= 128)
        {
            ch->invertLoopDelay = 0;

            if (ch->invertLoopPtr != NULL) // safety check that wasn't present in original PT
            {
                ch->invertLoopPtr++;
                if (ch->invertLoopPtr >= (ch->invertLoopStart + ch->invertLoopLength))
                    ch->invertLoopPtr  =  ch->invertLoopStart;

                *ch->invertLoopPtr = -1 - *ch->invertLoopPtr;
            }
        }
    }
}

static void efxSetLEDFilter(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
        setLEDFilter(!(ch->param & 1));
}

static void efxFinePortamentoSlideUp(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
    {
        if (tempPeriod > 0)
        {
            ch->period -= (ch->param & 0x0F);

            if ((ch->period & 0x0FFF) < 113)
            {
                ch->period &= 0xF000;
                ch->period |= 113;
            }

            tempPeriod = ch->period & 0x0FFF;
        }
    }
}

static void efxFinePortamentoSlideDown(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
    {
        if (tempPeriod > 0)
        {
            ch->period += (ch->param & 0x0F);

            if ((ch->period & 0x0FFF) > 856)
            {
                ch->period &= 0xF000;
                ch->period |= 856;
            }

            tempPeriod = ch->period & 0x0FFF;
        }
    }
}

static void efxGlissandoControl(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
        ch->glissandoControl = ch->param & 0x0F;
}

static void efxVibratoControl(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
        ch->waveControl = (ch->waveControl & 0xF0) | (ch->param & 0x0F);
}

static void efxSetFineTune(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
        ch->fineTune = ch->param & 0x0F;
}

static void efxPatternLoop(moduleChannel_t *ch)
{
    uint8_t tempParam;

    if (editor.modTick == 0)
    {
        tempParam = ch->param & 0x0F;
        if (tempParam == 0)
        {
            ch->patternLoopRow = modEntry->row;

            return;
        }

        if (ch->pattLoopCounter == 0)
        {
            ch->pattLoopCounter = tempParam;
        }
        else
        {
            if (--ch->pattLoopCounter == 0)
                return;
        }

        pBreakPosition = ch->patternLoopRow;
        pBreakFlag = true;

        if (editor.isWAVRendering)
        {
            for (tempParam = pBreakPosition; tempParam <= modEntry->row; ++tempParam)
                editor.rowVisitTable[(modOrder * MOD_ROWS) + tempParam] = false;
        }
    }
}

static void efxTremoloControl(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
        ch->waveControl = (ch->param << 4) | (ch->waveControl & 0x0F);
}

static void efxKarplusStrong(moduleChannel_t *ch)
{
    // Karplus-Strong, my enemy...
    // 1) Almost no one used it as Karplus-Strong
    // 2) It trashes sample data, and it's not even worth it (invertloop (EFx) is much cooler)
    // 3) Often used as effect syncing, so we don't want to destroy sample(s) when playing

    (void)(ch);
}

static void efxRetrigNote(moduleChannel_t *ch)
{
    uint8_t retrigTick;

    retrigTick = ch->param & 0x0F;
    if (retrigTick > 0)
    {
        if ((editor.modTick % retrigTick) == 0)
            tempFlags |= TEMPFLAG_START;
    }
}

static void efxFineVolumeSlideUp(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
    {
        ch->volume += (ch->param & 0x0F);
        if (ch->volume > 64)
            ch->volume = 64;

        tempVolume = ch->volume;
    }
}

static void efxFineVolumeSlideDown(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
    {
        ch->volume -= (ch->param & 0x0F);
        if (ch->volume < 0)
            ch->volume = 0;

        tempVolume = ch->volume;
    }
}

static void efxNoteCut(moduleChannel_t *ch)
{
    if (editor.modTick == (ch->param & 0x0F))
    {
        ch->volume = 0;
        tempVolume = 0;
        ch->scopeVolume = volumeToScopeVolume(0);
    }
}

static void efxNoteDelay(moduleChannel_t *ch)
{
    uint8_t delayTick;

    delayTick = ch->param & 0x0F;

    if (editor.modTick == 0)
        ch->tempFlagsBackup = tempFlags;

         if (editor.modTick  < delayTick) tempFlags = TEMPFLAG_DELAY;
    else if (editor.modTick == delayTick) tempFlags = ch->tempFlagsBackup;
}

static void efxPatternDelay(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
    {
        if (pattDelayTime2 == 0)
        {
            pattDelayFlag = true;
            pattDelayTime = (ch->param & 0x0F) + 1;
        }
    }
}

static void efxInvertLoop(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
    {
        ch->invertLoopSpeed = ch->param & 0x0F;

        if (ch->invertLoopSpeed > 0)
            processInvertLoop(ch);
    }
}

void setTonePorta(moduleChannel_t *ch, uint16_t period)
{
    uint8_t i;
    uint16_t *tablePointer;

    tablePointer = &periodTable[37 * ch->fineTune];

    i = 0;
    for (;;)
    {
        if (period >= tablePointer[i])
            break;

        if (++i >= 37)
        {
            i = 35;
            break;
        }
    }

    if ((ch->fineTune & 8) && i) i--; // not sure why PT does this...

    ch->wantedperiod  = tablePointer[i];
    ch->tonePortDirec = 0;

         if (ch->period == ch->wantedperiod) ch->wantedperiod  = 0; // don't do any more slides
    else if (ch->period >  ch->wantedperiod) ch->tonePortDirec = 1;
}

static void processGlissando(moduleChannel_t *ch)
{
    uint8_t i;
    uint16_t *tablePointer;

    if (tempPeriod == 0)
    {
        ch->wantedperiod = 0; // don't do any more sliding
        return;
    }

    if (ch->wantedperiod > 0)
    {
        if (ch->tonePortDirec == 0)
        {
            ch->period += ch->tonePortSpeed;
            if ((signed)(int16_t)(ch->period) >= ch->wantedperiod)
            {
                ch->period = ch->wantedperiod;
                ch->wantedperiod = 0; // don't do any more sliding
            }
        }
        else
        {
            ch->period -= ch->tonePortSpeed;
            if ((signed)(int16_t)(ch->period) <= ch->wantedperiod)
            {
                ch->period = ch->wantedperiod;
                ch->wantedperiod = 0; // don't do any more sliding
            }
        }

        if (tempPeriod > 0)
        {
            if (ch->glissandoControl == 0)
            {
                // smooth sliding (default)
                tempPeriod = ch->period;
            }
            else 
            {
                // semitone sliding
                tablePointer = &periodTable[37 * ch->fineTune];

                i = 0;
                for (;;)
                {
                    if (ch->period >= tablePointer[i])
                        break;

                    if (++i >= 37)
                    {
                        i = 35;
                        break;
                    }
                }

                tempPeriod = tablePointer[i];
            }
        }
    }
}

static void processVibrato(moduleChannel_t *ch)
{
    int16_t vibratoData;

    if (tempPeriod > 0)
    {
        switch (ch->waveControl & 3)
        {
            // sine (0)
            case 0: vibratoData = vibratoTable[ch->vibratoPos & 31]; break;

            // ramp (1)
            case 1:
            {
                if (ch->vibratoPos < 32)
                    vibratoData = 255 - ((ch->vibratoPos & 31) * 8);
                else
                    vibratoData = (ch->vibratoPos & 31) * 8;
            }
            break;

            // square (2,3)   3 is NOT "random mode" in PT, it's the same as square
            default: vibratoData = 255; break;
        }

        vibratoData = (vibratoData * (ch->vibratoCmd & 0x0F)) / 128;

        // ProTracker doesn't clamp the period here! Big mistake...
        if (ch->vibratoPos < 32)
            tempPeriod += vibratoData; // max. end result period (C-1 finetune -8): 936
        else
            tempPeriod -= vibratoData; // min. end result period (B-3 finetune +7): 79

        ch->vibratoPos += (ch->vibratoCmd >> 4);
        ch->vibratoPos &= 63;

        // for quadrascope
        ch->scopeKeepDelta = false;
        periodToScopeDelta(ch, tempPeriod);
    }
}

static void processTremolo(moduleChannel_t *ch)
{
    uint8_t tremoloData;

    switch ((ch->waveControl >> 4) & 3)
    {
        // sine (0)
        case 0: tremoloData = vibratoTable[ch->tremoloPos & 31]; break;

        // ramp (1)
        case 1:
        {
            if (ch->vibratoPos < 32) // ProTracker bug (should've been tremoloPos)
                tremoloData = 255 - ((ch->tremoloPos & 31) * 8);
            else
                tremoloData = (ch->tremoloPos & 31) * 8;
        }
        break;

        // square (2,3)   3 is NOT "random mode" in PT, it's the same as square
        default: tremoloData = 255; break;
    }

    tremoloData = (tremoloData * (ch->tremoloCmd & 0x0F)) / 64;

    if (ch->tremoloPos < 32)
    {
        tempVolume += tremoloData;
        if (tempVolume > 64)
            tempVolume = 64;
    }
    else
    {
        tempVolume -= tremoloData;
        if (tempVolume < 0)
            tempVolume = 0;
    }

    ch->tremoloPos += (ch->tremoloCmd >> 4);
    ch->tremoloPos &= 63;

    // for quadrascope
    ch->scopeKeepVolume = false;
    ch->scopeVolume = volumeToScopeVolume(tempVolume);
}

static void fxArpeggio(moduleChannel_t *ch)
{
    uint8_t i, noteToAdd, arpeggioTick;
    uint16_t *tablePointer;

    if (tempPeriod > 0)
    {
        noteToAdd    = 0;
        arpeggioTick = editor.modTick % 3;

        if (arpeggioTick == 0)
        {
            tempPeriod = ch->period;
            return;
        }
        else if (arpeggioTick == 1)
        {
            noteToAdd = ch->param >> 4;
        }
        else if (arpeggioTick == 2)
        {
            noteToAdd = ch->param & 0x0F;
        }

        tablePointer = &periodTable[37 * ch->fineTune];
        for (i = 0; i < 36; ++i)
        {
            if (ch->period >= tablePointer[i])
            {
                tempPeriod = tablePointer[i + noteToAdd];

                // for quadrascope
                ch->scopeKeepDelta = false;
                periodToScopeDelta(ch, tempPeriod);

                break;
            }
        }
    }
}

static void fxPitchSlideUp(moduleChannel_t *ch)
{
    if ((editor.modTick > 0) || (pattDelayTime2 > 0)) // all ticks while EDx (weird)
    {
        if (tempPeriod > 0)
        {
            // This has a PT bug where the period overflows,
            // resulting in extremely high periods.

            ch->period -= ch->param;

            if ((ch->period & 0x0FFF) < 113)
            {
                ch->period &= 0xF000;
                ch->period |= 113;
            }

            tempPeriod = ch->period & 0x0FFF;
        }
    }
}

static void fxPitchSlideDown(moduleChannel_t *ch)
{
    if ((editor.modTick > 0) || (pattDelayTime2 > 0)) // all ticks while EDx (weird)
    {
        if (tempPeriod > 0)
        {
            ch->period += ch->param;

            if ((ch->period & 0x0FFF) > 856)
            {
                ch->period &= 0xF000;
                ch->period |= 856;
            }

            tempPeriod = ch->period & 0x0FFF;
        }
    }
}

static void fxGlissando(moduleChannel_t *ch)
{
    if ((editor.modTick > 0) || (pattDelayTime2 > 0)) // all ticks while EDx (weird)
    {
        if (ch->param != 0)
        {
            ch->tonePortSpeed = ch->param;
            ch->param = 0;
        }

        processGlissando(ch);
    }
}

static void fxVibrato(moduleChannel_t *ch)
{
    if ((editor.modTick > 0) || (pattDelayTime2 > 0)) // all ticks while EDx (weird)
    {
        if (ch->param != 0)
        {
            if (ch->param & 0x0F) ch->vibratoCmd = (ch->vibratoCmd & 0xF0) | (ch->param & 0x0F);
            if (ch->param & 0xF0) ch->vibratoCmd = (ch->param & 0xF0) | (ch->vibratoCmd & 0x0F);
        }

        processVibrato(ch);
    }
}

static void fxGlissandoVolumeSlide(moduleChannel_t *ch)
{
    if ((editor.modTick > 0) || (pattDelayTime2 > 0)) // all ticks while EDx (weird)
    {
        processGlissando(ch);
        fxVolumeSlide(ch);
    }
}

static void fxVibratoVolumeSlide(moduleChannel_t *ch)
{
    if ((editor.modTick > 0) || (pattDelayTime2 > 0)) // all ticks while EDx (weird)
    {
        processVibrato(ch);
        fxVolumeSlide(ch);
    }
}

static void fxTremolo(moduleChannel_t *ch)
{
    if ((editor.modTick > 0) || (pattDelayTime2 > 0)) // all ticks while EDx (weird)
    {
        if (ch->param != 0)
        {
            if (ch->param & 0x0F) ch->tremoloCmd = (ch->tremoloCmd & 0xF0) | (ch->param & 0x0F);
            if (ch->param & 0xF0) ch->tremoloCmd = (ch->param & 0xF0) | (ch->tremoloCmd & 0x0F);
        }

        processTremolo(ch);
    }
}

static void fxNotInUse(moduleChannel_t *ch)
{
    // This is an empty effect slot not used in
    // ProTracker... often used for demo efx syncing
    //
    // Used for panning in Fasttracker and other .MOD trackers

    (void)(ch);
}

static void fxSampleOffset(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
    {
        if (ch->param > 0)
            ch->offsetTemp = 256 * ch->param;

        ch->offset += ch->offsetTemp;

        if (!ch->noNote)
            ch->offsetBugNotAdded = false;
    }
}

static void fxVolumeSlide(moduleChannel_t *ch)
{
    uint8_t hiNybble, loNybble;

    if ((editor.modTick > 0) || (pattDelayTime2 > 0)) // all ticks while EDx (weird)
    {
        hiNybble = ch->param >> 4;
        loNybble = ch->param & 0x0F;

        if (hiNybble == 0)
        {
            ch->volume -= loNybble;
            if (ch->volume < 0)
                ch->volume = 0;
        }
        else
        {
            ch->volume += hiNybble;
            if (ch->volume > 64)
                ch->volume = 64;
        }

        tempVolume = ch->volume;
    }
}

static void fxPositionJump(moduleChannel_t *ch)
{
    if (editor.isSMPRendering)
    {
        modHasBeenPlayed = true;
    }
    else
    {
        if (editor.modTick == 0)
        {
            if (editor.isWAVRendering)
            {
                modOrder = ch->param - 1;
            }
            else
            {
                if (editor.playMode != PLAY_MODE_PATTERN)
                {
                    modOrder = (ch->param & 0x7F) - 1;
                    if (modOrder == -1)
                        modOrder = modEntry->head.orderCount - 1;
                }
            }

            pBreakPosition  = 0;
            pattBreakBugPos = 0;

            pattBreakFlag = true;
            posJumpAssert = true;
        }
    }
}

static void fxSetVolume(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
    {
        if (ch->param > 64)
            ch->param = 64;

        ch->volume = ch->param;
        tempVolume = ch->param;
    }
}

static void fxPatternBreak(moduleChannel_t *ch)
{
    uint8_t pos;

    if (editor.isSMPRendering)
    {
        modHasBeenPlayed = true;
    }
    else
    {
        if (editor.modTick == 0)
        {
            pos = ((ch->param >> 4) * 10) + (ch->param & 0x0F);

            pBreakPosition  = (pos <= 63) ? pos : 0;
            pattBreakBugPos = pBreakPosition;

            pattBreakFlag = true;
            posJumpAssert = true;
        }
    }
}

static void fxExtended(moduleChannel_t *ch)
{
    efxRoutines[ch->param >> 4](ch);
}

static void fxSetTempo(moduleChannel_t *ch)
{
    if (editor.modTick == 0)
    {
        if ((ch->param < 32) || (editor.timingMode == TEMPO_MODE_VBLANK))
        {
            if (ch->param == 0)
            {
                // stop song (literally)

                if (editor.isWAVRendering)
                {
                    modHasBeenPlayed = true;
                    return;
                }

                editor.songPlaying = false;

                editor.currMode = MODE_IDLE;
                pointerSetMode(POINTER_MODE_IDLE, DO_CARRY);
                setStatusMessage(editor.allRightText, DO_CARRY);
            }
            else
            {
                modSetSpeed(ch->param);
            }
        }
        else
        {
            modSetTempo(ch->param);
        }
    }
}

static void processEffects(moduleChannel_t *ch)
{
    if (editor.modTick > 0)
        processInvertLoop(ch);

    if ((!ch->command && !ch->param) == 0)
        fxRoutines[ch->command](ch);
}

static void fetchPatternData(moduleChannel_t *ch)
{
    int8_t tempNote;
    uint8_t i;
    int16_t tempPer;
    note_t *note;

    note = &modEntry->patterns[modPattern][(modEntry->row * AMIGA_VOICES) + ch->chanIndex];
    if ((note->sample >= 1) && (note->sample <= 31))
    {
        ch->flags |= FLAG_SAMPLE;

        if (ch->sample != note->sample)
            ch->flags  |= FLAG_NEWSAMPLE;

        ch->sample   = note->sample;
        ch->fineTune = modEntry->samples[ch->sample - 1].fineTune;
    }

    ch->command = note->command;
    ch->param   = note->param;

    if ((note->period & 0x0FFF) > 0)
    {
        ch->flags |= FLAG_NOTE;
        ch->noNote = false;

        // set finetune if E5x is called next to a note
        if (ch->command == 0x0E)
        {
            if ((ch->param >> 4) == 0x05)
                ch->fineTune = ch->param & 0x0F;
        }

        // setup 3xx/5xx (portamento)
        if ((ch->command == 0x03) || (ch->command == 0x05))
            setTonePorta(ch, note->period);

        tempPer = note->period & 0x0FFF;
        for (i = 0; i < 37; ++i)
        {
            // periodTable[36] = 0, so i=36 is safe
            if (tempPer >= periodTable[i])
                break;
        }

        // BUG: yes it's 'safe' if i=37 because of padding at the end of the period table
        ch->tempPeriod = periodTable[(37 * ch->fineTune) + i];
    }
    else
    {
        ch->noNote = true;
    }

    // handle the metronome (if active)
    if (editor.metroFlag && (editor.metroChannel > 0))
    {
        if ((ch->chanIndex == (editor.metroChannel - 1)) && ((modEntry->row % editor.metroSpeed) == 0))
        {
            ch->flags |= FLAG_SAMPLE;
            ch->flags |= FLAG_NOTE;
            ch->noNote = false;

            ch->sample   = 0x1F; // the metronome always uses the very last sample slot
            ch->fineTune = modEntry->samples[ch->sample - 1].fineTune;

            // set finetune if E5x is entered (this time even if no note was entered)
            if (ch->command == 0x0E)
            {
                if ((ch->param >> 4) == 0x05)
                    ch->fineTune = ch->param & 0x0F;
            }

            tempNote = (((modEntry->row / editor.metroSpeed) % editor.metroSpeed) == 0) ? 29 : 24;
            ch->tempPeriod = periodTable[(37 * ch->fineTune) + tempNote];
        }
    }
}

static void processChannel(moduleChannel_t *ch)
{
    uint8_t setVUMeters;
    moduleSample_t *s;

    tempFlags = 0;

    setVUMeters = false;
    if (editor.modTick == 0)
    {
        if (pattDelayTime2 == 0)
            fetchPatternData(ch);

        // In PT, the spectrum analyzer doesn't use the last volume affected
        // by volume effects. It just uses the last s->sample value for
        // the current channel.
        // So we use a separate variable for this, and update it before
        // we update the spectrum analyzer for correct visual results.
        if (ch->flags & FLAG_SAMPLE)
        {
            if (ch->sample != 0)
                ch->rawVolume = modEntry->samples[ch->sample - 1].volume;
        }

        if (ch->flags & FLAG_NOTE)
        {
            ch->flags &= ~FLAG_NOTE;

            setVUMeters = true;

            if ((ch->command != 0x03) && (ch->command != 0x05))
            {
                if (ch->sample != 0)
                    tempFlags |= TEMPFLAG_START;

                ch->period = ch->tempPeriod; // ranging 113..856 at this point

                // update spectrum analyzer if neeeded
                if (!editor.isWAVRendering && !editor.isSMPRendering &&
                    (editor.ui.visualizerMode == VISUAL_SPECTRUM) &&
                    !editor.muted[ch->chanIndex])
                {
                    updateSpectrumAnalyzer(ch->period, ch->rawVolume);
                }
            }

            ch->tempFlagsBackup = 0;

            if ((ch->waveControl & 0x04) == 0) ch->vibratoPos = 0;
            if ((ch->waveControl & 0x40) == 0) ch->tremoloPos = 0;
        }

        if (ch->flags & FLAG_SAMPLE)
        {
            ch->flags &= ~FLAG_SAMPLE;

            if (ch->sample != 0)
            {
                s = &modEntry->samples[ch->sample - 1];

                setVUMeters = true;

                ch->volume = s->volume;

                ch->invertLoopPtr    = &modEntry->sampleData[s->offset + s->loopStart];
                ch->invertLoopStart  = ch->invertLoopPtr;
                ch->invertLoopLength = s->loopLength;

                if ((ch->command != 0x03) && (ch->command != 0x05))
                {
                    ch->offset = 0;
                    ch->offsetBugNotAdded = true;
                }

                if (ch->flags & FLAG_NEWSAMPLE)
                {
                    ch->flags &= ~FLAG_NEWSAMPLE;

                    // sample swap (even during note delays... makes sense, doesn't it?)
                    if (ch->sample != 0)
                    {
                        s = &modEntry->samples[ch->sample - 1];
                        mixerSwapVoiceSource(ch->chanIndex, modEntry->sampleData + s->offset, s->length, s->loopStart, s->loopLength, s->offset);
                    }
                }
            }
        }
    }

    tempPeriod = ch->period;
    tempVolume = ch->volume;

    ch->scopeKeepDelta  = true;
    ch->scopeKeepVolume = true;

    if (!forceEffectsOff)
        processEffects(ch);

    if ((editor.modTick == 0) && setVUMeters)
    {
        if (!editor.isWAVRendering && !editor.isSMPRendering && !editor.muted[ch->chanIndex])
            editor.vuMeterVolumes[ch->chanIndex] = vuMeterHeights[tempVolume];
    }

    mixerSetVoiceVol(ch->chanIndex, tempVolume); // update volume even if note delay (PT behavior)

    if (!(tempFlags & TEMPFLAG_DELAY))
    {
        if (tempFlags & TEMPFLAG_START)
        {
            if (ch->sample != 0)
            {
                s = &modEntry->samples[ch->sample - 1];

                if (s->length > 0)
                {
                    if (!editor.isWAVRendering)
                        ch->scopeTrigger = true;

                    if (ch->offset > 0)
                    {
                        mixerSetVoiceSource(ch->chanIndex, modEntry->sampleData + s->offset, s->length, s->loopStart, s->loopLength, ch->offset);

                        if (!editor.isWAVRendering && !editor.isSMPRendering)
                        {
                            ch->scopeChangePos = true;
                            if (s->loopLength > 2)
                            {
                                if (ch->offset >= (s->loopStart + s->loopLength))
                                    ch->scopePos_f = s->offset + (s->loopStart + s->loopLength);
                                else
                                    ch->scopePos_f = s->offset + ch->offset;
                            }
                            else
                            {
                                if (ch->offset >= s->length)
                                {
                                    ch->scopeEnabled = false;
                                    ch->scopeTrigger = false;
                                }
                                else
                                {
                                    ch->scopePos_f = s->offset + ch->offset;
                                }
                            }

                            // PT bug: Sample len >65535 + 9xx efx = stop sample (scopes this time)
                            if ((s->length > 65535) && (ch->offset > 0))
                            {
                                ch->scopeEnabled = false;
                                ch->scopeTrigger = false;
                            }
                        }

                        if (!ch->offsetBugNotAdded)
                        {
                            ch->offset += ch->offsetTemp; // emulates add sample offset bug (fixes some quirky MODs)
                            if (ch->offset > 0xFFFF)
                                ch->offset = 0xFFFF;

                            ch->offsetBugNotAdded = true;
                        }
                    }
                    else
                    {
                        mixerSetVoiceSource(ch->chanIndex, modEntry->sampleData + s->offset, s->length, s->loopStart, s->loopLength, 0);
                    }
                }
                else
                {
                    ch->scopeEnabled = false;
                    ch->scopeTrigger = false;

                    mixerKillVoice(ch->chanIndex);
                }
            }
        }

        mixerSetVoiceDelta(ch->chanIndex, tempPeriod);
    }
}

void doStopIt(void)
{
    uint8_t i;

    pattDelayTime      = 0;
    pattDelayTime2     = 0;
    editor.playMode    = PLAY_MODE_NORMAL;
    editor.currMode    = MODE_IDLE;
    editor.songPlaying = false;

    pointerSetMode(POINTER_MODE_IDLE, DO_CARRY);

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        modEntry->channels[i].fineTune         = 0;
        modEntry->channels[i].waveControl      = 0;
        modEntry->channels[i].invertLoopSpeed  = 0;
        modEntry->channels[i].pattLoopCounter  = 0;
        modEntry->channels[i].glissandoControl = 0;
    }
}

void setPattern(int16_t pattern)
{
    modPattern = pattern;
    if (modPattern > (MAX_PATTERNS - 1))
        modPattern =  MAX_PATTERNS - 1;

    modEntry->currPattern = modPattern;
}

static void nextPosition(void)
{
    modEntry->row = pBreakPosition;
    pBreakPosition = 0;
    posJumpAssert  = false;

    if ((editor.playMode != PLAY_MODE_PATTERN) ||
        ((editor.currMode == MODE_RECORD) && (editor.recordMode != RECORD_PATT)))
    {
        if (editor.stepPlayEnabled)
        {
            doStopIt();

            editor.stepPlayEnabled   = false;
            editor.stepPlayBackwards = false;

            if (!editor.isWAVRendering && !editor.isSMPRendering)
                modEntry->currRow = modEntry->row;

            return;
        }

        if (++modOrder >= modEntry->head.orderCount)
        {
            modOrder = 0;
            modHasBeenPlayed = true;
        }

        modPattern = modEntry->head.order[modOrder];
        if (modPattern > (MAX_PATTERNS - 1))
            modPattern =  MAX_PATTERNS - 1;

        if (!editor.isWAVRendering && !editor.isSMPRendering)
        {
            modEntry->currPattern = modPattern;

            editor.currPatternDisp   = &modEntry->head.order[modOrder];
            editor.currPosEdPattDisp = &modEntry->head.order[modOrder];

            if (editor.ui.posEdScreenShown)
                editor.ui.updatePosEd = true;

            editor.ui.updateSongPos      = true;
            editor.ui.updateSongPattern  = true;
            editor.ui.updateCurrPattText = true;
        }
    }
}

int8_t processTick(void)
{
    if (!editor.songPlaying)
        return (false);

    if ((editor.isSMPRendering || editor.isWAVRendering) && modHasBeenPlayed)
    {
        modHasBeenPlayed = false;
        return (false);
    }

    // EEx + Dxx/Bxx quirk simulation
    if (editor.modTick == 0)
    {
        if (forceEffectsOff)
        {
            if (modEntry->row != pattBreakBugPos)
            {
                forceEffectsOff = false;
                pattBreakBugPos = -1;
            }
        }
    }

    if (editor.isWAVRendering)
    {
        if (editor.modTick == 0)
        {
            if (modOrder < 0)
                editor.rowVisitTable[((modEntry->head.orderCount - 1) * MOD_ROWS) + modEntry->row] = true;
            else
                editor.rowVisitTable[(modOrder * MOD_ROWS) + modEntry->row] = true;
        }
    }

    processChannel(modEntry->channels + 0);
    processChannel(modEntry->channels + 1);
    processChannel(modEntry->channels + 2);
    processChannel(modEntry->channels + 3);

    // EEx + Dxx/Bxx quirk simulation
    if (editor.modTick == 0)
    {
        if (pattBreakFlag && pattDelayFlag)
            forceEffectsOff = true;
    }

    if (!editor.stepPlayEnabled)
        editor.modTick++;

    if ((editor.modTick >= editor.modSpeed) || editor.stepPlayEnabled)
    {
        editor.modTick = 0;
        pattBreakFlag  = false;
        pattDelayFlag  = false;

        if (!editor.stepPlayBackwards)
        {
            modEntry->row++;
            modEntry->rowsCounter++;
        }

        if (pattDelayTime > 0)
        {
            pattDelayTime2 = pattDelayTime;
            pattDelayTime  = 0;
        }

        if (pattDelayTime2 > 0)
        {
            if (--pattDelayTime2 > 0)
                modEntry->row--;
        }

        if (pBreakFlag)
        {
            pBreakFlag = false;
            modEntry->row = pBreakPosition;
            pBreakPosition = 0;
        }

        if (editor.blockMarkFlag)
            editor.ui.updateStatusText = true;

        if (editor.stepPlayEnabled)
        {
            doStopIt();

            modEntry->currRow = modEntry->row & 0x3F;

            editor.stepPlayEnabled      = false;
            editor.stepPlayBackwards    = false;
            editor.ui.updatePatternData = true;

            return (true);
        }

        if ((modEntry->row >= MOD_ROWS) || posJumpAssert)
        {
            if (editor.isSMPRendering)
                return (false);

            nextPosition();
        }

        if (editor.isWAVRendering && !pattDelayTime2 && editor.rowVisitTable[(modOrder * MOD_ROWS) + modEntry->row])
        {
            modHasBeenPlayed = true;
            return (false);
        }

        if (!editor.isWAVRendering && !editor.isSMPRendering)
        {
            modEntry->currRow = modEntry->row;

            if (editor.playMode != PLAY_MODE_PATTERN)
            {
                modEntry->currOrder      = modOrder;
                editor.currPatternDisp   = &modEntry->head.order[modOrder];
                editor.currPosEdPattDisp = &modEntry->head.order[modOrder];
            }

            editor.ui.updatePatternData = true;
        }
    }

    return (true);
}

void modSetPos(int16_t order, int16_t row)
{
    uint8_t i;
    int16_t posEdPos;

    if (row != -1)
    {
        row = CLAMP(row, 0, 63);

        editor.modTick     = 0;
        modEntry->row     = (int8_t)(row);
        modEntry->currRow = (int8_t)(row);

        pBreakPosition = 0;
        posJumpAssert  = false;
    }

    if (order != -1)
    {
        if (order >= 0)
        {
            for (i = 0; i < AMIGA_VOICES; ++i)
            {
                modEntry->channels[i].patternLoopRow  = 0;
                modEntry->channels[i].pattLoopCounter = 0;
            }

            modOrder = order;
            modEntry->currOrder = order;

            editor.ui.updateSongPos = true;

            if ((editor.currMode == MODE_PLAY) && (editor.playMode == PLAY_MODE_NORMAL))
            {
                modPattern = modEntry->head.order[order];
                if (modPattern > (MAX_PATTERNS - 1))
                    modPattern =  MAX_PATTERNS - 1;

                modEntry->currPattern = modPattern;
                editor.ui.updateCurrPattText = true;

                pBreakPosition = 0;
                posJumpAssert  = false;
            }

            editor.ui.updateSongPattern = true;

            editor.currPatternDisp = &modEntry->head.order[modOrder];

            posEdPos = modEntry->currOrder;
            if (posEdPos > (modEntry->head.orderCount - 1))
                posEdPos =  modEntry->head.orderCount - 1;

            editor.currPosEdPattDisp = &modEntry->head.order[posEdPos];

            if (editor.ui.posEdScreenShown)
                editor.ui.updatePosEd = true;
        }
    }

    editor.ui.updatePatternData = true;

    if (editor.blockMarkFlag)
        editor.ui.updateStatusText = true;
}

void modSetSpeed(uint8_t speed)
{
    editor.modSpeed      = speed;
    modEntry->currSpeed = speed;

    editor.modTick = 0;
}

void modSetTempo(uint16_t bpm)
{
    uint16_t ciaVal;
    float f_hz, f_smp;

    if (bpm > 0)
    {
        modBPM = bpm;

        if (!editor.isSMPRendering && !editor.isWAVRendering)
        {
            modEntry->currBPM = bpm;
            editor.ui.updateSongBPM = true;
        }

        ciaVal = 1773447 / bpm; // yes, truncate here
        f_hz   = (float)(CIA_PAL_CLK) / ciaVal;

        if (editor.isSMPRendering)
            f_smp = (editor.pat2SmpHQ ? 28836.0f : 22168.0f) / f_hz;
        else
            f_smp = editor.outputFreq_f / f_hz;

        mixerSetSamplesPerTick((int32_t)(f_smp + 0.5f));
    }
}

void modStop(void)
{
    uint8_t i;
    moduleChannel_t *ch;

    editor.songPlaying = false;

    mixerKillAllVoices();

    // don't reset sample, volume or command memory/flags
    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        ch = &modEntry->channels[i];

        ch->chanIndex          = i;
        ch->command            = 0;
        ch->fineTune           = 0;
        ch->flags              = 0;
        ch->noNote             = true;
        ch->offset             = 0;
        ch->offsetBugNotAdded  = true;
        ch->param              = 0;
        ch->pattLoopCounter    = 0;
        ch->patternLoopRow     = 0;
        ch->period             = 0;
        // -- these two gets zeroed anyways --
        ch->glissandoControl   = 0;
        ch->waveControl        = 0;
        // -----------------------------------
        ch->scopeLoopQuirk_f   = 0.0;
        ch->scopeEnabled       = false;
        ch->scopeTrigger       = false;
        ch->scopeLoopFlag      = false;
        ch->scopeKeepDelta     = true;
        ch->scopeKeepVolume    = true;
        ch->tempFlags          = 0;
        ch->tempFlagsBackup    = 0;
        ch->tempPeriod         = 0;
    }

    tempFlags        = 0;
    pattBreakBugPos  = -1;
    pattBreakFlag    = false;
    pattDelayFlag    = false;
    forceEffectsOff  = false;
    pattDelayTime    = 0;
    pattDelayTime2   = 0;
    pBreakPosition   = 0;
    posJumpAssert    = false;
    modHasBeenPlayed = true;
}

void playPattern(int8_t startRow)
{
    uint8_t i;

    for (i = 0; i < AMIGA_VOICES; ++i)
        modEntry->channels[i].chanIndex = i;

    modEntry->row     = startRow & 0x3F;
    modEntry->currRow = modEntry->row;
    editor.modTick     = 0;
    tempFlags          = 0;
    editor.playMode    = PLAY_MODE_PATTERN;
    editor.currMode    = MODE_PLAY;

    if (!editor.stepPlayEnabled)
        pointerSetMode(POINTER_MODE_PLAY, DO_CARRY);

    editor.songPlaying = true;
}

void incPatt(void)
{
    if (++modPattern > (MAX_PATTERNS - 1))
          modPattern = 0;
    modEntry->currPattern = modPattern;

    editor.ui.updatePatternData  = true;
    editor.ui.updateCurrPattText = true;
}

void decPatt(void)
{
    if (--modPattern < 0)
          modPattern = MAX_PATTERNS - 1;
    modEntry->currPattern = modPattern;

    editor.ui.updatePatternData  = true;
    editor.ui.updateCurrPattText = true;
}

void modPlay(int16_t patt, int16_t order, int8_t row)
{
    uint8_t i;
    moduleChannel_t *ch;

    if (row != -1)
    {
        if ((row >= 0) && (row <= 63))
        {
            modEntry->row     = row;
            modEntry->currRow = row;
        }
    }
    else
    {
        modEntry->row     = 0;
        modEntry->currRow = 0;
    }

    if (editor.playMode != PLAY_MODE_PATTERN)
    {
        if (modOrder >= modEntry->head.orderCount)
        {
            modOrder = 0;
            modEntry->currOrder = 0;
        }

        if ((order >= 0) && (order < modEntry->head.orderCount))
        {
            modOrder = order;
            modEntry->currOrder = order;
        }

        if (order >= modEntry->head.orderCount)
        {
            modOrder = 0;
            modEntry->currOrder = 0;
        }
    }

    if ((patt >= 0) && (patt <= (MAX_PATTERNS - 1)))
    {
        modPattern = patt;
        modEntry->currPattern = patt;
    }
    else
    {
        modPattern = modEntry->head.order[modOrder];
        modEntry->currPattern = modEntry->head.order[modOrder];
    }

    editor.currPatternDisp   = &modEntry->head.order[modOrder];
    editor.currPosEdPattDisp = &modEntry->head.order[modOrder];

    mixerKillAllVoices();

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        ch = &modEntry->channels[i];

        ch->chanIndex         = i;
        ch->fineTune          = 0;
        ch->flags             = 0;
        ch->noNote            = true;
        ch->period            = 0;
        ch->sample            = 0;
        ch->wantedperiod      = 0;
        ch->offset            = 0;
        ch->volume            = 0;
        ch->offsetBugNotAdded = true;
        ch->pattLoopCounter   = 0;
        ch->glissandoControl  = 0;
        ch->waveControl       = 0;
        ch->scopeLoopQuirk_f  = 0.0;
        ch->scopeEnabled      = false;
        ch->scopeTrigger      = false;
        ch->scopeLoopFlag     = false;
        ch->scopeKeepDelta    = true;
        ch->scopeKeepVolume   = true;
        ch->tempFlags         = 0;
        ch->tempFlagsBackup   = 0;
        ch->tempPeriod        = 0;
        ch->didQuantize       = false;
    }

    if (editor.playMode == PLAY_MODE_NORMAL)
    {
        editor.ticks50Hz = 0;
        editor.playTime  = 0;
    }

    tempFlags          = 0;
    tempPeriod         = 0;
    pBreakFlag         = false;
    posJumpAssert      = false;
    pattDelayTime      = 0;
    posJumpAssert      = false;
    pattBreakFlag      = false;
    pattDelayFlag      = false;
    pattDelayTime2     = 0;
    pBreakPosition     = 0;
    editor.modTick     = 0;
    pattBreakBugPos    = -1;
    forceEffectsOff    = false;
    modHasBeenPlayed   = false;
    editor.songPlaying = true;

    if (!editor.isSMPRendering && !editor.isWAVRendering)
    {
        editor.ui.updateSongPos      = true;
        editor.ui.updateSongTime     = true;
        editor.ui.updatePatternData  = true;
        editor.ui.updateSongPattern  = true;
        editor.ui.updateCurrPattText = true;
    }
}

void clearSong(void)
{
    uint8_t i;

    if (modEntry != NULL)
    {
        memset(editor.ui.pattNames,         0, MAX_PATTERNS * 16);
        memset(modEntry->head.order,       0, sizeof (modEntry->head.order));
        memset(modEntry->head.moduleTitle, 0, sizeof (modEntry->head.moduleTitle));

        editor.muted[0] = false;
        editor.muted[1] = false;
        editor.muted[2] = false;
        editor.muted[3] = false;

        editor.f6Pos  = 0;
        editor.f7Pos  = 16;
        editor.f8Pos  = 32;
        editor.f9Pos  = 48;
        editor.f10Pos = 63;

        editor.playTime        = 0;
        editor.ticks50Hz       = 0;
        editor.metroFlag       = false;
        editor.currSample      = 0;
        editor.editMoveAdd     = 1;
        editor.blockMarkFlag   = false;
        editor.swapChannelFlag = false;

        modEntry->head.orderCount   = 1;
        modEntry->head.patternCount = 1;

        for (i = 0; i < MAX_PATTERNS; ++i)
            memset(modEntry->patterns[i], 0, (MOD_ROWS * AMIGA_VOICES) * sizeof (note_t));

        for (i = 0; i < AMIGA_VOICES; ++i)
        {
            modEntry->channels[i].fineTune         = 0;
            modEntry->channels[i].waveControl      = 0;
            modEntry->channels[i].pattLoopCounter  = 0;
            modEntry->channels[i].invertLoopSpeed  = 0;
            modEntry->channels[i].glissandoControl = 0;
        }

        modSetPos(0, 0); // this also refreshes pattern data

        modEntry->currOrder     = 0;
        modEntry->currPattern   = 0;
        editor.currPatternDisp   = &modEntry->head.order[0];
        editor.currPosEdPattDisp = &modEntry->head.order[0];

        modSetTempo(editor.initialTempo);
        modSetSpeed(editor.initialSpeed);

        setLEDFilter(false); // real PT doesn't do this there, but that's insane

        editor.ui.updateSongSize = true;

        updateWindowTitle(MOD_IS_MODIFIED);
    }
}

void clearSamples(void)
{
    uint8_t i;

    if (modEntry != NULL)
    {
        for (i = 0; i < MOD_SAMPLES; ++i)
        {
            modEntry->samples[i].fineTune   = 0;
            modEntry->samples[i].length     = 0;
            modEntry->samples[i].loopLength = 2;
            modEntry->samples[i].loopStart  = 0;
            modEntry->samples[i].volume     = 0;

            memset(modEntry->samples[i].text, 0, sizeof (modEntry->samples[i].text));
        }

        memset(modEntry->sampleData, 0, MOD_SAMPLES * MAX_SAMPLE_LEN);

        editor.currSample           = 0;
        editor.keypadSampleOffset   = 0;
        editor.sampleZero           = false;
        editor.ui.editOpScreenShown = false;
        editor.ui.aboutScreenShown  = false;
        editor.blockMarkFlag        = false;

        editor.samplePos = 0;
        updateCurrSample();

        updateWindowTitle(MOD_IS_MODIFIED);
    }
}

void clearAll(void)
{
    if (modEntry != NULL)
    {
        clearSamples();
        clearSong();
    }
}

void modFree(void)
{
    uint8_t i;

    if (modEntry != NULL)
    {
        for (i = 0; i < MAX_PATTERNS; ++i)
        {
            if (modEntry->patterns[i] != NULL)
                free(modEntry->patterns[i]);
        }

        if (modEntry->sampleData != NULL)
            free(modEntry->sampleData);

        free(modEntry);
        modEntry = NULL;
    }
}

uint8_t getSongProgressInPercentage(void)
{
    return (uint8_t)((((float)(modEntry->rowsCounter) / modEntry->rowsInTotal) * 100.0f));
}

void restartSong(void) // for the beginning of MOD2WAV/PAT2SMP
{
    if (editor.songPlaying)
        modStop();

    editor.playMode = PLAY_MODE_NORMAL;
    editor.blockMarkFlag = false;
    forceMixerOff = true;

    modEntry->row = 0;
    modEntry->currRow = 0;
    modEntry->rowsCounter = 0;

    memset(editor.rowVisitTable, 0, MOD_ORDERS * MOD_ROWS); // for MOD2WAV

    if (editor.isSMPRendering)
    {
        modPlay(DONT_SET_PATTERN, DONT_SET_ORDER, DONT_SET_ROW);
    }
    else
    {
        modEntry->currBPM = 125;
        modSetSpeed(6);
        modSetTempo(125);

        modPlay(DONT_SET_PATTERN, 0, 0);
    }
}

// this function is meant for the end of MOD2WAV/PAT2SMP
void resetSong(void) // only call this after storeTempVariables() has been called!
{
    int8_t i;

    modStop();

    editor.songPlaying = false;
    editor.playMode    = PLAY_MODE_NORMAL;
    editor.currMode    = MODE_IDLE;

    mixerKillAllVoices();

    memset((int8_t *)(editor.vuMeterVolumes),    0, sizeof (editor.vuMeterVolumes));
    memset((float  *)(editor.realVuMeterVolumes),0, sizeof (editor.realVuMeterVolumes));
    memset((int8_t *)(editor.spectrumVolumes),   0, sizeof (editor.spectrumVolumes));

    memset(modEntry->channels, 0, sizeof (modEntry->channels));
    for (i = 0; i < AMIGA_VOICES; ++i)
        modEntry->channels[i].chanIndex = i;

    modOrder   = oldOrder;
    modPattern = oldPattern;

    modEntry->row         = oldRow;
    modEntry->currRow     = oldRow;
    modEntry->currBPM     = oldBPM;
    modEntry->currOrder   = oldOrder;
    modEntry->currPattern = oldPattern;

    editor.currPosDisp         = &modEntry->currOrder;
    editor.currEditPatternDisp = &modEntry->currPattern;
    editor.currPatternDisp     = &modEntry->head.order[modEntry->currOrder];
    editor.currPosEdPattDisp   = &modEntry->head.order[modEntry->currOrder];

    modSetSpeed(oldSpeed);
    modSetTempo(oldBPM);

    editor.modTick   = 0;
    tempFlags        = 0;
    pBreakPosition   = 0;
    posJumpAssert    = false;
    pattBreakBugPos  = -1;
    pattBreakFlag    = false;
    pattDelayFlag    = false;
    forceEffectsOff  = false;
    pattDelayTime    = 0;
    pattDelayTime2   = 0;
    pBreakFlag       = false;
    modHasBeenPlayed = false;
    forceMixerOff    = false;
}
