#ifndef CORE_DEBUG_H
#define CORE_DEBUG_H

#include <stdarg.h>

void DebugPrintV(const char* format, va_list args);
void DebugPrint(const char* format, ...);

#endif // CORE_DEBUG_H
