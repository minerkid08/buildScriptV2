#include <cstddef>
#include <fileapi.h>
#include <iostream>
#include <minwindef.h>
#include <timezoneapi.h>
#include <winnt.h>
#ifdef __WIN64

#include "compat.hpp"

#include <ctime>
#include <windows.h>

unsigned long long getFileWriteTime(const char* filename)
{
  HANDLE file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  FILETIME writeTime;

  GetFileTime(file, nullptr, nullptr, &writeTime);
	SYSTEMTIME systemTime;
  FileTimeToSystemTime(&writeTime, &systemTime);

	std::tm timeStruct;
	timeStruct.tm_year = systemTime.wYear - 1900;
	timeStruct.tm_mon = systemTime.wMonth - 1;
	timeStruct.tm_mday = systemTime.wDay;
	timeStruct.tm_hour = systemTime.wHour;
	timeStruct.tm_min = systemTime.wMinute;
	timeStruct.tm_sec = systemTime.wSecond;
	timeStruct.tm_isdst = -1;

	std::time_t timeSinceEpoch = std::mktime(&timeStruct);
	if (timeSinceEpoch == -1)
	{
		return -1; // Indicate an error
	}

	return timeSinceEpoch;
}

unsigned long long getCurrentTime()
{
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);

	std::tm timeStruct;
	timeStruct.tm_year = systemTime.wYear - 1900;
	timeStruct.tm_mon = systemTime.wMonth - 1;
	timeStruct.tm_mday = systemTime.wDay;
	timeStruct.tm_hour = systemTime.wHour;
	timeStruct.tm_min = systemTime.wMinute;
	timeStruct.tm_sec = systemTime.wSecond;
	timeStruct.tm_isdst = -1;

	std::time_t timeSinceEpoch = std::mktime(&timeStruct);
	if (timeSinceEpoch == -1)
	{
		return -1; // Indicate an error
	}

	return timeSinceEpoch;
}

#endif
