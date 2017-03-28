#ifndef __PT_HEADER_H
#define __PT_HEADER_H

#include <SDL2/SDL.h>
#include <stdint.h>

#ifdef _WIN32
#define PATH_MAX_LEN 260
#else
#define PATH_MAX_LEN 4096
#endif

#include "pt_config.h" // this must be included after PATH_MAX_LEN definition

#ifdef _MSC_VER
#pragma warning(disable:4244) // disable 'conversion from' warings
#pragma warning(disable:4820) // disable struct padding warnings
#pragma warning(disable:4996) // disable deprecated POSIX warnings
#pragma warning(disable:4127) // disable while (true) warnings
#endif

#define SAMPLE_VIEW_HEIGHT 64
#define SAMPLE_AREA_WIDTH 314
#define SAMPLE_AREA_WIDTH_F 314.0f

#define FILTERS_BASE_FREQ 16574

#define SCREEN_W 320
#define SCREEN_H 255

#define VBLANK_HZ 60

#define FONT_CHAR_W 8 // actual data length is 7, includes right spacing (1px column)
#define FONT_CHAR_H 5

#define MOD_ROWS 64
#define MOD_SAMPLES 31
#define MOD_ORDERS 128
#define MAX_PATTERNS 100

#define MAX_SAMPLE_LEN (65535 * 2)

#define AMIGA_VOICES 4
#define SCOPE_WIDTH 40
#define SPECTRUM_BAR_NUM 23
#define SPECTRUM_BAR_HEIGHT 36
#define SPECTRUM_BAR_WIDTH 6

#define POSED_LIST_SIZE 12

#define PAULA_PAL_CLK 3546895
#define CIA_PAL_CLK   709379

#define KEYB_REPEAT_DELAY 20

enum
{
    FORMAT_MK,     // ProTracker 1.x
    FORMAT_MK2,    // ProTracker 2.x (if tune has >64 patterns)
    FORMAT_FLT4,   // StarTrekker
    FORMAT_4CHN,   // FastTracker II (only 4 channel MODs)
    FORMAT_STK,    // The Ultimate SoundTracker (15 samples)
    FORMAT_NT,     // NoiseTracker 1.0
    FORMAT_FEST,   // NoiseTracker (special one)
    FORMAT_UNKNOWN
};

enum
{
    FLAG_NOTE      = 1,
    FLAG_SAMPLE    = 2,
    FLAG_NEWSAMPLE = 4,

    TEMPFLAG_START = 1,
    TEMPFLAG_DELAY = 2,

    FILTER_LP_ENABLED  = 1,
    FILTER_LED_ENABLED = 2,

    NO_CARRY = 0,
    DO_CARRY = 1,

    INCREMENT_SLOW = 0,
    INCREMENT_FAST = 1,

    NO_SAMPLE_CUT = 0,
    SAMPLE_CUT    = 1,

    EDIT_SPECIAL = 0,
    EDIT_NORMAL  = 1,

    EDIT_TEXT_NO_UPDATE = 0,
    EDIT_TEXT_UPDATE    = 1,

    TRANSPOSE_ALL = 1,

    MOUSE_BUTTON_NOT_HELD = 0,
    MOUSE_BUTTON_HELD     = 1,

    OPENGL_WINDOWED   = 0,
    OPENGL_FULLSCREEN = 1,

    DONT_SET_ORDER   = -1,
    DONT_SET_PATTERN = -1,
    DONT_SET_ROW     = -1,

    REMOVE_SAMPLE_MARKING = 0,
    KEEP_SAMPLE_MARKING   = 1,

    MOD_NOT_MODIFIED = 0,
    MOD_IS_MODIFIED  = 1,

    DONT_CHECK_IF_FILE_EXIST = 0,
    CHECK_IF_FILE_EXIST      = 1,

    DONT_GIVE_NEW_FILENAME = 0,
    GIVE_NEW_FILENAME      = 1,

    DONT_DOWNSAMPLE = 0,
    DO_DOWNSAMPLE   = 1,

