#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "pt_header.h"
#include "pt_mouse.h"
#include "pt_helpers.h"
#include "pt_palette.h"
#include "pt_diskop.h"
#include "pt_sampler.h"
#include "pt_modloader.h"
#include "pt_edit.h"
#include "pt_sampleloader.h"
#include "pt_visuals.h"
#include "pt_tables.h"
#include "pt_audio.h"
#include "pt_textout.h"
#include "pt_keyboard.h"
#include "pt_terminal.h"

extern SDL_Renderer *renderer;
extern SDL_Window *window;
extern uint8_t fullscreen;

// button tables taken from the ptplay project + modified

static const guiButton_t bAsk[] =
{
    {171, 71,196, 81, PTB_SUREY},
    {234, 71,252, 81, PTB_SUREN}
};

static const guiButton_t bPat2SmpAsk[] =
{
    {168, 71,185, 81, PTB_PAT2SMP_HI},
    {192, 71,210, 81, PTB_PAT2SMP_LO},
    {217, 71,256, 81, PTB_PAT2SMP_ABORT}
};

static const guiButton_t bClear[] =
{
    {166, 57,198, 67, PTB_CLEARSONG},
    {204, 57,257, 67, PTB_CLEARSAMPLES},

    {166, 73,198, 83, PTB_CLEARALL},
    {204, 73,257, 83, PTB_CLEARCANCEL}
};

static const guiButton_t bTopScreen[] =
{
    {  0,  0, 39, 10, PTB_POSED},
    { 40,  0, 50, 10, PTB_POSINS},
    { 51,  0, 61, 10, PTB_POSDEL},
    { 62,  0, 97, 10, PTB_POSS},
    { 98,  0,108, 10, PTB_POSU},
    {109,  0,119, 10, PTB_POSD},
    {120,  0,181, 10, PTB_PLAY},
    {182,  0,243, 10, PTB_STOP},
    {244,  0,305, 10, PTB_MOD2WAV},
    {306,  0,319, 10, PTB_CHAN1},

    { 62, 11, 97, 21, PTB_PATTERNS},
    { 98, 11,108, 21, PTB_PATTERNU},
    {109, 11,119, 21, PTB_PATTERND},
    {120, 11,181, 21, PTB_PATTERN},
    {182, 11,243, 21, PTB_CLEAR},
    {244, 11,305, 21, PTB_PAT2SMP},
    {306, 11,319, 21, PTB_CHAN2},

    { 62, 22, 97, 32, PTB_LENGTHS},
    { 98, 22,108, 32, PTB_LENGTHU},
    {109, 22,119, 32, PTB_LENGTHD},
    {120, 22,181, 32, PTB_EDIT},
    {182, 22,243, 32, PTB_EDITOP},
    {244, 22,305, 32, PTB_POSED},
    {306, 22,319, 32, PTB_CHAN3},

    { 98, 33,108, 43, PTB_FTUNEU},
    {109, 33,119, 43, PTB_FTUNED},
    {120, 33,181, 43, PTB_RECORD},
    {182, 33,243, 43, PTB_DISKOP},
    {244, 33,305, 43, PTB_SAMPLER},
    {306, 33,319, 43, PTB_CHAN4},

    { 62, 44, 97, 54, PTB_SAMPLES},
    { 98, 44,108, 54, PTB_SAMPLEU},
    {109, 44,119, 54, PTB_SAMPLED},
    {306, 44,319, 54, PTB_ABOUT}, // 'about' has priority over PTB_VISUALS
    {120, 44,319, 98, PTB_VISUALS},

    { 62, 55, 97, 65, PTB_SVOLUMES},
    { 98, 55,108, 65, PTB_SVOLUMEU},
    {109, 55,119, 65, PTB_SVOLUMED},

    { 54, 66, 97, 76, PTB_SLENGTHS},
    { 98, 66,108, 76, PTB_SLENGTHU},
    {109, 66,119, 76, PTB_SLENGTHD},

    { 54, 77, 97, 87, PTB_SREPEATS},
    { 98, 77,108, 87, PTB_SREPEATU},
    {109, 77,119, 87, PTB_SREPEATD},

    { 54, 88, 97, 98, PTB_SREPLENS},
    { 98, 88,108, 98, PTB_SREPLENU},
    {109, 88,119, 98, PTB_SREPLEND}
};

static const guiButton_t bMidScreen[] =
{
    {  0, 99,319,109, PTB_SONGNAME},

    {  0,110,286,120, PTB_SAMPLENAME},
    {287,110,319,120, PTB_LOADSAMPLE}
};

static const guiButton_t bBotScreen[] =
{
    {  0,121, 25,137, PTB_PATTBOX},
    { 26,121, 43,137, PTB_TEMPOU},
    { 43,121, 59,137, PTB_TEMPOD},

    {  0,138,319,254, PTB_PATTDATA}
};

static const guiButton_t bDiskOp[] =
{
    {  0,  0, 79, 21, PTB_DO_BADGE},
    { 80,  0,145, 10, PTB_DO_PACKMOD},
    {146,  0,155, 10, PTB_DO_MODARROW},
    {156,  0,237, 10, PTB_DO_LOADMODULE},
    {238,  0,319, 10, PTB_DO_SAVEMODULE},

    { 80, 11,145, 21, PTB_DO_SAMPLEFORMAT},
    {146, 11,155, 21, PTB_DO_SAMPLEARROW},
    {156, 11,237, 21, PTB_DO_LOADSAMPLE},
    {238, 11,319, 21, PTB_DO_SAVESAMPLE},

    {  0, 22,237, 32, PTB_DO_DATAPATH},
    {238, 22,272, 32, PTB_DO_PARENT},
    {273, 22,307, 32, PTB_DO_REFRESH},
    {308, 22,319, 30, PTB_DO_SCROLLTOP},

    {308, 31,319, 39, PTB_DO_SCROLLUP},
    {  2, 34,304, 93, PTB_DO_FILEAREA},
    {308, 40,319, 80, PTB_DO_EXIT},
    {308, 81,319, 89, PTB_DO_SCROLLDOWN},
    {308, 90,319, 99, PTB_DO_SCROLLBOT},
};

static const guiButton_t bPosEd[] =
{
    { 120,  0,171, 10, PTB_POSINS},
    { 172,  0,265, 21, PTB_STOP},
    { 266,  0,319, 10, PTB_PLAY},

    { 120, 11,171, 21, PTB_POSDEL},
    { 266, 11,319, 21, PTB_PATTERN},

    { 120, 22,177, 98, PTB_PE_PATT},
    { 178, 22,307, 98, PTB_PE_PATTNAME},
    { 308, 22,319, 32, PTB_PE_SCROLLTOP},

    { 308, 33,319, 43, PTB_PE_SCROLLUP},
    { 308, 44,319, 76, PTB_PE_EXIT},
    { 308, 77,319, 87, PTB_PE_SCROLLDOWN},
    { 308, 88,319, 98, PTB_PE_SCROLLBOT}
};

static const guiButton_t bEditOp1[] =
{
    {120, 44,319, 54, PTB_EO_TITLEBAR},

    {120, 55,212, 65, PTB_EO_TRACK_NOTE_UP},
    {213, 55,305, 65, PTB_EO_PATT_NOTE_UP},
    {306, 55,319, 65, PTB_DUMMY},

    {120, 66,212, 76, PTB_EO_TRACK_NOTE_DOWN},
    {213, 66,305, 76, PTB_EO_PATT_NOTE_DOWN},
    {306, 66,319, 76, PTB_EO_2},

    {120, 77,212, 87, PTB_EO_TRACK_OCTA_UP},
    {213, 77,305, 87, PTB_EO_PATT_OCTA_UP},
    {306, 77,319, 87, PTB_EO_3},

    {120, 88,212, 98, PTB_EO_TRACK_OCTA_DOWN},
    {213, 88,305, 98, PTB_EO_PATT_OCTA_DOWN},
    {306, 88,319, 98, PTB_EO_EXIT}
};

static const guiButton_t bEditOp2[] =
{
    {120, 44,319, 54, PTB_EO_TITLEBAR},

    {120, 55,212, 65, PTB_EO_RECORD},
    {213, 55,259, 65, PTB_EO_DELETE},
    {260, 55,305, 65, PTB_EO_KILL},
    {306, 55,319, 65, PTB_EO_1},

    {120, 66,212, 76, PTB_EO_QUANTIZE},
    {213, 66,259, 76, PTB_EO_EXCHGE},
    {260, 66,305, 76, PTB_EO_COPY},
    {306, 66,319, 76, PTB_DUMMY},

    {120, 77,188, 87, PTB_EO_METRO_1},
    {189, 77,212, 87, PTB_EO_METRO_2},
    {213, 77,259, 87, PTB_EO_FROM},
    {260, 77,283, 87, PTB_EO_FROM_NUM},
    {284, 77,294, 87, PTB_EO_FROM_UP},
    {295, 77,305, 87, PTB_EO_FROM_DOWN},
    {306, 77,319, 87, PTB_EO_3},

    {120, 88,212, 98, PTB_EO_KEYS},
    {213, 88,259, 98, PTB_EO_TO},
    {260, 88,283, 98, PTB_EO_TO_NUM},
    {284, 88,294, 98, PTB_EO_TO_UP},
    {295, 88,305, 98, PTB_EO_TO_DOWN},
    {306, 88,319, 98, PTB_EO_EXIT},
};

static const guiButton_t bEditOp3[] =
{
    {120, 44,319, 54, PTB_EO_TITLEBAR},

    {120, 55,165, 65, PTB_EO_MIX},
    {166, 55,212, 65, PTB_EO_ECHO},
    {213, 55,237, 65, PTB_DUMMY},
    {238, 55,283, 65, PTB_EO_POS_NUM},
    {284, 55,294, 65, PTB_EO_POS_UP},
    {295, 55,305, 65, PTB_EO_POS_DOWN},
    {306, 55,319, 65, PTB_EO_1},

    {120, 66,165, 76, PTB_EO_BOOST},
    {166, 66,212, 76, PTB_EO_FILTER},
    {213, 66,243, 76, PTB_EO_MOD},
    {244, 66,283, 76, PTB_EO_MOD_NUM},
    {284, 66,294, 76, PTB_EO_MOD_UP},
    {295, 66,305, 76, PTB_EO_MOD_DOWN},
    {306, 66,319, 76, PTB_EO_2},

    {120, 77,165, 87, PTB_EO_X_FADE},
    {166, 77,212, 87, PTB_EO_BACKWD},
    {213, 77,230, 87, PTB_EO_CB},
    {231, 77,269, 87, PTB_EO_CHORD},
    {270, 77,287, 87, PTB_EO_FU},
    {288, 77,305, 87, PTB_EO_FD},
    {306, 77,319, 87, PTB_DUMMY},

    {120, 88,165, 98, PTB_EO_UPSAMP},
    {166, 88,212, 98, PTB_EO_DNSAMP},
    {213, 88,243, 98, PTB_EO_VOL},
    {244, 88,283, 98, PTB_EO_VOL_NUM},
    {284, 88,294, 98, PTB_EO_VOL_UP},
    {295, 88,305, 98, PTB_EO_VOL_DOWN},
    {306, 88,319, 98, PTB_EO_EXIT}
};

static const guiButton_t bEditOp4[] =
{
    {120, 44,319, 54, PTB_EO_TITLEBAR},

    {120, 55,165, 65, PTB_EO_DOCHORD},
    {166, 55,204, 65, PTB_EO_MAJOR},
    {205, 55,251, 65, PTB_EO_MAJOR7},
    {251, 55,283, 65, PTB_EO_NOTE1},
    {284, 55,294, 65, PTB_EO_NOTE1_UP},
    {295, 55,305, 65, PTB_EO_NOTE1_DOWN},
    {306, 55,319, 65, PTB_EO_1},

    {120, 66,165, 76, PTB_EO_RESET},
    {166, 66,204, 76, PTB_EO_MINOR},
    {205, 66,251, 76, PTB_EO_MINOR7},
    {251, 66,283, 76, PTB_EO_NOTE2},
    {284, 66,294, 76, PTB_EO_NOTE2_UP},
    {295, 66,305, 76, PTB_EO_NOTE2_DOWN},
    {306, 66,319, 76, PTB_EO_2},

    {120, 77,165, 87, PTB_EO_UNDO},
    {166, 77,204, 87, PTB_EO_SUS4},
    {205, 77,251, 87, PTB_EO_MAJOR6},
    {251, 77,283, 87, PTB_EO_NOTE3},
    {284, 77,294, 87, PTB_EO_NOTE3_UP},
    {295, 77,305, 87, PTB_EO_NOTE3_DOWN},
    {306, 77,319, 87, PTB_EO_3},

    {120, 88,157, 98, PTB_EO_LENGTH},
    {158, 88,204, 98, PTB_DUMMY},
    {205, 88,251, 98, PTB_EO_MINOR6},
    {251, 88,283, 98, PTB_EO_NOTE4},
    {284, 88,294, 98, PTB_EO_NOTE4_UP},
    {295, 88,305, 98, PTB_EO_NOTE4_DOWN},
    {306, 88,319, 98, PTB_EO_EXIT}
};

static const guiButton_t bSampler[] =
{
    {  6,124, 25,134, PTB_SA_EXIT},
    {  0,138,319,201, PTB_SA_SAMPLEAREA},
    {  3,205,316,210, PTB_SA_ZOOMBARAREA},

    { 32,211, 95,221, PTB_SA_PLAYWAVE},
    { 96,211,175,221, PTB_SA_SHOWRANGE},
    {176,211,245,221, PTB_SA_ZOOMOUT},

    {  0,222, 30,243, PTB_SA_STOP},
    { 32,222, 95,232, PTB_SA_PLAYDISPLAYED},
    { 96,222,175,232, PTB_SA_SHOWALL},
    {176,222,245,232, PTB_SA_RANGEALL},
    {246,222,319,232, PTB_SA_LOOP},

    { 32,233, 94,243, PTB_SA_PLAYRANGE},
    { 96,233,115,243, PTB_SA_RANGEBEG},
    {116,233,135,243, PTB_SA_RANGEEND},
    {136,233,174,243, PTB_SA_RANGECENTER},
    {176,233,245,243, PTB_SA_RESAMPLE},
    {246,233,319,243, PTB_SA_RESAMPLENOTE},

    {  0,244, 31,254, PTB_SA_CUT},
    { 32,244, 63,254, PTB_SA_COPY},
    { 64,244, 95,254, PTB_SA_PASTE},
    { 96,244,135,254, PTB_SA_VOLUME},
    {136,244,175,254, PTB_SA_TUNETONE},
    {176,244,210,254, PTB_SA_FIXDC},
    {211,244,245,254, PTB_SA_FILTERS}
};

static const guiButton_t bTerminal[] =
{
    {  0,  0, 39, 10, PTB_TERM_CLEAR},
    {309,  0,319, 10, PTB_TERM_EXIT},

    {309, 11,319, 22, PTB_TERM_SCROLL_UP},
    {311, 24,317,229, PTB_TERM_SCROLL_BAR},
    {309,232,319,243, PTB_TERM_SCROLL_DOWN}
};

// MODIFY THESE EVERY TIME YOU REMOVE/ADD A BUTTON!
#define ASK_BUTTONS         2
#define PAT2SMP_ASK_BUTTONS 3
#define CLEAR_BUTTONS       4
#define TOPSCREEN_BUTTONS   47
#define MIDSCREEN_BUTTONS   3
#define BOTSCREEN_BUTTONS   4
#define DISKOP_BUTTONS      18
#define POSED_BUTTONS       12
#define EDITOP1_BUTTONS     13
#define EDITOP2_BUTTONS     22
#define EDITOP3_BUTTONS     29
#define EDITOP4_BUTTONS     29
#define SAMPLER_BUTTONS     24
#define TERMINAL_BUTTONS    5
// -----------------------------------------------

void edNote1UpButton(void);
void edNote1DownButton(void);
void edNote2UpButton(void);
void edNote2DownButton(void);
void edNote3UpButton(void);
void edNote3DownButton(void);
void edNote4UpButton(void);
void edNote4DownButton(void);
void edPosUpButton(int8_t fast);
void edPosDownButton(int8_t fast);
void edModUpButton(void);
void edModDownButton(void);
void edVolUpButton(void);
void edVolDownButton(void);
void sampleUpButton(void);
void sampleDownButton(void);
void sampleFineTuneUpButton(void);
void sampleFineTuneDownButton(void);
void sampleVolumeUpButton(void);
void sampleVolumeDownButton(void);
void sampleLengthUpButton(int8_t fast);
void sampleLengthDownButton(int8_t fast);
void sampleRepeatUpButton(int8_t fast);
void sampleRepeatDownButton(int8_t fast);
void sampleRepeatLengthUpButton(int8_t fast);
void sampleRepeatLengthDownButton(int8_t fast);
void tempoUpButton(void);
void tempoDownButton(void);
void songLengthUpButton(void);
void songLengthDownButton(void);
void patternUpButton(void);
void patternDownButton(void);
void positionUpButton(void);
void positionDownButton(void);
void handleSamplerVolumeBox(void);

int32_t checkGUIButtons(void);
void handleTextEditing(uint8_t mouseButton);
int8_t handleRightMouseButton(void);
int8_t handleLeftMouseButton(void);

void updateMouseScaling(void)
{
    float scaleX_f, scaleY_f;

    SDL_RenderGetScale(renderer, &scaleX_f, &scaleY_f);

    if (scaleX_f == 0.0f) scaleX_f = 1.0f;
    if (scaleY_f == 0.0f) scaleY_f = 1.0f;

    input.mouse.scaleX_f = scaleX_f;
    input.mouse.scaleY_f = scaleY_f;
}

void updateMousePos(void)
{
    int32_t mx, my;
    float mx_f, my_f;

    SDL_PumpEvents(); // force input read right now

    SDL_GetMouseState(&mx, &my);

    mx_f = mx / input.mouse.scaleX_f;
    my_f = my / input.mouse.scaleY_f;

    mx = (int32_t)(mx_f + 0.5f);
    my = (int32_t)(my_f + 0.5f);

    // clamp to edges
    mx = CLAMP(mx, 0, SCREEN_W - 1);
    my = CLAMP(my, 0, SCREEN_H - 1);

    setSpritePos(SPRITE_MOUSE_POINTER, mx, my);

    input.mouse.x = mx;
    input.mouse.y = my;
}

