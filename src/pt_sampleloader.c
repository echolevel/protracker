#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h> // for toupper()/tolower()
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pt_header.h"
#include "pt_textout.h"
#include "pt_palette.h"
#include "pt_sampler.h"
#include "pt_audio.h"
#include "pt_sampleloader.h"
#include "pt_visuals.h"
#include "pt_helpers.h"
#include "pt_terminal.h"

enum
{
    WAVE_FORMAT_PCM        = 0x0001,
    WAVE_FORMAT_IEEE_FLOAT = 0x0003
};

int8_t loadWAVSample(const char *fileName, char *entryName, int8_t forceDownSampling);
int8_t loadIFFSample(const char *fileName, char *entryName);
int8_t loadRAWSample(const char *fileName, char *entryName);

void extLoadWAVSampleCallback(int8_t downsample)
{
    loadWAVSample(editor.fileNameTmp, editor.entryNameTmp, downsample);
}

int8_t loadWAVSample(const char *fileName, char *entryName, int8_t forceDownSampling)
{
    // Stereo is combined to mono
    // '32-bit float' and 16-bit is quantized to 8-bit
    // pre-"2x downsampling" (if wanted by the user)

    int8_t skippingChunks;
    uint8_t *audioDataU8, wavSampleNameFound;
    int16_t *audioDataS16, tempVol;
    uint16_t audioFormat, numChannels, bitsPerSample;
    int32_t *audioDataS32;
    uint32_t *audioDataU32, i, nameLen, chunkID, chunkSize;
    uint32_t sampleLength, sampleRate, fileLength, loopFlags;
    uint32_t loopStart, loopEnd, endOfChunkPos;
    float *audioDataFloat, smp_f;
    double smp_d;
    FILE *f;

    wavSampleNameFound = false;

    if (forceDownSampling == -1)
    {
        // these two *must* be fully wiped, for outputting reasons
        memset(editor.fileNameTmp,  0, PATH_MAX_LEN);
        memset(editor.entryNameTmp, 0, PATH_MAX_LEN);

        strcpy(editor.fileNameTmp,  fileName);
        strcpy(editor.entryNameTmp, entryName);
    }

    f = fopen(fileName, "rb");
    if (f == NULL)
    {
        displayErrorMsg("FILE I/O ERROR !");
        terminalPrintf("WAV sample loading failed: file input/output error\n");

        return (false);
    }

    fseek(f, 0, SEEK_END);
    fileLength = ftell(f);
    fseek(f, 0, SEEK_SET);

    fread(&chunkID, 4, 1, f); if (bigEndian) chunkID = SWAP32(chunkID);
    if (chunkID != 0x46464952)
    {
        displayErrorMsg("NOT A WAV !");
        terminalPrintf("WAV sample loading failed: not a valid .WAV\n");

        fclose(f);

        return (false);
    }

    fseek(f, 8, SEEK_CUR); // unneeded stuff

    fread(&chunkID,     4, 1, f); if (bigEndian) chunkID     = SWAP32(chunkID);
    fread(&chunkSize,   4, 1, f); if (bigEndian) chunkSize   = SWAP32(chunkSize);
    fread(&audioFormat, 2, 1, f); if (bigEndian) audioFormat = SWAP16(audioFormat);
    fread(&numChannels, 2, 1, f); if (bigEndian) numChannels = SWAP16(numChannels);
    fread(&sampleRate,  4, 1, f); if (bigEndian) sampleRate  = SWAP16(sampleRate);

    fseek(f, 6, SEEK_CUR); // unneeded stuff

    fread(&bitsPerSample, 2, 1, f); if (bigEndian) bitsPerSample = SWAP16(bitsPerSample);

    if (chunkID != 0x20746D66)
    {
        displayErrorMsg("NOT A WAV !");
        terminalPrintf("WAV sample loading failed: not a valid .WAV\n");

        fclose(f);

        return (false);
    }

    if ((chunkSize > 16) && ((ftell(f) + chunkSize) < fileLength))
        fseek(f, chunkSize - 16, SEEK_CUR);

    skippingChunks = true;
    while (skippingChunks && !feof(f))
    {
        fread(&chunkID,   4, 1, f); if (bigEndian) chunkID   = SWAP32(chunkID);
        fread(&chunkSize, 4, 1, f); if (bigEndian) chunkSize = SWAP32(chunkSize);

        switch (chunkID)
        {
            case 0x61746164: // "data"
                skippingChunks = false;
            break;

            default:
            {
                // uninteresting chunk, skip its content

                // if odd chunk size, take pad byte into consideration
                if (chunkSize & 1)
                    chunkSize++;

                if ((ftell(f) + chunkSize) >= fileLength)
                {
                    fclose(f);

                    displayErrorMsg("WAV IS CORRUPT !");
                    terminalPrintf("WAV sample loading failed: not a valid .WAV\n");

                    return (false);
                }

                if (chunkSize > 0)
                    fseek(f, chunkSize, SEEK_CUR);
            }
            break;
        }
    }

    sampleLength = chunkSize;

    if ((chunkID != 0x61746164) || (sampleLength >= fileLength))
    {
        displayErrorMsg("NOT A WAV !");
        terminalPrintf("WAV sample loading failed: not a valid .WAV\n");

        fclose(f);

        return (false);
    }

    if ((audioFormat != WAVE_FORMAT_PCM) && (audioFormat != WAVE_FORMAT_IEEE_FLOAT))
    {
        displayErrorMsg("WAV UNSUPPORTED !");
        terminalPrintf("WAV sample loading failed: unsupported type (not PCM integer or PCM float)\n");
        fclose(f);

        return (false);
    }

    if ((numChannels == 0) || (numChannels > 2))
    {
        displayErrorMsg("WAV UNSUPPORTED !");
        terminalPrintf("WAV sample loading failed: unsupported type (doesn't have 1 or 2 channels)\n");

        fclose(f);

        return (false);
    }

    if ((audioFormat == WAVE_FORMAT_IEEE_FLOAT) && (bitsPerSample != 32))
    {
        displayErrorMsg("WAV UNSUPPORTED !");
        terminalPrintf("WAV sample loading failed: unsupported type (not 8-bit, 16-bit, 24-bit, 32-bit or 32-bit float)\n");

        fclose(f);

        return (false);
    }

    if ((bitsPerSample != 8) && (bitsPerSample != 16) && (bitsPerSample != 24) && (bitsPerSample != 32))
    {
        displayErrorMsg("WAV UNSUPPORTED !");
        terminalPrintf("WAV sample loading failed: unsupported type (not 8-bit, 16-bit, 24-bit, 32-bit or 32-bit float)\n");

        fclose(f);

        return (false);
    }

    if (sampleRate > 22050)
    {
        if (forceDownSampling == -1)
        {
            editor.ui.askScreenShown = true;
            editor.ui.askScreenType  = ASK_DOWNSAMPLING;

            pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
            setStatusMessage("2X DOWNSAMPLING ?", NO_CARRY);
            renderAskDialog();

            fclose(f);

            return (true);
        }
    }
    else
    {
        forceDownSampling = false;
    }

    endOfChunkPos = ftell(f) + sampleLength;
    if (sampleLength & 1)
        endOfChunkPos++; // align byte

    if (bitsPerSample == 8) // 8-BIT INTEGER SAMPLE
    {
        if (sampleLength > (MAX_SAMPLE_LEN * 4))
            sampleLength =  MAX_SAMPLE_LEN * 4;

        audioDataU8 = (uint8_t *)(malloc(sampleLength * sizeof (uint8_t)));
        if (audioDataU8 == NULL)
        {
            fclose(f);
            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf("WAV sample loading failed: out of memory!\n");

            return (false);
        }

        // read sample data
        if (fread(audioDataU8, 1, sampleLength, f) != sampleLength)
        {
            fclose(f);
            free(audioDataU8);
            displayErrorMsg("I/O ERROR !");
            terminalPrintf("WAV sample loading failed: I/O error!\n");

            return (false);
        }

        // convert from stereo to mono (if needed)
        if (numChannels == 2)
        {
            sampleLength /= 2;

            // add right channel to left channel
            for (i = 0; i < (sampleLength - 1); i++)
            {
                smp_d = (audioDataU8[(i * 2) + 0] + audioDataU8[(i * 2) + 1]) / 2.0;
                smp_d = ROUND_SMP_D(smp_d);
                smp_d = CLAMP(smp_d, 0.0, 255.0);

                audioDataU8[i] = (uint8_t)(smp_d);
            }
        }

        // 2x downsampling - remove every other sample (if needed)
        if (forceDownSampling)
        {
            sampleLength /= 2;
            for (i = 1; i < sampleLength; i++)
                audioDataU8[i] = audioDataU8[i * 2];
        }

        if (sampleLength > MAX_SAMPLE_LEN)
            sampleLength = MAX_SAMPLE_LEN;

        mixerKillVoiceIfReadingSample(editor.currSample);
        for (i = 0; i < MAX_SAMPLE_LEN; ++i)
        {
            if (i <= (sampleLength & 0xFFFFFFFE))
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = audioDataU8[i] - 128;
            else
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = 0;
        }

        free(audioDataU8);
    }
    else if (bitsPerSample == 16) // 16-BIT INTEGER SAMPLE
    {
        sampleLength /= 2;
        if (sampleLength > (MAX_SAMPLE_LEN * 4))
            sampleLength =  MAX_SAMPLE_LEN * 4;

        audioDataS16 = (int16_t *)(malloc(sampleLength * sizeof (int16_t)));
        if (audioDataS16 == NULL)
        {
            fclose(f);
            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf("WAV sample loading failed: out of memory!\n");

            return (false);
        }

        // read sample data
        if (fread(audioDataS16, 2, sampleLength, f) != sampleLength)
        {
            fclose(f);
            free(audioDataS16);
            displayErrorMsg("I/O ERROR !");
            terminalPrintf("WAV sample loading failed: I/O error!\n");

            return (false);
        }

        // convert endianness (if needed)
        if (bigEndian)
        {
            for (i = 0; i < sampleLength; ++i)
                audioDataS16[i] = SWAP16(audioDataS16[i]);
        }

        // convert from stereo to mono (if needed)
        if (numChannels == 2)
        {
            sampleLength /= 2;

            // add right channel to left channel
            for (i = 0; i < (sampleLength - 1); i++)
            {
                smp_d = (audioDataS16[(i * 2) + 0] + audioDataS16[(i * 2) + 1]) / 2.0;
                smp_d = ROUND_SMP_D(smp_d);
                smp_d = CLAMP(smp_d, -32768.0, 32767.0);

                audioDataS16[i] = (int16_t)(smp_d);
            }
        }

        // 2x downsampling - remove every other sample (if needed)
        if (forceDownSampling)
        {
            sampleLength /= 2;
            for (i = 1; i < sampleLength; i++)
                audioDataS16[i] = audioDataS16[i * 2];
        }

        if (sampleLength > MAX_SAMPLE_LEN)
            sampleLength = MAX_SAMPLE_LEN;

        normalize16bitSigned(audioDataS16, sampleLength);

        mixerKillVoiceIfReadingSample(editor.currSample);
        for (i = 0; i < MAX_SAMPLE_LEN; ++i)
        {
            if (i <= (sampleLength & 0xFFFFFFFE))
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = quantize16bitTo8bit(audioDataS16[i]);
            else
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = 0;
        }

        free(audioDataS16);
    }
    else if (bitsPerSample == 24) // 24-BIT INTEGER SAMPLE
    {
        sampleLength /= (4 - 1);
        if (sampleLength > (MAX_SAMPLE_LEN * 4))
            sampleLength =  MAX_SAMPLE_LEN * 4;

        audioDataS32 = (int32_t *)(malloc(sampleLength * sizeof (int32_t)));
        if (audioDataS32 == NULL)
        {
            fclose(f);
            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf("WAV sample loading failed: out of memory!\n");

            return (false);
        }

        // read sample data
        audioDataU8 = (uint8_t *)(audioDataS32);
        for (i = 0; i < sampleLength; i++)
        {
            fread(&audioDataU8[1], 1, 1, f);
            fread(&audioDataU8[2], 1, 1, f);
            fread(&audioDataU8[3], 1, 1, f);
            audioDataU8[0] = 0;

            audioDataU8 += 4;
        }

        // convert endianness (if needed)
        if (bigEndian)
        {
            for (i = 0; i < sampleLength; ++i)
                audioDataS32[i] = SWAP32(audioDataS32[i]);
        }

        // convert from stereo to mono (if needed)
        if (numChannels == 2)
        {
            sampleLength /= 2;

            // add right channel to left channel
            for (i = 0; i < (sampleLength - 1); i++)
            {
                smp_d = (audioDataS32[(i * 2) + 0] / 2.0) + (audioDataS32[(i * 2) + 1] / 2.0);
                smp_d = ROUND_SMP_D(smp_d);
                smp_d = CLAMP(smp_d, -8388608.0, 8388607.0);

                audioDataS32[i] = (int32_t)(smp_d);
            }
        }

        // 2x downsampling - remove every other sample (if needed)
        if (forceDownSampling)
        {
            sampleLength /= 2;
            for (i = 1; i < sampleLength; i++)
                audioDataS32[i] = audioDataS32[i * 2];
        }

        if (sampleLength > MAX_SAMPLE_LEN)
            sampleLength = MAX_SAMPLE_LEN;

        normalize24bitSigned(audioDataS32, sampleLength);

        mixerKillVoiceIfReadingSample(editor.currSample);
        for (i = 0; i < MAX_SAMPLE_LEN; ++i)
        {
            if (i <= (sampleLength & 0xFFFFFFFE))
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = quantize24bitTo8bit(audioDataS32[i]);
            else
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = 0;
        }

        free(audioDataS32);
    }
    else if ((audioFormat == 1) && (bitsPerSample == 32)) // 32-BIT INTEGER SAMPLE
    {
        sampleLength /= 4;
        if (sampleLength > (MAX_SAMPLE_LEN * 4))
            sampleLength =  MAX_SAMPLE_LEN * 4;

        audioDataS32 = (int32_t *)(malloc(sampleLength * sizeof (int32_t)));
        if (audioDataS32 == NULL)
        {
            fclose(f);
            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf("WAV sample loading failed: out of memory!\n");

            return (false);
        }

        // read sample data
        if (fread(audioDataS32, 4, sampleLength, f) != sampleLength)
        {
            fclose(f);
            free(audioDataS32);
            displayErrorMsg("I/O ERROR !");
            terminalPrintf("WAV sample loading failed: I/O error!\n");

            return (false);
        }

        // convert endianness (if needed)
        if (bigEndian)
        {
            for (i = 0; i < sampleLength; ++i)
                audioDataS32[i] = SWAP32(audioDataS32[i]);
        }

        // convert from stereo to mono (if needed)
        if (numChannels == 2)
        {
            sampleLength /= 2;

            // add right channel to left channel
            for (i = 0; i < (sampleLength - 1); i++)
                audioDataS32[i] = (audioDataS32[(i * 2) + 0] / 2) + (audioDataS32[(i * 2) + 1] / 2);
        }

        // 2x downsampling - remove every other sample (if needed)
        if (forceDownSampling)
        {
            sampleLength /= 2;
            for (i = 1; i < sampleLength; i++)
                audioDataS32[i] = audioDataS32[i * 2];
        }

        if (sampleLength > MAX_SAMPLE_LEN)
            sampleLength = MAX_SAMPLE_LEN;

        normalize32bitSigned(audioDataS32, sampleLength);

        mixerKillVoiceIfReadingSample(editor.currSample);
        for (i = 0; i < MAX_SAMPLE_LEN; ++i)
        {
            if (i <= (sampleLength & 0xFFFFFFFE))
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = quantize32bitTo8bit(audioDataS32[i]);
            else
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = 0;
        }

        free(audioDataS32);
    }
    else if ((audioFormat == 3) && (bitsPerSample == 32)) // 32-BIT FLOAT SAMPLE
    {
        sampleLength /= 4;
        if (sampleLength > (MAX_SAMPLE_LEN * 4))
            sampleLength =  MAX_SAMPLE_LEN * 4;

        audioDataU32 = (uint32_t *)(malloc(sampleLength * sizeof (uint32_t)));
        if (audioDataU32 == NULL)
        {
            fclose(f);
            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf("WAV sample loading failed: out of memory!\n");

            return (false);
        }

        // read sample data
        if (fread(audioDataU32, 4, sampleLength, f) != sampleLength)
        {
            fclose(f);
            free(audioDataU32);
            displayErrorMsg("I/O ERROR !");
            terminalPrintf("WAV sample loading failed: I/O error!\n");

            return (false);
        }

        // convert endianness (if needed)
        if (bigEndian)
        {
            for (i = 0; i < sampleLength; ++i)
                audioDataU32[i] = SWAP32(audioDataU32[i]);
        }

        audioDataFloat = (float *)(audioDataU32);

        // convert from stereo to mono (if needed)
        if (numChannels == 2)
        {
            sampleLength /= 2;

            // add right channel to left channel
            for (i = 0; i < (sampleLength - 1); i++)
            {
                smp_f = (audioDataFloat[(i * 2) + 0] + audioDataFloat[(i * 2) + 1]) / 2.0f;
                smp_f = CLAMP(smp_f, -1.0f, 1.0f);

                audioDataFloat[i] = smp_f;
            }
        }

        // 2x downsampling - remove every other sample (if needed)
        if (forceDownSampling)
        {
            sampleLength /= 2;
            for (i = 1; i < sampleLength; i++)
                audioDataFloat[i] = audioDataFloat[i * 2];
        }

        if (sampleLength > MAX_SAMPLE_LEN)
            sampleLength = MAX_SAMPLE_LEN;

        normalize8bitFloatSigned(audioDataFloat, sampleLength);

        mixerKillVoiceIfReadingSample(editor.currSample);
        for (i = 0; i < MAX_SAMPLE_LEN; ++i)
        {
            if (i <= (sampleLength & 0xFFFFFFFE))
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = quantizeFloatTo8bit(audioDataFloat[i]);
            else
                modEntry->sampleData[(editor.currSample * MAX_SAMPLE_LEN) + i] = 0;
        }

        free(audioDataU32);
    }

    fseek(f, endOfChunkPos, SEEK_SET);

    // set sample length
    if (sampleLength & 1)
    {
        if (++sampleLength > MAX_SAMPLE_LEN)
              sampleLength = MAX_SAMPLE_LEN;
    }

    modEntry->samples[editor.currSample].length = sampleLength;

    modEntry->samples[editor.currSample].fineTune   = 0;
    modEntry->samples[editor.currSample].volume     = 64;
    modEntry->samples[editor.currSample].loopStart  = 0;
    modEntry->samples[editor.currSample].loopLength = 2;

    // look for extra chunks
    while (!feof(f) && ((uint32_t)(ftell(f)) <= (fileLength - 8)))
    {
        fread(&chunkID,   sizeof (int32_t), 1, f); if (bigEndian) chunkID   = SWAP32(chunkID);
        fread(&chunkSize, sizeof (int32_t), 1, f); if (bigEndian) chunkSize = SWAP32(chunkSize);

        // if odd chunk size, take pad byte into consideration
        if (chunkSize & 1)
            chunkSize++;

        endOfChunkPos = ftell(f) + chunkSize;
        if (endOfChunkPos > fileLength)
            break;

        switch (chunkID)
        {
            case 0x6C706D73: // "smpl"
            {
                if (chunkSize >= 52)
                {
                    fseek(f, 28, SEEK_CUR);
                    fread(&loopFlags, 4, 1, f); if (bigEndian) loopFlags = SWAP32(loopFlags);
                    fseek(f, 12, SEEK_CUR);
                    fread(&loopStart, 4, 1, f); if (bigEndian) loopStart = SWAP32(loopStart);
                    fread(&loopEnd,   4, 1, f); if (bigEndian) loopEnd   = SWAP32(loopEnd);

                    if (loopFlags)
                    {
                        if (forceDownSampling)
                        {
                            // we already downsampled 2x, so we're half the original length
                            loopStart /= 2;
                            loopEnd   /= 2;
                        }

                        loopStart &= 0xFFFFFFFE;
                        loopEnd = (loopEnd + 1) & 0xFFFFFFFE;

                        if (loopEnd <= sampleLength)
                        {
                            modEntry->samples[editor.currSample].loopStart  = loopStart;
                            modEntry->samples[editor.currSample].loopLength = loopEnd - loopStart;
                        }
                    }
                }
            }
            break;

            case 0x61727478: // "xtra"
            {
                if (chunkSize >= 8)
                {
                    fseek(f, 6, SEEK_CUR);
                    fread(&tempVol, 2, 1, f);

                    tempVol = CLAMP(tempVol, 0, 256);
                    modEntry->samples[editor.currSample].volume = (int8_t)(((tempVol * (64.0f / 256.0f)) + 0.5f));
                }
            }
            break;

            case 0x5453494C: // "LIST"
            {
                if (chunkSize >= 13)
                {
                    fread(&chunkID, sizeof (int32_t), 1, f);
                    if (chunkID == 0x4f464e49) // "INFO"
                    {
                        /* This is hackish, it assumes that INAM follows INFO, which
                        ** may not always be the case. This is better than nothing, anyways.
                        */

                        fread(&chunkID,   sizeof (int32_t), 1, f);
                        fread(&chunkSize, sizeof (int32_t), 1, f);

                        if ((chunkID == 0x4D414E49) && (chunkSize > 0)) // "INAM"
                        {
                            for (i = 0; i < 21; ++i)
                            {
                                if (i < chunkSize)
                                    modEntry->samples[editor.currSample].text[i] = (char)(tolower(fgetc(f)));
                                else
                                    modEntry->samples[editor.currSample].text[i] = '\0';
                            }

                            modEntry->samples[editor.currSample].text[21] = '\0';
                            modEntry->samples[editor.currSample].text[22] = '\0';

                            wavSampleNameFound = true;
                        }
                    }
                }
            }
            break;

            default: break;
        }

        fseek(f, endOfChunkPos, SEEK_SET);
    }

    fclose(f);

    // copy over sample name
    if (!wavSampleNameFound)
    {
        nameLen = strlen(entryName);
        for (i = 0; i < 21; ++i)
            modEntry->samples[editor.currSample].text[i] = (i < nameLen) ? (char)(tolower(entryName[i])) : '\0';

        modEntry->samples[editor.currSample].text[21] = '\0';
        modEntry->samples[editor.currSample].text[22] = '\0';
    }

    // remove .wav from end of sample name (if present)
    nameLen = strlen(modEntry->samples[editor.currSample].text);
    if ((nameLen >= 4) && !strncmp(&modEntry->samples[editor.currSample].text[nameLen - 4], ".wav", 4))
          memset(&modEntry->samples[editor.currSample].text[nameLen - 4], '\0',   4);

    editor.sampleZero = false;
    editor.samplePos  = 0;

    updateVoiceParams();
    updateCurrSample();
    fillSampleRedoBuffer(editor.currSample);

    terminalPrintf("WAV sample \"%s\" loaded into sample slot %02x\n", modEntry->samples[editor.currSample].text, editor.currSample + 1);

    updateWindowTitle(MOD_IS_MODIFIED);
    return (true);
}