    SCREEN_ALL         = 0,
    SCREEN_MAINSCREEN  = 1,
    SCREEN_DISKOP      = 2,
    SCREEN_SAMPLER     = 4,
    SCREEN_QUIT        = 8,
    SCREEN_CLEAR       = 16,

    VISUAL_QUADRASCOPE = 0,
    VISUAL_SPECTRUM    = 1,
    VISUAL_MONOSCOPE = 2,

    MODE_IDLE   = 0,
    MODE_EDIT   = 1,
    MODE_PLAY   = 2,
    MODE_RECORD = 3,

    RECORD_PATT = 0,
    RECORD_SONG = 1,

    CURSOR_NOTE    = 0,
    CURSOR_SAMPLE1 = 1,
    CURSOR_SAMPLE2 = 2,
    CURSOR_CMD     = 3,
    CURSOR_PARAM1  = 4,
    CURSOR_PARAM2  = 5,

    PLAY_MODE_NORMAL  = 0,
    PLAY_MODE_PATTERN = 1,

    OCTAVE_HIGH = 0,
    OCTAVE_LOW  = 1,

    AMIGA_PAL  = 0,
    AMIGA_NTSC = 1,

    DISKOP_MODE_MOD = 0,
    DISKOP_MODE_SMP = 1,

    DISKOP_SMP_WAV = 0,
    DISKOP_SMP_IFF = 1,
    DISKOP_SMP_RAW = 2,

    ASK_QUIT                  =  0,
    ASK_SAVE_MODULE           =  1,
    ASK_SAVE_SONG             =  2,
    ASK_SAVE_SAMPLE           =  3,
    ASK_MOD2WAV               =  4,
    ASK_MOD2WAV_OVERWRITE     =  5,
    ASK_SAVEMOD_OVERWRITE     =  6,
    ASK_SAVESMP_OVERWRITE     =  7,
    ASK_DOWNSAMPLING          =  8,
    ASK_RESAMPLE              = 9,
    ASK_KILL_SAMPLE           = 10,
    ASK_UPSAMPLE              = 11,
    ASK_DOWNSAMPLE            = 12,
    ASK_FILTER_ALL_SAMPLES    = 13,
    ASK_BOOST_ALL_SAMPLES     = 14,
    ASK_MAKE_CHORD            = 15,
    ASK_SAVE_ALL_SAMPLES      = 16,
    ASK_PAT2SMP               = 17,
    ASK_RESTORE_SAMPLE        = 18,

    TEMPO_MODE_CIA    = 0,
    TEMPO_MODE_VBLANK = 1,

    TEXT_EDIT_STRING  = 0,
    TEXT_EDIT_DECIMAL = 1,
    TEXT_EDIT_HEX     = 2
};

typedef struct wavHeader_t
{
    uint32_t chunkID, chunkSize, format, subchunk1ID, subchunk1Size;
    uint16_t audioFormat, numChannels;
    uint32_t sampleRate, byteRate;
    uint16_t blockAlign, bitsPerSample;
    uint32_t subchunk2ID, subchunk2Size;
} wavHeader_t;

typedef struct sampleLoop_t
{
    uint32_t dwIdentifier, dwType, dwStart;
    uint32_t dwEnd, dwFraction, dwPlayCount;
} sampleLoop_t;

typedef struct samplerChunk_t
{
    uint32_t chunkID, chunkSize, dwManufacturer, dwProduct;
    uint32_t dwSamplePeriod, dwMIDIUnityNote, wMIDIPitchFraction;
    uint32_t dwSMPTEFormat, dwSMPTEOffset, cSampleLoops, cbSamplerData;
    sampleLoop_t loop;
} samplerChunk_t;

typedef struct note_t
{
    uint8_t param, sample, command;
    uint16_t period;
} note_t;

typedef struct moduleHeader_t
{
    char moduleTitle[20 + 1];
    uint8_t ticks, format, restartPos;
    int16_t order[MOD_ORDERS], orderCount, patternCount;
    uint16_t tempo, initBPM;
    uint32_t moduleSize, totalSampleSize;
} moduleHeader_t;