void mouseButtonUpHandler(uint8_t mouseButton)
{
#ifndef __APPLE__
    if (!fullscreen)
        SDL_SetWindowGrab(window, SDL_FALSE);
#endif

    input.mouse.buttonWaitCounter = 0;
    input.mouse.buttonWaiting = false;

    if (mouseButton == SDL_BUTTON_LEFT)
    {
        input.mouse.leftButtonPressed = false;
        editor.ui.forceTermBarDrag    = false;
        editor.ui.forceSampleDrag     = false;
        editor.ui.forceVolDrag        = false;
        editor.ui.leftLoopPinMoving   = false;
        editor.ui.rightLoopPinMoving  = false;
        editor.ui.sampleMarkingPos    = -1;

        switch (input.mouse.lastGUIButton)
        {
            case PTB_SLENGTHU:
            case PTB_SLENGTHD:
            {
                if (editor.ui.samplerScreenShown)
                    redrawSample();

                updateVoiceParams();
                recalcChordLength();
                updateSamplePos();

                editor.ui.updateSongSize = true;
            }
            break;

            case PTB_LENGTHU:
            case PTB_LENGTHD:
            case PTB_PATTERNU:
            case PTB_PATTERND:
            {
                editor.ui.updateSongSize = true;

                if (editor.ui.posEdScreenShown)
                    editor.ui.updatePosEd = true;
            }
            break;

            default:
                break;
        }

        input.mouse.lastGUIButton   = -1;
        input.mouse.lastGUIButton_2 = -1;
    }

    if (mouseButton == SDL_BUTTON_RIGHT)
    {
        input.mouse.rightButtonPressed = false;
        editor.ui.forceSampleEdit      = false;
    }
}

void mouseButtonDownHandler(uint8_t mouseButton)
{
#ifndef __APPLE__
    if (!fullscreen)
        SDL_SetWindowGrab(window, SDL_TRUE);
#endif

    if (mouseButton == SDL_BUTTON_LEFT)
    {
        input.mouse.leftButtonPressed = true;
        input.mouse.buttonWaiting     = true;
    }

    if (mouseButton == SDL_BUTTON_RIGHT)
        input.mouse.rightButtonPressed = true;

    // when red mouse pointer (error), block further input for a while
    if (editor.errorMsgActive && editor.errorMsgBlock)
        return;

    if (handleRightMouseButton())
        return;

    if (handleLeftMouseButton())
        return;

    handleTextEditing(mouseButton);
}

void handleMouseButtons(void)
{
    if (!input.mouse.leftButtonPressed)
    {
        // left mouse button released, stop repeating buttons
        input.mouse.repeatCounter = 0;
        return;
    }

    if (input.mouse.lastGUIButton != checkGUIButtons())
    {
        // only repeat the button that was first clicked (e.g. if you hold and move mouse to another button)
        input.mouse.repeatCounter = 0;
        return;
    }

    // repeat button
    switch (input.mouse.lastGUIButton)
    {
        case PTB_EO_NOTE1_UP:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;
                edNote1UpButton();
            }
        }
        break;

        case PTB_EO_NOTE1_DOWN:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;
                edNote1DownButton();
            }
        }
        break;

        case PTB_EO_NOTE2_UP:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;
                edNote2UpButton();
            }
        }
        break;

        case PTB_EO_NOTE2_DOWN:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;
                edNote2DownButton();
            }
        }
        break;

        case PTB_EO_NOTE3_UP:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;
                edNote3UpButton();
            }
        }
        break;

        case PTB_EO_NOTE3_DOWN:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;
                edNote3DownButton();
            }
        }
        break;

        case PTB_EO_NOTE4_UP:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;
                edNote4UpButton();
            }
        }
        break;

        case PTB_EO_NOTE4_DOWN:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;
                edNote4DownButton();
            }
        }
        break;

        case PTB_EO_VOL_UP:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;
                edVolUpButton();
            }
        }
        break;

        case PTB_EO_VOL_DOWN:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;
                edVolDownButton();
            }
        }
        break;

        case PTB_EO_MOD_UP:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;
                edModUpButton();
            }
        }
        break;

        case PTB_EO_MOD_DOWN:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;
                edModDownButton();
            }
        }
        break;

        case PTB_EO_POS_UP:
        {
            if (input.mouse.repeatCounter >= 1)
            {
                input.mouse.repeatCounter = 0;
                edPosUpButton(INCREMENT_FAST);
            }
        }
        break;

        case PTB_EO_POS_DOWN:
        {
            if (input.mouse.repeatCounter >= 1)
            {
                input.mouse.repeatCounter = 0;
                edPosDownButton(INCREMENT_FAST);
            }
        }
        break;

        case PTB_EO_FROM_UP:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;

                if (editor.sampleFrom < 0x1F)
                {
                    editor.sampleFrom++;
                    editor.ui.updateFromText = true;
                }
            }
        }
        break;

        case PTB_EO_FROM_DOWN:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;

                if (editor.sampleFrom > 0x00)
                {
                    editor.sampleFrom--;
                    editor.ui.updateFromText = true;
                }
            }
        }
        break;

        case PTB_EO_TO_UP:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;

                if (editor.sampleTo < 0x1F)
                {
                    editor.sampleTo++;
                    editor.ui.updateToText = true;
                }
            }
        }
        break;

        case PTB_EO_TO_DOWN:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;

                if (editor.sampleTo > 0x00)
                {
                    editor.sampleTo--;
                    editor.ui.updateToText = true;
                }
            }
        }
        break;

        case PTB_SAMPLEU:
        {
            if (input.mouse.repeatCounter >= 5)
            {
                input.mouse.repeatCounter = 0;

                if (!input.mouse.rightButtonPressed)
                    sampleUpButton();
                else
                    editor.ui.updateCurrSampleNum = true;
            }
        }
        break;

        case PTB_SAMPLED:
        {
            if (input.mouse.repeatCounter >= 5)
            {
                input.mouse.repeatCounter = 0;

                if (!input.mouse.rightButtonPressed)
                    sampleDownButton();
                else
                    editor.ui.updateCurrSampleNum = true;
            }
        }
        break;

        case PTB_FTUNEU:
        {
            if (input.mouse.repeatCounter >= 5)
            {
                input.mouse.repeatCounter = 0;
                sampleFineTuneUpButton();
            }
        }
        break;

        case PTB_FTUNED:
        {
            if (input.mouse.repeatCounter >= 5)
            {
                input.mouse.repeatCounter = 0;
                sampleFineTuneDownButton();
            }
        }
        break;

        case PTB_SVOLUMEU:
        {
            if (input.mouse.repeatCounter >= 5)
            {
                input.mouse.repeatCounter = 0;
                sampleVolumeUpButton();
            }
        }
        break;

        case PTB_SVOLUMED:
        {
            if (input.mouse.repeatCounter >= 5)
            {
                input.mouse.repeatCounter = 0;
                sampleVolumeDownButton();
            }
        }
        break;

        case PTB_SLENGTHU:
        {
            if (input.mouse.rightButtonPressed || (input.mouse.repeatCounter >= 1))
            {
                input.mouse.repeatCounter = 0;
                sampleLengthUpButton(INCREMENT_FAST);
            }
        }
        break;

        case PTB_SLENGTHD:
        {
            if (input.mouse.rightButtonPressed || (input.mouse.repeatCounter >= 1))
            {
                input.mouse.repeatCounter = 0;
                sampleLengthDownButton(INCREMENT_FAST);
            }
        }
        break;

        case PTB_SREPEATU:
        {
            if (input.mouse.rightButtonPressed || (input.mouse.repeatCounter >= 1))
            {
                input.mouse.repeatCounter = 0;
                sampleRepeatUpButton(INCREMENT_FAST);
            }
        }
        break;

        case PTB_SREPEATD:
        {
            if (input.mouse.rightButtonPressed || (input.mouse.repeatCounter >= 1))
            {
                input.mouse.repeatCounter = 0;
                sampleRepeatDownButton(INCREMENT_FAST);
            }
        }
        break;

        case PTB_SREPLENU:
        {
            if (input.mouse.rightButtonPressed || (input.mouse.repeatCounter >= 1))
            {
                input.mouse.repeatCounter = 0;
                sampleRepeatLengthUpButton(INCREMENT_FAST);
            }
        }
        break;

        case PTB_SREPLEND:
        {
            if (input.mouse.rightButtonPressed || (input.mouse.repeatCounter >= 1))
            {
                input.mouse.repeatCounter = 0;
                sampleRepeatLengthDownButton(INCREMENT_FAST);
            }
        }
        break;

        case PTB_TEMPOU:
        {
            if (input.mouse.repeatCounter >= 3)
            {
                input.mouse.repeatCounter = 0;
                tempoUpButton();
            }
        }
        break;

        case PTB_TEMPOD:
        {
            if (input.mouse.repeatCounter >= 3)
            {
                input.mouse.repeatCounter = 0;
                tempoDownButton();
            }
        }
        break;

        case PTB_LENGTHU:
        {
            if (input.mouse.repeatCounter >= 7)
            {
                input.mouse.repeatCounter = 0;
                songLengthUpButton();
            }
        }
        break;

        case PTB_LENGTHD:
        {
            if (input.mouse.repeatCounter >= 7)
            {
                input.mouse.repeatCounter = 0;
                songLengthDownButton();
            }
        }
        break;

        case PTB_PATTERNU:
        {
            if (input.mouse.repeatCounter >= 7)
            {
                input.mouse.repeatCounter = 0;
                patternUpButton();
            }
        }
        break;

        case PTB_PATTERND:
        {
            if (input.mouse.repeatCounter >= 7)
            {
                input.mouse.repeatCounter = 0;
                patternDownButton();
            }
        }
        break;

        case PTB_POSU:
        {
            if (input.mouse.repeatCounter >= 7)
            {
                input.mouse.repeatCounter = 0;
                positionUpButton();
            }
        }
        break;

        case PTB_POSD:
        {
            if (input.mouse.repeatCounter >= 7)
            {
                input.mouse.repeatCounter = 0;
                positionDownButton();
            }
        }
        break;

        case PTB_PE_SCROLLUP:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;

                if (modEntry->currOrder > 0)
                    modSetPos(modEntry->currOrder - 1, DONT_SET_ROW);
            }
        }
        break;

        case PTB_PE_SCROLLDOWN:
        {
            if (input.mouse.repeatCounter >= 2)
            {
                input.mouse.repeatCounter = 0;

                if (modEntry->currOrder < (modEntry->head.orderCount - 1))
                    modSetPos(modEntry->currOrder + 1, DONT_SET_ROW);
            }
        }
        break;

        case PTB_DO_SCROLLUP:
        {
            if (input.mouse.repeatCounter >= 1)
            {
                input.mouse.repeatCounter = 0;

                editor.diskop.scrollOffset--;

                if (input.mouse.rightButtonPressed)
                    editor.diskop.scrollOffset -= 3;

                if (editor.diskop.scrollOffset < 0)
                    editor.diskop.scrollOffset = 0;

                editor.ui.updateDiskOpFileList = true;
            }
        }
        break;

        case PTB_DO_SCROLLDOWN:
        {
            if (input.mouse.repeatCounter >= 1)
            {
                input.mouse.repeatCounter = 0;

                if (editor.diskop.numFiles > DISKOP_LIST_SIZE)
                {
                    editor.diskop.scrollOffset++;

                    if (input.mouse.rightButtonPressed)
                        editor.diskop.scrollOffset += 3;

                    if (editor.diskop.scrollOffset > (editor.diskop.numFiles - DISKOP_LIST_SIZE))
                        editor.diskop.scrollOffset =  editor.diskop.numFiles - DISKOP_LIST_SIZE;

                    editor.ui.updateDiskOpFileList = true;
                }
            }
        }
        break;

        case PTB_SA_ZOOMBARAREA:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;

                if (!editor.ui.forceSampleDrag)
                    samplerBarPressed(MOUSE_BUTTON_NOT_HELD);
            }
        }
        break;

        case PTB_TERM_SCROLL_UP:
        {
            if (input.mouse.repeatCounter >= 1)
            {
                input.mouse.repeatCounter = 0;

                terminalScrollUp();
            }
        }
        break;

        case PTB_TERM_SCROLL_BAR:
        {
            if (input.mouse.repeatCounter >= 4)
            {
                input.mouse.repeatCounter = 0;

                if (!editor.ui.forceTermBarDrag)
                    terminalHandleScrollBar(MOUSE_BUTTON_NOT_HELD);
            }
        }
        break;

        case PTB_TERM_SCROLL_DOWN:
        {
            if (input.mouse.repeatCounter >= 1)
            {
                input.mouse.repeatCounter = 0;

                terminalScrollDown();
            }
        }
        break;

        default: break;
    }

    input.mouse.repeatCounter++;
}

void edNote1UpButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.note1 += 12;
    else
        editor.note1++;

    if (editor.note1 > 36)
        editor.note1 = 36;

    editor.ui.updateNote1Text = true;
    recalcChordLength();
}

void edNote1DownButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.note1 -= 12;
    else
        editor.note1--;

    if (editor.note1 < 0)
        editor.note1 = 0;

    editor.ui.updateNote1Text = true;
    recalcChordLength();
}

void edNote2UpButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.note2 += 12;
    else
        editor.note2++;

    if (editor.note2 > 36)
        editor.note2 = 36;

    editor.ui.updateNote2Text = true;
    recalcChordLength();
}

void edNote2DownButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.note2 -= 12;
    else
        editor.note2--;

    if (editor.note2 < 0)
        editor.note2 = 0;

    editor.ui.updateNote2Text = true;
    recalcChordLength();
}

void edNote3UpButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.note3 += 12;
    else
        editor.note3++;

    if (editor.note3 > 36)
        editor.note3 = 36;

    editor.ui.updateNote3Text = true;
    recalcChordLength();
}

void edNote3DownButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.note3 -= 12;
    else
        editor.note3--;

    if (editor.note3 < 0)
        editor.note3 = 0;

    editor.ui.updateNote3Text = true;
    recalcChordLength();
}

void edNote4UpButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.note4 += 12;
    else
        editor.note4++;

    if (editor.note4 > 36)
        editor.note4 = 36;

    editor.ui.updateNote4Text = true;
    recalcChordLength();
}

void edNote4DownButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.note4 -= 12;
    else
        editor.note4--;

    if (editor.note4 < 0)
        editor.note4 = 0;

    editor.ui.updateNote4Text = true;
    recalcChordLength();
}

void edPosUpButton(int8_t fast)
{
    if (input.mouse.rightButtonPressed)
    {
        if (fast)
            editor.samplePos += 544; // 50Hz/60Hz scaled value
        else
            editor.samplePos += 16;
    }
    else
    {
        if (fast)
            editor.samplePos += 37; // 50Hz/60Hz scaled value
        else
            editor.samplePos++;
    }

    if (editor.samplePos > modEntry->samples[editor.currSample].length)
        editor.samplePos = modEntry->samples[editor.currSample].length;

    editor.ui.updatePosText = true;
}

void edPosDownButton(int8_t fast)
{
    if (input.mouse.rightButtonPressed)
    {
        if (fast)
            editor.samplePos -= 544; // 50Hz/60Hz scaled value
        else
            editor.samplePos -= 16;
    }
    else
    {
        if (fast)
            editor.samplePos -= 37; // 50Hz/60Hz scaled value
        else
            editor.samplePos--;
    }

    if (editor.samplePos < 0)
        editor.samplePos = 0;

    editor.ui.updatePosText = true;
}

void edModUpButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.modulateSpeed += 10;
    else
        editor.modulateSpeed++;

    if (editor.modulateSpeed > 127)
        editor.modulateSpeed = 127;

    editor.ui.updateModText = true;
}

void edModDownButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.modulateSpeed -= 10;
    else
        editor.modulateSpeed--;

    if (editor.modulateSpeed < -128)
        editor.modulateSpeed = -128;

    editor.ui.updateModText = true;
}

void edVolUpButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.sampleVol += 10;
    else
        editor.sampleVol++;

    if (editor.sampleVol > 999)
        editor.sampleVol = 999;

    editor.ui.updateVolText = true;
}

void edVolDownButton(void)
{
    if (input.mouse.rightButtonPressed)
        editor.sampleVol -= 10;
    else
        editor.sampleVol--;

    if (editor.sampleVol < 0)
        editor.sampleVol = 0;

    editor.ui.updateVolText = true;
}

void sampleUpButton(void)
{
    if (editor.sampleZero)
    {
        editor.sampleZero    = false;
        editor.currSample = 0;
    }
    else if (editor.currSample < 30)
    {
        editor.currSample++;
    }

    updateCurrSample();
}

void sampleDownButton(void)
{
    if (!editor.sampleZero && (editor.currSample > 0))
    {
        editor.currSample--;
        updateCurrSample();
    }
}

void sampleFineTuneUpButton(void)
{
    int8_t finetune;

    finetune = modEntry->samples[editor.currSample].fineTune & 0x0F;
    if (finetune != 7)
        modEntry->samples[editor.currSample].fineTune = (finetune + 1) & 0x0F;

    if (input.mouse.rightButtonPressed)
        modEntry->samples[editor.currSample].fineTune = 0;

    recalcChordLength();
    editor.ui.updateCurrSampleFineTune = true;
}

void sampleFineTuneDownButton(void)
{
    int8_t finetune;

    finetune = modEntry->samples[editor.currSample].fineTune & 0x0F;
    if (finetune != 8)
        modEntry->samples[editor.currSample].fineTune = (finetune - 1) & 0x0F;

    if (input.mouse.rightButtonPressed)
        modEntry->samples[editor.currSample].fineTune = 0;

    recalcChordLength();
    editor.ui.updateCurrSampleFineTune = true;
}

void sampleVolumeUpButton(void)
{
    int8_t val;

    val = modEntry->samples[editor.currSample].volume;

    if (input.mouse.rightButtonPressed)
        val += 16;
    else
        val++;

    if (val > 64)
        val = 64;

    modEntry->samples[editor.currSample].volume = (uint8_t)(val);
    editor.ui.updateCurrSampleVolume = true;
}

void sampleVolumeDownButton(void)
{
    int8_t val;

    val = modEntry->samples[editor.currSample].volume;

    if (input.mouse.rightButtonPressed)
        val -= 16;
    else
        val--;

    if (val < 0)
        val = 0;

    modEntry->samples[editor.currSample].volume = (uint8_t)(val);
    editor.ui.updateCurrSampleVolume = true;
}

void sampleLengthUpButton(int8_t fast)
{
    int32_t val;

    mixerKillVoiceIfReadingSample(editor.currSample);

    val = modEntry->samples[editor.currSample].length;

    if (input.mouse.rightButtonPressed)
    {
        if (fast)
            val += 64;
        else
            val += 16;
    }
    else
    {
        if (fast)
            val += 10;
        else
            val += 2;
    }

    if (val > MAX_SAMPLE_LEN)
        val = MAX_SAMPLE_LEN;

    testTempLoopPoints(val);
    modEntry->samples[editor.currSample].length = val;
    editor.ui.updateCurrSampleLength = true;
}

void sampleLengthDownButton(int8_t fast)
{
    int32_t val;
    moduleSample_t *s;

    mixerKillVoiceIfReadingSample(editor.currSample);

    val = modEntry->samples[editor.currSample].length;

    if (input.mouse.rightButtonPressed)
    {
        if (fast)
            val -= 64;
        else
            val -= 16;
    }
    else
    {
        if (fast)
            val -= 10;
        else
            val -= 2;
    }

    if (val < 0)
        val = 0;

    s = &modEntry->samples[editor.currSample];

    s->length = val;
    if ((s->loopLength > 2) || (s->loopStart > 0))
    {
        if (s->length < (s->loopStart + s->loopLength))
            s->length =  s->loopStart + s->loopLength;
    }

    testTempLoopPoints(s->length);
    editor.ui.updateCurrSampleLength = true;
}

