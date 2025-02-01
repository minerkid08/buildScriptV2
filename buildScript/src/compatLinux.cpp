#ifdef __linux
#include "compat.hpp"

#include <sys/stat.h>
#include <sys/types.h>

unsigned long long getFileWriteTime(const char* filename)
{
	struct stat fileStat;
	stat(filename, &fileStat);
	return fileStat.st_mtime;
}
#endif