typedef struct moduleSample_t
{
    volatile int8_t *volumeDisp;
    volatile int32_t *lengthDisp;
    volatile int32_t *loopStartDisp;
    volatile int32_t *loopLengthDisp;
    char text[22 + 1];
    int8_t volume;
    uint8_t fineTune;
    int32_t length, offset, loopStart, loopLength, tmpLoopStart;
} moduleSample_t;

typedef struct moduleChannel_t
{
    int8_t scopeVolume;
    double scopeReadDelta_f, scopeDrawDelta_f, scopePos_f;
    int8_t *invertLoopPtr, *invertLoopStart, volume, noNote, rawVolume;
    int8_t scopeEnabled, scopeTrigger, scopeLoopFlag, patternLoopRow;
    int8_t scopeChangePos, scopeKeepDelta, scopeKeepVolume, offsetBugNotAdded;
    int8_t pattLoopCounter, tonePortDirec, didQuantize;
    uint8_t param, flags, sample, command, fineTune, chanIndex, tempFlags;
    uint8_t tremoloCmd, vibratoCmd, tremoloPos, vibratoPos, waveControl;
    uint8_t tonePortSpeed, invertLoopDelay, invertLoopSpeed, tempFlagsBackup, glissandoControl;
    uint16_t period, wantedperiod, tempPeriod;
    int32_t offset, offsetTemp, invertLoopLength, scopeLoopQuirk;
    int32_t scopeEnd, scopeLoopBegin, scopeLoopEnd;
    double scopeLoopQuirk_f, scopeEnd_f, scopeLoopBegin_f, scopeLoopEnd_f;
} moduleChannel_t;

typedef struct module_t
{
    int8_t *sampleData;
    int8_t currRow, modified, row;
    uint8_t currSpeed, moduleLoaded;
    int16_t currOrder, currPattern;
    uint16_t currBPM;
    uint32_t rowsCounter, rowsInTotal;
    moduleHeader_t head;
    moduleSample_t samples[MOD_SAMPLES];
    note_t *patterns[MAX_PATTERNS];
    moduleChannel_t channels[AMIGA_VOICES];
} module_t;

struct input_t
{
    struct keyb_t
    {
        int8_t repeatKey, delayKey;
        uint8_t shiftKeyDown, controlKeyDown, altKeyDown;
        uint8_t leftAmigaKeyDown, keypadEnterKeyDown;
        uint8_t repeatCounter, delayCounter;
        SDL_Scancode lastRepKey, lastKey;
    } keyb;

    struct mouse_t
    {
        int8_t buttonWaiting, leftButtonPressed, rightButtonPressed;
        uint8_t repeatCounter, repeatCounter_2, buttonWaitCounter;
        int32_t lastGUIButton, lastGUIButton_2, prevX, prevY;
        int32_t x, y, lastMouseX;
        float scaleX_f, scaleY_f;
    } mouse;
} input;

// this is massive...
struct editor_t
{
    volatile int8_t vuMeterVolumes[AMIGA_VOICES];
    volatile float realVuMeterVolumes[AMIGA_VOICES];
    volatile int8_t spectrumVolumes[SPECTRUM_BAR_NUM];
    volatile int8_t *sampleFromDisp;
    volatile int8_t *sampleToDisp;
    volatile int8_t *currSampleDisp;
    volatile uint8_t isWAVRendering;
    volatile uint8_t isSMPRendering;
    volatile uint8_t modTick;
    volatile uint8_t modSpeed;
    volatile uint8_t programRunning;
    volatile int16_t *quantizeValueDisp;
    volatile int16_t *metroSpeedDisp;
    volatile int16_t *metroChannelDisp;
    volatile int16_t *sampleVolDisp;
    volatile int16_t *vol1Disp;
    volatile int16_t *vol2Disp;
    volatile int16_t *currEditPatternDisp;
    volatile int16_t *currPosDisp;
    volatile int16_t *currPatternDisp;
    volatile int16_t *currPosEdPattDisp;
    volatile int16_t *currLengthDisp;
    volatile int32_t *samplePosDisp;
    volatile int32_t *chordLengthDisp;
    volatile int32_t *lpCutOffDisp;
    volatile int32_t *hpCutOffDisp;

