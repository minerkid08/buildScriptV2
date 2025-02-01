#ifdef __WIN64
#include "compat.hpp"
#include <windows.h>

unsigned long long getFileWriteTime(const char *filename)
{
   WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (GetFileAttributesEx(filename, GetFileExInfoStandard, &fileData) == 0) {
        return -1; // Error getting file attributes
    }

    ULARGE_INTEGER fileTime;
    fileTime.LowPart = fileData.ftLastWriteTime.dwLowDateTime;
    fileTime.HighPart = fileData.ftLastWriteTime.dwHighDateTime;

    return fileTime.QuadPart;
}

#endif
