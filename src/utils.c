#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <varargs.h>
#include <stdio.h>

#define MAX_DEBUG_MSG 512

void DebugPrint(const char* format, ...)
{
	char buff[MAX_DEBUG_MSG];

	va_list args;
	va_start(args, format);
	vsnprintf(buff, MAX_DEBUG_MSG, format, args);
	va_end(args);

	OutputDebugString(buff);
	OutputDebugString("\n");
}