#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h> // for toupper()/tolower()
#include <math.h>
#include <SDL2/SDL.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <limits.h>
#include "pt_header.h"
#include "pt_dirent.h"
#include "pt_textout.h"
#include "pt_diskop.h"
#include "pt_tables.h"
#include "pt_palette.h"
#include "pt_modloader.h"
#include "pt_audio.h"
#include "pt_sampler.h"
#include "pt_config.h"
#include "pt_helpers.h"
#include "pt_terminal.h"
#include "pt_keyboard.h"

typedef struct fileEntry_t
{
    uint8_t type;
    uint32_t size;
    char *filename, *dateChanged;
} fileEntry_t;

static fileEntry_t *diskOpEntry = NULL;

void handleEntryJumping(char jumpToChar)
{
    char cmpChar;
    int8_t j;
    int32_t i;

    if (diskOpEntry != NULL)
    {
        // slow and unintelligent method, but whatever
        if (editor.diskop.numFiles >= 10)
        {
            for (i = 0; i <= (editor.diskop.numFiles - 10); i++)
            {
                for (j = 0; j < 10; ++j)
                {
                    if (diskOpEntry[i + j].filename != NULL)
                    {
                        if (diskOpEntry[i + j].type == DISKOP_DIR)
                            cmpChar = (char)(tolower(diskOpEntry[i + j].filename[1])); // skip dir. sorting byte
                        else
                            cmpChar = (char)(tolower(diskOpEntry[i + j].filename[0]));

                        if (jumpToChar == cmpChar)
                        {
                            // fix visual overrun
                            while ((i + j) > (editor.diskop.numFiles - 10)) j--;

                            editor.diskop.scrollOffset = i + j;

                            editor.ui.updateDiskOpFileList = true;
                            return;
                        }
                    }
                }
            }
        }
        else
        {
            // the reason we still search, is to throw an error
            // message if there is no such file name starting
            // with given character.
            for (i = 0; i < editor.diskop.numFiles; ++i)
            {
                if (diskOpEntry[i].filename != NULL)
                {
                    if (diskOpEntry[i].type == DISKOP_DIR)
                        cmpChar = (char)(tolower(diskOpEntry[i].filename[1])); // skip dir. sorting byte
                    else
                        cmpChar = (char)(tolower(diskOpEntry[i].filename[0]));

                    if (jumpToChar == cmpChar)
                    {
                        editor.ui.updateDiskOpFileList = true;
                        return;
                    }
                }
            }
        }
    }

    // character not found in file list, show red (error) mouse pointer!
    editor.errorMsgActive  = true;
    editor.errorMsgBlock   = true;
    editor.errorMsgCounter = 0;

    pointerErrorMode();
}

int8_t diskOpEntryIsEmpty(int32_t fileIndex)
{
    if ((editor.diskop.scrollOffset + fileIndex) >= editor.diskop.numFiles)
        return (true);

    return (false);
}

int8_t diskOpEntryIsDir(int32_t fileIndex)
{
    if (diskOpEntry != NULL)
    {
        if (!diskOpEntryIsEmpty(fileIndex))
            return (diskOpEntry[editor.diskop.scrollOffset + fileIndex].type); // 0 = file, 1 = dir
    }

    return (-1); // couldn't look up entry
}

char *diskOpGetEntry(int32_t fileIndex)
{
    char *filename;

    if (diskOpEntry != NULL)
    {
        if (!diskOpEntryIsEmpty(fileIndex))
        {
            filename = diskOpEntry[editor.diskop.scrollOffset + fileIndex].filename;
            if ((filename == NULL) || (filename[0] == '\0'))
                return (NULL);

            if (diskOpEntry[editor.diskop.scrollOffset + fileIndex].type == DISKOP_FILE)
                return (&filename[0]);
            else
                return (&filename[1]); // skip dir. sorting byte
        }
    }

    return (NULL);
}

void setVisualPathToCwd(void)
{
    memset(editor.currPath, 0, PATH_MAX_LEN + 1);
    getcwd(editor.currPath, PATH_MAX_LEN);
    editor.ui.updateDiskOpPathText = true;
}