    char mixText[16], outOfMemoryText[18], modLoadOoMText[39], diskOpListOoMText[42];
    char allRightText[10], *fileNameTmp, *entryNameTmp, *currPath;

    int8_t smpRedoFinetunes[MOD_SAMPLES], smpRedoVolumes[MOD_SAMPLES], multiModeNext[4];
    int8_t *smpRedoBuffer[MOD_SAMPLES], *tempSample, errorMsgActive, errorMsgBlock, currSample;
    int8_t metroFlag, recordMode, sampleFrom, multiFlag, sampleTo, keypadSampleOffset;
    int8_t keypadToggle8CFlag, normalizeFiltersFlag, sampleAllFlag, trackPattFlag, halfClipFlag;
    int8_t newOldFlag, pat2SmpHQ, note1, note2, note3, note4, oldNote1, oldNote2, oldNote3, oldNote4;
    int8_t mixFlag, modLoaded, fullScreenFlag, autoInsFlag, autoInsSlot, repeatKeyFlag, sampleZero;
    int8_t accidental, transDelFlag, chordLengthMin;
    uint8_t muted[AMIGA_VOICES], *rowVisitTable, playMode, songPlaying, currMode, useLEDFilter;
    uint8_t tuningFlag, pNoteFlag, tuningVol, errorMsgCounter, stepPlayEnabled, stepPlayBackwards;
    uint8_t blockBufferFlag, buffFromPos, buffToPos, blockFromPos, blockToPos, blockMarkFlag;
    uint8_t timingMode, swapChannelFlag, f6Pos, f7Pos, f8Pos, f9Pos, f10Pos, keyOctave, tuningNote;
    uint8_t resampleNote, initialTempo, initialSpeed, editMoveAdd, configFound, abortMod2Wav, blepSynthesis;

    int16_t *mod2WavBuffer, *pat2SmpBuf, vol1, vol2, quantizeValue;
    int16_t metroSpeed, metroChannel, sampleVol, modulateSpeed;
    uint16_t effectMacros[10], oldTempo, currPlayNote, ticks50Hz;

    int32_t smpRedoLoopStarts[MOD_SAMPLES], smpRedoLoopLengths[MOD_SAMPLES], smpRedoLengths[MOD_SAMPLES];
    int32_t markStartOfs, markEndOfs, samplePos, modulatePos, modulateOffset, chordLength, playTime;
    int32_t lpCutOff, hpCutOff;
    uint32_t *scopeBuffer, pat2SmpPos, outputFreq, audioBufferSize;

    float outputFreq_f;

    note_t trackBuffer[MOD_ROWS], cmdsBuffer[MOD_ROWS], blockBuffer[MOD_ROWS];
    note_t patternBuffer[MOD_ROWS * AMIGA_VOICES], undoBuffer[MOD_ROWS * AMIGA_VOICES];

    SDL_Thread *mod2WavThread, *pat2SmpThread;

    struct diskop_t
    {
        volatile uint8_t cached;
        volatile uint8_t isFilling;
        volatile uint8_t forceStopReading;
        int8_t modDot, mode, modPackFlg, smpSaveType;
        int32_t numFiles;
        int32_t scrollOffset;
        SDL_Thread *fillThread;
    } diskop;

    struct cursor_t
    {
        uint8_t lastPos, pos, mode, channel;
        uint32_t bgBuffer[11 * 14];
    } cursor;

    struct text_offsets_t
    {
        uint16_t diskOpPath, posEdPattName;
    } textofs;

    struct ui_t
    {
        char statusMessage[18], prevStatusMessage[18], *pattNames;
        char *dstPtr, *editPos, *textEndPtr, *showTextPtr;