void sampleRepeatUpButton(int8_t fast)
{
    int32_t val, loopLen, len;

    val     = modEntry->samples[editor.currSample].loopStart;
    loopLen = modEntry->samples[editor.currSample].loopLength;
    len     = modEntry->samples[editor.currSample].length;

    if (len == 0)
    {
        modEntry->samples[editor.currSample].loopStart = 0;

        return;
    }

    if (input.mouse.rightButtonPressed)
    {
        if (fast)
            val += 64;
        else
            val += 16;
    }
    else
    {
        if (fast)
            val += 10;
        else
            val += 2;
    }

    if (val > (len - loopLen))
        val =  len - loopLen;

    modEntry->samples[editor.currSample].loopStart = val;
    editor.ui.updateCurrSampleRepeat = true;

    updateVoiceParams();

    if (editor.ui.samplerScreenShown)
        setLoopSprites();

    if (editor.ui.editOpScreenShown && (editor.ui.editOpScreen == 3)) // sample chord editor
        editor.ui.updateLengthText = true;
}

void sampleRepeatDownButton(int8_t fast)
{
    int32_t val, len;

    val = modEntry->samples[editor.currSample].loopStart;
    len = modEntry->samples[editor.currSample].length;

    if (len == 0)
    {
        modEntry->samples[editor.currSample].loopStart = 0;

        return;
    }

    if (input.mouse.rightButtonPressed)
    {
        if (fast)
            val -= 64;
        else
            val -= 16;
    }
    else
    {
        if (fast)
            val -= 10;
        else
            val -= 2;
    }

    if (val < 0)
        val = 0;

    modEntry->samples[editor.currSample].loopStart = val;
    editor.ui.updateCurrSampleRepeat = true;

    updateVoiceParams();

    if (editor.ui.samplerScreenShown)
        setLoopSprites();

    if (editor.ui.editOpScreenShown && (editor.ui.editOpScreen == 3)) // sample chord editor
        editor.ui.updateLengthText = true;
}

void sampleRepeatLengthUpButton(int8_t fast)
{
    int32_t val, loopStart, len;

    val       = modEntry->samples[editor.currSample].loopLength;
    loopStart = modEntry->samples[editor.currSample].loopStart;
    len       = modEntry->samples[editor.currSample].length;

    if (len == 0)
    {
        modEntry->samples[editor.currSample].loopLength = 2;

        return;
    }

    if (input.mouse.rightButtonPressed)
    {
        if (fast)
            val += 64;
        else
            val += 16;
    }
    else
    {
        if (fast)
            val += 10;
        else
            val += 2;
    }

    if (val > (len - loopStart))
        val =  len - loopStart;

    modEntry->samples[editor.currSample].loopLength = val;
    editor.ui.updateCurrSampleReplen = true;

    updateVoiceParams();

    if (editor.ui.samplerScreenShown)
        setLoopSprites();

    if (editor.ui.editOpScreenShown && (editor.ui.editOpScreen == 3)) // sample chord editor
        editor.ui.updateLengthText = true;
}

void sampleRepeatLengthDownButton(int8_t fast)
{
    int32_t val, len;

    val = modEntry->samples[editor.currSample].loopLength;
    len = modEntry->samples[editor.currSample].length;

    if (len == 0)
    {
        modEntry->samples[editor.currSample].loopLength = 2;
        return;
    }

    if (input.mouse.rightButtonPressed)
    {
        if (fast)
            val -= 64;
        else
            val -= 16;
    }
    else
    {
        if (fast)
            val -= 10;
        else
            val -= 2;
    }

    if (val < 2)
        val = 2;

    modEntry->samples[editor.currSample].loopLength = val;
    editor.ui.updateCurrSampleReplen = true;

    updateVoiceParams();

    if (editor.ui.samplerScreenShown)
        setLoopSprites();

    if (editor.ui.editOpScreenShown && (editor.ui.editOpScreen == 3)) // sample chord editor
        editor.ui.updateLengthText = true;
}

void tempoUpButton(void)
{
    int16_t val;

    if (editor.timingMode == TEMPO_MODE_VBLANK)
        return;

    val = modEntry->currBPM;

    if (input.mouse.rightButtonPressed)
        val += 10;
    else
        val++;

    if (val > 255)
        val = 255;

    modEntry->currBPM = val;
    modSetTempo(modEntry->currBPM);
    editor.ui.updateSongBPM = true;
}

void tempoDownButton(void)
{
    int16_t val;

    if (editor.timingMode == TEMPO_MODE_VBLANK)
        return;

    val = modEntry->currBPM;

    if (input.mouse.rightButtonPressed)
        val -= 10;
    else
        val--;

    if (val < 32)
        val = 32;

    modEntry->currBPM = val;
    modSetTempo(modEntry->currBPM);
    editor.ui.updateSongBPM = true;
}

void songLengthUpButton(void)
{
    int16_t val;

    val = modEntry->head.orderCount;

    if (input.mouse.rightButtonPressed)
        val += 10;
    else
        val++;

    if (val > 127)
        val = 127;

    modEntry->head.orderCount = (uint8_t)(val);

    val = modEntry->currOrder;
    if (val > (modEntry->head.orderCount - 1))
        val =  modEntry->head.orderCount - 1;

    editor.currPosEdPattDisp = &modEntry->head.order[val];
    editor.ui.updateSongLength = true;
}

void songLengthDownButton(void)
{
    int16_t val;

    val = modEntry->head.orderCount;

    if (input.mouse.rightButtonPressed)
        val -= 10;
    else
        val--;

    if (val < 1)
        val = 1;

    modEntry->head.orderCount = (uint8_t)(val);

    val = modEntry->currOrder;
    if (val > (modEntry->head.orderCount - 1))
        val =  modEntry->head.orderCount - 1;

    editor.currPosEdPattDisp = &modEntry->head.order[val];
    editor.ui.updateSongLength = true;
}

void patternUpButton(void)
{
    int16_t val;

    val = modEntry->head.order[modEntry->currOrder];

    if (input.mouse.rightButtonPressed)
        val += 10;
    else
        val++;

    if (val > (MAX_PATTERNS - 1))
        val =  MAX_PATTERNS - 1;

    modEntry->head.order[modEntry->currOrder] = (uint8_t)(val);

    if (editor.ui.posEdScreenShown)
        editor.ui.updatePosEd = true;

    editor.ui.updateSongPattern = true;
}

void patternDownButton(void)
{
    int16_t val;

    val = modEntry->head.order[modEntry->currOrder];

    if (input.mouse.rightButtonPressed)
        val -= 10;
    else
        val--;

    if (val < 0)
        val = 0;

    modEntry->head.order[modEntry->currOrder] = (uint8_t)(val);

    if (editor.ui.posEdScreenShown)
        editor.ui.updatePosEd = true;

    editor.ui.updateSongPattern = true;
}

void positionUpButton(void)
{
    int16_t val;

    val = modEntry->currOrder;

    if (input.mouse.rightButtonPressed)
        val += 10;
    else
        val++;

    if (val > 126)
        val = 126;

    modSetPos(val, DONT_SET_ROW);
}

void positionDownButton(void)
{
    int16_t val;

    val = modEntry->currOrder;

    if (input.mouse.rightButtonPressed)
        val -= 10;
    else
        val--;

    if (val < 0)
        val = 0;

    modSetPos(val, DONT_SET_ROW);
}

void diskOpLoadFile(uint32_t fileEntryRow)
{
    char *filePath;
    module_t *tempMod;

    // if we clicked on an empty space, return...
    if (diskOpEntryIsEmpty(fileEntryRow))
        return;

    if (diskOpEntryIsDir(fileEntryRow))
    {
        diskOpSetPath(diskOpGetEntry(fileEntryRow), DISKOP_CACHE);
    }
    else
    {
        filePath = diskOpGetEntry(fileEntryRow);

        if (filePath != NULL)
        {
            if (editor.diskop.mode == DISKOP_MODE_MOD)
            {
                tempMod = modLoad(filePath);
                if (tempMod != NULL)
                {
                    modStop();
                    modFree();

                    modEntry = tempMod;
                    setupNewMod();
                    modEntry->moduleLoaded = true;

                    editor.currMode = MODE_IDLE;
                    pointerSetMode(POINTER_MODE_IDLE, DO_CARRY);
                    setStatusMessage(editor.allRightText, DO_CARRY);

                    editor.ui.diskOpScreenShown = false;
                    displayMainScreen();
                }
                else
                {
                    editor.errorMsgActive  = true;
                    editor.errorMsgBlock   = true;
                    editor.errorMsgCounter = 0;

                    // status/error message is set in the mod loader
                    pointerErrorMode();
                }
            }
            else if (editor.diskop.mode == DISKOP_MODE_SMP)
            {
                loadSample(filePath, diskOpGetEntry(fileEntryRow));
            }
        }
    }
}

void handleSamplerVolumeBox(void)
{
    int8_t *sampleData;
    uint8_t i;
    int16_t sample, sampleVol;
    int32_t sampleIndex, sampleLength;
    double smp;
    moduleSample_t *s;

    if (input.mouse.rightButtonPressed)
    {
        if (editor.ui.getLineFlag)
        {
            exitGetTextLine(EDIT_TEXT_NO_UPDATE);
        }
        else
        {
            editor.ui.samplerVolBoxShown = false;
            removeSamplerVolBox();
        }

        return;
    }

    if (editor.ui.getLineFlag)
        return;

    // check buttons
    if (input.mouse.leftButtonPressed)
    {
        // restore sample ask dialog
        if (editor.ui.askScreenShown && (editor.ui.askScreenType == ASK_RESTORE_SAMPLE))
        {
            if ((input.mouse.y >= 71) && (input.mouse.y <= 81))
            {
                if ((input.mouse.x >= 171) && (input.mouse.x <= 196))
                {
                    // YES button

                    editor.ui.askScreenShown = false;

                    editor.ui.answerNo  = false;
                    editor.ui.answerYes = true;

                    handleAskYes();
                }
                else if ((input.mouse.x >= 234) && (input.mouse.x <= 252))
                {
                    // NO button

                    editor.ui.askScreenShown = false;

                    editor.ui.answerNo  = true;
                    editor.ui.answerYes = false;

                    handleAskNo();
                }
            }

            return;
        }

        // MAIN SCREEN STOP
        if (!editor.ui.diskOpScreenShown)
        {
            if (!editor.ui.posEdScreenShown)
            {
                if ((input.mouse.x >= 182) && (input.mouse.x <= 243) &&
                    (input.mouse.y >=   0) && (input.mouse.y <=  10))
                {
                    modStop();
                    return;
                }
            }
        }

        // SAMPLER SCREEN PLAY WAVEFORM
        if ((input.mouse.x >=  32) && (input.mouse.x <=  95) &&
            (input.mouse.y >= 211) && (input.mouse.y <= 221))
        {
            samplerPlayWaveform();

            return;
        }

        // SAMPLER SCREEN PLAY DISPLAY
        if ((input.mouse.x >=  32) && (input.mouse.x <=  95) &&
            (input.mouse.y >= 222) && (input.mouse.y <= 232))
        {
            samplerPlayDisplay();

            return;
        }

        // SAMPLER SCREEN PLAY RANGE
        if ((input.mouse.x >=  32) && (input.mouse.x <=  95) &&
            (input.mouse.y >= 233) && (input.mouse.y <= 243))
        {
            samplerPlayRange();

            return;
        }

        // SAMPLER SCREEN STOP
        if ((input.mouse.x >=   0) && (input.mouse.x <=  31) &&
            (input.mouse.y >= 222) && (input.mouse.y <= 243))
        {
            for (i = 0; i < AMIGA_VOICES; ++i)
            {
                // shutdown scope
                modEntry->channels[i].scopeLoopQuirk = false;
                modEntry->channels[i].scopeEnabled   = false;
                modEntry->channels[i].scopeTrigger   = false;

                // shutdown voice
                mixerKillVoice(i);
            }

            return;
        }

        // VOLUME button (toggle)
        if ((input.mouse.x >=  96) && (input.mouse.x <= 135) &&
            (input.mouse.y >= 244) && (input.mouse.y <= 254))
        {
            editor.ui.samplerVolBoxShown = false;
            removeSamplerVolBox();

            return;
        }

        // DRAG BOXES
        if ((input.mouse.x >=  72) && (input.mouse.x <= 173) &&
            (input.mouse.y >= 154) && (input.mouse.y <= 175))
        {
            volBoxBarPressed(MOUSE_BUTTON_NOT_HELD);
            return;
        }

        // FROM NUM
        if ((input.mouse.x >= 174) && (input.mouse.x <= 207) &&
            (input.mouse.y >= 154) && (input.mouse.y <= 164))
        {
            editor.ui.tmpDisp16 = editor.vol1;
            editor.vol1Disp = &editor.ui.tmpDisp16;

            editor.ui.numPtr16    = &editor.ui.tmpDisp16;
            editor.ui.numLen      = 3;
            editor.ui.editTextPos = 6342; // (y * 40) + x

            getNumLine(TEXT_EDIT_DECIMAL, PTB_SA_VOL_FROM_NUM);

            return;
        }

        // TO NUM
        if ((input.mouse.x >= 174) && (input.mouse.x <= 207) &&
            (input.mouse.y >= 165) && (input.mouse.y <= 175))
        {
            editor.ui.tmpDisp16 = editor.vol2;
            editor.vol2Disp = &editor.ui.tmpDisp16;

            editor.ui.numPtr16    = &editor.ui.tmpDisp16;
            editor.ui.numLen      = 3;
            editor.ui.editTextPos = 6782; // (y * 40) + x

            getNumLine(TEXT_EDIT_DECIMAL, PTB_SA_VOL_TO_NUM);

            return;
        }

        // NORMALIZE
        if ((input.mouse.x >= 101) && (input.mouse.x <= 143) &&
            (input.mouse.y >= 176) && (input.mouse.y <= 186))
        {
            s = &modEntry->samples[editor.currSample];
            if (s->length == 0)
            {
                displayErrorMsg("SAMPLE IS EMPTY");
                return;
            }

            sampleData = &modEntry->sampleData[s->offset];

            if ((editor.markEndOfs - editor.markStartOfs) > 0)
            {
                sampleData  += editor.markStartOfs;
                sampleLength = editor.markEndOfs - editor.markStartOfs;
            }
            else
            {
                sampleLength = s->length;
            }

            sampleVol   = 0;
            sampleIndex = 0;

            while (sampleIndex < sampleLength)
            {
                sample = *sampleData++;
                sample = ABS(sample);

                if (sampleVol < sample)
                    sampleVol = sample;

                sampleIndex++;
            }

            if ((sampleVol <= 0) || (sampleVol > 127))
            {
                editor.vol1 = 100;
                editor.vol2 = 100;
            }
            else if (sampleVol < 64)
            {
                editor.vol1 = 200;
                editor.vol2 = 200;
            }
            else
            {
                editor.vol1 = (int16_t)((12700.0f / sampleVol) + 0.5f);
                editor.vol2 = (int16_t)((12700.0f / sampleVol) + 0.5f);
            }

            editor.ui.updateVolFromText = true;
            editor.ui.updateVolToText   = true;

            showVolFromSlider();
            showVolToSlider();

            return;
        }

        // RAMP DOWN
        if ((input.mouse.x >= 144) && (input.mouse.x <= 153) &&
            (input.mouse.y >= 176) && (input.mouse.y <= 186))
        {
            editor.vol1 = 100;
            editor.vol2 = 0;

            editor.ui.updateVolFromText = true;
            editor.ui.updateVolToText   = true;

            showVolFromSlider();
            showVolToSlider();

            return;
        }

        // RAMP UP
        if ((input.mouse.x >= 154) && (input.mouse.x <= 163) &&
            (input.mouse.y >= 176) && (input.mouse.y <= 186))
        {
            editor.vol1 = 0;
            editor.vol2 = 100;

            editor.ui.updateVolFromText = true;
            editor.ui.updateVolToText = true;

            showVolFromSlider();
            showVolToSlider();

            return;
        }

        // RAMP UNITY
        if ((input.mouse.x >= 164) && (input.mouse.x <= 173) &&
            (input.mouse.y >= 176) && (input.mouse.y <= 186))
        {
            editor.vol1 = 100;
            editor.vol2 = 100;

            editor.ui.updateVolFromText = true;
            editor.ui.updateVolToText = true;

            showVolFromSlider();
            showVolToSlider();

            return;
        }

        // CANCEL
        if ((input.mouse.x >= 174) && (input.mouse.x <= 207) &&
            (input.mouse.y >= 176) && (input.mouse.y <= 186))
        {
            editor.ui.samplerVolBoxShown = false;
            removeSamplerVolBox();

            return;
        }

        // RAMP
        if ((input.mouse.x >=  72) && (input.mouse.x <= 100) &&
            (input.mouse.y >= 176) && (input.mouse.y <= 186))
        {
            s = &modEntry->samples[editor.currSample];
            if (s->length == 0)
            {
                displayErrorMsg("SAMPLE IS EMPTY");
                return;
            }

            if ((editor.vol1 == 100) && (editor.vol2 == 100))
            {
                editor.ui.samplerVolBoxShown = false;
                removeSamplerVolBox();

                return;
            }

            sampleData = &modEntry->sampleData[s->offset];

            if ((editor.markEndOfs - editor.markStartOfs) > 0)
            {
                sampleData  += editor.markStartOfs;
                sampleLength = editor.markEndOfs - editor.markStartOfs;
            }
            else
            {
                sampleLength = s->length;
            }

            sampleIndex = 0;
            while (sampleIndex < sampleLength)
            {
                smp  = (sampleIndex * editor.vol2) / (double)(sampleLength);
                smp += ((sampleLength - sampleIndex) * editor.vol1) / (double)(sampleLength);
                smp *= (double)(*sampleData);
                smp /= 100.0;
                smp = ROUND_SMP_D(smp);

                *sampleData++ = (int8_t)(CLAMP(smp, -128.0, 127.0));

                sampleIndex++;
            }

            editor.ui.samplerVolBoxShown = false;
            removeSamplerVolBox();

            updateWindowTitle(MOD_IS_MODIFIED);
        }
    }
}