int8_t diskOpSetPath(const char *path, uint8_t cache)
{
    if ((path != NULL) && (*path != '\0') && (chdir(path) == 0))
    {
        setVisualPathToCwd();

        if (cache)
            editor.diskop.cached = false;

        if (editor.ui.diskOpScreenShown)
            editor.ui.updateDiskOpFileList = true;

        editor.diskop.scrollOffset = 0;

        return (true);
    }

    setVisualPathToCwd();

    displayErrorMsg("CAN'T OPEN DIR !");
    return (false);
}

void diskOpSetInitPath(void)
{
    if (ptConfig.defaultDiskOpDir[0] != '\0') // if DEFAULTDIR is set or not in config
        diskOpSetPath(ptConfig.defaultDiskOpDir, DISKOP_CACHE);
}

int8_t allocDiskOpVars(void)
{
    editor.fileNameTmp  = (char *)(calloc(PATH_MAX_LEN + 1, sizeof (char)));
    editor.entryNameTmp = (char *)(calloc(PATH_MAX_LEN + 1, sizeof (char)));
    editor.currPath     = (char *)(calloc(PATH_MAX_LEN + 1, sizeof (char)));

    if ((editor.fileNameTmp == NULL) || (editor.entryNameTmp == NULL) ||(editor.currPath  == NULL))
    {
        return (false); // allocated leftovers are free'd lateron
    }

    return (true);
}

void deAllocDiskOpVars(void)
{
    if (editor.fileNameTmp  != NULL) free(editor.fileNameTmp);
    if (editor.entryNameTmp != NULL) free(editor.entryNameTmp);
    if (editor.currPath     != NULL) free(editor.currPath);
}

void freeDiskOpFileMem(void)
{
    int32_t i;

    if (editor.diskop.numFiles > 0)
    {
        if (!editor.errorMsgActive)
            pointerSetMode(POINTER_MODE_READ_DIR, NO_CARRY);

        for (i = 0; i < editor.diskop.numFiles; ++i)
        {
            if (diskOpEntry[i].filename    != NULL) free(diskOpEntry[i].filename);
            if (diskOpEntry[i].dateChanged != NULL) free(diskOpEntry[i].dateChanged);
        }

        if (diskOpEntry != NULL)
        {
            free(diskOpEntry);
            diskOpEntry = NULL;
        }
    }
}

void allocDiskOpMem(void)
{
    int32_t i;

    if (!editor.errorMsgActive)
        pointerSetMode(POINTER_MODE_READ_DIR, NO_CARRY);

    diskOpEntry = (fileEntry_t *)(malloc(sizeof (fileEntry_t) * editor.diskop.numFiles));
    if (diskOpEntry == NULL)
    {
        displayErrorMsg(editor.outOfMemoryText);
        terminalPrintf(editor.diskOpListOoMText);
    }

    // make sure we don't zero out pointers, safely assign them to NULL instead (for weird platforms)
    for (i = 0; i < editor.diskop.numFiles; ++i)
    {
        diskOpEntry[i].dateChanged = NULL;
        diskOpEntry[i].filename = NULL;
        diskOpEntry[i].size = 0;
        diskOpEntry[i].type = DISKOP_FILE;
    }
}

static int32_t diskOpSortCmp(const void *a, const void *b)
{
    const char *s1, *s2;
    int32_t f, l;

    // no need to check pointers, this routine is used sanely

    s1 = (const char *)((*(fileEntry_t *)(a)).filename);
    s2 = (const char *)((*(fileEntry_t *)(b)).filename);

    do
    {
        f = ((*s1 >= 'A') && (*s1 <= 'Z')) ? (*s1++ + ('a' - 'A')) : *s1++;
        l = ((*s2 >= 'A') && (*s2 <= 'Z')) ? (*s2++ + ('a' - 'A')) : *s2++;
    }
    while (f && (f == l));

    return (f - l);
}

static int32_t my_strnicmp(const char *s1, const char *s2, int32_t n)
{
    int32_t f, l;

    // no need to check pointers, this routine is used sanely

    do
    {
        f = ((*s1 >= 'A') && (*s1 <= 'Z')) ? (*s1++ + ('a' - 'A')) : *s1++;
        l = ((*s2 >= 'A') && (*s2 <= 'Z')) ? (*s2++ + ('a' - 'A')) : *s2++;
    }
    while (--n && f && (f == l));

    return (f - l);
}

