#include <SDL2/SDL.h>
#include <stdint.h>
#include <ctype.h> // tolower()
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include "pt_header.h"
#include "pt_helpers.h"
#include "pt_textout.h"
#include "pt_tables.h"
#include "pt_audio.h"
#include "pt_helpers.h"
#include "pt_palette.h"
#include "pt_diskop.h"
#include "pt_mouse.h"
#include "pt_sampler.h"
#include "pt_visuals.h"
#include "pt_keyboard.h"

void setPattern(int16_t pattern); // pt_modplayer.c

void jamAndPlaceSample(SDL_Keycode keyEntry, int8_t normalModes);
uint8_t quantizeCheck(moduleChannel_t *ch, uint8_t row);
uint8_t handleSpecialKeys(SDL_Scancode keyEntry);
int8_t keyToNote(SDL_Keycode keyEntry);

// used for re-rendering text object while editing it
void updateTextObject(int16_t editObject)
{
    switch (editObject)
    {
        default: break;
        case PTB_SONGNAME:         editor.ui.updateSongName = true; break;
        case PTB_SAMPLENAME:       editor.ui.updateCurrSampleName = true; break;
        case PTB_PE_PATT:          editor.ui.updatePosEd = true; break;
        case PTB_PE_PATTNAME:      editor.ui.updatePosEd = true; break;
        case PTB_EO_QUANTIZE:      editor.ui.updateQuantizeText = true; break;
        case PTB_EO_METRO_1:       editor.ui.updateMetro1Text = true; break;
        case PTB_EO_METRO_2:       editor.ui.updateMetro2Text = true; break;
        case PTB_EO_FROM_NUM:      editor.ui.updateFromText = true; break;
        case PTB_EO_TO_NUM:        editor.ui.updateToText = true; break;
        case PTB_EO_MIX:           editor.ui.updateMixText = true; break;
        case PTB_EO_POS_NUM:       editor.ui.updatePosText = true; break;
        case PTB_EO_MOD_NUM:       editor.ui.updateModText = true; break;
        case PTB_EO_VOL_NUM:       editor.ui.updateVolText = true; break;
        case PTB_DO_DATAPATH:      editor.ui.updateDiskOpPathText = true; break;
        case PTB_POSS:             editor.ui.updateSongPos = true; break;
        case PTB_PATTERNS:         editor.ui.updateSongPattern = true; break;
        case PTB_LENGTHS:          editor.ui.updateSongLength = true; break;
        case PTB_SAMPLES:          editor.ui.updateCurrSampleNum = true; break;
        case PTB_SVOLUMES:         editor.ui.updateCurrSampleVolume = true; break;
        case PTB_SLENGTHS:         editor.ui.updateCurrSampleLength = true; break;
        case PTB_SREPEATS:         editor.ui.updateCurrSampleRepeat = true; break;
        case PTB_SREPLENS:         editor.ui.updateCurrSampleReplen = true; break;
        case PTB_PATTDATA:         editor.ui.updateCurrPattText = true; break;
        case PTB_SA_VOL_FROM_NUM:  editor.ui.updateVolFromText = true; break;
        case PTB_SA_VOL_TO_NUM:    editor.ui.updateVolToText = true; break;
        case PTB_SA_FIL_LP_CUTOFF: editor.ui.updateLPText = true; break;
        case PTB_SA_FIL_HP_CUTOFF: editor.ui.updateHPText = true; break;
    }
}

