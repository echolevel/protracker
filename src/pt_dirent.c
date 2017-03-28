/* This is a very crude implementation of dirent for Windows.
** It only has the features I need for this program.
*/

#ifdef _WIN32 // this file is for Windows only

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "pt_dirent.h"

DIR *opendir(const char *name)
{
    DIR *dirp;

    if (strlen(name) > (FILENAME_MAX - 3))
        return (NULL);

    dirp = (DIR *)(malloc(sizeof (struct DIR)));
    if (dirp == NULL)
        return (NULL);

    if (GetFullPathNameA(name, FILENAME_MAX - 3, dirp->path, NULL) == 0)
    {
        free(dirp);
        return (NULL);
    }

    strcat(dirp->path, "\\*"); /* append '\*' to end of path */

    dirp->fHandle = FindFirstFileA(dirp->path, &dirp->fData);
    if (dirp->fHandle == INVALID_HANDLE_VALUE)
    {
        free(dirp);
        return (NULL);
    }

    return (dirp);
}

struct dirent *readdir(DIR *dirp)
{
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
