#ifndef CORE_MACROS_H
#define CORE_MACROS_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "Debug.h"

#define FATAL(format, ...) \
    do { DebugPrint("Fatal error!\n"); \
         DebugPrint(format, ##__VA_ARGS__); \
         abort(); \
    } while (0)

#define ASSERT assert

#endif // CORE_MACROS_H