void handleSamplerFiltersBoxRepeats(void)
{
    if (!editor.ui.samplerFiltersBoxShown)
        return;

    if (!input.mouse.leftButtonPressed)
    {
        input.mouse.repeatCounter_2 = 0;
        return;
    }

    if (input.mouse.lastGUIButton_2 > -1)
    {
        if (++input.mouse.repeatCounter_2 >= 1)
        {
            input.mouse.repeatCounter_2 = 0;

            switch (input.mouse.lastGUIButton_2)
            {
                case 0:
                {
                    if (input.mouse.rightButtonPressed)
                        editor.lpCutOff += 50;
                    else
                        editor.lpCutOff++;

                    if (editor.lpCutOff > (FILTERS_BASE_FREQ / 2))
                        editor.lpCutOff =  FILTERS_BASE_FREQ / 2;

                    editor.ui.updateLPText = true;
                }
                break;

                case 1:
                {
                    if (input.mouse.rightButtonPressed)
                        editor.lpCutOff -= 50;
                    else
                        editor.lpCutOff--;

                    if (editor.lpCutOff < 0)
                        editor.lpCutOff = 0;

                    editor.ui.updateLPText = true;
                }
                break;

                case 2:
                {
                    if (input.mouse.rightButtonPressed)
                        editor.hpCutOff += 50;
                    else
                        editor.hpCutOff++;

                    if (editor.hpCutOff > (FILTERS_BASE_FREQ / 2))
                        editor.hpCutOff =  FILTERS_BASE_FREQ / 2;

                    editor.ui.updateHPText = true;
                }
                break;

                case 3:
                {
                    if (input.mouse.rightButtonPressed)
                        editor.hpCutOff -= 50;
                    else
                        editor.hpCutOff--;

                    if (editor.hpCutOff < 0)
                        editor.hpCutOff = 0;

                    editor.ui.updateHPText = true;
                }
                break;

                default:
                    break;
            }
        }
    }
}

void handleSamplerFiltersBox(void)
{
    uint8_t i;
    moduleSample_t *s;

    if (input.mouse.rightButtonPressed && editor.ui.getLineFlag)
    {
        exitGetTextLine(EDIT_TEXT_NO_UPDATE);
        return;
    }

    if (editor.ui.getLineFlag || (input.mouse.lastGUIButton_2 > -1))
        return;

    if (input.mouse.leftButtonPressed)
    {
        // restore sample ask dialog
        if (editor.ui.askScreenShown && (editor.ui.askScreenType == ASK_RESTORE_SAMPLE))
        {
            if ((input.mouse.y >= 71) && (input.mouse.y <= 81))
            {
                if ((input.mouse.x >= 171) && (input.mouse.x <= 196))
                {
                    // YES button

                    editor.ui.askScreenShown = false;

                    editor.ui.answerNo  = false;
                    editor.ui.answerYes = true;

                    handleAskYes();
                }
                else if ((input.mouse.x >= 234) && (input.mouse.x <= 252))
                {
                    // NO button

                    editor.ui.askScreenShown = false;

                    editor.ui.answerNo  = true;
                    editor.ui.answerYes = false;

                    handleAskNo();
                }
            }

            return;
        }

        // FILTERS button (toggle)
        if ((input.mouse.x >= 211) && (input.mouse.x <= 245) &&
            (input.mouse.y >= 244) && (input.mouse.y <= 254))
        {
            editor.ui.samplerFiltersBoxShown = false;
            removeSamplerFiltersBox();

            return;
        }

        // MAIN SCREEN STOP
        if (!editor.ui.diskOpScreenShown)
        {
            if (!editor.ui.posEdScreenShown)
            {
                if ((input.mouse.x >= 182) && (input.mouse.x <= 243) &&
                    (input.mouse.y >=   0) && (input.mouse.y <=  10))
                {
                    modStop();
                    return;
                }
            }
        }

        // SAMPLER SCREEN PLAY WAVEFORM
        if ((input.mouse.x >=  32) && (input.mouse.x <=  95) &&
            (input.mouse.y >= 211) && (input.mouse.y <= 221))
        {
            samplerPlayWaveform();

            return;
        }

        // SAMPLER SCREEN PLAY DISPLAY
        if ((input.mouse.x >=  32) && (input.mouse.x <=  95) &&
            (input.mouse.y >= 222) && (input.mouse.y <= 232))
        {
            samplerPlayDisplay();

            return;
        }

        // SAMPLER SCREEN PLAY RANGE
        if ((input.mouse.x >=  32) && (input.mouse.x <=  95) &&
            (input.mouse.y >= 233) && (input.mouse.y <= 243))
        {
            samplerPlayRange();

            return;
        }

        // SAMPLER SCREEN STOP
        if ((input.mouse.x >=   0) && (input.mouse.x <=  31) &&
            (input.mouse.y >= 222) && (input.mouse.y <= 243))
        {
            for (i = 0; i < AMIGA_VOICES; ++i)
            {
                // shutdown scope
                modEntry->channels[i].scopeLoopQuirk = false;
                modEntry->channels[i].scopeEnabled   = false;
                modEntry->channels[i].scopeTrigger   = false;

                // shutdown voice
                mixerKillVoice(i);
            }

            return;
        }

        // UNDO
        if ((input.mouse.x >=  65) && (input.mouse.x <=  75) &&
            (input.mouse.y >= 154) && (input.mouse.y <= 184))
        {
            s = &modEntry->samples[editor.currSample];

            if (s->length == 0)
            {
                displayErrorMsg("SAMPLE IS EMPTY");
            }
            else
            {
                memcpy(&modEntry->sampleData[s->offset], editor.tempSample, 131070);

                redrawSample();
                updateWindowTitle(MOD_IS_MODIFIED);
                renderSamplerFiltersBox();
            }

            return;
        }

        // DO LOW-PASS FILTER
        if ((input.mouse.x >=  76) && (input.mouse.x <= 157) &&
            (input.mouse.y >= 154) && (input.mouse.y <= 164))
        {
            lowPassSample(editor.lpCutOff);
            renderSamplerFiltersBox();

            return;
        }

        // LOW-PASS CUTOFF
        if ((input.mouse.x >= 158) && (input.mouse.x <= 217) &&
            (input.mouse.y >= 154) && (input.mouse.y <= 164))
        {
            if (input.mouse.rightButtonPressed)
            {
                editor.lpCutOff = 0;
                editor.ui.updateLPText = true;
            }
            else
            {
                editor.ui.tmpDisp32 = editor.lpCutOff;
                editor.lpCutOffDisp = &editor.ui.tmpDisp32;

                editor.ui.numPtr32    = &editor.ui.tmpDisp32;
                editor.ui.numLen      = 5;
                editor.ui.editTextPos = 6340; // (y * 40) + x

                getNumLine(TEXT_EDIT_DECIMAL, PTB_SA_FIL_LP_CUTOFF);
            }

            return;
        }

        // LOW-PASS CUTOFF UP
        if ((input.mouse.x >= 218) && (input.mouse.x <= 228) &&
            (input.mouse.y >= 154) && (input.mouse.y <= 164))
        {
            input.mouse.lastGUIButton_2 = 0;

            if (input.mouse.rightButtonPressed)
                editor.lpCutOff += 100;
            else
                editor.lpCutOff++;

            if (editor.lpCutOff > (FILTERS_BASE_FREQ / 2))
                editor.lpCutOff =  FILTERS_BASE_FREQ / 2;

            editor.ui.updateLPText = true;

            return;
        }

        // LOW-PASS CUTOFF DOWN
        if ((input.mouse.x >= 229) && (input.mouse.x <= 239) &&
            (input.mouse.y >= 154) && (input.mouse.y <= 164))
        {
            input.mouse.lastGUIButton_2 = 1;

            if (input.mouse.rightButtonPressed)
                editor.lpCutOff -= 100;
            else
                editor.lpCutOff--;

            if (editor.lpCutOff < 0)
                editor.lpCutOff = 0;

            editor.ui.updateLPText = true;

            return;
        }

        // DO HIGH-PASS FILTER
        if ((input.mouse.x >=  76) && (input.mouse.x <= 157) &&
            (input.mouse.y >= 164) && (input.mouse.y <= 174))
        {
            highPassSample(editor.hpCutOff);
            renderSamplerFiltersBox();

            return;
        }

        // HIGH-PASS CUTOFF
        if ((input.mouse.x >= 158) && (input.mouse.x <= 217) &&
            (input.mouse.y >= 164) && (input.mouse.y <= 174))
        {
            if (input.mouse.rightButtonPressed)
            {
                editor.hpCutOff = 0;
                editor.ui.updateHPText = true;
            }
            else
            {
                editor.ui.tmpDisp32 = editor.hpCutOff;
                editor.hpCutOffDisp = &editor.ui.tmpDisp32;

                editor.ui.numPtr32    = &editor.ui.tmpDisp32;
                editor.ui.numLen      = 5;
                editor.ui.editTextPos = 6780; // (y * 40) + x

                getNumLine(TEXT_EDIT_DECIMAL, PTB_SA_FIL_HP_CUTOFF);
            }

            return;
        }

        // HIGH-PASS CUTOFF UP
        if ((input.mouse.x >= 218) && (input.mouse.x <= 228) &&
            (input.mouse.y >= 164) && (input.mouse.y <= 174))
        {
            input.mouse.lastGUIButton_2 = 2;

            if (input.mouse.rightButtonPressed)
                editor.hpCutOff += 100;
            else
                editor.hpCutOff++;

            if (editor.hpCutOff > (FILTERS_BASE_FREQ / 2))
                editor.hpCutOff =  FILTERS_BASE_FREQ / 2;

            editor.ui.updateHPText = true;

            return;
        }

        // HIGH-PASS CUTOFF DOWN
        if ((input.mouse.x >= 229) && (input.mouse.x <= 239) &&
            (input.mouse.y >= 164) && (input.mouse.y <= 174))
        {
            input.mouse.lastGUIButton_2 = 3;

            if (input.mouse.rightButtonPressed)
                editor.hpCutOff -= 100;
            else
                editor.hpCutOff--;

            if (editor.hpCutOff < 0)
                editor.hpCutOff = 0;

            editor.ui.updateHPText = true;

            return;
        }

        // NORMALIZE SAMPLE FLAG
        if ((input.mouse.x >=  76) && (input.mouse.x <= 239) &&
            (input.mouse.y >= 174) && (input.mouse.y <= 184))
        {
            editor.normalizeFiltersFlag ^= 1;
            editor.ui.updateNormFlag = true;

            return;
        }

        // EXIT
        if ((input.mouse.x >= 240) && (input.mouse.x <= 250) &&
            (input.mouse.y >= 154) && (input.mouse.y <= 186))
        {
            editor.ui.samplerFiltersBoxShown = false;
            removeSamplerFiltersBox();
        }
    }
}

int32_t checkGUIButtons(void)
{
    uint32_t i;

    // terminal has first priority
    if (editor.ui.terminalShown)
    {
        for (i = 0; i < TERMINAL_BUTTONS; ++i)
        {
            if ((input.mouse.x >= bTerminal[i].x1) && (input.mouse.x <= bTerminal[i].x2)
             && (input.mouse.y >= bTerminal[i].y1) && (input.mouse.y <= bTerminal[i].y2))
                 return (bTerminal[i].b);
        }

        return (-1);
    }

    // these two makes *no other* buttons clickable
    if (editor.ui.askScreenShown)
    {
        if (editor.ui.pat2SmpDialogShown)
        {
            for (i = 0; i < PAT2SMP_ASK_BUTTONS; ++i)
            {
                if ((input.mouse.x >= bPat2SmpAsk[i].x1) && (input.mouse.x <= bPat2SmpAsk[i].x2)
                 && (input.mouse.y >= bPat2SmpAsk[i].y1) && (input.mouse.y <= bPat2SmpAsk[i].y2))
                     return (bPat2SmpAsk[i].b);
            }
        }
        else
        {
            for (i = 0; i < ASK_BUTTONS; ++i)
            {
                if ((input.mouse.x >= bAsk[i].x1) && (input.mouse.x <= bAsk[i].x2)
                 && (input.mouse.y >= bAsk[i].y1) && (input.mouse.y <= bAsk[i].y2))
                     return (bAsk[i].b);
            }
        }

        return (-1);
    }
    else if (editor.ui.clearScreenShown)
    {
        for (i = 0; i < CLEAR_BUTTONS; ++i)
        {
            if ((input.mouse.x >= bClear[i].x1) && (input.mouse.x <= bClear[i].x2)
             && (input.mouse.y >= bClear[i].y1) && (input.mouse.y <= bClear[i].y2))
                 return (bClear[i].b);
        }

        return (-1);
    }

    // QUIT (xy 0,0) works on all screens except for ask/clear screen
    if ((input.mouse.x == 0) && (input.mouse.y == 0))
        return (PTB_QUIT);

    // top screen buttons
    if (editor.ui.diskOpScreenShown)
    {
        for (i = 0; i < DISKOP_BUTTONS; ++i)
        {
            if ((input.mouse.x >= bDiskOp[i].x1) && (input.mouse.x <= bDiskOp[i].x2)
                && (input.mouse.y >= bDiskOp[i].y1) && (input.mouse.y <= bDiskOp[i].y2))
                return (bDiskOp[i].b);
        }
    }
    else
    {
        if (editor.ui.posEdScreenShown)
        {
            for (i = 0; i < POSED_BUTTONS; ++i)
            {
                if ((input.mouse.x >= bPosEd[i].x1) && (input.mouse.x <= bPosEd[i].x2)
                    && (input.mouse.y >= bPosEd[i].y1) && (input.mouse.y <= bPosEd[i].y2))
                        return (bPosEd[i].b);
            }
        }
        else if (editor.ui.editOpScreenShown)
        {
            switch (editor.ui.editOpScreen)
            {
                default:
                case 0:
                {
                    for (i = 0; i < EDITOP1_BUTTONS; ++i)
                    {
                        if ((input.mouse.x >= bEditOp1[i].x1) && (input.mouse.x <= bEditOp1[i].x2)
                         && (input.mouse.y >= bEditOp1[i].y1) && (input.mouse.y <= bEditOp1[i].y2))
                            return (bEditOp1[i].b);
                    }
                }
                break;

                case 1:
                {
                    for (i = 0; i < EDITOP2_BUTTONS; ++i)
                    {
                        if ((input.mouse.x >= bEditOp2[i].x1) && (input.mouse.x <= bEditOp2[i].x2)
                         && (input.mouse.y >= bEditOp2[i].y1) && (input.mouse.y <= bEditOp2[i].y2))
                            return (bEditOp2[i].b);
                    }
                }
                break;

                case 2:
                {
                    for (i = 0; i < EDITOP3_BUTTONS; ++i)
                    {
                        if ((input.mouse.x >= bEditOp3[i].x1) && (input.mouse.x <= bEditOp3[i].x2)
                         && (input.mouse.y >= bEditOp3[i].y1) && (input.mouse.y <= bEditOp3[i].y2))
                            return (bEditOp3[i].b);
                    }
                }
                break;

                case 3:
                {
                    for (i = 0; i < EDITOP4_BUTTONS; ++i)
                    {
                        if ((input.mouse.x >= bEditOp4[i].x1) && (input.mouse.x <= bEditOp4[i].x2)
                         && (input.mouse.y >= bEditOp4[i].y1) && (input.mouse.y <= bEditOp4[i].y2))
                            return (bEditOp4[i].b);
                    }
                }
                break;
            }
        }

        for (i = 0; i < TOPSCREEN_BUTTONS; ++i)
        {
            if ((input.mouse.x >= bTopScreen[i].x1) && (input.mouse.x <= bTopScreen[i].x2)
                && (input.mouse.y >= bTopScreen[i].y1) && (input.mouse.y <= bTopScreen[i].y2))
                    return (bTopScreen[i].b);
        }
    }

    // middle buttons (always present)
    for (i = 0; i < MIDSCREEN_BUTTONS; ++i)
    {
        if ((input.mouse.x >= bMidScreen[i].x1) && (input.mouse.x <= bMidScreen[i].x2)
            && (input.mouse.y >= bMidScreen[i].y1) && (input.mouse.y <= bMidScreen[i].y2))
            return (bMidScreen[i].b);
    }

    // bottom screen buttons
    if (editor.ui.samplerScreenShown)
    {
        for (i = 0; i < SAMPLER_BUTTONS; ++i)
        {
            if ((input.mouse.x >= bSampler[i].x1) && (input.mouse.x <= bSampler[i].x2)
                && (input.mouse.y >= bSampler[i].y1) && (input.mouse.y <= bSampler[i].y2))
                return (bSampler[i].b);
        }
    }
    else
    {
        for (i = 0; i < BOTSCREEN_BUTTONS; ++i)
        {
            if ((input.mouse.x >= bBotScreen[i].x1) && (input.mouse.x <= bBotScreen[i].x2)
                && (input.mouse.y >= bBotScreen[i].y1) && (input.mouse.y <= bBotScreen[i].y2))
                return (bBotScreen[i].b);
        }
    }

    return (-1);
}

void handleTextEditing(uint8_t mouseButton)
{
    char *tmpRead;
    int32_t tmp32;

    // handle mouse while editing text/numbers
    if (editor.ui.getLineFlag)
    {
        if (editor.ui.getLineType != TEXT_EDIT_STRING)
        {
            if (mouseButton == SDL_BUTTON_RIGHT)
                exitGetTextLine(EDIT_TEXT_NO_UPDATE);
        }
        else if ((mouseButton == SDL_BUTTON_LEFT) && !editor.mixFlag)
        {
            tmp32 = input.mouse.y - editor.ui.lineCurY;
            if ((tmp32 <= 2) && (tmp32 >= -9))
            {
                tmp32 = (input.mouse.x - editor.ui.lineCurX) + 4;

                // 68k simulation on signed number: ASR.L #3,D1
                if (tmp32 < 0)
                    tmp32 = 0xE0000000 | ((uint32_t)(tmp32) >> 3); // 0xE0000000 = 2^32 - 2^(32-3)
                else
                    tmp32 /= (1 << 3);

                while (tmp32 != 0) // 0 = pos we want
                {
                    if (tmp32 > 0)
                    {
                        if (editor.ui.editPos < editor.ui.textEndPtr)
                        {
                            if (*editor.ui.editPos != '\0')
                            {
                                editor.ui.editPos++;
                                textMarkerMoveRight();
                            }
                        }

                        tmp32--;
                    }
                    else if (tmp32 < 0)
                    {
                        if (editor.ui.editPos > editor.ui.dstPtr)
                        {
                            editor.ui.editPos--;
                            textMarkerMoveLeft();
                        }

                        tmp32++;
                    }
                }
            }
            else
            {
                exitGetTextLine(EDIT_TEXT_UPDATE);
            }
        }
        else if (mouseButton == SDL_BUTTON_RIGHT)
        {
            if (editor.mixFlag)
            {
                exitGetTextLine(EDIT_TEXT_UPDATE);

                editor.mixFlag = false;
                editor.ui.updateMixText = true;
            }
            else
            {
                tmpRead = editor.ui.dstPtr;
                while (tmpRead < editor.ui.textEndPtr)
                    *tmpRead++ = '\0';

                *editor.ui.textEndPtr = '\0';

                // don't exit text edit mode if the disk op. path was about to be deleted
                if (editor.ui.editObject == PTB_DO_DATAPATH)
                {
                    // move text cursor to pos 0
                    while (editor.ui.editPos > editor.ui.dstPtr)
                    {
                        editor.ui.editPos--;
                        textMarkerMoveLeft();
                    }

                    editor.ui.updateDiskOpPathText = true;
                }
                else
                {
                         if (editor.ui.editObject == PTB_SONGNAME)   editor.ui.updateSongName       = true;
                    else if (editor.ui.editObject == PTB_SAMPLENAME) editor.ui.updateCurrSampleName = true;

                    exitGetTextLine(EDIT_TEXT_UPDATE);
                }
            }
        }
    }
}