int8_t loadIFFSample(const char *fileName, char *entryName)
{
    char tmpCharBuf[23];
    int8_t *sampleData;
    uint8_t nameFound, skippingBlocks, is16Bit;
    int16_t sample16, *ptr16;
    uint32_t i, nameLen, sampleLength, sampleLoopStart;
    uint32_t sampleLoopLength, sampleVolume;
    uint32_t blockName, blockSize, fileLength;
    FILE *f;

    f = fopen(fileName, "rb");
    if (f == NULL)
    {
        displayErrorMsg("FILE I/O ERROR !");
        terminalPrintf("IFF sample loading failed: file input/output error\n");

        return (false);
    }

    fseek(f, 0, SEEK_END);
    fileLength = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileLength == 0)
    {
        displayErrorMsg("IFF IS CORRUPT !");
        terminalPrintf("IFF sample loading failed: not a valid .IFF\n");

        return (false);
    }

    fseek(f, 8, SEEK_SET);
    fread(tmpCharBuf, 1, 4, f);
    is16Bit = !strncmp(tmpCharBuf, "16SV", 4);

    sampleLength   = 0;
    skippingBlocks = true;
    nameFound      = false;
    sampleVolume   = 65536; // max volume

    fseek(f, 12, SEEK_SET);
    while (skippingBlocks && !feof(f))
    {
        // blockName is little-endian, blockSize is big-endian
        fread(&blockName, 4, 1, f); if ( bigEndian) blockName = SWAP32(blockName);
        fread(&blockSize, 4, 1, f); if (!bigEndian) blockSize = SWAP32(blockSize);

        if (blockSize & 1)
            blockSize++;

        switch (blockName)
        {
            case 0x52444856: // VHDR
            {
                if (blockSize < 20)
                {
                    fclose(f);

                    displayErrorMsg("IFF IS CORRUPT !");
                    terminalPrintf("IFF sample loading failed: not a valid .IFF\n");

                    return (false);
                }

                fread(&sampleLoopStart,  4, 1, f); if (!bigEndian) sampleLoopStart  = SWAP32(sampleLoopStart);
                fread(&sampleLoopLength, 4, 1, f); if (!bigEndian) sampleLoopLength = SWAP32(sampleLoopLength);

                fseek(f, 4 + 2 + 1, SEEK_CUR);

                if (fgetc(f) != 0) // sample type
                {
                    fclose(f);

                    displayErrorMsg("UNSUPPORTED IFF !");
                    terminalPrintf("IFF sample loading failed: unsupported .IFF\n");

                    return (false);
                }

                fread(&sampleVolume, 4, 1, f); if (!bigEndian) sampleVolume = SWAP32(sampleVolume);
                if (sampleVolume > 65536)
                    sampleVolume = 65536;
                sampleVolume = (uint32_t)((sampleVolume / 1024.0f) + 0.5f);

                if (blockSize > 20)
                    fseek(f, blockSize - 20, SEEK_CUR);
            }
            break;

            case 0x454D414E: // NAME
            {
                memset(tmpCharBuf, 0, 22);

                if (blockSize > 21)
                {
                    fread(tmpCharBuf, 1, 21, f);
                    fseek(f, blockSize - 21, SEEK_CUR);
                }
                else
                {
                    fread(tmpCharBuf, 1, blockSize, f);
                }

                nameFound = true;
            }
            break;

            case 0x59444F42: // BODY
            {
                skippingBlocks = false;
                sampleLength   = blockSize;
            }
            break;

            default:
            {
                // uninteresting block, skip its content

                if ((ftell(f) + blockSize) >= fileLength)
                {
                    fclose(f);

                    displayErrorMsg("IFF IS CORRUPT !");
                    terminalPrintf("IFF sample loading failed: not a valid .IFF\n");

                    return (false);
                }

                if (blockSize > 0)
                    fseek(f, blockSize, SEEK_CUR);
            }
            break;
        }
    }

    if ((sampleLength == 0) || skippingBlocks || feof(f))
    {
        fclose(f);

        displayErrorMsg("NOT A VALID IFF !");
        terminalPrintf("IFF sample loading failed: not a valid .IFF\n");

        return (false);
    }

    sampleData = (int8_t *)(malloc(sampleLength));
    if (sampleData == NULL)
    {
        fclose(f);

        displayErrorMsg(editor.outOfMemoryText);
        terminalPrintf("IFF sample loading failed: out of memory!\n");

        return (false);
    }

    sampleLength     &= 0xFFFFFFFE;
    sampleLoopStart  &= 0xFFFFFFFE;
    sampleLoopLength &= 0xFFFFFFFE;

    if (is16Bit)
    {
        sampleLength     /= 2;
        sampleLoopStart  /= 2;
        sampleLoopLength /= 2;
    }

    if (sampleLength > MAX_SAMPLE_LEN)
        sampleLength = MAX_SAMPLE_LEN;

    if (sampleLoopLength < 2)
    {
        sampleLoopStart  = 0;
        sampleLoopLength = 2;
    }

    if ((sampleLoopStart >= MAX_SAMPLE_LEN) || (sampleLoopLength > MAX_SAMPLE_LEN))
    {
        sampleLoopStart  = 0;
        sampleLoopLength = 2;
    }

    if ((sampleLoopStart + sampleLoopLength) > sampleLength)
    {
        sampleLoopStart  = 0;
        sampleLoopLength = 2;
    }

    if (sampleLoopStart > (sampleLength - 2))
    {
        sampleLoopStart  = 0;
        sampleLoopLength = 2;
    }

    mixerKillVoiceIfReadingSample(editor.currSample);
    memset(modEntry->sampleData + modEntry->samples[editor.currSample].offset, 0, MAX_SAMPLE_LEN);

    if (is16Bit) // FT2 specific 16SV format (little-endian samples)
    {
        fread(sampleData, 1, sampleLength * 2, f);

        ptr16 = (int16_t *)(sampleData);
        for (i = 0; i < sampleLength; ++i)
        {
            sample16 = ptr16[i]; if (bigEndian) sample16 = SWAP16(sample16);
            modEntry->sampleData[modEntry->samples[editor.currSample].offset + i] = quantize16bitTo8bit(sample16);
        }
    }
    else
    {
        fread(sampleData, 1, sampleLength, f);
        memcpy(modEntry->sampleData + modEntry->samples[editor.currSample].offset, sampleData, sampleLength);
    }

    fclose(f);
    free(sampleData);

    // set sample attributes
    modEntry->samples[editor.currSample].volume     = sampleVolume;
    modEntry->samples[editor.currSample].fineTune   = 0;
    modEntry->samples[editor.currSample].length     = sampleLength;
    modEntry->samples[editor.currSample].loopStart  = sampleLoopStart;
    modEntry->samples[editor.currSample].loopLength = sampleLoopLength;

    // copy over sample name

    memset(modEntry->samples[editor.currSample].text, '\0', 23);

    if (nameFound)
    {
        nameLen = strlen(tmpCharBuf);
        for (i = 0; i < nameLen; ++i)
            modEntry->samples[editor.currSample].text[i] = (char)(tolower(tmpCharBuf[i]));
    }
    else
    {
        nameLen = strlen(entryName);
        for (i = 0; i < nameLen; ++i)
            modEntry->samples[editor.currSample].text[i] = (char)(tolower(entryName[i]));
    }

    // remove .iff from end of sample name (if present)
    nameLen = strlen(modEntry->samples[editor.currSample].text);
    if ((nameLen >= 4) && !strncmp(&modEntry->samples[editor.currSample].text[nameLen - 4], ".iff", 4))
          memset(&modEntry->samples[editor.currSample].text[nameLen - 4], '\0', 4);

    editor.sampleZero = false;
    editor.samplePos  = 0;

    updateVoiceParams();
    updateCurrSample();
    fillSampleRedoBuffer(editor.currSample);

    terminalPrintf("IFF sample \"%s\" loaded into sample slot %02x\n", modEntry->samples[editor.currSample].text, editor.currSample + 1);

    updateWindowTitle(MOD_IS_MODIFIED);
    return (false);
}

