/* This is a very crude implementation of dirent for Windows.
** It only has the features I need for this program.
*/

#ifdef _WIN32 // this file is for Windows only

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include "pt_dirent.h"

DIR *opendir(const char *name)
{
    char path[FILENAME_MAX];
    DIR *dirp;

    if (strlen(name) > (FILENAME_MAX - 3))
        return (NULL);

    dirp = (DIR *)(malloc(sizeof (struct DIR)));
    if (dirp == NULL)
        return (NULL);

    if (GetFullPathNameA(name, FILENAME_MAX - 3, path, NULL) == 0)
    {
        free(dirp);
        return (NULL);
    }

    strcat(path, "\\*"); // append '\*' to end of path

    dirp->fHandle = FindFirstFileA(path, &dirp->fData);
    if (dirp->fHandle == INVALID_HANDLE_VALUE)
    {
        free(dirp);
        return (NULL);
    }

    return (dirp);
}

struct dirent *readdir(DIR *dirp)
{
    SYSTEMTIME stUTC;

    if ((dirp == NULL) || (dirp->fHandle == NULL))
        return (NULL);

    if (FindNextFileA(dirp->fHandle, &dirp->fData) == 0)
    {
        FindClose(dirp->fHandle);
        dirp->fHandle = NULL;
        return (NULL);
    }

    strcpy(dirp->fd.d_name, dirp->fData.cFileName);
    dirp->fd.d_namlen = (uint16_t)(strlen(dirp->fd.d_name));
    dirp->fd.d_type = (dirp->fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
    dirp->fd.d_size = (dirp->fData.nFileSizeHigh > 0) ? 0xFFFFFFFF : dirp->fData.nFileSizeLow;

    if (dirp->fd.d_type == DT_REG)
    {
        FileTimeToSystemTime(&dirp->fData.ftLastWriteTime, &stUTC);
        // we want to be ABSOLUTELY sure that this doesn't overflow in ANY circumstance, hence unneeded modulus on day/month
        sprintf(dirp->fd.lastModDate, "%02d%02d%02d", stUTC.wDay % (99 + 1), stUTC.wMonth % (99 + 1), stUTC.wYear % (99 + 1));
    }

    return (&dirp->fd);
}

int32_t closedir(DIR *dirp)
{
    if (dirp == NULL)
        return (-1);

    if (dirp->fHandle != NULL)
    {
        FindClose(dirp->fHandle);
        dirp->fHandle = NULL;
    }

    free(dirp);

    return (0);
}
#endif