static int8_t diskOpFillBufferMod(void)
{
    char tempStr[18];
    uint16_t tickCounter;
    int32_t fileHandle;
    uint32_t fileIndex, fileNameLength;
    DIR *dir;
    struct dirent *ent;
    struct stat fileStat;

    editor.diskop.scrollOffset = 0;

    // do we have a path set?
    if (editor.currPath[0] == '\0')
        setVisualPathToCwd();

    dir = opendir(editor.currPath);
    if (dir == NULL)
    {
        setVisualPathToCwd();
        return (false);
    }

    // evaluate number of files (used as a variable for memory allocation)

    freeDiskOpFileMem();

    if (!editor.errorMsgActive)
        pointerSetMode(POINTER_MODE_READ_DIR, NO_CARRY);

    tickCounter = 0;

    editor.diskop.numFiles = 0;
    while ((ent = readdir(dir)) != NULL)
    {
        if (editor.diskop.forceStopReading)
            return (false);

        if (!editor.errorMsgActive)
        {
                 if (tickCounter >= 1536) setStatusMessage("COUNTING FILES..",  NO_CARRY);
            else if (tickCounter >= 1024) setStatusMessage("COUNTING FILES.",   NO_CARRY);
            else if (tickCounter >=  512) setStatusMessage("COUNTING FILES",    NO_CARRY);
            else if (tickCounter ==    0) setStatusMessage("COUNTING FILES...", NO_CARRY);

            tickCounter = (tickCounter + 1) & 2047;
        }

        if (ent->d_name[0] == '\0')
            continue;

        fileNameLength = strlen(ent->d_name);
        if (ent->d_type == DT_REG)
        {
            if (fileNameLength >= 4)
            {
                if (
                       (!my_strnicmp(ent->d_name, "MOD.", 4) || !my_strnicmp(&ent->d_name[fileNameLength - 4], ".MOD", 4))
                    || (!my_strnicmp(ent->d_name, "STK.", 4) || !my_strnicmp(&ent->d_name[fileNameLength - 4], ".STK", 4))
                    || (!my_strnicmp(ent->d_name, "M15.", 4) || !my_strnicmp(&ent->d_name[fileNameLength - 4], ".M15", 4))
                    || (!my_strnicmp(ent->d_name, "NST.", 4) || !my_strnicmp(&ent->d_name[fileNameLength - 4], ".NST", 4))
                    || (!my_strnicmp(ent->d_name, "UST.", 4) || !my_strnicmp(&ent->d_name[fileNameLength - 4], ".UST", 4))
                    || (!my_strnicmp(ent->d_name, "PP.",  3) || !my_strnicmp(&ent->d_name[fileNameLength - 3],  ".PP", 3))
                    || (!my_strnicmp(ent->d_name, "NT.",  3) || !my_strnicmp(&ent->d_name[fileNameLength - 3],  ".NT", 3))
                   )
                {
                    editor.diskop.numFiles++;
                }
            }
        }
        else
        {
            if (!((ent->d_name[0] == '.') && (ent->d_name[1] == '\0')))
                editor.diskop.numFiles++;
        }
    }

    closedir(dir);

    allocDiskOpMem();
    if (diskOpEntry == NULL)
        return (false);

    if (!editor.errorMsgActive)
        pointerSetMode(POINTER_MODE_READ_DIR, NO_CARRY);

    dir = opendir(editor.currPath); // we opened this earlier, we know it works

    // fill disk op. buffer (type, size, path, file name, date changed)

    fileIndex = 0;
    while ((ent = readdir(dir)) != NULL)
    {
        if (editor.diskop.forceStopReading)
            return (false);

        if (!editor.errorMsgActive && (editor.diskop.numFiles > 0))
        {
            sprintf(tempStr, "READING DIR %3d%%", (uint8_t)(((((float)(fileIndex) / editor.diskop.numFiles) * 100.0f) + 0.5f)));
            setStatusMessage(tempStr, NO_CARRY);
        }

        // don't handle empty entries
        if (ent->d_name[0] == '\0')
            continue;

        fileNameLength = strlen(ent->d_name);
        if (ent->d_type == DT_REG)
        {
            if (fileNameLength < 4)
                continue;

            if (
                   (my_strnicmp(ent->d_name, "MOD.", 4) && my_strnicmp(&ent->d_name[fileNameLength - 4], ".MOD", 4))
                && (my_strnicmp(ent->d_name, "STK.", 4) && my_strnicmp(&ent->d_name[fileNameLength - 4], ".STK", 4))
                && (my_strnicmp(ent->d_name, "M15.", 4) && my_strnicmp(&ent->d_name[fileNameLength - 4], ".M15", 4))
                && (my_strnicmp(ent->d_name, "NST.", 4) && my_strnicmp(&ent->d_name[fileNameLength - 4], ".NST", 4))
                && (my_strnicmp(ent->d_name, "UST.", 4) && my_strnicmp(&ent->d_name[fileNameLength - 4], ".UST", 4))
                && (my_strnicmp(ent->d_name,  "PP.", 3) && my_strnicmp(&ent->d_name[fileNameLength - 3],  ".PP", 3))
                && (my_strnicmp(ent->d_name,  "NT.", 3) && my_strnicmp(&ent->d_name[fileNameLength - 3],  ".NT", 3))
                )
            {
                continue;
            }

            diskOpEntry[fileIndex].type = DISKOP_FILE;
        }
        else
        {
            if ((ent->d_name[0] == '.') && (ent->d_name[1] == '\0'))
                continue;

            diskOpEntry[fileIndex].type = DISKOP_DIR;
        }

        // file/dir is valid, let's move on

        diskOpEntry[fileIndex].filename = (char *)(malloc(1 + fileNameLength + 1));
        if (diskOpEntry[fileIndex].filename == NULL)
        {
            closedir(dir);
            freeDiskOpFileMem();

            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf(editor.diskOpListOoMText);

            return (false);
        }

        strcpy(diskOpEntry[fileIndex].filename, ent->d_name);

        // get file size and modification date
        if (diskOpEntry[fileIndex].type == DISKOP_FILE)
        {
            fileHandle = open(diskOpEntry[fileIndex].filename, O_RDONLY);
            if (fileHandle != -1)
            {
                diskOpEntry[fileIndex].dateChanged = (char *)(malloc(7));
                if (diskOpEntry[fileIndex].dateChanged == NULL)
                {
                    closedir(dir);
                    freeDiskOpFileMem();

                    displayErrorMsg(editor.outOfMemoryText);
                    terminalPrintf(editor.diskOpListOoMText);

                    return (false);
                }

                fstat(fileHandle, &fileStat);
                close(fileHandle);

                diskOpEntry[fileIndex].size = (uint32_t)(fileStat.st_size);
                strftime(diskOpEntry[fileIndex].dateChanged, 7, "%d%m%y", localtime(&fileStat.st_mtime));
            }
        }
        else
        {
            diskOpEntry[fileIndex].filename = (char *)(malloc(1 + fileNameLength + 1));
            if (diskOpEntry[fileIndex].filename == NULL)
            {
                closedir(dir);
                freeDiskOpFileMem();

                displayErrorMsg(editor.outOfMemoryText);
                terminalPrintf(editor.diskOpListOoMText);

                return (false);
            }

            diskOpEntry[fileIndex].filename[0] = 2; // normal dir sort priority
            if (fileNameLength == 2)
            {
                if ((ent->d_name[0] == '.') && (ent->d_name[1] == '.'))
                    diskOpEntry[fileIndex].filename[0] = 1; // sort ".." folder first by adding a dummy sort char
            }

            strcpy(diskOpEntry[fileIndex].filename + 1, ent->d_name);
        }

        fileIndex++;
    }

    closedir(dir);

    if (!editor.errorMsgActive)
        setStatusMessage("SORTING FILES..", NO_CARRY);

    qsort(diskOpEntry, editor.diskop.numFiles, sizeof (fileEntry_t), diskOpSortCmp);

    if (!editor.errorMsgActive)
    {
        pointerSetPreviousMode();
        setPrevStatusMessage();
    }

    return (true);
}