void exitGetTextLine(uint8_t updateValue)
{
    int8_t tmp8;
    uint8_t i;
    int16_t posEdPos, tmp16;
    int32_t tmp32;
    moduleSample_t *s;

    // if user updated the disk op path text
    if (editor.ui.diskOpScreenShown && (editor.ui.editObject == PTB_DO_DATAPATH))
        diskOpSetPath(editor.currPath, DISKOP_CACHE);

    if (editor.ui.getLineType != TEXT_EDIT_STRING)
    {
        if (editor.ui.dstPos != editor.ui.numLen)
            removeTextEditMarker();

        updateTextObject(editor.ui.editObject);
    }
    else
    {
        removeTextEditMarker();

        // yet another hack to fix my stupid way of coding
        if ((editor.ui.editObject == PTB_PE_PATT) || (editor.ui.editObject == PTB_PE_PATTNAME))
            editor.ui.updatePosEd = true;
    }

    editor.ui.getLineFlag = false;
    editor.ui.lineCurX = 0;
    editor.ui.lineCurY = 0;
    editor.ui.editPos = NULL;
    editor.ui.dstPos  = 0;

    if (editor.ui.getLineType == TEXT_EDIT_STRING)
    {
        if (editor.ui.dstOffset != NULL)
            *editor.ui.dstOffset = '\0';

        if (editor.ui.editObject == PTB_SONGNAME)
        {
            for (i = 0; i < 20; ++i)
                modEntry->head.moduleTitle[i] = (char)(tolower(modEntry->head.moduleTitle[i]));
        }

        if (editor.ui.editObject != PTB_DO_DATAPATH) // special case for disk op. right mouse button
        {
            pointerSetPreviousMode();

            if (!editor.mixFlag && (editor.ui.editObject != PTB_PE_PATTNAME))
                updateWindowTitle(MOD_IS_MODIFIED);
        }
    }
    else
    {
        // set back GUI text pointers and update values (if requested)

        s = &modEntry->samples[editor.currSample];
        switch (editor.ui.editObject)
        {
            case PTB_SA_FIL_LP_CUTOFF:
            {
                editor.lpCutOffDisp = &editor.lpCutOff;

                if (updateValue)
                {
                    editor.lpCutOff = editor.ui.tmpDisp32;
                    if (editor.lpCutOff > (FILTERS_BASE_FREQ / 2))
                        editor.lpCutOff =  FILTERS_BASE_FREQ / 2;

                    editor.ui.updateLPText = true;
                }
            }
            break;

            case PTB_SA_FIL_HP_CUTOFF:
            {
                editor.hpCutOffDisp = &editor.hpCutOff;

                if (updateValue)
                {
                    editor.hpCutOff = editor.ui.tmpDisp32;
                    if (editor.hpCutOff > (FILTERS_BASE_FREQ / 2))
                        editor.hpCutOff =  FILTERS_BASE_FREQ / 2;

                    editor.ui.updateHPText = true;
                }
            }
            break;

            case PTB_SA_VOL_FROM_NUM:
            {
                editor.vol1Disp = &editor.vol1;

                if (updateValue)
                {
                    editor.vol1 = editor.ui.tmpDisp16;
                    if (editor.vol1 > 200)
                        editor.vol1 = 200;

                    editor.ui.updateVolFromText = true;
                    showVolFromSlider();
                }
            }
            break;

            case PTB_SA_VOL_TO_NUM:
            {
                editor.vol2Disp = &editor.vol2;

                if (updateValue)
                {
                    editor.vol2 = editor.ui.tmpDisp16;
                    if (editor.vol2 > 200)
                        editor.vol2 = 200;

                    editor.ui.updateVolToText = true;
                    showVolToSlider();
                }
            }
            break;

            case PTB_EO_VOL_NUM:
            {
                editor.sampleVolDisp = &editor.sampleVol;

                if (updateValue)
                {
                    editor.sampleVol = editor.ui.tmpDisp16;
                    editor.ui.updateVolText = true;
                }
            }
            break;

            case PTB_EO_POS_NUM:
            {
                editor.samplePosDisp = &editor.samplePos;

                if (updateValue)
                {
                    editor.samplePos = editor.ui.tmpDisp32;
                    if (editor.samplePos > modEntry->samples[editor.currSample].length)
                        editor.samplePos = modEntry->samples[editor.currSample].length;

                    editor.ui.updatePosText = true;
                }
            }
            break;

            case PTB_EO_QUANTIZE:
            {
                editor.quantizeValueDisp = &editor.quantizeValue;

                if (updateValue)
                {
                    if (editor.ui.tmpDisp16 > 63)
                        editor.ui.tmpDisp16 = 63;

                    editor.quantizeValue = editor.ui.tmpDisp16;
                    editor.ui.updateQuantizeText = true;
                }
            }
            break;

            case PTB_EO_METRO_1: // metronome speed
            {
                editor.metroSpeedDisp = &editor.metroSpeed;

                if (updateValue)
                {
                    if (editor.ui.tmpDisp16 > 64)
                        editor.ui.tmpDisp16 = 64;

                    editor.metroSpeed = editor.ui.tmpDisp16;
                    editor.ui.updateMetro1Text = true;
                }
            }
            break;

            case PTB_EO_METRO_2: // metronome channel
            {
                editor.metroChannelDisp = &editor.metroChannel;

                if (updateValue)
                {
                    if (editor.ui.tmpDisp16 > 4)
                        editor.ui.tmpDisp16 = 4;

                    editor.metroChannel = editor.ui.tmpDisp16;
                    editor.ui.updateMetro2Text = true;
                }
            }
            break;

            case PTB_EO_FROM_NUM:
            {
                editor.sampleFromDisp = &editor.sampleFrom;

                if (updateValue)
                {
                    editor.sampleFrom = editor.ui.tmpDisp8;

                    // signed check + normal check
                    if ((editor.sampleFrom < 0x00) || (editor.sampleFrom > 0x1F))
                        editor.sampleFrom = 0x1F;

                    editor.ui.updateFromText = true;
                }
            }
            break;

            case PTB_EO_TO_NUM:
            {
                editor.sampleToDisp = &editor.sampleTo;

                if (updateValue)
                {
                    editor.sampleTo = editor.ui.tmpDisp8;

                    // signed check + normal check
                    if ((editor.sampleTo < 0x00) || (editor.sampleTo > 0x1F))
                        editor.sampleTo = 0x1F;

                    editor.ui.updateToText = true;
                }
            }
            break;

            case PTB_PE_PATT:
            {
                posEdPos = modEntry->currOrder;
                if (posEdPos > (modEntry->head.orderCount - 1))
                    posEdPos =  modEntry->head.orderCount - 1;

                editor.currPosEdPattDisp = &modEntry->head.order[posEdPos];

                if (updateValue)
                {
                    if (editor.ui.tmpDisp16 > (MAX_PATTERNS - 1))
                        editor.ui.tmpDisp16 =  MAX_PATTERNS - 1;

                    modEntry->head.order[posEdPos] = editor.ui.tmpDisp16;

                    updateWindowTitle(MOD_IS_MODIFIED);

                    if (editor.ui.posEdScreenShown)
                        editor.ui.updatePosEd = true;

                    editor.ui.updateSongPattern = true;
                    editor.ui.updateSongSize = true;
                }
            }
            break;

            case PTB_POSS:
            {
                editor.currPosDisp = &modEntry->currOrder;

                if (updateValue)
                {
                    tmp16 = editor.ui.tmpDisp16;
                    if (tmp16 > 126)
                        tmp16 = 126;

                    if (modEntry->currOrder != tmp16)
                    {
                        modEntry->currOrder = tmp16;
                        editor.currPatternDisp = &modEntry->head.order[modEntry->currOrder];

                        if (editor.ui.posEdScreenShown)
                            editor.ui.updatePosEd = true;

                        editor.ui.updateSongPos = true;
                        editor.ui.updatePatternData = true;
                    }
                }
            }
            break;

            case PTB_PATTERNS:
            {
                editor.currPatternDisp = &modEntry->head.order[modEntry->currOrder];

                if (updateValue)
                {
                    tmp16 = editor.ui.tmpDisp16;
                    if (tmp16 > (MAX_PATTERNS - 1))
                        tmp16 =  MAX_PATTERNS - 1;

                    if (modEntry->head.order[modEntry->currOrder] != tmp16)
                    {
                        modEntry->head.order[modEntry->currOrder] = tmp16;

                        updateWindowTitle(MOD_IS_MODIFIED);

                        if (editor.ui.posEdScreenShown)
                            editor.ui.updatePosEd = true;

                        editor.ui.updateSongPattern = true;
                        editor.ui.updateSongSize = true;
                    }
                }
            }
            break;

            case PTB_LENGTHS:
            {
                editor.currLengthDisp = &modEntry->head.orderCount;

                if (updateValue)
                {
                    tmp16 = CLAMP(editor.ui.tmpDisp16, 1, 127);

                    if (modEntry->head.orderCount != tmp16)
                    {
                        modEntry->head.orderCount = tmp16;

                        posEdPos = modEntry->currOrder;
                        if (posEdPos > (modEntry->head.orderCount - 1))
                            posEdPos =  modEntry->head.orderCount - 1;

                        editor.currPosEdPattDisp = &modEntry->head.order[posEdPos];

                        if (editor.ui.posEdScreenShown)
                            editor.ui.updatePosEd = true;

                        editor.ui.updateSongLength = true;
                        editor.ui.updateSongSize = true;
                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
            }
            break;

            case PTB_PATTDATA:
            {
                editor.currEditPatternDisp = &modEntry->currPattern;

                if (updateValue)
                {
                    if (modEntry->currPattern != editor.ui.tmpDisp16)
                    {
                        setPattern(editor.ui.tmpDisp16);

                        editor.ui.updatePatternData = true;
                        editor.ui.updateCurrPattText = true;
                    }
                }
            }
            break;

            case PTB_SAMPLES:
            {
                editor.currSampleDisp = &editor.currSample;

                if (updateValue)
                {
                    tmp8 = editor.ui.tmpDisp8;
                    if (tmp8 < 0x00) // (signed) if >0x7F was entered, clamp to 0x1F
                        tmp8 = 0x1F;

                    tmp8 = CLAMP(tmp8, 0x01, 0x1F) - 1;

                    if (tmp8 != editor.currSample)
                    {
                        editor.currSample = tmp8;

                        updateCurrSample();
                    }
                }
            }
            break;

            case PTB_SVOLUMES:
            {
                s->volumeDisp = &s->volume;

                if (updateValue)
                {
                    tmp8 = editor.ui.tmpDisp8;

                    // signed check + normal check
                    if ((tmp8 < 0x00) || (tmp8 > 0x40))
                         tmp8 = 0x40;

                    if (s->volume != tmp8)
                    {
                        s->volume = tmp8;

                        editor.ui.updateCurrSampleVolume = true;
                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
            }
            break;

            case PTB_SLENGTHS:
            {
                s->lengthDisp = &s->length;

                if (updateValue)
                {
                    tmp32 = editor.ui.tmpDisp32 & 0xFFFFFFFE; // even'ify
                    if (tmp32 > MAX_SAMPLE_LEN)
                        tmp32 = MAX_SAMPLE_LEN;

                    if ((s->loopLength > 2) || (s->loopStart > 0))
                    {
                        if (tmp32 < (s->loopStart + s->loopLength))
                            tmp32 =  s->loopStart + s->loopLength;
                    }

                    if (s->length != tmp32)
                    {
                        mixerKillVoiceIfReadingSample(editor.currSample);
                        s->length = tmp32;
                        updateVoiceParams();

                        editor.ui.updateCurrSampleLength = true;
                        editor.ui.updateSongSize = true;
                        updateSamplePos();

                        testTempLoopPoints(s->length);

                        if (editor.ui.samplerScreenShown)
                            redrawSample();

                        recalcChordLength();
                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
            }
            break;

            case PTB_SREPEATS:
            {
                s->loopStartDisp = &s->loopStart;

                if (updateValue)
                {
                    tmp32 = editor.ui.tmpDisp32 & 0xFFFFFFFE; // even'ify
                    if (tmp32 > MAX_SAMPLE_LEN)
                        tmp32 = MAX_SAMPLE_LEN;

                    if (s->length >= s->loopLength)
                    {
                        if ((tmp32 + s->loopLength) > s->length)
                             tmp32 = s->length - s->loopLength;
                    }
                    else
                    {
                        tmp32 = 0;
                    }

                    if (s->loopStart != tmp32)
                    {
                        mixerKillVoiceIfReadingSample(editor.currSample);
                        s->loopStart = tmp32;
                        updateVoiceParams();

                        editor.ui.updateCurrSampleRepeat = true;

                        if (editor.ui.editOpScreenShown && (editor.ui.editOpScreen == 3))
                            editor.ui.updateLengthText = true;

                        if (editor.ui.samplerScreenShown)
                            setLoopSprites();

                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
            }
            break;

            case PTB_SREPLENS:
            {
                s->loopLengthDisp = &s->loopLength;

                if (updateValue)
                {
                    tmp32 = editor.ui.tmpDisp32 & 0xFFFFFFFE; // even'ify
                    if (tmp32 > MAX_SAMPLE_LEN)
                        tmp32 = MAX_SAMPLE_LEN;

                    if (s->length >= s->loopStart)
                    {
                        if ((s->loopStart + tmp32) > s->length)
                            tmp32 = s->length - s->loopStart;
                    }
                    else
                    {
                        tmp32 = 2;
                    }

                    if (tmp32 < 2)
                        tmp32 = 2;

                    if (s->loopLength != tmp32)
                    {
                        mixerKillVoiceIfReadingSample(editor.currSample);
                        s->loopLength = tmp32;
                        updateVoiceParams();

                        editor.ui.updateCurrSampleReplen = true;
                        if (editor.ui.editOpScreenShown && (editor.ui.editOpScreen == 3))
                            editor.ui.updateLengthText = true;

                        if (editor.ui.samplerScreenShown)
                            setLoopSprites();

                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
            }
            break;

            default: break;
        }

        pointerSetPreviousMode();
    }

    editor.ui.getLineType = 0;
}

void getTextLine(int16_t editObject)
{
    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);

    editor.ui.lineCurY    =  (editor.ui.editTextPos / 40) + 5;
    editor.ui.lineCurX    = ((editor.ui.editTextPos % 40) * FONT_CHAR_W) + 4;
    editor.ui.dstPtr      = editor.ui.showTextPtr;
    editor.ui.editPos     = editor.ui.showTextPtr;
    editor.ui.dstPos      = 0;
    editor.ui.getLineFlag = true;
    editor.ui.getLineType = TEXT_EDIT_STRING;
    editor.ui.editObject  = editObject;

    if (editor.ui.dstOffset != NULL)
        *editor.ui.dstOffset = '\0';

    if (editor.mixFlag)
    {
        textCharNext();
        textCharNext();
        textCharNext();
        textCharNext();
    }

    renderTextEditMarker();
}

void getNumLine(uint8_t type, int16_t editObject)
{
    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);

    editor.ui.lineCurY    =  (editor.ui.editTextPos / 40) + 5;
    editor.ui.lineCurX    = ((editor.ui.editTextPos % 40) * FONT_CHAR_W) + 4;
    editor.ui.dstPos      = 0;
    editor.ui.getLineFlag = true;
    editor.ui.getLineType = type;
    editor.ui.editObject  = editObject;

    renderTextEditMarker();
}

void handleEditKeys(SDL_Scancode keyEntry, int8_t normalMode)
{
    int8_t key, hexKey, numberKey;
    SDL_Keycode keyCode;
    note_t *note;

    if (editor.ui.getLineFlag)
        return;

    keyCode = scanCodeToUSKey(keyEntry);

    if (editor.ui.samplerScreenShown || (editor.currMode == MODE_IDLE) || (editor.currMode == MODE_PLAY))
    {
        // it will only jam, not place it
        jamAndPlaceSample(keyCode, normalMode);
        return;
    }

    if ((editor.currMode == MODE_EDIT) || (editor.currMode == MODE_RECORD))
    {
        if (handleSpecialKeys(keyEntry))
        {
            if (editor.currMode != MODE_RECORD)
                modSetPos(DONT_SET_ORDER, (modEntry->currRow + editor.editMoveAdd) & 63);

            return;
        }
    }

    // are we editing a note, or other stuff?
    if (editor.cursor.mode != CURSOR_NOTE)
    {
        if ((editor.currMode == MODE_EDIT) || (editor.currMode == MODE_RECORD))
        {
            numberKey = ((keyCode > 47) && (keyCode <  58)) ? ((int8_t)(keyCode) - 48) : -1;
            hexKey    = ((keyCode > 96) && (keyCode < 103)) ? ((int8_t)(keyCode) - 87) : -1;
            key       = -1;

            if (numberKey != -1)
            {
                if (key == -1)
                    key = 0;

                key += numberKey;
            }

            if (hexKey != -1)
            {
                if (key == -1)
                    key = 0;

                key += hexKey;
            }

            note = &modEntry->patterns[modEntry->currPattern][(modEntry->currRow * AMIGA_VOICES) + editor.cursor.channel];

            switch (editor.cursor.mode)
            {
                case CURSOR_SAMPLE1:
                {
                    if ((key != -1) && (key < 2))
                    {
                        note->sample = (uint8_t)((note->sample % 0x10) | (key << 4));

                        if (editor.currMode != MODE_RECORD)
                            modSetPos(DONT_SET_ORDER, (modEntry->currRow + editor.editMoveAdd) & 0x3F);

                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
                break;

                case CURSOR_SAMPLE2:
                {
                    if ((key != -1) && (key < 16))
                    {
                        note->sample = (uint8_t)((note->sample & 16) | key);

                        if (editor.currMode != MODE_RECORD)
                            modSetPos(DONT_SET_ORDER, (modEntry->currRow + editor.editMoveAdd) & 0x3F);

                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
                break;

                case CURSOR_CMD:
                {
                    if ((key != -1) && (key < 16))
                    {
                        note->command = (uint8_t)(key);

                        if (editor.currMode != MODE_RECORD)
                            modSetPos(DONT_SET_ORDER, (modEntry->currRow + editor.editMoveAdd) & 0x3F);

                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
                break;

                case CURSOR_PARAM1:
                {
                    if ((key != -1) && (key < 16))
                    {
                        note->param = (uint8_t)((note->param % 0x10) | (key << 4));

                        if (editor.currMode != MODE_RECORD)
                            modSetPos(DONT_SET_ORDER, (modEntry->currRow + editor.editMoveAdd) & 0x3F);

                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
                break;

                case CURSOR_PARAM2:
                {
                    if ((key != -1) && (key < 16))
                    {
                        note->param = (uint8_t)((note->param & 0xF0) | key);

                        if (editor.currMode != MODE_RECORD)
                            modSetPos(DONT_SET_ORDER, (modEntry->currRow + editor.editMoveAdd) & 0x3F);

                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
                break;

                default: break;
            }
        }
    }
    else
    {
        if (keyEntry == SDL_SCANCODE_DELETE)
        {
            if ((editor.currMode == MODE_EDIT) || (editor.currMode == MODE_RECORD))
            {
                note = &modEntry->patterns[modEntry->currPattern][(modEntry->currRow * AMIGA_VOICES) + editor.cursor.channel];

                if (!input.keyb.altKeyDown)
                {
                    note->sample = 0;
                    note->period = 0;
                }

                if (input.keyb.shiftKeyDown || input.keyb.altKeyDown)
                {
                    note->command = 0;
                    note->param   = 0;
                }

                if (editor.currMode != MODE_RECORD)
                    modSetPos(DONT_SET_ORDER, (modEntry->currRow + editor.editMoveAdd) & 63);

                updateWindowTitle(MOD_IS_MODIFIED);
            }
        }
        else
        {
            jamAndPlaceSample(keyCode, normalMode);
        }
    }
}

uint8_t handleSpecialKeys(SDL_Scancode keyEntry)
{
    note_t *note, *prevNote;

    if (input.keyb.altKeyDown)
    {
        note     = &modEntry->patterns[modEntry->currPattern][(modEntry->currRow * AMIGA_VOICES) + editor.cursor.channel];
        prevNote = &modEntry->patterns[modEntry->currPattern][(((modEntry->currRow - 1) & 0x3F) * AMIGA_VOICES) + editor.cursor.channel];

        if ((keyEntry >= SDL_SCANCODE_1) && (keyEntry <= SDL_SCANCODE_0))
        {
            // insert stored effect (buffer[0..8])
            note->command = editor.effectMacros[keyEntry - SDL_SCANCODE_1] >> 8;
            note->param   = editor.effectMacros[keyEntry - SDL_SCANCODE_1] & 0xFF;

            updateWindowTitle(MOD_IS_MODIFIED);

            return (true);
        }

        // copy command+effect from above into current command+effect
        if (keyEntry == SDL_SCANCODE_BACKSLASH)
        {
            note->command = prevNote->command;
            note->param   = prevNote->param;

            updateWindowTitle(MOD_IS_MODIFIED);

            return (true);
        }

        // copy command+(effect + 1) from above into current command+effect
        if (keyEntry == SDL_SCANCODE_EQUALS/* || (keyEntry == SDL_SCANCODE_WORLD_0)*/) // SDL_SCANCODE_WORLD_0 on OS X (weird)
        {
            note->command = prevNote->command;
            note->param   = prevNote->param + 1; // wraps 0x00..0xFF

            updateWindowTitle(MOD_IS_MODIFIED);

            return (true);
        }

        // copy command+(effect - 1) from above into current command+effect
        if (keyEntry == SDL_SCANCODE_MINUS/*|| (keyEntry == SDL_SCANCODE_PLUS)*/) // SDL_SCANCODE_PLUS on OS X (weird)
        {
            note->command = prevNote->command;
            note->param   = prevNote->param - 1; // wraps 0x00..0xFF

            updateWindowTitle(MOD_IS_MODIFIED);

            return (true);
        }
    }

    return (false);
}

void jamAndPlaceSample(SDL_Keycode keyEntry, int8_t normalMode)
{
    int8_t noteVal;
    int16_t tempPeriod;
    uint16_t cleanPeriod;
    moduleSample_t *s;
    note_t *note;
    moduleChannel_t *chn;

    chn  = &modEntry->channels[editor.cursor.channel];
    note = &modEntry->patterns[modEntry->currPattern][(quantizeCheck(chn, modEntry->currRow) * AMIGA_VOICES) + editor.cursor.channel];

    noteVal = normalMode ? keyToNote(keyEntry) : pNoteTable[editor.currSample];
    if (noteVal >= 0)
    {
        s  = &modEntry->samples[editor.currSample];

        tempPeriod  = periodTable[(37 * s->fineTune) + noteVal];
        cleanPeriod = periodTable[noteVal];

        editor.currPlayNote = noteVal;

        // play current sample
        if (!editor.muted[editor.cursor.channel])
        {
            // don't play sample if we quantized to another row (will be played in modplayer instead)
            if (!((editor.currMode == MODE_RECORD) && chn->didQuantize))
            {
                if (s->length == 0)
                {
                    // shutdown scope
                    chn->scopeLoopQuirk_f = 0.0;
                    chn->scopeLoopQuirk   = false;
                    chn->scopeEnabled     = false;
                    chn->scopeTrigger     = false;

                    // shutdown voice
                    mixerKillVoice(editor.cursor.channel);
                }
                else
                {
                    chn->sample     = editor.currSample + 1;
                    chn->volume     = s->volume;
                    chn->period     = tempPeriod;
                    chn->tempPeriod = tempPeriod;

                    chn->scopeEnabled    = true;
                    chn->scopeKeepDelta  = false;
                    chn->scopeKeepVolume = false;
                    chn->scopeVolume     = volumeToScopeVolume(s->volume);
                    periodToScopeDelta(chn, tempPeriod);
                    chn->scopeLoopFlag   = (s->loopStart + s->loopLength) > 2;

                    chn->scopePos_f = s->offset;

                    chn->scopeEnd        = s->offset + s->length;
                    chn->scopeLoopBegin  = s->offset + s->loopStart;
                    chn->scopeLoopEnd    = s->offset + s->loopStart + s->loopLength;

                    // one-shot loop simulation (real PT didn't show this in the scopes...)
                    chn->scopeLoopQuirk = false;
                    if ((s->loopLength > 2) && (s->loopStart == 0))
                    {
                        chn->scopeLoopQuirk = chn->scopeLoopEnd;
                        chn->scopeLoopEnd   = chn->scopeEnd;
                    }

                    chn->scopeLoopQuirk_f = chn->scopeLoopQuirk;
                    chn->scopeEnd_f       = chn->scopeEnd;
                    chn->scopeLoopBegin_f = chn->scopeLoopBegin;
                    chn->scopeLoopEnd_f   = chn->scopeLoopEnd;

                    mixerSetVoiceSource(editor.cursor.channel, modEntry->sampleData + s->offset, s->length, s->loopStart, s->loopLength, 0);
                    mixerSetVoiceDelta(editor.cursor.channel, tempPeriod);
                    mixerSetVoiceVol(editor.cursor.channel, s->volume);
                }
            }
        }

        // normalMode = normal keys, or else keypad keys (in jam mode)
        if (normalMode || (editor.pNoteFlag != 0))
        {
            if (normalMode || (editor.pNoteFlag == 2))
            {
                // insert note and sample number
                if (!editor.ui.samplerScreenShown && ((editor.currMode == MODE_EDIT) || (editor.currMode == MODE_RECORD)))
                {
                    note->sample = editor.sampleZero ? 0 : (editor.currSample + 1);
                    note->period = cleanPeriod;

                    if (editor.autoInsFlag)
                    {
                        note->command = editor.effectMacros[editor.autoInsSlot] >> 8;
                        note->param   = editor.effectMacros[editor.autoInsSlot] & 0xFF;
                    }

                    if (editor.currMode != MODE_RECORD)
                        modSetPos(DONT_SET_ORDER, (modEntry->currRow + editor.editMoveAdd) & 0x3F);

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
            }

            if (editor.multiFlag)
            {
                editor.cursor.channel = (editor.multiModeNext[editor.cursor.channel] - 1) & 3;
                editor.cursor.pos = editor.cursor.channel * 6;
                updateCursorPos();
            }
        }

        // update spectrum bars if neeeded
        if ((editor.ui.visualizerMode == VISUAL_SPECTRUM) && !editor.muted[editor.cursor.channel])
            updateSpectrumAnalyzer(tempPeriod, s->volume);
    }
    else if (noteVal == -2)
    {
        // delete note and sample if illegal note (= -2, -1 = ignore) key was entered

        if (normalMode || (!normalMode && editor.pNoteFlag == 2))
        {
            if (!editor.ui.samplerScreenShown && ((editor.currMode == MODE_EDIT) || (editor.currMode == MODE_RECORD)))
            {
                note->period = 0;
                note->sample = 0;

                if (editor.currMode != MODE_RECORD)
                    modSetPos(DONT_SET_ORDER, (modEntry->currRow + editor.editMoveAdd) & 0x3F);

                updateWindowTitle(MOD_IS_MODIFIED);
            }
        }
    }
}

uint8_t quantizeCheck(moduleChannel_t *ch, uint8_t row)
{
    uint8_t tempRow, quantize;

    quantize = (uint8_t)(editor.quantizeValue);

    ch->didQuantize = false;

    if (editor.currMode == MODE_RECORD)
    {
        if (quantize == 0)
        {
            return (row);
        }
        else if (quantize == 1)
        {
            if (editor.modTick > (editor.modSpeed / 2))
            {
                row = (row + 1) & 0x3F;
                ch->didQuantize = true;
            }
        }
        else
        {
            tempRow = ((((quantize / 2) + row) & 0x3F) / quantize) * quantize;
            if (tempRow > row)
                ch->didQuantize = true;

            return (tempRow);
        }
    }

    return (row);
}

void saveUndo(void)
{
    memcpy(editor.undoBuffer, modEntry->patterns[modEntry->currPattern], sizeof (note_t) * (AMIGA_VOICES * MOD_ROWS));
}

void undoLastChange(void)
{
    uint16_t i;
    note_t data;

    for (i = 0; i < (MOD_ROWS * AMIGA_VOICES); ++i)
    {
        data = editor.undoBuffer[i];
        editor.undoBuffer[i] = modEntry->patterns[modEntry->currPattern][i];

        modEntry->patterns[modEntry->currPattern][i] = data;
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

void copySampleTrack(void)
{
    uint8_t i, j;
    uint32_t tmpOffset;
    note_t *noteSrc;
    moduleSample_t *smpFrom, *smpTo;

    if (editor.trackPattFlag == 2)
    {
        // copy from one sample slot to another

        // never attempt to swap if from and/or to is 0
        if ((editor.sampleFrom == 0) || (editor.sampleTo == 0))
        {
            displayErrorMsg("FROM/TO = 0 !");
            return;
        }

        smpTo   = &modEntry->samples[editor.sampleTo   - 1];
        smpFrom = &modEntry->samples[editor.sampleFrom - 1];

        mixerKillVoiceIfReadingSample(editor.sampleTo - 1);

        // copy
        tmpOffset     = smpTo->offset;
        *smpTo        = *smpFrom;
        smpTo->offset = tmpOffset;

        // update the copied sample's GUI text pointers
        smpTo->volumeDisp     = &smpTo->volume;
        smpTo->lengthDisp     = &smpTo->length;
        smpTo->loopStartDisp  = &smpTo->loopStart;
        smpTo->loopLengthDisp = &smpTo->loopLength;

        // copy sample data
        memcpy(&modEntry->sampleData[smpTo->offset], &modEntry->sampleData[smpFrom->offset], MAX_SAMPLE_LEN);

        updateCurrSample();

        editor.ui.updateSongSize = true;
    }
    else
    {
        // copy sample number in track/pattern
        if (editor.trackPattFlag == 0)
        {
            for (i = 0; i < MOD_ROWS; ++i)
            {
                noteSrc = &modEntry->patterns[modEntry->currPattern][(i * AMIGA_VOICES) + editor.cursor.channel];
                if (noteSrc->sample == editor.sampleFrom)
                    noteSrc->sample = editor.sampleTo;
            }
        }
        else
        {
            for (i = 0; i < AMIGA_VOICES; ++i)
            {
                for (j = 0; j < MOD_ROWS; ++j)
                {
                    noteSrc = &modEntry->patterns[modEntry->currPattern][(j * AMIGA_VOICES) + i];
                    if (noteSrc->sample == editor.sampleFrom)
                        noteSrc->sample = editor.sampleTo;
                }
            }
        }

        editor.ui.updatePatternData = true;
    }

    editor.samplePos = 0;
    updateSamplePos();

    updateWindowTitle(MOD_IS_MODIFIED);
}

void exchSampleTrack(void)
{
    int8_t smp;
    uint8_t i, j;
    uint32_t k, tmpOffset;
    moduleSample_t *smpFrom, *smpTo, smpTmp;
    note_t *noteSrc;

    if (editor.trackPattFlag == 2)
    {
        // exchange sample slots

        // never attempt to swap if from and/or to is 0
        if ((editor.sampleFrom == 0) || (editor.sampleTo == 0))
        {
            displayErrorMsg("FROM/TO = 0 !");
            return;
        }

        smpTo   = &modEntry->samples[editor.sampleTo   - 1];
        smpFrom = &modEntry->samples[editor.sampleFrom - 1];

        mixerKillVoiceIfReadingSample(editor.sampleFrom - 1);
        mixerKillVoiceIfReadingSample(editor.sampleTo   - 1);

        // swap offsets first so that the next swap will leave offsets intact
        tmpOffset       = smpFrom->offset;
        smpFrom->offset = smpTo->offset;
        smpTo->offset   = tmpOffset;

        // swap sample (now offsets are left as before)
        smpTmp   = *smpFrom;
        *smpFrom = *smpTo;
        *smpTo   = smpTmp;

        // update the swapped sample's GUI text pointers
        smpFrom->volumeDisp     = &smpFrom->volume;
        smpFrom->lengthDisp     = &smpFrom->length;
        smpFrom->loopStartDisp  = &smpFrom->loopStart;
        smpFrom->loopLengthDisp = &smpFrom->loopLength;
        smpTo->volumeDisp       = &smpTo->volume;
        smpTo->lengthDisp       = &smpTo->length;
        smpTo->loopStartDisp    = &smpTo->loopStart;
        smpTo->loopLengthDisp   = &smpTo->loopLength;

        // swap sample data
        for (k = 0; k < MAX_SAMPLE_LEN; ++k)
        {
            smp = modEntry->sampleData[smpFrom->offset + k];

            modEntry->sampleData[smpFrom->offset + k] = modEntry->sampleData[smpTo->offset + k];
            modEntry->sampleData[smpTo->offset   + k] = smp;
        }

        editor.sampleZero = false;

        updateCurrSample();
    }
    else
    {
        // exchange sample number in track/pattern
        if (editor.trackPattFlag == 0)
        {
            for (i = 0; i < MOD_ROWS; ++i)
            {
                noteSrc = &modEntry->patterns[modEntry->currPattern][(i * AMIGA_VOICES) + editor.cursor.channel];

                     if (noteSrc->sample == editor.sampleFrom) noteSrc->sample = editor.sampleTo;
                else if (noteSrc->sample == editor.sampleTo)   noteSrc->sample = editor.sampleFrom;
            }
        }
        else
        {
            for (i = 0; i < AMIGA_VOICES; ++i)
            {
                for (j = 0; j < MOD_ROWS; ++j)
                {
                    noteSrc = &modEntry->patterns[modEntry->currPattern][(j * AMIGA_VOICES) + i];

                         if (noteSrc->sample == editor.sampleFrom) noteSrc->sample = editor.sampleTo;
                    else if (noteSrc->sample == editor.sampleTo)   noteSrc->sample = editor.sampleFrom;
                }
            }
        }

        editor.ui.updatePatternData = true;
    }

    editor.samplePos = 0;
    updateSamplePos();

    updateWindowTitle(MOD_IS_MODIFIED);
}

void delSampleTrack(void)
{
    uint8_t i, j;
    note_t *noteSrc;

    saveUndo();

    if (editor.trackPattFlag == 0)
    {
        for (i = 0; i < MOD_ROWS; ++i)
        {
            noteSrc = &modEntry->patterns[modEntry->currPattern][(i * AMIGA_VOICES) + editor.cursor.channel];
            if (noteSrc->sample == (editor.currSample + 1))
            {
                noteSrc->period  = 0;
                noteSrc->sample  = 0;
                noteSrc->command = 0;
                noteSrc->param   = 0;
            }
        }
    }
    else
    {
        for (i = 0; i < AMIGA_VOICES; ++i)
        {
            for (j = 0; j < MOD_ROWS; ++j)
            {
                noteSrc = &modEntry->patterns[modEntry->currPattern][(j * AMIGA_VOICES) + i];
                if (noteSrc->sample == (editor.currSample + 1))
                {
                    noteSrc->period  = 0;
                    noteSrc->sample  = 0;
                    noteSrc->command = 0;
                    noteSrc->param   = 0;
                }
            }
        }
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

void trackNoteUp(uint8_t sampleAllFlag, uint8_t from, uint8_t to)
{
    uint8_t i, j, noteDeleted;
    note_t *noteSrc;

    if (from > to)
    {
        j    = from;
        from = to;
        to   = j;
    }

    saveUndo();

    for (i = from; i <= to; ++i)
    {
        noteSrc = &modEntry->patterns[modEntry->currPattern][(i * AMIGA_VOICES) + editor.cursor.channel];

        if (!sampleAllFlag && (noteSrc->sample != (editor.currSample + 1)))
            continue;

        if (noteSrc->period)
        {
            // period -> note
            for (j = 0; j < 36; ++j)
            {
                if (noteSrc->period >= periodTable[j])
                    break;
            }

            noteDeleted = false;
            if (++j > 35)
            {
                j = 35;

                if (editor.transDelFlag)
                {
                    noteSrc->period = 0;
                    noteSrc->sample = 0;

                    noteDeleted = true;
                }
            }

            if (!noteDeleted)
                noteSrc->period = periodTable[j];
        }
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

void trackNoteDown(uint8_t sampleAllFlag, uint8_t from, uint8_t to)
{
    int8_t j;
    uint8_t i, noteDeleted;
    note_t *noteSrc;

    if (from > to)
    {
        j    = from;
        from = to;
        to   = j;
    }

    saveUndo();

    for (i = from; i <= to; ++i)
    {
        noteSrc = &modEntry->patterns[modEntry->currPattern][(i * AMIGA_VOICES) + editor.cursor.channel];

        if (!sampleAllFlag && (noteSrc->sample != (editor.currSample + 1)))
            continue;

        if (noteSrc->period)
        {
            // period -> note
            for (j = 0; j < 36; ++j)
            {
                if (noteSrc->period >= periodTable[j])
                    break;
            }

            noteDeleted = false;
            if (--j < 0)
            {
                j = 0;

                if (editor.transDelFlag)
                {
                    noteSrc->period = 0;
                    noteSrc->sample = 0;

                    noteDeleted = true;
                }
            }

            if (!noteDeleted)
                noteSrc->period = periodTable[j];
        }
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

void trackOctaUp(uint8_t sampleAllFlag, uint8_t from, uint8_t to)
{
    uint8_t i, j, noteDeleted;
    note_t *noteSrc;

    if (from > to)
    {
        j    = from;
        from = to;
        to   = j;
    }

    saveUndo();

    for (i = from; i <= to; ++i)
    {
        noteSrc = &modEntry->patterns[modEntry->currPattern][(i * AMIGA_VOICES) + editor.cursor.channel];

        if (!sampleAllFlag && (noteSrc->sample != (editor.currSample + 1)))
            continue;

        if (noteSrc->period)
        {
            // period -> note
            for (j = 0; j < 36; ++j)
            {
                if (noteSrc->period >= periodTable[j])
                    break;
            }

            noteDeleted = false;
            if (((j + 12) > 35) && editor.transDelFlag)
            {
                noteSrc->period = 0;
                noteSrc->sample = 0;

                noteDeleted = true;
            }

            if (j <= 23)
                j += 12;

            if (!noteDeleted)
                noteSrc->period = periodTable[j];
        }
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

void trackOctaDown(uint8_t sampleAllFlag, uint8_t from, uint8_t to)
{
    int8_t j;
    uint8_t i, noteDeleted;
    note_t *noteSrc;

    if (from > to)
    {
        j    = from;
        from = to;
        to   = j;
    }

    saveUndo();

    for (i = from; i <= to; ++i)
    {
        noteSrc = &modEntry->patterns[modEntry->currPattern][(i * AMIGA_VOICES) + editor.cursor.channel];

        if (!sampleAllFlag && (noteSrc->sample != (editor.currSample + 1)))
            continue;

        if (noteSrc->period)
        {
            // period -> note
            for (j = 0; j < 36; ++j)
            {
                if (noteSrc->period >= periodTable[j])
                    break;
            }

            noteDeleted = false;
            if (((j - 12) < 0) && editor.transDelFlag)
            {
                noteSrc->period = 0;
                noteSrc->sample = 0;

                noteDeleted = true;
            }

            if (j >= 12)
                j -= 12;

            if (!noteDeleted)
                noteSrc->period = periodTable[j];
        }
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

void pattNoteUp(uint8_t sampleAllFlag)
{
    uint8_t i, j, k, noteDeleted;
    note_t *noteSrc;

    saveUndo();

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        for (j = 0; j < MOD_ROWS; ++j)
        {
            noteSrc = &modEntry->patterns[modEntry->currPattern][(j * AMIGA_VOICES) + i];

            if (!sampleAllFlag && (noteSrc->sample != (editor.currSample + 1)))
                continue;

            if (noteSrc->period)
            {
                // period -> note
                for (k = 0; k < 36; ++k)
                {
                    if (noteSrc->period >= periodTable[k])
                        break;
                }

                noteDeleted = false;
                if (++k > 35)
                {
                    k = 35;

                    if (editor.transDelFlag)
                    {
                        noteSrc->period = 0;
                        noteSrc->sample = 0;

                        noteDeleted = true;
                    }
                }

                if (!noteDeleted)
                    noteSrc->period = periodTable[k];
            }
        }
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

void pattNoteDown(uint8_t sampleAllFlag)
{
    int8_t k;
    uint8_t i, j, noteDeleted;
    note_t *noteSrc;

    saveUndo();

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        for (j = 0; j < MOD_ROWS; ++j)
        {
            noteSrc = &modEntry->patterns[modEntry->currPattern][(j * AMIGA_VOICES) + i];

            if (!sampleAllFlag && (noteSrc->sample != (editor.currSample + 1)))
                continue;

            if (noteSrc->period)
            {
                // period -> note
                for (k = 0; k < 36; ++k)
                {
                    if (noteSrc->period >= periodTable[k])
                        break;
                }

                noteDeleted = false;
                if (--k < 0)
                {
                    k = 0;

                    if (editor.transDelFlag)
                    {
                        noteSrc->period = 0;
                        noteSrc->sample = 0;

                        noteDeleted = true;
                    }
                }

                if (!noteDeleted)
                    noteSrc->period = periodTable[k];
            }
        }
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

void pattOctaUp(uint8_t sampleAllFlag)
{
    uint8_t i, j, k, noteDeleted;
    note_t *noteSrc;

    saveUndo();

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        for (j = 0; j < MOD_ROWS; ++j)
        {
            noteSrc = &modEntry->patterns[modEntry->currPattern][(j * AMIGA_VOICES) + i];

            if (!sampleAllFlag && (noteSrc->sample != (editor.currSample + 1)))
                continue;

            if (noteSrc->period)
            {
                // period -> note
                for (k = 0; k < 36; ++k)
                {
                    if (noteSrc->period >= periodTable[k])
                        break;
                }

                noteDeleted = false;
                if (((k + 12) > 35) && editor.transDelFlag)
                {
                    noteSrc->period = 0;
                    noteSrc->sample = 0;

                    noteDeleted = true;
                }

                if (k <= 23)
                    k += 12;

                if (!noteDeleted)
                    noteSrc->period = periodTable[k];
            }
        }
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

void pattOctaDown(uint8_t sampleAllFlag)
{
    int8_t k;
    uint8_t i, j, noteDeleted;
    note_t *noteSrc;

    saveUndo();

    for (i = 0; i < AMIGA_VOICES; ++i)
    {
        for (j = 0; j < MOD_ROWS; ++j)
        {
            noteSrc = &modEntry->patterns[modEntry->currPattern][(j * AMIGA_VOICES) + i];

            if (!sampleAllFlag && (noteSrc->sample != (editor.currSample + 1)))
                continue;

            if (noteSrc->period)
            {
                // period -> note
                for (k = 0; k < 36; ++k)
                {
                    if (noteSrc->period >= periodTable[k])
                        break;
                }

                noteDeleted = false;
                if (((k - 12) < 0) && editor.transDelFlag)
                {
                    noteSrc->period = 0;
                    noteSrc->sample = 0;

                    noteDeleted = true;
                }

                if (k >= 12)
                    k -= 12;

                if (!noteDeleted)
                    noteSrc->period = periodTable[k];
            }
        }
    }

    updateWindowTitle(MOD_IS_MODIFIED);
    editor.ui.updatePatternData = true;
}

int8_t keyToNote(SDL_Keycode keyEntry)
{
    int8_t noteFound;
    uint8_t rawNote, rawKey, i, keyTransition;

    rawNote = 0;
    rawKey  = 0;

    noteFound = false;

    // lookup note by raw key
    for (i = 0; i < sizeof (unshiftedKeymap); ++i)
    {
        if (keyEntry == unshiftedKeymap[i])
        {
            rawKey = i + 1;
            break;
        }
    }

    for (i = 0; i < sizeof (rawKeyScaleTable); ++i)
    {
        if (rawKey == rawKeyScaleTable[i])
        {
            rawNote = i;

            noteFound = true;
            break;
        }
    }

    // was a note found in range?
    if (noteFound)
    {
        keyTransition = kbdTransTable[editor.keyOctave][rawNote];

        if (keyTransition <  36) return (keyTransition);
        if (keyTransition == 37) return (-2); // delete note/sample (only for pattern editing/record)
    }

    return (-1); // not a note key
}