void mouseWheelUpHandler(void)
{
    if (!editor.ui.getLineFlag      && !editor.ui.askScreenShown &&
        !editor.ui.clearScreenShown && !editor.swapChannelFlag)
    {
        if (editor.ui.terminalShown)
        {
            terminalScrollUp();
            terminalScrollUp();
        }
        else if (editor.ui.posEdScreenShown)
        {
            if (modEntry->currOrder > 0)
                modSetPos(modEntry->currOrder - 1, DONT_SET_ROW);
        }
        else if (editor.ui.diskOpScreenShown)
        {
            if (editor.diskop.scrollOffset > 0)
            {
                editor.diskop.scrollOffset--;
                editor.ui.updateDiskOpFileList = true;
            }
        }
        else if (!editor.ui.samplerScreenShown && !editor.songPlaying)
        {
            if (modEntry->currRow > 0)
                modSetPos(DONT_SET_ORDER, modEntry->currRow - 1);
        }
    }
}

void mouseWheelDownHandler(void)
{
    if (!editor.ui.getLineFlag      && !editor.ui.askScreenShown &&
        !editor.ui.clearScreenShown && !editor.swapChannelFlag)
    {
        if (editor.ui.terminalShown)
        {
            terminalScrollDown();
            terminalScrollDown();
        }
        else if (editor.ui.posEdScreenShown)
        {
            if (modEntry->currOrder < (modEntry->head.orderCount - 1))
                modSetPos(modEntry->currOrder + 1, DONT_SET_ROW);
        }
        else if (editor.ui.diskOpScreenShown)
        {
            if (editor.diskop.numFiles > DISKOP_LIST_SIZE)
            {
                if (editor.diskop.scrollOffset < (editor.diskop.numFiles - DISKOP_LIST_SIZE))
                {
                    editor.diskop.scrollOffset++;
                    editor.ui.updateDiskOpFileList = true;
                }
            }
        }
        else if (!editor.ui.samplerScreenShown && !editor.songPlaying)
        {
            if (modEntry->currRow < MOD_ROWS)
                modSetPos(DONT_SET_ORDER, modEntry->currRow + 1);
        }
    }
}

int8_t handleRightMouseButton(void)
{
    if (!input.mouse.rightButtonPressed)
        return (false);

    // exit sample swap mode with right mouse button (if present)
    if (editor.swapChannelFlag)
    {
        editor.swapChannelFlag = false;

        pointerSetPreviousMode();
        setPrevStatusMessage();

        return (true);
    }

    // close clear dialog with right mouse button
    if (editor.ui.clearScreenShown)
    {
        editor.ui.clearScreenShown = false;

        setPrevStatusMessage();
        pointerSetPreviousMode();

        editor.errorMsgActive  = true;
        editor.errorMsgBlock   = true;
        editor.errorMsgCounter = 0;

        pointerErrorMode();
        removeClearScreen();

        return (true);
    }

    // close ask dialogs with right mouse button
    if (editor.ui.askScreenShown)
    {
        editor.ui.askScreenShown = false;

        editor.ui.answerNo  = true;
        editor.ui.answerYes = false;

        handleAskNo(); // mouse pointer is set to error (red) in here
        return (true);
    }

    // toggle channel muting with right mouse button
    if ( !editor.ui.posEdScreenShown  &&
         !editor.ui.editOpScreenShown &&
         !editor.ui.diskOpScreenShown &&
         !editor.ui.aboutScreenShown  &&
         !editor.ui.samplerVolBoxShown &&
         !editor.ui.samplerFiltersBoxShown &&
         !editor.ui.terminalShown &&
         (editor.ui.visualizerMode == VISUAL_QUADRASCOPE)
       )
    {
        if ((input.mouse.y >= 55) && (input.mouse.y <= 87))
        {
                 if ((input.mouse.x > 127) && (input.mouse.x <= (127 + 40))) editor.muted[0] ^= 1;
            else if ((input.mouse.x > 175) && (input.mouse.x <= (175 + 40))) editor.muted[1] ^= 1;
            else if ((input.mouse.x > 223) && (input.mouse.x <= (223 + 40))) editor.muted[2] ^= 1;
            else if ((input.mouse.x > 271) && (input.mouse.x <= (271 + 40))) editor.muted[3] ^= 1;

            renderMuteButtons();
        }
    }

    // sample hand drawing
    if (editor.ui.samplerScreenShown && !editor.ui.samplerVolBoxShown & !editor.ui.samplerFiltersBoxShown)
    {
        if ((input.mouse.y >= 138) && (input.mouse.y <= 201) &&
            (input.mouse.x >=   3) && (input.mouse.x <= 316)
           )
        {
            samplerEditSample(false);
        }
    }

    return (false);
}