static int8_t diskOpFillBufferSmp(void)
{
    char tempStr[18];
    uint16_t tickCounter;
    int32_t fileHandle;
    uint32_t fileIndex, fileNameLength;
    DIR *dir;
    struct dirent *ent;
    struct stat fileStat;

    editor.diskop.scrollOffset = 0;

    // do we have a path set?
    if (editor.currPath[0] == '\0')
        setVisualPathToCwd();

    dir = opendir(editor.currPath);
    if (dir == NULL)
    {
        setVisualPathToCwd();
        return (false);
    }

    // evaluate number of files (used as a variable for memory allocation)

    freeDiskOpFileMem();

    if (!editor.errorMsgActive)
        pointerSetMode(POINTER_MODE_READ_DIR, NO_CARRY);

    tickCounter = 0;

    editor.diskop.numFiles = 0;
    while ((ent = readdir(dir)) != NULL)
    {
        if (editor.diskop.forceStopReading)
            return (false);

        if (!editor.errorMsgActive)
        {
                 if (tickCounter >= 1536) setStatusMessage("COUNTING FILES..",  NO_CARRY);
            else if (tickCounter >= 1024) setStatusMessage("COUNTING FILES.",   NO_CARRY);
            else if (tickCounter >=  512) setStatusMessage("COUNTING FILES",    NO_CARRY);
            else if (tickCounter ==    0) setStatusMessage("COUNTING FILES...", NO_CARRY);

            tickCounter = (tickCounter + 1) & 2047;
        }

        if (ent->d_name[0] == '\0')
            continue;

        // don't count "." directory (not read lateron)
        if (!((ent->d_type == DT_DIR) && ((ent->d_name[0] == '.') && (ent->d_name[1] == '\0'))))
            editor.diskop.numFiles++;
    }

    closedir(dir);

    allocDiskOpMem();
    if (diskOpEntry == NULL)
        return (false);

    if (!editor.errorMsgActive)
        pointerSetMode(POINTER_MODE_READ_DIR, NO_CARRY);

    dir = opendir(editor.currPath); // we opened this earlier, we know it works

    // fill disk op. buffer (type, size, path, file name, date changed)

    fileIndex = 0;
    while ((ent = readdir(dir)) != NULL)
    {
        if (editor.diskop.forceStopReading)
            return (false);

        if (!editor.errorMsgActive && (editor.diskop.numFiles > 0))
        {
            sprintf(tempStr, "READING DIR %3d%%", (uint8_t)(((((float)(fileIndex) / editor.diskop.numFiles) * 100.0f) + 0.5f)));
            setStatusMessage(tempStr, NO_CARRY);
        }

        // don't handle empty entries
        if (ent->d_name[0] == '\0')
            continue;

        // don't handle "." directory
        if ((ent->d_type == DT_DIR) && ((ent->d_name[0] == '.') && (ent->d_name[1] == '\0')))
            continue;

        diskOpEntry[fileIndex].type = (ent->d_type == DT_REG) ? DISKOP_FILE : DISKOP_DIR;

        fileNameLength = strlen(ent->d_name);

        diskOpEntry[fileIndex].filename = (char *)(malloc(fileNameLength + 1));
        if (diskOpEntry[fileIndex].filename == NULL)
        {
            closedir(dir);
            freeDiskOpFileMem();

            displayErrorMsg(editor.outOfMemoryText);
            terminalPrintf(editor.diskOpListOoMText);

            return (false);
        }

        strcpy(diskOpEntry[fileIndex].filename, ent->d_name);

        // get file size and modification date
        if (diskOpEntry[fileIndex].type == DISKOP_FILE)
        {
            fileHandle = open(diskOpEntry[fileIndex].filename, O_RDONLY);
            if (fileHandle != -1)
            {
                diskOpEntry[fileIndex].dateChanged = (char *)(malloc(7));
                if (diskOpEntry[fileIndex].dateChanged == NULL)
                {
                    closedir(dir);
                    freeDiskOpFileMem();

                    displayErrorMsg(editor.outOfMemoryText);
                    terminalPrintf(editor.diskOpListOoMText);

                    return (false);
                }

                fstat(fileHandle, &fileStat);
                close(fileHandle);

                diskOpEntry[fileIndex].size = (uint32_t)(fileStat.st_size);
                strftime(diskOpEntry[fileIndex].dateChanged, 7, "%d%m%y", localtime(&fileStat.st_mtime));
            }
        }
        else
        {
            diskOpEntry[fileIndex].filename = (char *)(malloc(1 + fileNameLength + 1));
            if (diskOpEntry[fileIndex].filename == NULL)
            {
                closedir(dir);
                freeDiskOpFileMem();

                displayErrorMsg(editor.outOfMemoryText);
                terminalPrintf(editor.diskOpListOoMText);

                return (false);
            }

            diskOpEntry[fileIndex].filename[0] = 2; // normal dir sort priority
            if (fileNameLength == 2)
            {
                if ((ent->d_name[0] == '.') && (ent->d_name[1] == '.'))
                    diskOpEntry[fileIndex].filename[0] = 1; // sort ".." folder first by adding a dummy sort char
            }

            strcpy(diskOpEntry[fileIndex].filename + 1, ent->d_name);
        }

        fileIndex++;
    }

    closedir(dir);

    if (!editor.errorMsgActive)
        setStatusMessage("SORTING FILES..", NO_CARRY);

    qsort(diskOpEntry, editor.diskop.numFiles, sizeof (fileEntry_t), diskOpSortCmp);

    if (!editor.errorMsgActive)
    {
        pointerSetPreviousMode();
        setPrevStatusMessage();
    }

    return (true);
}

