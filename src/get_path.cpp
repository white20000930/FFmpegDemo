#include "get_path.h"

#include <stdlib.h>
#include <string.h>
#include <cstdio>

const char *GetAbsolutePath(const char *relativePath)
{
    char *fullPath = new char[strlen(PROJECT_PATH) + strlen(relativePath) + 1]; // +1 for the null-terminator
    if (fullPath == NULL)
    {
        return NULL;
    }
    snprintf(fullPath, strlen(PROJECT_PATH) + strlen(relativePath) + 1, "%s%s", PROJECT_PATH, relativePath);
    return fullPath;
}