        int8_t *numPtr8, tmpDisp8, answerNo, answerYes, throwExit, pointerMode;
        int8_t getLineFlag, getLineType ,askScreenType, askScreenShown, visualizerMode;
        int8_t leftLoopPinMoving, rightLoopPinMoving, samplerScreenShown, previousPointerMode;
        int8_t videoScaleFactor, changingSmpResample, changingDrumPadNote, changingChordNote;
        int8_t forceSampleDrag, forceTermBarDrag, forceVolDrag, forceSampleEdit, introScreenShown;
        int8_t aboutScreenShown, clearScreenShown, posEdScreenShown, diskOpScreenShown;
        int8_t samplerVolBoxShown, samplerFiltersBoxShown, editOpScreenShown, editOpScreen;
        int8_t terminalShown, terminalWasClosed, realVuMeters;
        uint8_t numLen, numBits, blankZeroFlag, dottedCenterFlag, pattDots;

        // render/update flags
        uint8_t refreshMousePointer, updateStatusText, updatePatternData, updateSongTime;
        uint8_t updateSongName, updateMod2WavDialog ,mod2WavFinished;

        // edit op. #2
        uint8_t updateRecordText, updateQuantizeText, updateMetro1Text, updateMetro2Text;
        uint8_t updateFromText, updateKeysText, updateToText;

        // edit op. #3
        uint8_t updateMixText, updatePosText, updateModText, updateVolText;

        // edit op. #4 (sample chord editor)
        uint8_t updateLengthText, updateNote1Text, updateNote2Text;
        uint8_t updateNote3Text, updateNote4Text;

        //sampler
        uint8_t updateResampleNote, updateVolFromText, updateVolToText ,updateLPText;
        uint8_t updateHPText, updateNormFlag, update9xxPos;

        // general
        uint8_t updateSongPos, updateSongPattern, updateSongLength, updateCurrSampleFineTune;
        uint8_t updateCurrSampleNum ,updateCurrSampleVolume, updateCurrSampleLength;
        uint8_t updateCurrSampleRepeat, updateCurrSampleReplen, updateCurrSampleName;
        uint8_t updateSongSize, updateSongTiming, updateSongBPM;
        uint8_t updateCurrPattText, updateTrackerFlags, pat2SmpDialogShown;

        // disk op.
        uint8_t updateLoadMode, updatePackText, updateSaveFormatText, updateDiskOpPathText;

        // pos ed.
        uint8_t updatePosEd, updateDiskOpFileList;

        // these are used when things are drawn on top, for example clear/ask dialogs
        uint8_t disablePosEd, disableVisualizer;

        int16_t *numPtr16, tmpDisp16, lineCurX, lineCurY, editObject, sampleMarkingPos;
        uint16_t *dstOffset, dstPos, textLength, editTextPos, dstOffsetEnd, lastSampleOffset;

        int32_t *numPtr32, tmpDisp32, askTempData;

        SDL_PixelFormat *pixelFormat;
    } ui;

    struct sampler_t
    {
        const int8_t *samStart;
        int8_t *blankSample, *copyBuf, loopOnOffFlag;
        int16_t loopStartPos, loopEndPos;
        uint16_t dragStart, dragEnd, saveMouseX, lastSamPos;
        int32_t samPointWidth, samOffset, samDisplay, samLength;
        int32_t lastMouseX, lastMouseY, tmpLoopStart, tmpLoopLength;
        uint32_t copyBufSize, samDrawStart, samDrawEnd;
    } sampler;
} editor;

void restartSong(void);
void resetSong(void);
void incPatt(void);
void decPatt(void);
void modSetPos(int16_t order, int16_t row);
void modStop(void);
void doStopIt(void);
void playPattern(int8_t startRow);
void modPlay(int16_t patt, int16_t order, int8_t row);
void modSetSpeed(uint8_t speed);
void modSetTempo(uint16_t bpm);
void modFree(void);
int8_t setupAudio(void);
void audioClose(void);
void clearSong(void);
void clearSamples(void);
void clearAll(void);

extern uint8_t bigEndian; // pt_main.c
extern module_t *modEntry; // pt_main.c

#endif