int32_t diskOpFillThreadFunc(void *ptr)
{
    (void)(ptr); // make compiler happy

    editor.diskop.isFilling = true;

         if (editor.diskop.mode == DISKOP_MODE_MOD) diskOpFillBufferMod();
    else if (editor.diskop.mode == DISKOP_MODE_SMP) diskOpFillBufferSmp();

    editor.diskop.isFilling = false;
    editor.ui.updateDiskOpFileList = true;

    return (1);
}

void diskOpRenderFileList(uint32_t *frameBuffer)
{
    char tmpChar, tmpStr[7];
    uint8_t listYPos;
    int32_t i, j;
    uint32_t k, entryLength, fileSize;

    if ((editor.ui.pointerMode != POINTER_MODE_READ_DIR) && (editor.ui.pointerMode != POINTER_MODE_MSG1))
    {
        if (!editor.errorMsgActive)
        {
            if (editor.diskop.mode == DISKOP_MODE_MOD)
                setStatusMessage("SELECT MODULE", NO_CARRY);
            else
                setStatusMessage("SELECT SAMPLE", NO_CARRY);
        }
    }

    if (editor.diskop.forceStopReading)
        return;

    // if needed, update the file list and add entries
    if (!editor.diskop.cached)
    {
        editor.diskop.fillThread = SDL_CreateThread(diskOpFillThreadFunc, "ProTracker disk op fill thread", NULL);
        editor.diskop.cached = true;
    }

    // print filelist
    if (!editor.diskop.isFilling && (diskOpEntry != NULL))
    {
        // clear filelist
        for (i = 0; i < DISKOP_LIST_SIZE; ++i)
        {
            textOutBg(frameBuffer, 8, 35 + (i * (FONT_CHAR_H + 1)),
                "                                     ",
                palette[PAL_QADSCP], palette[PAL_BACKGRD]);
        }

        for (i = 0; (i < DISKOP_LIST_SIZE) && (i < editor.diskop.numFiles); ++i)
        {
            j = editor.diskop.scrollOffset + i;
            if (j < editor.diskop.numFiles)
            {
                listYPos = (uint8_t)(35 + (i * (FONT_CHAR_H + 1)));

                if (diskOpEntry[j].filename == NULL)
                {
                    if (diskOpEntry[j].type == DISKOP_FILE)
                        textOut(frameBuffer, 64, listYPos, "<COULDN'T LIST FILE>", palette[PAL_QADSCP]);
                    else
                        textOut(frameBuffer, 64, listYPos, "<COULDN'T LIST DIR>", palette[PAL_QADSCP]);

                    continue;
                }

                entryLength = strlen(diskOpEntry[j].filename);

                if (diskOpEntry[j].type == DISKOP_FILE)
                {
                    // print file name
                    if (entryLength > 23)
                    {
                        // shorten file name and add ".." to end
                        for (k = 0; k < (23 - 2); ++k)
                        {
                            tmpChar = diskOpEntry[j].filename[k];
                            if (((tmpChar < ' ') || (tmpChar > '~')) && (tmpChar != '\0'))
                                tmpChar = ' '; // was illegal character

                            charOut(frameBuffer, 64 + (k * FONT_CHAR_W), listYPos, tmpChar, palette[PAL_QADSCP]);
                        }

                        textOut(frameBuffer, 64 + ((23 - 2) * FONT_CHAR_W), listYPos, "..", palette[PAL_QADSCP]);
                    }
                    else
                    {
                        // print whole file name
                        for (k = 0; k < entryLength; ++k)
                        {
                            tmpChar = diskOpEntry[j].filename[k];
                            if (((tmpChar < ' ') || (tmpChar > '~')) && (tmpChar != '\0'))
                                tmpChar = ' '; // was illegal character

                            charOut(frameBuffer, 64 + (k * FONT_CHAR_W), listYPos, tmpChar, palette[PAL_QADSCP]);
                        }
                    }

                    // print modification date
                    if (diskOpEntry[j].dateChanged != NULL)
                        textOut(frameBuffer, 8, listYPos, diskOpEntry[j].dateChanged, palette[PAL_QADSCP]);
                    else
                        textOut(frameBuffer, 8, listYPos, "000000", palette[PAL_QADSCP]);

                    // print file size (can be optimized/cleansed further...)
                    fileSize = diskOpEntry[j].size;
                    if (fileSize == 0)
                    {
                        textOut(frameBuffer, 256, listYPos, "     0", palette[PAL_QADSCP]);
                    }
                    else if (fileSize > 999999)
                    {
                        if (fileSize > 9999999)
                        {
                            if (fileSize >= 0x80000000UL)
                            {
                                textOut(frameBuffer, 256, listYPos, "  >2GB", palette[PAL_QADSCP]);
                            }
                            else
                            {
                                fileSize /= 1000000;

                                tmpStr[3] = '0' + (fileSize % 10); fileSize /= 10;
                                tmpStr[2] = '0' + (fileSize % 10); fileSize /= 10;
                                tmpStr[1] = '0' + (fileSize % 10);
                                tmpStr[0] = '0' + (char)(fileSize / 10);
                                tmpStr[4] = 'M';
                                tmpStr[5] = 'B';
                                tmpStr[6] = '\0';

                                k = 0;
                                while (tmpStr[k]  == '0')
                                       tmpStr[k++] = ' ';

                                textOut(frameBuffer, 256, listYPos, tmpStr, palette[PAL_QADSCP]);
                            }
                        }
                        else
                        {
                            fileSize /= 1000;

                            tmpStr[3] = '0' + (fileSize % 10); fileSize /= 10;
                            tmpStr[2] = '0' + (fileSize % 10); fileSize /= 10;
                            tmpStr[1] = '0' + (fileSize % 10);
                            tmpStr[0] = '0' + (char)(fileSize / 10);
                            tmpStr[4] = 'K';
                            tmpStr[5] = 'B';
                            tmpStr[6] = '\0';
                        }

                        textOut(frameBuffer, 256, listYPos, tmpStr, palette[PAL_QADSCP]);
                    }
                    else
                    {
                        tmpStr[5] = '0' + (fileSize % 10); fileSize /= 10;
                        tmpStr[4] = '0' + (fileSize % 10); fileSize /= 10;
                        tmpStr[3] = '0' + (fileSize % 10); fileSize /= 10;
                        tmpStr[2] = '0' + (fileSize % 10); fileSize /= 10;
                        tmpStr[1] = '0' + (fileSize % 10);
                        tmpStr[0] = '0' + (char)(fileSize / 10);
                        tmpStr[6] = '\0';

                        k = 0;
                        while (tmpStr[k]  == '0')
                               tmpStr[k++] = ' ';

                        textOut(frameBuffer, 256, listYPos, tmpStr, palette[PAL_QADSCP]);
                    }
                }
                else
                {
                    // print folder
                    entryLength--; // skip / character used for dir sorting
                    if (entryLength > 24)
                    {
                        for (k = 0; k < (24 - 2); ++k)
                        {
                            tmpChar = diskOpEntry[j].filename[1 + k];
                            if (((tmpChar < ' ') || (tmpChar > '~')) && (tmpChar != '\0'))
                                tmpChar = ' '; // was illegal character

                            charOut(frameBuffer, 64 + (k * FONT_CHAR_W), listYPos, tmpChar, palette[PAL_QADSCP]);
                        }

                        textOut(frameBuffer, 64 + ((24 - 2) * FONT_CHAR_W), listYPos, "..", palette[PAL_QADSCP]);
                    }
                    else
                    {
                        // print whole folder name
                        for (k = 0; k < entryLength; ++k)
                        {
                            tmpChar = diskOpEntry[j].filename[1 + k];
                            if (((tmpChar < ' ') || (tmpChar > '~')) && (tmpChar != '\0'))
                                tmpChar = ' '; // was illegal character

                            charOut(frameBuffer, 64 + (k * FONT_CHAR_W), listYPos, tmpChar, palette[PAL_QADSCP]);
                        }
                    }

                    textOut(frameBuffer, 264, listYPos, "(DIR)", palette[PAL_QADSCP]);
                }
            }
        }
    }
}
