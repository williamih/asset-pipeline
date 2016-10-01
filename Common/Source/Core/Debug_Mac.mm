#include "Debug.h"
#include <Foundation/Foundation.h>

void DebugPrintV(const char* format, va_list args)
{
    NSLogv([NSString stringWithUTF8String:format], args);
}

void DebugPrint(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    DebugPrintV(format, args);
    va_end(args);
}