int8_t loadRAWSample(const char *fileName, char *entryName)
{
    uint8_t i;
    uint32_t nameLen, fileSize;
    FILE *f;

    f = fopen(fileName, "rb");
    if (f == NULL)
    {
        displayErrorMsg("FILE I/O ERROR !");
        terminalPrintf("RAW sample loading failed: file input/output error\n");

        return (false);
    }

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    fileSize &= 0xFFFFFFFE;
    if (fileSize > MAX_SAMPLE_LEN)
        fileSize = MAX_SAMPLE_LEN;

    mixerKillVoiceIfReadingSample(editor.currSample);

    memset(modEntry->sampleData + modEntry->samples[editor.currSample].offset, 0, MAX_SAMPLE_LEN);
    fread(modEntry->sampleData + modEntry->samples[editor.currSample].offset, 1, fileSize, f);
    fclose(f);

    // set sample attributes
    modEntry->samples[editor.currSample].volume     = 64;
    modEntry->samples[editor.currSample].fineTune   = 0;
    modEntry->samples[editor.currSample].length     = fileSize;
    modEntry->samples[editor.currSample].loopStart  = 0;
    modEntry->samples[editor.currSample].loopLength = 2;

    // copy over sample name
    nameLen = strlen(entryName);
    for (i = 0; i < 21; ++i)
        modEntry->samples[editor.currSample].text[i] = (i < nameLen) ? (char)(tolower(entryName[i])) : '\0';

    modEntry->samples[editor.currSample].text[21] = '\0';
    modEntry->samples[editor.currSample].text[22] = '\0';

    editor.sampleZero = false;
    editor.samplePos  = 0;

    updateVoiceParams();
    updateCurrSample();
    fillSampleRedoBuffer(editor.currSample);

    terminalPrintf("RAW sample \"%s\" loaded into sample slot %02x\n", modEntry->samples[editor.currSample].text, editor.currSample + 1);

    updateWindowTitle(MOD_IS_MODIFIED);
    return (true);
}