int8_t handleLeftMouseButton(void)
{
    char pat2SmpText[18];
    int8_t *ptr8_1, *ptr8_2, *ptr8_3, *ptr8_4;
    int8_t tmpSmp, modTmp, modDat;
    uint8_t i;
    int16_t tmp16;
    int32_t j, modPos, guiButton;
    double smp;
    moduleSample_t *s;

    if (editor.swapChannelFlag || editor.ui.getLineFlag)
        return (false);

    // handle volume toolbox in sampler screen
    if (editor.ui.samplerVolBoxShown)
    {
        handleSamplerVolumeBox();
        return (true);
    }

    // handle filters toolbox in sampler
    else if (editor.ui.samplerFiltersBoxShown)
    {
        handleSamplerFiltersBox();
        return (true);
    }

    // cancel note input gadgets with left/right mouse button
    if (editor.ui.changingSmpResample || editor.ui.changingChordNote || editor.ui.changingDrumPadNote)
    {
        if (input.mouse.leftButtonPressed || input.mouse.rightButtonPressed)
        {
            editor.ui.changingSmpResample = false;
            editor.ui.changingChordNote   = false;
            editor.ui.changingDrumPadNote = false;

            editor.ui.updateResampleNote = true;
            editor.ui.updateNote1Text = true;
            editor.ui.updateNote2Text = true;
            editor.ui.updateNote3Text = true;
            editor.ui.updateNote4Text = true;

            setPrevStatusMessage();
            pointerSetPreviousMode();
        }

        return (true);
    }

    if (input.mouse.leftButtonPressed)
    {
        // handle QUIT ask dialog while Disk Op. filling is ongoing
        if (editor.diskop.isFilling)
        {
            if (editor.ui.askScreenShown && (editor.ui.askScreenType == ASK_QUIT))
            {
                if ((input.mouse.y >= 71) && (input.mouse.y <= 81))
                {
                    if ((input.mouse.x >= 171) && (input.mouse.x <= 196))
                    {
                        // YES button

                        editor.ui.askScreenShown = false;

                        editor.ui.answerNo  = false;
                        editor.ui.answerYes = true;

                        handleAskYes();
                    }
                    else if ((input.mouse.x >= 234) && (input.mouse.x <= 252))
                    {
                        // NO button

                        editor.ui.askScreenShown = false;

                        editor.ui.answerNo  = true;
                        editor.ui.answerYes = false;

                        handleAskNo();
                    }
                }
            }

            return (true);
        }

        // CANCEL and YES/NO (ask exit) buttons while MOD2WAV is ongoing
        if (editor.isWAVRendering)
        {
            if (editor.ui.askScreenShown && (editor.ui.askScreenType == ASK_QUIT))
            {
                if ((input.mouse.x >= 171) && (input.mouse.x <= 196))
                {
                    // YES button

                    editor.isWAVRendering = false;
                    SDL_WaitThread(editor.mod2WavThread, NULL);

                    editor.ui.askScreenShown = false;

                    editor.ui.answerNo  = false;
                    editor.ui.answerYes = true;

                    handleAskYes();
                }
                else if ((input.mouse.x >= 234) && (input.mouse.x <= 252))
                {
                    // NO button

                    editor.ui.askScreenShown = false;

                    editor.ui.answerNo  = true;
                    editor.ui.answerYes = false;

                    handleAskNo();

                    pointerSetMode(POINTER_MODE_READ_DIR, NO_CARRY);
                    setStatusMessage("RENDERING MOD...", NO_CARRY);
                }
            }
            else if ((input.mouse.y >= 58) && (input.mouse.y <= 68) && (input.mouse.x >= 133) && (input.mouse.x <= 186))
            {
                // CANCEL button
                editor.abortMod2Wav = true;
            }

            return (true);
        }

        guiButton = checkGUIButtons();
        if (guiButton != -1)
        {
            switch (guiButton)
            {
                case PTB_DUMMY: break; // for gaps/empty spaces/dummies

                case PTB_TERM_CLEAR: terminalClear(); break;
                case PTB_TERM_EXIT:
                {
                    editor.ui.terminalShown = false;
                    editor.ui.terminalWasClosed = true; // special case for exit button
                    removeTerminalScreen();
                }
                break;

                case PTB_TERM_SCROLL_UP:
                {
                    input.mouse.lastGUIButton = guiButton; // button repeat
                    terminalScrollUp();
                }
                break;

                case PTB_TERM_SCROLL_BAR:
                {
                    input.mouse.lastGUIButton = guiButton; // button repeat

                    if (!editor.ui.forceTermBarDrag)
                    {
                        terminalHandleScrollBar(MOUSE_BUTTON_NOT_HELD);
                        return (true);
                    }
                }
                break;

                case PTB_TERM_SCROLL_DOWN:
                {
                    input.mouse.lastGUIButton = guiButton; // button repeat
                    terminalScrollDown();
                }
                break;

                case PTB_PAT2SMP:
                {
                    editor.ui.askScreenShown     = true;
                    editor.ui.askScreenType      = ASK_PAT2SMP;
                    editor.ui.pat2SmpDialogShown = true;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);

                    if (editor.songPlaying)
                        sprintf(pat2SmpText, "ROW 00 TO SMP %02X?", editor.currSample + 1);
                    else
                        sprintf(pat2SmpText, "ROW %02d TO SMP %02X?", modEntry->currRow, editor.currSample + 1);

                    setStatusMessage(pat2SmpText, NO_CARRY);
                    renderAskDialog();
                }
                break;

                // Edit Op. All Screens
                case PTB_EO_TITLEBAR:
                {
                         if (editor.ui.editOpScreen == 0) editor.sampleAllFlag ^= 1;
                    else if (editor.ui.editOpScreen == 1) editor.trackPattFlag  = (editor.trackPattFlag + 1) % 3;
                    else if (editor.ui.editOpScreen == 2) editor.halfClipFlag  ^= 1;
                    else if (editor.ui.editOpScreen == 3) editor.newOldFlag    ^= 1;

                    renderEditOpMode();
                }
                break;

                case PTB_EO_1:
                {
                    editor.ui.editOpScreen = 0;
                    renderEditOpScreen();
                }
                break;

                case PTB_EO_2:
                {
                    editor.ui.editOpScreen = 1;
                    renderEditOpScreen();
                }
                break;

                case PTB_EO_3:
                {
                    editor.ui.editOpScreen = 2;
                    renderEditOpScreen();
                }
                break;

                case PTB_EO_EXIT:
                {
                    editor.ui.aboutScreenShown  = false;
                    editor.ui.editOpScreenShown = false;
                    displayMainScreen();
                }
                break;
                // ----------------------------------------------------------

                // Edit Op. Screen #1
                case PTB_EO_TRACK_NOTE_UP:   trackNoteUp(editor.sampleAllFlag,   0, MOD_ROWS - 1); break;
                case PTB_EO_TRACK_NOTE_DOWN: trackNoteDown(editor.sampleAllFlag, 0, MOD_ROWS - 1); break;
                case PTB_EO_TRACK_OCTA_UP:   trackOctaUp(editor.sampleAllFlag,   0, MOD_ROWS - 1); break;
                case PTB_EO_TRACK_OCTA_DOWN: trackOctaDown(editor.sampleAllFlag, 0, MOD_ROWS - 1); break;
                case PTB_EO_PATT_NOTE_UP:    pattNoteUp(editor.sampleAllFlag); break;
                case PTB_EO_PATT_NOTE_DOWN:  pattNoteDown(editor.sampleAllFlag); break;
                case PTB_EO_PATT_OCTA_UP:    pattOctaUp(editor.sampleAllFlag); break;
                case PTB_EO_PATT_OCTA_DOWN:  pattOctaDown(editor.sampleAllFlag); break;
                // ----------------------------------------------------------

                // Edit Op. Screen #2
                case PTB_EO_RECORD:
                {
                    editor.recordMode ^= 1;
                    editor.ui.updateRecordText = true;
                }
                break;

                case PTB_EO_DELETE: delSampleTrack();  break;
                case PTB_EO_EXCHGE: exchSampleTrack(); break;
                case PTB_EO_COPY:   copySampleTrack(); break;

                case PTB_EO_FROM:
                {
                    editor.sampleFrom = editor.currSample + 1;
                    editor.ui.updateFromText = true;
                }
                break;

                case PTB_EO_TO:
                {
                    editor.sampleTo = editor.currSample + 1;
                    editor.ui.updateToText = true;
                }
                break;

                case PTB_EO_KILL:
                {
                    editor.ui.askScreenShown = true;
                    editor.ui.askScreenType  = ASK_KILL_SAMPLE;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("KILL SAMPLE ?", NO_CARRY);
                    renderAskDialog();
                }
                break;

                case PTB_EO_QUANTIZE:
                {
                    editor.ui.tmpDisp16 = editor.quantizeValue;
                    editor.quantizeValueDisp = &editor.ui.tmpDisp16;

                    editor.ui.numPtr16    = &editor.ui.tmpDisp16;
                    editor.ui.numLen      = 2;
                    editor.ui.editTextPos = 2824; // (y * 40) + x

                    getNumLine(TEXT_EDIT_DECIMAL, PTB_EO_QUANTIZE);
                }
                break;

                case PTB_EO_METRO_1: // metronome speed
                {
                    editor.ui.tmpDisp16 = editor.metroSpeed;
                    editor.metroSpeedDisp = &editor.ui.tmpDisp16;

                    editor.ui.numPtr16    = &editor.ui.tmpDisp16;
                    editor.ui.numLen      = 2;
                    editor.ui.editTextPos = 3261; // (y * 40) + x

                    getNumLine(TEXT_EDIT_DECIMAL, PTB_EO_METRO_1);
                }
                break;

                case PTB_EO_METRO_2: // metronome channel
                {
                    editor.ui.tmpDisp16 = editor.metroChannel;
                    editor.metroChannelDisp = &editor.ui.tmpDisp16;

                    editor.ui.numPtr16    = &editor.ui.tmpDisp16;
                    editor.ui.numLen      = 2;
                    editor.ui.editTextPos = 3264; // (y * 40) + x

                    getNumLine(TEXT_EDIT_DECIMAL, PTB_EO_METRO_2);
                }
                break;

                case PTB_EO_FROM_NUM:
                {
                    editor.ui.tmpDisp8 = editor.sampleFrom;
                    editor.sampleFromDisp = &editor.ui.tmpDisp8;

                    editor.ui.numPtr8     = &editor.ui.tmpDisp8;
                    editor.ui.numLen      = 2;
                    editor.ui.numBits     = 8;
                    editor.ui.editTextPos = 3273; // (y * 40) + x

                    getNumLine(TEXT_EDIT_HEX, PTB_EO_FROM_NUM);
                }
                break;

                case PTB_EO_TO_NUM:
                {
                    editor.ui.tmpDisp8 = editor.sampleTo;
                    editor.sampleToDisp = &editor.ui.tmpDisp8;

                    editor.ui.numPtr8     = &editor.ui.tmpDisp8;
                    editor.ui.numLen      = 2;
                    editor.ui.numBits     = 8;
                    editor.ui.editTextPos = 3713; // (y * 40) + x

                    getNumLine(TEXT_EDIT_HEX, PTB_EO_TO_NUM);
                }
                break;

                case PTB_EO_FROM_UP:
                {
                    if (editor.sampleFrom < 0x1F)
                    {
                        editor.sampleFrom++;
                        editor.ui.updateFromText = true;

                        input.mouse.lastGUIButton = guiButton; // button repeat
                    }
                }
                break;

                case PTB_EO_FROM_DOWN:
                {
                    if (editor.sampleFrom > 0x00)
                    {
                        editor.sampleFrom--;
                        editor.ui.updateFromText = true;

                        input.mouse.lastGUIButton = guiButton; // button repeat
                    }
                }
                break;

                case PTB_EO_TO_UP:
                {
                    if (editor.sampleTo < 0x1F)
                    {
                        editor.sampleTo++;
                        editor.ui.updateToText = true;

                        input.mouse.lastGUIButton = guiButton; // button repeat
                    }
                }
                break;

                case PTB_EO_TO_DOWN:
                {
                    if (editor.sampleTo > 0x00)
                    {
                        editor.sampleTo--;
                        editor.ui.updateToText = true;

                        input.mouse.lastGUIButton = guiButton; // button repeat
                    }
                }
                break;

                case PTB_EO_KEYS:
                {
                    editor.multiFlag ^= 1;

                    editor.ui.updateTrackerFlags = true;
                    editor.ui.updateKeysText = true;
                }
                break;
                // ----------------------------------------------------------

                // Edit Op. Screen #3

                case PTB_EO_MIX:
                {
                    if (!input.mouse.rightButtonPressed)
                    {
                        editor.mixFlag = true;

                        editor.ui.showTextPtr   = editor.mixText;
                        editor.ui.textEndPtr    = editor.mixText + 15;
                        editor.ui.textLength    = 16;
                        editor.ui.editTextPos   = 1936; // (y * 40) + x
                        editor.ui.dstOffset     = NULL;
                        editor.ui.dstOffsetEnd  = false;

                        editor.ui.updateMixText = true;

                        getTextLine(PTB_EO_MIX);
                    }
                    else
                    {
                        s = &modEntry->samples[editor.currSample];

                        if (s->length == 0)
                        {
                            displayErrorMsg("SAMPLE IS EMPTY");
                            break;
                        }

                        if (editor.samplePos == s->length)
                        {
                            displayErrorMsg("INVALID POS !");
                            break;
                        }

                        ptr8_1 = (int8_t *)(malloc(MAX_SAMPLE_LEN));
                        if (ptr8_1 == NULL)
                        {
                            displayErrorMsg(editor.outOfMemoryText);
                            terminalPrintf("Sample mixing failed: out of memory!\n");

                            return (true);
                        }

                        memcpy(ptr8_1, &modEntry->sampleData[s->offset], MAX_SAMPLE_LEN);

                        ptr8_2 = &modEntry->sampleData[s->offset + editor.samplePos];
                        ptr8_3 = &modEntry->sampleData[s->offset + (s->length - 1)];
                        ptr8_4 = ptr8_1;

                        editor.modulateOffset = 0;
                        editor.modulatePos    = 0;

                        do
                        {
                            tmp16 = *ptr8_2 + *ptr8_1;

                            if (editor.halfClipFlag == 0)
                            {
                                smp = tmp16 / 2.0;
                                smp = ROUND_SMP_D(smp);

                                *ptr8_2++ = (int8_t)(CLAMP(smp, -128.0, 127.0));
                            }
                            else
                            {
                                *ptr8_2++ = (int8_t)(CLAMP(tmp16, -128, 127));
                            }

                            if (editor.modulateSpeed == 0)
                            {
                                ptr8_1++;
                            }
                            else
                            {
                                editor.modulatePos += editor.modulateSpeed;

                                modTmp = (editor.modulatePos / 4096) & 0x000000FF;
                                modDat = vibratoTable[modTmp & 0x1F] / 4;
                                modPos = ((modTmp & 32) ? (editor.modulateOffset - modDat) : (editor.modulateOffset + modDat)) + 2048;

                                editor.modulateOffset = modPos;

                                ptr8_1 = &ptr8_4[CLAMP(modPos / 2048, 0, s->length - 1)];
                            }
                        }
                        while (ptr8_2 < ptr8_3);

                        free(ptr8_4);

                        if (editor.ui.samplerScreenShown)
                            displaySample();

                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
                break;

                case PTB_EO_ECHO:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    if (editor.samplePos == 0)
                    {
                        displayErrorMsg("SET SAMPLE POS !");
                        break;
                    }

                    if (editor.samplePos == s->length)
                    {
                        displayErrorMsg("INVALID POS !");
                        break;
                    }

                    ptr8_1 = &modEntry->sampleData[s->offset + editor.samplePos];
                    ptr8_2 = &modEntry->sampleData[s->offset];
                    ptr8_3 = ptr8_2;

                    editor.modulateOffset = 0;
                    editor.modulatePos    = 0;

                    for (j = 0; j < s->length; ++j)
                    {
                        smp = (*ptr8_2 + *ptr8_1) / 2.0;
                        smp = ROUND_SMP_D(smp);

                        *ptr8_1++ = (int8_t)(CLAMP(smp, -128.0, 127.0));

                        if (editor.modulateSpeed == 0)
                        {
                            ptr8_2++;
                        }
                        else
                        {
                            editor.modulatePos += editor.modulateSpeed;

                            modTmp = (editor.modulatePos / 4096) & 0x000000FF;
                            modDat = vibratoTable[modTmp & 0x1F] / 4;
                            modPos = ((modTmp & 32) ? (editor.modulateOffset - modDat) : (editor.modulateOffset + modDat)) + 2048;

                            editor.modulateOffset = modPos;

                            ptr8_2 = &ptr8_3[CLAMP(modPos / 2048, 0, s->length - 1)];
                        }
                    }

                    if (editor.halfClipFlag != 0)
                    {
                        for (j = 0; j < s->length; ++j)
                            ptr8_3[j] = (int8_t)(CLAMP(ptr8_3[j] * 2, -128, 127));
                    }

                    if (editor.ui.samplerScreenShown)
                        displaySample();

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
                break;

                case PTB_EO_POS_NUM:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.samplePos = 0;
                        editor.ui.updatePosText = true;
                    }
                    else
                    {
                        editor.ui.tmpDisp32 = editor.samplePos;
                        editor.samplePosDisp = &editor.ui.tmpDisp32;

                        editor.ui.numPtr32    = &editor.ui.tmpDisp32;
                        editor.ui.numLen      = 5;
                        editor.ui.numBits     = 17;
                        editor.ui.editTextPos = 2390; // (y * 40) + x

                        getNumLine(TEXT_EDIT_HEX, PTB_EO_POS_NUM);
                    }
                }
                break;

                case PTB_EO_POS_UP:
                {
                    edPosUpButton(INCREMENT_SLOW);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_POS_DOWN:
                {
                    edPosDownButton(INCREMENT_SLOW);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_BOOST: // this is actually treble increase
                {
                    s = &modEntry->samples[editor.currSample];
                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    boostSample(editor.currSample, false);

                    if (editor.ui.samplerScreenShown)
                        displaySample();

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
                break;

                case PTB_EO_FILTER: // this is actually treble decrease
                {
                    s = &modEntry->samples[editor.currSample];
                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    filterSample(editor.currSample, false);

                    if (editor.ui.samplerScreenShown)
                        displaySample();

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
                break;

                case PTB_EO_MOD_NUM:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.modulateSpeed = 0;
                        editor.ui.updateModText = true;
                    }
                }
                break;

                case PTB_EO_MOD:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    if (editor.modulateSpeed == 0)
                    {
                        displayErrorMsg("SET MOD. SPEED !");
                        break;
                    }

                    ptr8_1 = &modEntry->sampleData[s->offset];

                    ptr8_3 = (int8_t *)(malloc(MAX_SAMPLE_LEN));
                    if (ptr8_3 == NULL)
                    {
                        displayErrorMsg(editor.outOfMemoryText);
                        terminalPrintf("Sample modulation failed: out of memory!\n");

                        return (true);
                    }

                    ptr8_2 = ptr8_3;

                    memcpy(ptr8_2, ptr8_1, MAX_SAMPLE_LEN);

                    editor.modulateOffset = 0;
                    editor.modulatePos    = 0;

                    for (j = 0; j < s->length; ++j)
                    {
                        *ptr8_1++ = *ptr8_2;

                        if (editor.modulateSpeed == 0)
                        {
                            ptr8_2++;
                        }
                        else
                        {
                            editor.modulatePos += editor.modulateSpeed;

                            modTmp = (editor.modulatePos / 4096) & 0x000000FF;
                            modDat = vibratoTable[modTmp & 0x1F] / 4;
                            modPos = ((modTmp & 32) ? (editor.modulateOffset - modDat) : (editor.modulateOffset + modDat)) + 2048;

                            editor.modulateOffset = modPos;

                            ptr8_2 = &ptr8_3[CLAMP(modPos / 2048, 0, s->length - 1)];
                        }
                    }

                    free(ptr8_3);

                    if (editor.ui.samplerScreenShown)
                        displaySample();

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
                break;

                case PTB_EO_MOD_UP:
                {
                    edModUpButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_MOD_DOWN:
                {
                    edModDownButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_X_FADE:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    ptr8_1 = &modEntry->sampleData[s->offset];
                    ptr8_2 = &modEntry->sampleData[s->offset + (s->length - 1)];

                    do
                    {
                        tmp16 = *ptr8_1 + *ptr8_2;
                        if (editor.halfClipFlag == 0)
                        {
                            smp = tmp16 / 2.0;
                            smp = ROUND_SMP_D(smp);
                            smp = CLAMP(smp, -128.0, 127.0);

                            *ptr8_1++ = (int8_t)(smp);
                            *ptr8_2-- = (int8_t)(smp);
                        }
                        else
                        {
                            tmp16 = CLAMP(tmp16, -128, 127);

                            *ptr8_1++ = (int8_t)(tmp16);
                            *ptr8_2-- = (int8_t)(tmp16);
                        }
                    }
                    while (ptr8_1 < ptr8_2);

                    if (editor.ui.samplerScreenShown)
                        displaySample();

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
                break;

                case PTB_EO_BACKWD:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    if ((editor.markEndOfs - editor.markStartOfs) > 0)
                    {
                        ptr8_1 = &modEntry->sampleData[s->offset + editor.markStartOfs];
                        ptr8_2 = &modEntry->sampleData[s->offset + editor.markEndOfs - 1];
                    }
                    else
                    {
                        ptr8_1 = &modEntry->sampleData[s->offset];
                        ptr8_2 = &modEntry->sampleData[s->offset + (s->length - 1)];
                    }

                    do
                    {
                        tmpSmp    = *ptr8_1;
                        *ptr8_1++ = *ptr8_2;
                        *ptr8_2-- = tmpSmp;
                    }
                    while (ptr8_1 < ptr8_2);

                    if (editor.ui.samplerScreenShown)
                        displaySample();

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
                break;

                case PTB_EO_CB:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    if (editor.samplePos == 0)
                    {
                        displayErrorMsg("SET SAMPLE POS !");
                        break;
                    }

                    if (editor.samplePos >= s->length)
                    {
                        displayErrorMsg("INVALID POS !");
                        break;
                    }

                    mixerKillVoiceIfReadingSample(editor.currSample);

                    memcpy(&modEntry->sampleData[s->offset], &modEntry->sampleData[s->offset + editor.samplePos], MAX_SAMPLE_LEN - editor.samplePos);
                    memset(&modEntry->sampleData[s->offset + (MAX_SAMPLE_LEN - editor.samplePos)], 0, editor.samplePos);

                    if (editor.samplePos > s->loopStart)
                    {
                        s->loopStart  = 0;
                        s->loopLength = 2;
                    }
                    else
                    {
                        s->loopStart = (s->loopStart - editor.samplePos) & 0xFFFFFFFE;
                    }

                    s->length = (s->length - editor.samplePos) & 0xFFFFFFFE;

                    editor.samplePos = 0;
                    updateCurrSample();

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
                break;

                case PTB_EO_CHORD:
                {
                    editor.ui.editOpScreen = 3;
                    renderEditOpScreen();
                }
                break;

                case PTB_EO_FU:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    if (editor.samplePos == 0)
                    {
                        displayErrorMsg("INVALID POS !");
                        break;
                    }

                    ptr8_1 = &modEntry->sampleData[s->offset];
                    for (j = 0; j < editor.samplePos; ++j)
                    {
                        smp = (*ptr8_1) * (j / 2.0) / (editor.samplePos / 2.0);
                        smp = ROUND_SMP_D(smp);

                        *ptr8_1 = (int8_t)(CLAMP(smp, -128.0, 127.0));
                        ptr8_1++;
                    }

                    if (editor.ui.samplerScreenShown)
                        displaySample();

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
                break;

                case PTB_EO_FD:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    if (editor.samplePos >= (s->length - 1))
                    {
                        displayErrorMsg("INVALID POS !");
                        break;
                    }

                    ptr8_1 = &modEntry->sampleData[s->offset + (s->length - 1)];
                    for (j = editor.samplePos; j < s->length; ++j)
                    {
                        smp = (((*ptr8_1) * ((j - editor.samplePos))) / (double)((s->length - 1) - editor.samplePos));
                        smp = ROUND_SMP_D(smp);

                        *ptr8_1 = (int8_t)(CLAMP(smp, -128.0, 127.0));
                        ptr8_1--;
                    }

                    if (editor.ui.samplerScreenShown)
                        displaySample();

                    updateWindowTitle(MOD_IS_MODIFIED);
                }
                break;

                case PTB_EO_UPSAMP:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    editor.ui.askScreenShown = true;
                    editor.ui.askScreenType  = ASK_UPSAMPLE;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("UPSAMPLE ?", NO_CARRY);
                    renderAskDialog();
                }
                break;

                case PTB_EO_DNSAMP:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    editor.ui.askScreenShown = true;
                    editor.ui.askScreenType  = ASK_DOWNSAMPLE;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("DOWNSAMPLE ?", NO_CARRY);
                    renderAskDialog();
                }
                break;

                case PTB_EO_VOL_NUM:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.sampleVol = 100;
                        editor.ui.updateVolText = true;
                    }
                    else
                    {
                        editor.ui.tmpDisp16 = editor.sampleVol;
                        editor.sampleVolDisp = &editor.ui.tmpDisp16;

                        editor.ui.numPtr16    = &editor.ui.tmpDisp16;
                        editor.ui.numLen      = 3;
                        editor.ui.editTextPos = 3711; // (y * 40) + x

                        getNumLine(TEXT_EDIT_DECIMAL, PTB_EO_VOL_NUM);
                    }
                }
                break;

                case PTB_EO_VOL:
                {
                    s = &modEntry->samples[editor.currSample];

                    if (s->length == 0)
                    {
                        displayErrorMsg("SAMPLE IS EMPTY");
                        break;
                    }

                    if (editor.sampleVol != 100)
                    {
                        ptr8_1 = &modEntry->sampleData[modEntry->samples[editor.currSample].offset];
                        for (j = 0; j < s->length; ++j)
                        {
                            smp = ((*ptr8_1) * editor.sampleVol) / 100.0;
                            smp = ROUND_SMP_D(smp);

                            *ptr8_1++ = (int8_t)(CLAMP(smp, -128.0, 127.0));
                        }

                        if (editor.ui.samplerScreenShown)
                            displaySample();

                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                }
                break;

                case PTB_EO_VOL_UP:
                {
                    edVolUpButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_VOL_DOWN:
                {
                    edVolDownButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;
                // ----------------------------------------------------------

                // Edit Op. Screen #4

                case PTB_EO_DOCHORD:
                {
                    editor.ui.askScreenShown = true;
                    editor.ui.askScreenType  = ASK_MAKE_CHORD;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("MAKE CHORD?", NO_CARRY);
                    renderAskDialog();
                }
                break;

                case PTB_EO_MAJOR:
                {
                    if (editor.note1 == 36)
                    {
                        displayErrorMsg("NO BASENOTE!");
                        break;
                    }

                    editor.oldNote1 = editor.note1;
                    editor.oldNote2 = editor.note2;
                    editor.oldNote3 = editor.note3;
                    editor.oldNote4 = editor.note4;

                    editor.note2 = editor.note1 + 4;
                    editor.note3 = editor.note1 + 7;

                    if (editor.note2 >= 36) editor.note2 -= 12;
                    if (editor.note3 >= 36) editor.note3 -= 12;

                    editor.note4 = 36;

                    editor.ui.updateNote2Text = true;
                    editor.ui.updateNote3Text = true;
                    editor.ui.updateNote4Text = true;

                    recalcChordLength();
                }
                break;

                case PTB_EO_MAJOR7:
                {
                    if (editor.note1 == 36)
                    {
                        displayErrorMsg("NO BASENOTE!");
                        break;
                    }

                    editor.oldNote1 = editor.note1;
                    editor.oldNote2 = editor.note2;
                    editor.oldNote3 = editor.note3;
                    editor.oldNote4 = editor.note4;

                    editor.note2 = editor.note1 + 4;
                    editor.note3 = editor.note1 + 7;
                    editor.note4 = editor.note1 + 11;

                    if (editor.note2 >= 36) editor.note2 -= 12;
                    if (editor.note3 >= 36) editor.note3 -= 12;
                    if (editor.note4 >= 36) editor.note4 -= 12;

                    editor.ui.updateNote2Text = true;
                    editor.ui.updateNote3Text = true;
                    editor.ui.updateNote4Text = true;

                    recalcChordLength();
                }
                break;

                case PTB_EO_NOTE1:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.note1 = 36;
                    }
                    else
                    {
                        editor.ui.changingChordNote = 1;

                        setStatusMessage("SELECT NOTE", NO_CARRY);
                        pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    }

                    editor.ui.updateNote1Text = true;
                }
                break;

                case PTB_EO_NOTE1_UP:
                {
                    edNote1UpButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_NOTE1_DOWN:
                {
                    edNote1DownButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_NOTE2:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.note2 = 36;
                    }
                    else
                    {
                        editor.ui.changingChordNote = 2;

                        setStatusMessage("SELECT NOTE", NO_CARRY);
                        pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    }

                    editor.ui.updateNote2Text = true;
                }
                break;

                case PTB_EO_NOTE2_UP:
                {
                    edNote2UpButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_NOTE2_DOWN:
                {
                    edNote2DownButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_NOTE3:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.note3 = 36;
                    }
                    else
                    {
                        editor.ui.changingChordNote = 3;

                        setStatusMessage("SELECT NOTE", NO_CARRY);
                        pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    }

                    editor.ui.updateNote3Text = true;
                }
                break;

                case PTB_EO_NOTE3_UP:
                {
                    edNote3UpButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_NOTE3_DOWN:
                {
                    edNote3DownButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_NOTE4:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.note4 = 36;
                    }
                    else
                    {
                        editor.ui.changingChordNote = 4;

                        setStatusMessage("SELECT NOTE", NO_CARRY);
                        pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    }

                    editor.ui.updateNote4Text = true;
                }
                break;

                case PTB_EO_NOTE4_UP:
                {
                    edNote4UpButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_NOTE4_DOWN:
                {
                    edNote4DownButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_EO_RESET:
                {
                    editor.note1 = 36;
                    editor.note2 = 36;
                    editor.note3 = 36;
                    editor.note4 = 36;

                    editor.chordLengthMin = false;

                    editor.ui.updateNote1Text = true;
                    editor.ui.updateNote2Text = true;
                    editor.ui.updateNote3Text = true;
                    editor.ui.updateNote4Text = true;

                    recalcChordLength();
                }
                break;

                case PTB_EO_MINOR:
                {
                    if (editor.note1 == 36)
                    {
                        displayErrorMsg("NO BASENOTE!");
                        break;
                    }

                    editor.oldNote1 = editor.note1;
                    editor.oldNote2 = editor.note2;
                    editor.oldNote3 = editor.note3;
                    editor.oldNote4 = editor.note4;

                    editor.note2 = editor.note1 + 3;
                    editor.note3 = editor.note1 + 7;

                    if (editor.note2 >= 36) editor.note2 -= 12;
                    if (editor.note3 >= 36) editor.note3 -= 12;

                    editor.note4 = 36;

                    editor.ui.updateNote2Text = true;
                    editor.ui.updateNote3Text = true;
                    editor.ui.updateNote4Text = true;

                    recalcChordLength();
                }
                break;

                case PTB_EO_MINOR7:
                {
                    if (editor.note1 == 36)
                    {
                        displayErrorMsg("NO BASENOTE!");
                        break;
                    }

                    editor.oldNote1 = editor.note1;
                    editor.oldNote2 = editor.note2;
                    editor.oldNote3 = editor.note3;
                    editor.oldNote4 = editor.note4;

                    editor.note2 = editor.note1 + 3;
                    editor.note3 = editor.note1 + 7;
                    editor.note4 = editor.note1 + 10;

                    if (editor.note2 >= 36) editor.note2 -= 12;
                    if (editor.note3 >= 36) editor.note3 -= 12;
                    if (editor.note4 >= 36) editor.note4 -= 12;

                    editor.ui.updateNote2Text = true;
                    editor.ui.updateNote3Text = true;
                    editor.ui.updateNote4Text = true;

                    recalcChordLength();
                }
                break;

                case PTB_EO_UNDO:
                {
                    editor.note1 = editor.oldNote1;
                    editor.note2 = editor.oldNote2;
                    editor.note3 = editor.oldNote3;
                    editor.note4 = editor.oldNote4;

                    editor.ui.updateNote1Text = true;
                    editor.ui.updateNote2Text = true;
                    editor.ui.updateNote3Text = true;
                    editor.ui.updateNote4Text = true;

                    recalcChordLength();
                }
                break;

                case PTB_EO_SUS4:
                {
                    if (editor.note1 == 36)
                    {
                        displayErrorMsg("NO BASENOTE!");
                        break;
                    }

                    editor.oldNote1 = editor.note1;
                    editor.oldNote2 = editor.note2;
                    editor.oldNote3 = editor.note3;
                    editor.oldNote4 = editor.note4;

                    editor.note2 = editor.note1 + 5;
                    editor.note3 = editor.note1 + 7;

                    if (editor.note2 >= 36) editor.note2 -= 12;
                    if (editor.note3 >= 36) editor.note3 -= 12;

                    editor.note4 = 36;

                    editor.ui.updateNote2Text = true;
                    editor.ui.updateNote3Text = true;
                    editor.ui.updateNote4Text = true;

                    recalcChordLength();
                }
                break;

                case PTB_EO_MAJOR6:
                {
                    if (editor.note1 == 36)
                    {
                        displayErrorMsg("NO BASENOTE!");
                        break;
                    }

                    editor.oldNote1 = editor.note1;
                    editor.oldNote2 = editor.note2;
                    editor.oldNote3 = editor.note3;
                    editor.oldNote4 = editor.note4;

                    editor.note2 = editor.note1 + 4;
                    editor.note3 = editor.note1 + 7;
                    editor.note4 = editor.note1 + 9;

                    if (editor.note2 >= 36) editor.note2 -= 12;
                    if (editor.note3 >= 36) editor.note3 -= 12;
                    if (editor.note4 >= 36) editor.note4 -= 12;

                    editor.ui.updateNote2Text = true;
                    editor.ui.updateNote3Text = true;
                    editor.ui.updateNote4Text = true;

                    recalcChordLength();
                }
                break;

                case PTB_EO_LENGTH:
                {
                    if ((modEntry->samples[editor.currSample].loopLength == 2) && (modEntry->samples[editor.currSample].loopStart == 0))
                    {
                        editor.chordLengthMin = input.mouse.rightButtonPressed ? true : false;
                        recalcChordLength();
                    }
                }
                break;

                case PTB_EO_MINOR6:
                {
                    if (editor.note1 == 36)
                    {
                        displayErrorMsg("NO BASENOTE!");
                        break;
                    }

                    editor.oldNote1 = editor.note1;
                    editor.oldNote2 = editor.note2;
                    editor.oldNote3 = editor.note3;
                    editor.oldNote4 = editor.note4;

                    editor.note2 = editor.note1 + 3;
                    editor.note3 = editor.note1 + 7;
                    editor.note4 = editor.note1 + 9;

                    if (editor.note2 >= 36) editor.note2 -= 12;
                    if (editor.note3 >= 36) editor.note3 -= 12;
                    if (editor.note4 >= 36) editor.note4 -= 12;

                    editor.ui.updateNote2Text = true;
                    editor.ui.updateNote3Text = true;
                    editor.ui.updateNote4Text = true;

                    recalcChordLength();
                }
                break;
                // ----------------------------------------------------------

                case PTB_ABOUT:
                {
                    editor.ui.aboutScreenShown ^= 1;
                    if (editor.ui.aboutScreenShown)
                    {
                        renderAboutScreen();
                    }
                    else
                    {
                             if (editor.ui.visualizerMode == VISUAL_QUADRASCOPE) renderQuadrascopeBg();
                        else if (editor.ui.visualizerMode == VISUAL_SPECTRUM   ) renderSpectrumAnalyzerBg();
                        else                                                     renderMonoscopeBg();
                    }
                }
                break;

                case PTB_DO_BADGE:
                {
                }
                break;

                case PTB_PE_PATT:
                {
                    if ((editor.currMode == MODE_IDLE) || (editor.currMode == MODE_EDIT))
                    {
                        editor.ui.tmpDisp16 = modEntry->currOrder;
                        if (editor.ui.tmpDisp16 > (modEntry->head.orderCount - 1))
                            editor.ui.tmpDisp16 =  modEntry->head.orderCount - 1;

                        editor.ui.tmpDisp16 = modEntry->head.order[editor.ui.tmpDisp16];
                        editor.currPosEdPattDisp = &editor.ui.tmpDisp16;

                        editor.ui.numPtr16    = &editor.ui.tmpDisp16;
                        editor.ui.numLen      = 2;
                        editor.ui.editTextPos = 2180; // (y * 40) + x

                        getNumLine(TEXT_EDIT_DECIMAL, PTB_PE_PATT);
                    }
                }
                break;

                case PTB_PE_PATTNAME:
                {
                    if ((editor.currMode == MODE_IDLE) || (editor.currMode == MODE_EDIT))
                    {
                        editor.ui.showTextPtr  = &editor.ui.pattNames[modEntry->head.order[modEntry->currOrder] * 16];
                        editor.ui.textEndPtr   = &editor.ui.pattNames[(modEntry->head.order[modEntry->currOrder] * 16) + 15];
                        editor.ui.textLength   = 15;
                        editor.ui.editTextPos  = 2183; // (y * 40) + x
                        editor.ui.dstOffset    = &editor.textofs.posEdPattName;
                        editor.ui.dstOffsetEnd = false;

                        getTextLine(PTB_PE_PATTNAME);

                        return (true); // yes
                    }
                }
                break;
                    
                case PTB_PE_SCROLLTOP:
                {
                    if (modEntry->currOrder != 0)
                        modSetPos(0, DONT_SET_ROW);
                }
                break;

                case PTB_PE_SCROLLUP:
                {
                    if (modEntry->currOrder > 0)
                        modSetPos(modEntry->currOrder - 1, DONT_SET_ROW);

                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_PE_SCROLLDOWN:
                {
                    if (modEntry->currOrder < (modEntry->head.orderCount - 1))
                        modSetPos(modEntry->currOrder + 1, DONT_SET_ROW);

                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_PE_SCROLLBOT:
                {
                    if (modEntry->currOrder != (modEntry->head.orderCount - 1))
                        modSetPos(modEntry->head.orderCount - 1, DONT_SET_ROW);
                }
                break;

                case PTB_PE_EXIT:
                {
                    editor.ui.aboutScreenShown = false;
                    editor.ui.posEdScreenShown = false;

                    displayMainScreen();
                }
                break;

                case PTB_POS:
                case PTB_POSED:
                {
                    editor.ui.posEdScreenShown ^= 1;
                    if (editor.ui.posEdScreenShown)
                    {
                        renderPosEdScreen();
                        editor.ui.updatePosEd = true;
                    }
                    else
                    {
                        displayMainScreen();
                    }
                }
                break;

                case PTB_POSS:
                {
                    if ((editor.currMode == MODE_IDLE) || (editor.currMode == MODE_EDIT))
                    {
                        if (input.mouse.rightButtonPressed)
                        {
                            modEntry->currOrder = 0;
                            editor.currPatternDisp = &modEntry->head.order[modEntry->currOrder];

                            if (editor.ui.posEdScreenShown)
                                editor.ui.updatePosEd = true;
                        }
                        else
                        {
                            editor.ui.tmpDisp16 = modEntry->currOrder;
                            editor.currPosDisp  = &editor.ui.tmpDisp16;


                            editor.ui.numPtr16    = &editor.ui.tmpDisp16;
                            editor.ui.numLen      = 3;
                            editor.ui.editTextPos = 169; // (y * 40) + x

                            getNumLine(TEXT_EDIT_DECIMAL, PTB_POSS);
                        }
                    }
                }
                break;

                case PTB_PATTERNS:
                {
                    if ((editor.currMode == MODE_IDLE) || (editor.currMode == MODE_EDIT))
                    {
                        if (input.mouse.rightButtonPressed)
                        {
                            modEntry->head.order[modEntry->currOrder] = 0;

                            editor.ui.updateSongSize = true;
                            updateWindowTitle(MOD_IS_MODIFIED);

                            if (editor.ui.posEdScreenShown)
                                editor.ui.updatePosEd = true;
                        }
                        else
                        {
                            editor.ui.tmpDisp16 = modEntry->head.order[modEntry->currOrder];
                            editor.currPatternDisp = &editor.ui.tmpDisp16;

                            editor.ui.numPtr16    = &editor.ui.tmpDisp16;
                            editor.ui.numLen      = 2;
                            editor.ui.editTextPos = 610; // (y * 40) + x

                            getNumLine(TEXT_EDIT_DECIMAL, PTB_PATTERNS);
                        }
                    }
                }
                break;

                case PTB_LENGTHS:
                {
                    if ((editor.currMode == MODE_IDLE) || (editor.currMode == MODE_EDIT))
                    {
                        if (input.mouse.rightButtonPressed)
                        {
                            modEntry->head.orderCount = 1;

                            tmp16 = modEntry->currOrder;
                            if (tmp16 > (modEntry->head.orderCount - 1))
                                tmp16 =  modEntry->head.orderCount - 1;

                            editor.currPosEdPattDisp = &modEntry->head.order[tmp16];

                            editor.ui.updateSongSize = true;
                            updateWindowTitle(MOD_IS_MODIFIED);

                            if (editor.ui.posEdScreenShown)
                                editor.ui.updatePosEd = true;
                        }
                        else
                        {
                            editor.ui.tmpDisp16 = modEntry->head.orderCount;
                            editor.currLengthDisp = &editor.ui.tmpDisp16;

                            editor.ui.numPtr16    = &editor.ui.tmpDisp16;
                            editor.ui.numLen      = 3;
                            editor.ui.editTextPos = 1049; // (y * 40) + x

                            getNumLine(TEXT_EDIT_DECIMAL, PTB_LENGTHS);
                        }
                    }
                }
                break;

                case PTB_PATTBOX:
                case PTB_PATTDATA:
                {
                    if (!editor.ui.introScreenShown)
                    {
                        if ((editor.currMode == MODE_IDLE) || (editor.currMode == MODE_EDIT) || (editor.playMode != PLAY_MODE_NORMAL))
                        {
                            editor.ui.tmpDisp16 = modEntry->currPattern;
                            editor.currEditPatternDisp = &editor.ui.tmpDisp16;

                            editor.ui.numPtr16    = &editor.ui.tmpDisp16;
                            editor.ui.numLen      = 2;
                            editor.ui.editTextPos = 5121; // (y * 40) + x

                            getNumLine(TEXT_EDIT_DECIMAL, PTB_PATTDATA);
                        }
                    }
                }
                break;

                case PTB_SAMPLES:
                {
                    editor.sampleZero = false;

                    editor.ui.tmpDisp8 = editor.currSample;
                    editor.currSampleDisp = &editor.ui.tmpDisp8;

                    editor.ui.numPtr8     = &editor.ui.tmpDisp8;
                    editor.ui.numLen      = 2;
                    editor.ui.numBits     = 8;
                    editor.ui.editTextPos = 1930; // (y * 40) + x

                    getNumLine(TEXT_EDIT_HEX, PTB_SAMPLES);
                }
                break;

                case PTB_SVOLUMES:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        modEntry->samples[editor.currSample].volume = 0;
                    }
                    else
                    {
                        editor.ui.tmpDisp8 = modEntry->samples[editor.currSample].volume;
                        modEntry->samples[editor.currSample].volumeDisp = &editor.ui.tmpDisp8;

                        editor.ui.numPtr8     = &editor.ui.tmpDisp8;
                        editor.ui.numLen      = 2;
                        editor.ui.numBits     = 8;
                        editor.ui.editTextPos = 2370; // (y * 40) + x

                        getNumLine(TEXT_EDIT_HEX, PTB_SVOLUMES);
                    }
                }
                break;

                case PTB_SLENGTHS:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        s = &modEntry->samples[editor.currSample];

                        mixerKillVoiceIfReadingSample(editor.currSample);

                        s->length = 0;

                        if ((s->loopLength > 2) || (s->loopStart > 0))
                        {
                            if (s->length < (s->loopStart + s->loopLength))
                                s->length =  s->loopStart + s->loopLength;
                        }

                        editor.ui.updateSongSize = true;
                        editor.ui.updateCurrSampleLength = true;

                        if (editor.ui.samplerScreenShown)
                            redrawSample();

                        updateVoiceParams();
                        recalcChordLength();
                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                    else
                    {
                        editor.ui.tmpDisp32 = modEntry->samples[editor.currSample].length;
                        modEntry->samples[editor.currSample].lengthDisp = &editor.ui.tmpDisp32;

                        editor.ui.numPtr32    = &editor.ui.tmpDisp32;
                        editor.ui.numLen      = 5;
                        editor.ui.numBits     = 17;
                        editor.ui.editTextPos = 2807; // (y * 40) + x

                        getNumLine(TEXT_EDIT_HEX, PTB_SLENGTHS);
                    }
                }
                break;

                case PTB_SREPEATS:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        s = &modEntry->samples[editor.currSample];

                        s->loopStart = 0;

                        if (s->length >= s->loopLength)
                        {
                            if ((s->loopStart + s->loopLength) > s->length)
                                 s->loopStart = s->length - s->loopLength;
                        }
                        else
                        {
                            s->loopStart = 0;
                        }

                        editor.ui.updateCurrSampleRepeat = true;
                        if (editor.ui.editOpScreenShown && (editor.ui.editOpScreen == 3))
                            editor.ui.updateLengthText = true;

                        if (editor.ui.samplerScreenShown)
                            setLoopSprites();

                        updateVoiceParams();
                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                    else
                    {
                        editor.ui.tmpDisp32 = modEntry->samples[editor.currSample].loopStart;
                        modEntry->samples[editor.currSample].loopStartDisp = &editor.ui.tmpDisp32;

                        editor.ui.numPtr32    = &editor.ui.tmpDisp32;
                        editor.ui.numLen      = 5;
                        editor.ui.numBits     = 17;
                        editor.ui.editTextPos = 3247; // (y * 40) + x

                        getNumLine(TEXT_EDIT_HEX, PTB_SREPEATS);
                    }
                }
                break;

                case PTB_SREPLENS:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        s = &modEntry->samples[editor.currSample];
                        s->loopLength = 0;

                        if (s->length >= s->loopStart)
                        {
                            if ((s->loopStart + s->loopLength) > s->length)
                                s->loopLength = s->length - s->loopStart;
                        }
                        else
                        {
                            s->loopLength = 2;
                        }

                        if (s->loopLength < 2)
                            s->loopLength = 2;

                        editor.ui.updateCurrSampleReplen = true;
                        if (editor.ui.editOpScreenShown && (editor.ui.editOpScreen == 3))
                            editor.ui.updateLengthText = true;

                        if (editor.ui.samplerScreenShown)
                            setLoopSprites();

                        updateVoiceParams();
                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                    else
                    {
                        editor.ui.tmpDisp32 = modEntry->samples[editor.currSample].loopLength;
                        modEntry->samples[editor.currSample].loopLengthDisp = &editor.ui.tmpDisp32;

                        editor.ui.numPtr32    = &editor.ui.tmpDisp32;
                        editor.ui.numLen      = 5;
                        editor.ui.numBits     = 17;
                        editor.ui.editTextPos = 3687; // (y * 40) + x

                        getNumLine(TEXT_EDIT_HEX, PTB_SREPLENS);
                    }
                }
                break;

                case PTB_EDITOP:
                {
                    if (editor.ui.editOpScreenShown)
                        editor.ui.editOpScreen = (editor.ui.editOpScreen + 1) % 3;
                    else
                        editor.ui.editOpScreenShown = true;

                    renderEditOpScreen();
                }
                break;

                case PTB_DO_LOADMODULE:
                {
                    editor.diskop.mode             = DISKOP_MODE_MOD;
                    editor.diskop.scrollOffset     = 0;
                    editor.diskop.cached           = false;
                    editor.ui.updateDiskOpFileList = true;
                    editor.ui.updateLoadMode       = true;
                }
                break;

                case PTB_DO_LOADSAMPLE:
                {
                    editor.diskop.mode             = DISKOP_MODE_SMP;
                    editor.diskop.scrollOffset     = 0;
                    editor.diskop.cached           = false;
                    editor.ui.updateDiskOpFileList = true;
                    editor.ui.updateLoadMode       = true;
                }
                break;

                case PTB_LOADSAMPLE:    // "LOAD" button next to sample name
                {
                    editor.ui.posEdScreenShown = false;

                    editor.diskop.mode         = DISKOP_MODE_SMP;
                    editor.diskop.scrollOffset = 0;
                    editor.diskop.cached       = false;

                    if (!editor.ui.diskOpScreenShown)
                    {
                        editor.ui.diskOpScreenShown = true;
                        renderDiskOpScreen();
                    }
                    else
                    {
                        editor.ui.updateDiskOpFileList = true;
                        editor.ui.updateLoadMode = true;
                    }
                }
                break;

                case PTB_DO_SAVESAMPLE:
                {
                    editor.ui.askScreenShown = true;
                    editor.ui.askScreenType  = ASK_SAVE_SAMPLE;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("SAVE SAMPLE ?", NO_CARRY);
                    renderAskDialog();
                }
                break;

                case PTB_MOD2WAV:
                {
                    editor.ui.askScreenShown = true;
                    editor.ui.askScreenType  = ASK_MOD2WAV;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("RENDER WAV FILE?", NO_CARRY);
                    renderAskDialog();
                }
                break;

                case PTB_SA_RESAMPLENOTE:
                {
                    editor.ui.changingSmpResample = true;
                    editor.ui.updateResampleNote = true;

                    setStatusMessage("SELECT NOTE", NO_CARRY);
                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                }
                break;

                case PTB_SA_RESAMPLE:
                {
                    editor.ui.askScreenShown = true;
                    editor.ui.askScreenType  = ASK_RESAMPLE;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("RESAMPLE?", NO_CARRY);
                    renderAskDialog();
                }
                break;

                case PTB_SA_SAMPLEAREA:
                {
                    if (editor.ui.sampleMarkingPos == -1)
                    {
                        samplerSamplePressed(MOUSE_BUTTON_NOT_HELD);
                        return (true);
                    }
                }
                break;

                case PTB_SA_ZOOMBARAREA:
                {
                    input.mouse.lastGUIButton = guiButton; // button repeat

                    if (!editor.ui.forceSampleDrag)
                    {
                        samplerBarPressed(MOUSE_BUTTON_NOT_HELD);
                        return (true);
                    }
                }
                break;

                case PTB_SA_FIXDC:         samplerRemoveDcOffset();      break;
                case PTB_SA_CUT:           samplerSamDelete(SAMPLE_CUT); break;
                case PTB_SA_PASTE:         samplerSamPaste();            break;
                case PTB_SA_COPY:          samplerSamCopy();             break;
                case PTB_SA_LOOP:          samplerLoopToggle();          break;
                case PTB_SA_PLAYWAVE:      samplerPlayWaveform();        break;
                case PTB_SA_PLAYDISPLAYED: samplerPlayDisplay();         break;
                case PTB_SA_PLAYRANGE:     samplerPlayRange();           break;
                case PTB_SA_RANGEALL:      samplerRangeAll();            break;
                case PTB_SA_SHOWALL:       samplerShowAll();             break;
                case PTB_SA_SHOWRANGE:     samplerShowRange();           break;
                case PTB_SA_RANGECENTER:   sampleMarkerToCenter();       break;
                case PTB_SA_RANGEBEG:      sampleMarkerToBeg();          break;
                case PTB_SA_RANGEEND:      sampleMarkerToEnd();          break;
                case PTB_SA_ZOOMOUT:       samplerZoomOut();             break;

                case PTB_SA_VOLUME:
                {
                    editor.ui.samplerVolBoxShown = true;
                    renderSamplerVolBox();
                }
                break;

                case PTB_SA_FILTERS:
                {
                    editor.ui.samplerFiltersBoxShown = true;
                    renderSamplerFiltersBox();
                    fillSampleFilterUndoBuffer();
                }
                break;

                case PTB_SA_STOP:
                {
                    for (i = 0; i < AMIGA_VOICES; ++i)
                    {
                        // shutdown scope
                        modEntry->channels[i].scopeLoopQuirk = false;
                        modEntry->channels[i].scopeEnabled   = false;
                        modEntry->channels[i].scopeTrigger   = false;

                        // shutdown voice
                        mixerKillVoice(i);
                    }
                }
                break;

                case PTB_DO_REFRESH:
                {
                    editor.diskop.scrollOffset    = 0;
                    editor.diskop.cached          = false;
                    editor.ui.updateDiskOpFileList = true;
                }
                break;

                // TODO: Find a PowerPacker packer and enable this
                // case PTB_DO_PACKMOD: editor.diskop.modPackFlg ^= 1; break;

                case PTB_DO_SAMPLEFORMAT:
                {
                    editor.diskop.smpSaveType = (editor.diskop.smpSaveType + 1) % 3;
                    editor.ui.updateSaveFormatText = true;
                }
                break;

                case PTB_DO_MODARROW:
                {
                    editor.diskop.mode             = DISKOP_MODE_MOD;
                    editor.diskop.scrollOffset     = 0;
                    editor.diskop.cached           = false;
                    editor.ui.updateDiskOpFileList = true;
                    editor.ui.updateLoadMode       = true;
                }
                break;

                case PTB_DO_SAMPLEARROW:
                {
                    editor.diskop.mode             = DISKOP_MODE_SMP;
                    editor.diskop.scrollOffset     = 0;
                    editor.diskop.cached           = false;
                    editor.ui.updateDiskOpFileList = true;
                    editor.ui.updateLoadMode       = true;
                }
                break;

                case PTB_SA_TUNETONE: toggleTuningTone(); break;

                case PTB_POSINS:
                {
                    if ((editor.currMode == MODE_IDLE) || (editor.currMode == MODE_EDIT))
                    {
                        if (modEntry->head.orderCount < 127)
                        {
                            for (i = 0; i < (127 - modEntry->currOrder); ++i)
                                modEntry->head.order[127 - i] = modEntry->head.order[126 - i];

                            modEntry->head.order[modEntry->currOrder] = 0;

                            modEntry->head.orderCount++;
                            if (modEntry->currOrder > (modEntry->head.orderCount - 1))
                                editor.currPosEdPattDisp = &modEntry->head.order[modEntry->head.orderCount - 1];
                        }

                        updateWindowTitle(MOD_IS_MODIFIED);

                        editor.ui.updateSongSize    = true;
                        editor.ui.updateSongLength  = true;
                        editor.ui.updateSongPattern = true;

                        if (editor.ui.posEdScreenShown)
                            editor.ui.updatePosEd = true;
                    }
                };
                break;

                case PTB_POSDEL:
                {
                    if ((editor.currMode == MODE_IDLE) || (editor.currMode == MODE_EDIT))
                    {
                        if (modEntry->head.orderCount > 1)
                        {
                            for (i = 0; i < (126 - modEntry->currOrder); ++i)
                                modEntry->head.order[modEntry->currOrder + i] = modEntry->head.order[modEntry->currOrder + 1 + i];

                            modEntry->head.orderCount--;
                            if (modEntry->currOrder > (modEntry->head.orderCount - 1))
                                editor.currPosEdPattDisp = &modEntry->head.order[modEntry->head.orderCount - 1];
                        }

                        updateWindowTitle(MOD_IS_MODIFIED);

                        editor.ui.updateSongSize    = true;
                        editor.ui.updateSongLength  = true;
                        editor.ui.updateSongPattern = true;

                        if (editor.ui.posEdScreenShown)
                            editor.ui.updatePosEd = true;
                    }
                }
                break;

                case PTB_DO_SAVEMODULE:
                {
                    editor.ui.askScreenShown = true;
                    editor.ui.askScreenType  = ASK_SAVE_MODULE;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("SAVE MODULE ?", NO_CARRY);
                    renderAskDialog();
                }
                break;

                case PTB_DO_DATAPATH:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        memset(editor.currPath, 0, PATH_MAX_LEN + 1);
                        editor.ui.updateDiskOpPathText = true;
                    }

                    editor.ui.showTextPtr  = editor.currPath;
                    editor.ui.textEndPtr   = &editor.currPath[PATH_MAX_LEN - 1];
                    editor.ui.textLength   = 26;
                    editor.ui.editTextPos  = 1043; // (y * 40) + x
                    editor.ui.dstOffset    = &editor.textofs.diskOpPath;
                    editor.ui.dstOffsetEnd = false;

                    getTextLine(PTB_DO_DATAPATH);
                }
                break;

                case PTB_SONGNAME:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        memset(modEntry->head.moduleTitle, 0, sizeof (modEntry->head.moduleTitle));

                        editor.ui.updateSongName = true;
                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                    else
                    {
                        editor.ui.showTextPtr  = modEntry->head.moduleTitle;
                        editor.ui.textEndPtr   = modEntry->head.moduleTitle + 19;
                        editor.ui.textLength   = 20;
                        editor.ui.editTextPos  = 4133; // (y * 40) + x
                        editor.ui.dstOffset    = NULL;
                        editor.ui.dstOffsetEnd = false;

                        getTextLine(PTB_SONGNAME);
                    }
                }
                break;

                case PTB_SAMPLENAME:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        memset(modEntry->samples[editor.currSample].text, 0, sizeof (modEntry->samples[editor.currSample].text));

                        editor.ui.updateCurrSampleName = true;
                        updateWindowTitle(MOD_IS_MODIFIED);
                    }
                    else
                    {
                        editor.ui.showTextPtr  = modEntry->samples[editor.currSample].text;
                        editor.ui.textEndPtr   = modEntry->samples[editor.currSample].text + 21;
                        editor.ui.textLength   = 22;
                        editor.ui.editTextPos  = 4573; // (y * 40) + x
                        editor.ui.dstOffset    = NULL;
                        editor.ui.dstOffsetEnd = false;

                        getTextLine(PTB_SAMPLENAME);
                    }
                }
                break;

                case PTB_PAT2SMP_HI:
                {
                    editor.ui.askScreenShown = false;
                    editor.ui.answerNo       = false;
                    editor.ui.answerYes      = true;

                    editor.pat2SmpHQ = true;
                    handleAskYes();
                }
                break;

                case PTB_PAT2SMP_LO:
                {
                    editor.ui.askScreenShown = false;
                    editor.ui.answerNo       = false;
                    editor.ui.answerYes      = true;

                    editor.pat2SmpHQ = false;
                    handleAskYes();
                }
                break;

                case PTB_SUREY:
                {
                    editor.ui.askScreenShown = false;
                    editor.ui.answerNo       = false;
                    editor.ui.answerYes      = true;

                    handleAskYes();
                }
                break;

                case PTB_PAT2SMP_ABORT:
                case PTB_SUREN:
                {
                    editor.ui.askScreenShown = false;
                    editor.ui.answerNo       = true;
                    editor.ui.answerYes      = false;

                    handleAskNo();
                }
                break;

                case PTB_VISUALS:
                {
                    if (editor.ui.aboutScreenShown)
                    {
                        editor.ui.aboutScreenShown = false;
                    }
                    else if (!input.mouse.rightButtonPressed)
                    {
                        editor.ui.visualizerMode = (editor.ui.visualizerMode + 1) % 3;
                        if (editor.ui.visualizerMode == VISUAL_SPECTRUM)
                            memset((int8_t *)(editor.spectrumVolumes), 0, sizeof (editor.spectrumVolumes));
                    }

                         if (editor.ui.visualizerMode == VISUAL_QUADRASCOPE) renderQuadrascopeBg();
                    else if (editor.ui.visualizerMode == VISUAL_SPECTRUM   ) renderSpectrumAnalyzerBg();
                    else                                                     renderMonoscopeBg();
                }
                break;

                case PTB_QUIT:
                {
                    editor.ui.askScreenShown = true;
                    editor.ui.askScreenType  = ASK_QUIT;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("REALLY QUIT ?", NO_CARRY);
                    renderAskDialog();
                }
                break;

                case PTB_CHAN1:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.muted[0] = false;
                        editor.muted[1] = true;
                        editor.muted[2] = true;
                        editor.muted[3] = true;
                    }
                    else
                    {
                        editor.muted[0] ^= 1;
                    }

                    renderMuteButtons();
                }
                break;

                case PTB_CHAN2:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.muted[0] = true;
                        editor.muted[1] = false;
                        editor.muted[2] = true;
                        editor.muted[3] = true;
                    }
                    else
                    {
                        editor.muted[1] ^= 1;
                    }

                    renderMuteButtons();
                }
                break;

                case PTB_CHAN3:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.muted[0] = true;
                        editor.muted[1] = true;
                        editor.muted[2] = false;
                        editor.muted[3] = true;
                    }
                    else
                    {
                        editor.muted[2] ^= 1;
                    }

                    renderMuteButtons();
                }
                break;

                case PTB_CHAN4:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.muted[0] = true;
                        editor.muted[1] = true;
                        editor.muted[2] = true;
                        editor.muted[3] = false;
                    }
                    else
                    {
                        editor.muted[3] ^= 1;
                    }

                    renderMuteButtons();
                }
                break;

                case PTB_SAMPLER: samplerScreen(); break;
                case PTB_SA_EXIT: exitFromSam();   break;

                case PTB_DO_FILEAREA: diskOpLoadFile((input.mouse.y - 34) / 6); break;
                case PTB_DO_PARENT:   diskOpSetPath("..", DISKOP_CACHE);        break;

                case PTB_DISKOP:
                {
                    editor.blockMarkFlag = false;

                    editor.ui.diskOpScreenShown = true;
                    renderDiskOpScreen();
                }
                break;

                case PTB_DO_EXIT:
                {
                    editor.ui.aboutScreenShown  = false;
                    editor.ui.diskOpScreenShown = false;

                    editor.blockMarkFlag = false;

                    pointerSetPreviousMode();
                    setPrevStatusMessage();

                    displayMainScreen();
                }
                break;

                case PTB_DO_SCROLLUP:
                {
                    if (editor.diskop.scrollOffset > 0)
                    {
                        editor.diskop.scrollOffset--;
                        editor.ui.updateDiskOpFileList = true;

                        input.mouse.lastGUIButton = guiButton; // button repeat
                    }
                }
                break;

                case PTB_DO_SCROLLTOP:
                {
                    editor.diskop.scrollOffset = 0;
                    editor.ui.updateDiskOpFileList = true;
                }
                break;

                case PTB_DO_SCROLLDOWN:
                {
                    if (editor.diskop.numFiles > DISKOP_LIST_SIZE)
                    {
                        if (editor.diskop.scrollOffset < (editor.diskop.numFiles - DISKOP_LIST_SIZE))
                        {
                            editor.diskop.scrollOffset++;
                            editor.ui.updateDiskOpFileList = true;

                            input.mouse.lastGUIButton = guiButton; // button repeat
                        }
                    }
                }
                break;

                case PTB_DO_SCROLLBOT:
                {
                    if (editor.diskop.numFiles > DISKOP_LIST_SIZE)
                    {
                        editor.diskop.scrollOffset = editor.diskop.numFiles - DISKOP_LIST_SIZE;
                        editor.ui.updateDiskOpFileList = true;
                    }
                }
                break;

                case PTB_STOP:
                {
                    editor.playMode = PLAY_MODE_NORMAL;

                    modStop();

                    editor.currMode = MODE_IDLE;
                    pointerSetMode(POINTER_MODE_IDLE, DO_CARRY);
                    setStatusMessage(editor.allRightText, DO_CARRY);
                }
                break;

                case PTB_PLAY:
                {
                    editor.playMode = PLAY_MODE_NORMAL;

                    if (input.mouse.rightButtonPressed)
                        modPlay(DONT_SET_PATTERN, modEntry->currOrder, modEntry->currRow);
                    else
                        modPlay(DONT_SET_PATTERN, modEntry->currOrder, DONT_SET_ROW);

                    editor.currMode = MODE_PLAY;
                    pointerSetMode(POINTER_MODE_PLAY, DO_CARRY);
                    setStatusMessage(editor.allRightText, DO_CARRY);
                }
                break;

                case PTB_PATTERN:
                {
                    editor.playMode = PLAY_MODE_PATTERN;

                    if (input.mouse.rightButtonPressed)
                        modPlay(modEntry->currPattern, DONT_SET_ORDER, modEntry->currRow);
                    else
                        modPlay(modEntry->currPattern, DONT_SET_ORDER, DONT_SET_ROW);

                    editor.currMode = MODE_PLAY;
                    pointerSetMode(POINTER_MODE_PLAY, DO_CARRY);
                    setStatusMessage(editor.allRightText, DO_CARRY);
                }
                break;

                case PTB_EDIT:
                {
                    if (!editor.ui.samplerScreenShown)
                    {
                        editor.playMode = PLAY_MODE_NORMAL;

                        modStop();

                        editor.currMode = MODE_EDIT;
                        pointerSetMode(POINTER_MODE_EDIT, DO_CARRY);
                        setStatusMessage(editor.allRightText, DO_CARRY);
                    }
                }
                break;

                case PTB_RECORD:
                {
                    if (!editor.ui.samplerScreenShown)
                    {
                        editor.playMode = PLAY_MODE_PATTERN;

                        if (input.mouse.rightButtonPressed)
                            modPlay(modEntry->currPattern, DONT_SET_ORDER, modEntry->currRow);
                        else
                            modPlay(modEntry->currPattern, DONT_SET_ORDER, DONT_SET_ROW);

                        editor.currMode = MODE_RECORD;
                        pointerSetMode(POINTER_MODE_EDIT, DO_CARRY);
                        setStatusMessage(editor.allRightText, DO_CARRY);
                    }
                }
                break;

                case PTB_CLEAR:
                {
                    editor.ui.clearScreenShown = true;

                    pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
                    setStatusMessage("PLEASE SELECT", NO_CARRY);
                    renderClearScreen();
                }
                break;

                case PTB_CLEARSONG:
                {
                    editor.ui.clearScreenShown = false;
                    removeClearScreen();

                    editor.playMode = PLAY_MODE_NORMAL;

                    modStop();
                    clearSong();

                    editor.currMode = MODE_IDLE;
                    pointerSetMode(POINTER_MODE_IDLE, DO_CARRY);
                    setStatusMessage(editor.allRightText, DO_CARRY);
                }
                break;

                case PTB_CLEARSAMPLES:
                {
                    editor.ui.clearScreenShown = false;
                    removeClearScreen();

                    editor.playMode = PLAY_MODE_NORMAL;

                    modStop();
                    clearSamples();

                    editor.currMode = MODE_IDLE;
                    pointerSetMode(POINTER_MODE_IDLE, DO_CARRY);
                    setStatusMessage(editor.allRightText, DO_CARRY);
                }
                break;

                case PTB_CLEARALL:
                {
                    editor.ui.clearScreenShown = false;
                    removeClearScreen();

                    editor.playMode = PLAY_MODE_NORMAL;

                    modStop();
                    clearAll();

                    editor.currMode = MODE_IDLE;
                    pointerSetMode(POINTER_MODE_IDLE, DO_CARRY);
                    setStatusMessage(editor.allRightText, DO_CARRY);
                }
                break;

                case PTB_CLEARCANCEL:
                {
                    editor.ui.clearScreenShown = false;
                    removeClearScreen();

                    setPrevStatusMessage();
                    pointerSetPreviousMode();

                    editor.errorMsgActive  = true;
                    editor.errorMsgBlock   = true;
                    editor.errorMsgCounter = 0;

                    pointerErrorMode();
                }
                break;

                case PTB_SAMPLEU:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.sampleZero = true;
                        editor.ui.updateCurrSampleNum = true;
                    }
                    else
                    {
                        sampleUpButton();
                        input.mouse.lastGUIButton = guiButton; // button repeat
                    }
                }
                break;

                case PTB_SAMPLED:
                {
                    if (input.mouse.rightButtonPressed)
                    {
                        editor.sampleZero = true;
                        editor.ui.updateCurrSampleNum = true;
                    }
                    else
                    {
                        sampleDownButton();
                        input.mouse.lastGUIButton = guiButton; // button repeat
                    }
                }
                break;

                case PTB_FTUNEU:
                {
                    sampleFineTuneUpButton();
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_FTUNED:
                {
                    sampleFineTuneDownButton();
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_SVOLUMEU:
                {
                    sampleVolumeUpButton();
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_SVOLUMED:
                {
                    sampleVolumeDownButton();
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_SLENGTHU:
                {
                    sampleLengthUpButton(INCREMENT_SLOW);
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_SLENGTHD:
                {
                    sampleLengthDownButton(INCREMENT_SLOW);
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_SREPEATU:
                {
                    sampleRepeatUpButton(INCREMENT_SLOW);
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_SREPEATD:
                {
                    sampleRepeatDownButton(INCREMENT_SLOW);
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_SREPLENU:
                {
                    sampleRepeatLengthUpButton(INCREMENT_SLOW);
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_SREPLEND:
                {
                    sampleRepeatLengthDownButton(INCREMENT_SLOW);
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_TEMPOU:
                {
                    tempoUpButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_TEMPOD:
                {
                    tempoDownButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_LENGTHU:
                {
                    songLengthUpButton();
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_LENGTHD:
                {
                    songLengthDownButton();
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_PATTERNU:
                {
                    patternUpButton();
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_PATTERND:
                {
                    patternDownButton();
                    updateWindowTitle(MOD_IS_MODIFIED);
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_POSU:
                {
                    positionUpButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                case PTB_POSD:
                {
                    positionDownButton();
                    input.mouse.lastGUIButton = guiButton; // button repeat
                }
                break;

                default: // button not mapped
                {
                    editor.errorMsgActive  = true;
                    editor.errorMsgBlock   = true;
                    editor.errorMsgCounter = 0;

                    pointerErrorMode();
                    setStatusMessage("NOT IMPLEMENTED", NO_CARRY);
                }
                break;
            }
        }
    }

    return (false);
}

void updateMouseCounters(void)
{
    if (input.mouse.buttonWaiting)
    {
        if (++input.mouse.buttonWaitCounter > 8)
        {
            input.mouse.buttonWaitCounter = 0;
            input.mouse.buttonWaiting = false;
        }
    }

    if (editor.errorMsgActive)
    {
        if (++editor.errorMsgCounter >= 60)
        {
            editor.errorMsgCounter = 0;

            // don't reset status text/mouse color during certain modes
            if (!editor.ui.askScreenShown &&
                !editor.ui.clearScreenShown &&
                !editor.ui.pat2SmpDialogShown &&
                !editor.ui.changingChordNote &&
                !editor.ui.changingDrumPadNote &&
                !editor.ui.changingSmpResample &&
                !editor.swapChannelFlag)
            {
                pointerSetPreviousMode();
                setPrevStatusMessage();
            }

            editor.errorMsgActive = false;
            editor.errorMsgBlock  = false;
        }
    }
}
