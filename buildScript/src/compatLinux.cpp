#ifdef __linux
#include "compat.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>

unsigned long long getFileWriteTime(const char* filename)
{
	struct stat fileStat;
	stat(filename, &fileStat);
	return fileStat.st_mtime;
}

unsigned long long getCurrentTime()
{
  return std::time(0);
}
#endif