int8_t loadSample(const char *fileName, char *entryName)
{
    uint32_t fileSize, ID;
    FILE *f;

    f = fopen(fileName, "rb");
    if (f == NULL)
    {
        displayErrorMsg("FILE I/O ERROR !");
        terminalPrintf("Sample loading failed: file input/output error\n");

        return (false);
    }

    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // first, check heades before we eventually load as RAW
    if (fileSize > 16)
    {
        fread(&ID, 4, 1, f); if (bigEndian) ID = SWAP32(ID);

        // check if it's actually a WAV sample
        if (ID == 0x46464952) // "RIFF"
        {
            fseek(f, 4, SEEK_CUR);
            fread(&ID, 4, 1, f); if (bigEndian) ID = SWAP32(ID);

            if (ID == 0x45564157) // "WAVE"
            {
                fread(&ID, 4, 1, f); if (bigEndian) ID = SWAP32(ID);

                if (ID == 0x20746D66) // "fmt "
                {
                    fclose(f);
                    return (loadWAVSample(fileName, entryName, -1));
                }
            }
        }

        // check if it's an Amiga IFF sample
        else if (ID == 0x4D524F46) // "FORM"
        {
            fseek(f, 4, SEEK_CUR);
            fread(&ID, 4, 1, f); if (bigEndian) ID = SWAP32(ID);

            if ((ID == 0x58565338) || (ID == 0x56533631)) // "8SVX" (normal) and "16SV" (FT2 sample)
            {
                fclose(f);
                return (loadIFFSample(fileName, entryName));
            }
        }
    }

    // nope, continue loading as RAW
    fclose(f);

    return (loadRAWSample(fileName, entryName));
}

