#include "Str.h"
#include <string.h>
#include <stdio.h> // for vsnprintf

int StrPrintf(char* dst, size_t dstChars, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = StrPrintfV(dst, dstChars, format, args);
    va_end(args);
    return result;
}

int StrPrintfV(char* dst, size_t dstChars, const char* format, va_list args)
{
    return vsnprintf(dst, dstChars, format, args);
}

void StrCopy(char* dst, size_t dstChars, const char* src)
{
    if (!dstChars)
        return;

    while (--dstChars) {
        if ((*dst = *src) == 0)
            return;
        ++dst;
        ++src;
    }

    *dst = 0;
}

size_t StrCopyLen(char* dst, size_t dstChars, const char* src)
{
    if (!dstChars)
        return 0;

    char* dstBase = dst;
    while (--dstChars) {
        if ((*dst = *src) == 0)
            return (size_t)(dst - dstBase);
        ++dst;
        ++src;
    }

    *dst = 0;
    return (size_t)(dst - dstBase);
}

size_t StrLen(const char* str)
{
    return strlen(str);
}

int StrCmp(const char* str1, const char* str2)
{
    return strcmp(str1, str2);
}

u32 StrNextCharUTF8(const char* str, u32* size)
{
    const u8* utf8Char = (const u8*)str;

    u32 ret = *utf8Char;
    if ((ret & 0x80U) == 0U) {
        if (size)
            *size = 1U;
        return ret;
    }

    u32 I = 0x20U;
    u32 len = 2U;
    u32 mask = 0xC0U; // initially set to bitmask of the first two leading bits
    while (ret & I) {
        mask |= I;
        I >>= 1U;
        ++len;
        if (I == 0U) {
            // invalid sequence
            if (size) {
                *size = 1U;
                return 0xFFFFFFFFU;
            }
        }
    }

    if (size)
        *size = len;
    if (len > 4U)
        return 0xFFFFFFFFU; // invalid sequence

    // mask off the high-order bits
    ret &= ~mask;

    for (u32 i = 0; i < len - 1; ++i) {
        u32 byte = *++utf8Char;
        if ((byte & 0xC0U) != 0x80U)
            return 0xFFFFFFFFU; // invalid
        ret <<= 6U;
        ret |= (byte & 0x3F); // 6 bits of actual data for each byte after the first
    }

    return ret;
}

u32 StrCharToUTF8(u32 character, u32* size)
{
    // handle single-char (ascii) characters
    if (character < 0x80U) {
        if (size)
            *size = 1U;
        return character;
    }

    // upper bounds check
    if (character > 0x10FFFFU) {
        if (size)
            *size = 1U;
        return '?'; // invalid character
    }

    // figure out the length of the encoding and the bitmask of the high-order set bits
    u32 high = 0x00000800U; // first value for which we must use a 3-byte encoding
    u32 mask = 0xC0U;
    u32 len = 2U;
    while (character > high) {
        high <<= 5U; // we gain 5 extra bits of actual data per extra byte used
        mask = (mask >> 1U) | 0x80U; // add the next highest-order bit to the mask
        ++len;
    }

    // encode the first byte
    u32 bottomMask = ~((mask >> 1U) | 0xFFFFFF80U);
    u32 firstByteData = (character >> ((len - 1U) * 6U)) & bottomMask;
    u32 ret = mask | firstByteData;

    // encode the remaining bytes
    u32 shift = 8U;
    for (u32 i = len - 1U; i--; ) {
        u32 byteData = (character >> (i * 6U)) & 0x3FU; // extract the next 6 bits
        u32 byte = byteData | 0x80U; // add the 10xxxxxx flag
        ret |= byte << shift;
        shift += 8U;
    }
    if (size)
        *size = len;

    return ret;
}

u32 StrDistToPrevCharUTF8(const char* str, u32 len)
{
    if (!len)
        return 0xFFFFFFFFU;
    const u8* utf8Char = (const u8*)str;
    const u8* utf8CharBase = utf8Char;
    --utf8Char;
    if ((*utf8Char & 0x80U) == 0U)
        return 1U;
    for (; --len && ((*utf8Char & 0xC0U) == 0x80U); --utf8Char)
        ;
    return (u32)(utf8CharBase - utf8Char);
}
