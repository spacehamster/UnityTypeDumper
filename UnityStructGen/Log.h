#ifndef LOG_H
#define LOG_H
#include <stdio.h>
static FILE* logFile;
inline void InitLogger()
{
	fopen_s(&logFile, "StructGenLog.txt", "w");
}
#define LOG(message, ...) \
	{ \
		fprintf(logFile, message, __VA_ARGS__); \
		fflush(logFile); \
	}
inline void FreeLogger()
{
	fclose(logFile);
}

#endif