#ifndef CORE_STR_H
#define CORE_STR_H

#include <stddef.h>
#include <stdarg.h>
#include "Types.h"

template<class BaseUnit>
struct StrEncodingInfo {
    u32 nCodePoints;
    u32 nBaseUnits; // includes the null terminator
    BaseUnit* str;
};

int StrPrintf(char* dst, size_t dstChars, const char* format, ...);
int StrPrintfV(char* dst, size_t dstChars, const char* format, va_list args);

void StrCopy(char* dst, size_t dstChars, const char* src);
size_t StrCopyLen(char* dst, size_t dstChars, const char* src);

size_t StrLen(const char* str);
int StrCmp(const char* str1, const char* str2);

u32 StrNextCharUTF8(const char* str, u32* size);
u32 StrCharToUTF8(u32 character, u32* size);
u32 StrDistToPrevCharUTF8(const char* str, u32 len);

u32 StrNextCharUTF16(const u16* str, u32* size);
u32 StrCharToUTF16(u32 character, u32* size);

StrEncodingInfo<u16> StrCreateUTF16From8(
    const char* utf8Str,
    void* (*alloc)(size_t, void*),
    void* userdata
);
StrEncodingInfo<char> StrCreateUTF8From16(
    const u16* utf16Str,
    void* (*alloc)(size_t, void*),
    void* userdata
);

// Returns NULL only if the allocator does.
template<class Alloc>
StrEncodingInfo<u16> StrCreateUTF16From8(const char* utf8Str, const Alloc& alloc)
{
    struct S {
        static void* Allocate(size_t size, void* userdata)
        {
            const Alloc* allocator = (const Alloc*)userdata;
            return (*allocator)(size);
        }
    };
    return StrCreateUTF16From8(utf8Str, &S::Allocate, (void*)&alloc);
}

// Returns NULL only if the allocator does.
template<class Alloc>
StrEncodingInfo<char> StrCreateUTF8From16(const u16* utf16Str, const Alloc& alloc)
{
    struct S {
        static void* Allocate(size_t size, void* userdata)
        {
            const Alloc* allocator = (const Alloc*)userdata;
            return (*allocator)(size);
        }
    };
    return StrCreateUTF8From16(utf16Str, &S::Allocate, (void*)&alloc);
}

#endif // CORE_STR_H