int8_t saveSample(int8_t checkIfFileExist, int8_t giveNewFreeFilename)
{
    char fileName[48];
    uint8_t smp;
    uint16_t j;
    int32_t i, sampleTextLen;
    uint32_t sampleSize, iffSize, iffSampleSize;
    uint32_t loopStart, loopLen, tmp32;
    FILE *f;
    struct stat statBuffer;
    moduleSample_t *s;
    wavHeader_t wavHeader;
    samplerChunk_t samplerChunk;

    if (modEntry->samples[editor.currSample].length == 0)
    {
        displayErrorMsg("SAMPLE IS EMPTY");
        terminalPrintf("Sample saving failed: sample data is empty\n");

        return (false);
    }

    memset(fileName, 0, sizeof (fileName));

    if (*modEntry->samples[editor.currSample].text == '\0')
    {
        strcpy(fileName, "untitled");
    }
    else
    {
        for (i = 0; i < 22; ++i)
        {
            fileName[i] = (char)(tolower(modEntry->samples[editor.currSample].text[i]));
            if (fileName[i] == '\0')
                break;

            // convert illegal file name characters to spaces
                 if (fileName[i] ==  '<') fileName[i] = ' ';
            else if (fileName[i] ==  '>') fileName[i] = ' ';
            else if (fileName[i] ==  ':') fileName[i] = ' ';
            else if (fileName[i] ==  '"') fileName[i] = ' ';
            else if (fileName[i] ==  '/') fileName[i] = ' ';
            else if (fileName[i] == '\\') fileName[i] = ' ';
            else if (fileName[i] ==  '|') fileName[i] = ' ';
            else if (fileName[i] ==  '?') fileName[i] = ' ';
            else if (fileName[i] ==  '*') fileName[i] = ' ';
        }
    }

    // remove .wav/.iff from end of sample name (if present)
    if (!strncmp(fileName + (strlen(fileName) - 4), ".wav", 4) || !strncmp(fileName + (strlen(fileName) - 4), ".iff", 4))
        fileName[strlen(fileName) - 4] = '\0';

    switch (editor.diskop.smpSaveType)
    {
        case DISKOP_SMP_WAV: strcat(fileName, ".wav"); break;
        case DISKOP_SMP_IFF: strcat(fileName, ".iff"); break;
        case DISKOP_SMP_RAW:                           break;

        default: return (false); // make compiler happy
    }

    if (giveNewFreeFilename)
    {
        if (stat(fileName, &statBuffer) == 0)
        {
            for (j = 1; j <= 9999; ++j) // This number should satisfy all! ;)
            {
                memset(fileName, 0, sizeof (fileName));

                if (*modEntry->samples[editor.currSample].text == '\0')
                {
                    sprintf(fileName, "untitled-%d", j);
                }
                else
                {
                    for (i = 0; i < 22; ++i)
                    {
                        fileName[i] = (char)(tolower(modEntry->samples[editor.currSample].text[i]));
                        if (fileName[i] == '\0')
                            break;

                        // convert illegal file name characters to spaces
                             if (fileName[i] ==  '<') fileName[i] = ' ';
                        else if (fileName[i] ==  '>') fileName[i] = ' ';
                        else if (fileName[i] ==  ':') fileName[i] = ' ';
                        else if (fileName[i] ==  '"') fileName[i] = ' ';
                        else if (fileName[i] ==  '/') fileName[i] = ' ';
                        else if (fileName[i] == '\\') fileName[i] = ' ';
                        else if (fileName[i] ==  '|') fileName[i] = ' ';
                        else if (fileName[i] ==  '?') fileName[i] = ' ';
                        else if (fileName[i] ==  '*') fileName[i] = ' ';
                    }

                    // remove .wav/.iff from end of sample name (if present)
                    if (!strncmp(fileName + (strlen(fileName) - 4), ".wav", 4) || !strncmp(fileName + (strlen(fileName) - 4), ".iff", 4))
                        fileName[strlen(fileName) - 4] = '\0';

                    sprintf(fileName, "%s-%d", fileName, j);
                }

                switch (editor.diskop.smpSaveType)
                {
                    case DISKOP_SMP_WAV: strcat(fileName, ".wav"); break;
                    case DISKOP_SMP_IFF: strcat(fileName, ".iff"); break;
                    case DISKOP_SMP_RAW:                           break;

                    default: return (false);  // make compiler happy
                }

                if (stat(fileName, &statBuffer) != 0)
                    break;
            }
        }
    }

    if (checkIfFileExist)
    {
        if (stat(fileName, &statBuffer) == 0)
        {
            editor.ui.askScreenShown = true;
            editor.ui.askScreenType  = ASK_SAVESMP_OVERWRITE;

            pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
            setStatusMessage("OVERWRITE FILE ?", NO_CARRY);
            renderAskDialog();

            return (-1);
        }
    }

    if (editor.ui.askScreenShown)
    {
        editor.ui.answerNo       = false;
        editor.ui.answerYes      = false;
        editor.ui.askScreenShown = false;
    }

    f = fopen(fileName, "wb");
    if (f == NULL)
    {
        displayErrorMsg("FILE I/O ERROR !");
        terminalPrintf("Sample saving failed: file input/output error\n");

        return (false);
    }

    sampleSize = modEntry->samples[editor.currSample].length;

    switch (editor.diskop.smpSaveType)
    {
        case DISKOP_SMP_WAV:
        {
            s = &modEntry->samples[editor.currSample];

            wavHeader.format        = bigEndian ? SWAP32(0x45564157) : 0x45564157; // "WAVE"
            wavHeader.chunkID       = bigEndian ? SWAP32(0x46464952) : 0x46464952; // "RIFF"
            wavHeader.subchunk1ID   = bigEndian ? SWAP32(0x20746D66) : 0x20746D66; // "fmt "
            wavHeader.subchunk2ID   = bigEndian ? SWAP32(0x61746164) : 0x61746164; // "data"
            wavHeader.subchunk1Size = bigEndian ? SWAP32(16) : 16;
            wavHeader.subchunk2Size = bigEndian ? SWAP32(sampleSize) : sampleSize;
            wavHeader.chunkSize     = bigEndian ? SWAP32(wavHeader.subchunk2Size + 36) : (wavHeader.subchunk2Size + 36);
            wavHeader.audioFormat   = bigEndian ? SWAP16(1) : 1;
            wavHeader.numChannels   = bigEndian ? SWAP16(1) : 1;
            wavHeader.bitsPerSample = bigEndian ? SWAP16(8) : 8;
            wavHeader.sampleRate    = bigEndian ? SWAP32(16574) : 16574;
            wavHeader.byteRate      = bigEndian ? SWAP32(wavHeader.sampleRate * wavHeader.numChannels * wavHeader.bitsPerSample / 8)
                                    : (wavHeader.sampleRate * wavHeader.numChannels * wavHeader.bitsPerSample / 8);
            wavHeader.blockAlign    = bigEndian ? SWAP16(wavHeader.numChannels * wavHeader.bitsPerSample / 8)
                                    : (wavHeader.numChannels * wavHeader.bitsPerSample / 8);

            if (s->loopLength > 2)
            {
                wavHeader.chunkSize += sizeof (samplerChunk_t);

                memset(&samplerChunk, 0, sizeof (samplerChunk_t));

                samplerChunk.chunkID         = bigEndian ? SWAP32(0x6C706D73) : 0x6C706D73; // "smpl"
                samplerChunk.chunkSize       = bigEndian ? SWAP32(60) : 60;
                samplerChunk.dwSamplePeriod  = bigEndian ? SWAP32(1000000000U / 16574) : (1000000000U / 16574);
                samplerChunk.dwMIDIUnityNote = bigEndian ? SWAP32(60) : (60); // 60 = C-4
                samplerChunk.cSampleLoops    = bigEndian ? SWAP32(1) : 1;
                samplerChunk.loop.dwStart    = bigEndian ? (uint32_t)(SWAP32(s->loopStart)) : (uint32_t)(s->loopStart);
                samplerChunk.loop.dwEnd      = bigEndian ? (uint32_t)(SWAP32((s->loopStart + s->loopLength) - 1)) :
                                                                  (uint32_t)((s->loopStart + s->loopLength) - 1);
            }

            fwrite(&wavHeader, sizeof (wavHeader_t), 1, f);

            for (i = 0; i < (int32_t)(sampleSize); ++i)
            {
                smp = modEntry->sampleData[modEntry->samples[editor.currSample].offset + i] + 128;
                fputc(smp, f);
            }

            if (sampleSize & 1)
                fputc(0, f); // pad align byte

            if (s->loopLength > 2)
                fwrite(&samplerChunk, sizeof (samplerChunk), 1, f);

            if (sampleNameIsEmpty(modEntry->samples[editor.currSample].text))
            {
                terminalPrintf("Sample %02x \"untitled\" saved as .WAV\n", editor.currSample + 1);
            }
            else
            {
                terminalPrintf("Sample %02x \"", editor.currSample + 1);

                sampleTextLen = 22;
                for (i = 21; i >= 0; --i)
                {
                    if (modEntry->samples[editor.currSample].text[i] == '\0')
                        sampleTextLen--;
                    else
                        break;
                }

                for (i = 0; i < sampleTextLen; ++i)
                {
                    if (modEntry->samples[editor.currSample].text[i] != '\0')
                        teriminalPutChar(tolower(modEntry->samples[editor.currSample].text[i]));
                    else
                        teriminalPutChar(' ');
                }

                terminalPrintf("\" saved as .WAV\n");
            }
        }
        break;

        case DISKOP_SMP_IFF:
        {
            // dwords are big-endian in IFF
            loopStart = modEntry->samples[editor.currSample].loopStart  & 0xFFFFFFFE;
            loopLen   = modEntry->samples[editor.currSample].loopLength & 0xFFFFFFFE;

            if (!bigEndian) loopStart = SWAP32(loopStart);
            if (!bigEndian) loopLen   = SWAP32(loopLen);

            iffSize = bigEndian ? (sampleSize + 100) : SWAP32(sampleSize + 100);
            iffSampleSize = bigEndian ? sampleSize : SWAP32(sampleSize);

            fputc(0x46, f);fputc(0x4F, f);fputc(0x52, f);fputc(0x4D, f);    // "FORM"
            fwrite(&iffSize, 4, 1, f);

            fputc(0x38, f);fputc(0x53, f);fputc(0x56, f);fputc(0x58, f);    // "8SVX"
            fputc(0x56, f);fputc(0x48, f);fputc(0x44, f);fputc(0x52, f);    // "VHDR"
            fputc(0x00, f);fputc(0x00, f);fputc(0x00, f);fputc(0x14, f);    // 0x00000014

            if (modEntry->samples[editor.currSample].loopLength > 2)
            {
                fwrite(&loopStart, 4, 1, f);
                fwrite(&loopLen,   4, 1, f);
            }
            else
            {
                fwrite(&iffSampleSize, 4, 1, f);
                fputc(0x00, f);fputc(0x00, f);fputc(0x00, f);fputc(0x00, f);// 0x00000000
            }

            fputc(0x00, f);fputc(0x00, f);fputc(0x00, f);fputc(0x00, f);// 0x00000000

            fputc(0x41, f);fputc(0x56, f); // 16726 (rate)
            fputc(0x01, f);fputc(0x00, f); // numSamples and compression

            tmp32 = modEntry->samples[editor.currSample].volume * 1024;
            if (!bigEndian) tmp32 = SWAP32(tmp32);
            fwrite(&tmp32, 4, 1, f);

            fputc(0x4E, f);fputc(0x41, f);fputc(0x4D, f);fputc(0x45, f);    // "NAME"
            fputc(0x00, f);fputc(0x00, f);fputc(0x00, f);fputc(0x16, f);    // 0x00000016

            for (i = 0; i < 22; ++i)
                fputc(tolower(modEntry->samples[editor.currSample].text[i]), f);

            fputc(0x41, f);fputc(0x4E, f);fputc(0x4E, f);fputc(0x4F, f);    // "ANNO"
            fputc(0x00, f);fputc(0x00, f);fputc(0x00, f);fputc(0x15, f);    // 0x00000015
            fprintf(f, "ProTracker 2.3D clone");
            fputc(0x00, f); // even padding

            fputc(0x42, f);fputc(0x4F, f);fputc(0x44, f);fputc(0x59, f);    // "BODY"
            fwrite(&iffSampleSize, 4, 1, f);
            fwrite(modEntry->sampleData + modEntry->samples[editor.currSample].offset, 1, sampleSize, f);

            // shouldn't happen, but in just case: safety even padding
            if (sampleSize & 1)
                fputc(0x00, f);

            if (sampleNameIsEmpty(modEntry->samples[editor.currSample].text))
            {
                terminalPrintf("Sample %02x \"untitled\" saved as .IFF\n", editor.currSample + 1);
            }
            else
            {
                terminalPrintf("Sample %02x \"", editor.currSample + 1);

                sampleTextLen = 22;
                for (i = 21; i >= 0; --i)
                {
                    if (modEntry->samples[editor.currSample].text[i] == '\0')
                        sampleTextLen--;
                    else
                        break;
                }

                for (i = 0; i < sampleTextLen; ++i)
                {
                    if (modEntry->samples[editor.currSample].text[i] != '\0')
                        teriminalPutChar(tolower(modEntry->samples[editor.currSample].text[i]));
                    else
                        teriminalPutChar(' ');
                }

                terminalPrintf("\" saved as .IFF\n");
            }
        }
        break;

        case DISKOP_SMP_RAW:
        {
            fwrite(modEntry->sampleData + modEntry->samples[editor.currSample].offset, 1, sampleSize, f);

            if (sampleNameIsEmpty(modEntry->samples[editor.currSample].text))
            {
                terminalPrintf("Sample %02x \"untitled\" saved as raw file\n", editor.currSample + 1);
            }
            else
            {
                terminalPrintf("Sample %02x \"", editor.currSample + 1);

                sampleTextLen = 22;
                for (i = 21; i >= 0; --i)
                {
                    if (modEntry->samples[editor.currSample].text[i] == '\0')
                        sampleTextLen--;
                    else
                        break;
                }

                for (i = 0; i < sampleTextLen; ++i)
                {
                    if (modEntry->samples[editor.currSample].text[i] != '\0')
                        teriminalPutChar(tolower(modEntry->samples[editor.currSample].text[i]));
                    else
                        teriminalPutChar(' ');
                }

                terminalPrintf("\" saved as raw file\n");
            }
        }
        break;

        default: return (false); break;  // make compiler happy
    }

    fclose(f);

    editor.diskop.cached = false;
    if (editor.ui.diskOpScreenShown)
        editor.ui.updateDiskOpFileList = true;

    displayMsg("SAMPLE SAVED !");
    setMsgPointer();

    return (true);
}
