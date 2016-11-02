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

u32 StrNextCharUTF16(const u16* str, u32* size)
{
    u32 ret = *str;

    // Check if this is a surrogate pair.
    // 0xFC00 == (10 low-order 0's and then 6 high-order 1's).
    if ((ret & 0xFC00U) == 0xD800U) {
        if (ret == 0) {
            // Invalid: there should be at least 2 chars for a surrogate pair,
            // but we had a null-terminator.
            if (size) *size = 1;
            return '?'; // invalid character
        }

        // Prepare to add the next u16 (the second half of the surrogate pair).
        // The first u16 we just read constitutes the upper 10 bits.
        // We also remove the 0xD800.
        ret &= ~(0xD800U);
        ret <<= 10U;

        u32 next = *++str;
        if ((next & 0xFC00U) != 0xDC00U) {
            // Invalid: second character should have 0xDC00 in the upper 6 bits.
            if (size) *size = 1;
            return '?'; // invalid character
        }

        if (size) *size = 2;

        // Add in the second u16.
        next &= ~(0xDC00U);
        ret |= next;
        ret += 0x10000U;
    } else {
        // Single character.
        if (size) *size = 1;
    }

    return ret;
}

u32 StrCharToUTF16(u32 character, u32* size)
{
    // Can the character be represented as a single 16-bit int?
    if ((character & 0xFFFF0000U) == 0) {
        if (size) *size = 1;
        return character;
    }

    // Otherwise, break into a surrogate pair.

    if (size) *size = 2;

    character -= 0x10000U;

    // Extract upper and lower 10 bits.
    u32 hi = (character >> 10U) & 0x3FFU; // 0x3FF = 10 set bits
    u32 lo = character & 0x3FFU;

    // Each of the two sets of 10 bits is masked with a certain number.
    // The upper 10 bits are used to form the first 16-bit value.
    // The lower 10 bits are then used to form the second 16-bit value.
    u32 first = 0xD800U | hi;
    u32 second = 0xDC00U | lo;
    return first | (second << 16U);
}

StrEncodingInfo<u16> StrCreateUTF16From8(const char* utf8Str,
                                         void* (*alloc)(size_t, void*),
                                         void* userdata)
{
    // Use a two-step process: first, calculate the required length; and then
    // allocate a string of that length and fill it in.
    //
    // This does mean that we iterate the string twice, but the upside is that
    // we know the exact amount of memory required before doing the allocation.

    u32 nU16s = 0;
    u32 nCodePoints = 0;

    for (const char* p = utf8Str; *p; ++nCodePoints) {
        u32 size;
        const u32 codePoint = StrNextCharUTF8(p, &size);
        p += size;

        u32 utf16Size;
        StrCharToUTF16(codePoint, &utf16Size);
        nU16s += utf16Size;
    }

    ++nU16s; // add an extra 16-bit int for the null-terminator

    const u16* const result = (u16*)alloc(sizeof(u16) * nU16s, userdata);
    if (!result) {
        StrEncodingInfo<u16> info = { 0, 0, NULL };
        return info;
    }

    u16* ptr = (u16*)result;
    for (const char* p = utf8Str; *p; ) {
        u32 size;
        const u32 codePoint = StrNextCharUTF8(p, &size);
        p += size;

        u32 utf16Size;
        u32 utf16Data = StrCharToUTF16(codePoint, &utf16Size);
        if (utf16Size >= 1) {
            *ptr++ = (u16)(utf16Data & 0xFFFFU);
            if (utf16Size == 2)
                *ptr++ = (u16)(utf16Data >> 16U);
        }
    }

    *ptr = 0; // null-terminate

    StrEncodingInfo<u16> info;
    info.nCodePoints = nCodePoints;
    info.nBaseUnits = nU16s;
    info.str = (u16*)result;
    return info;
}

StrEncodingInfo<char> StrCreateUTF8From16(const u16* utf16Str,
                                          void* (*alloc)(size_t, void*),
                                          void* userdata)
{
    u32 nBytes = 0;
    u32 nCodePoints = 0;

    for (const u16* p = utf16Str; *p; ++nCodePoints) {
        u32 size;
        const u32 codePoint = StrNextCharUTF16(p, &size);
        p += size;

        u32 utf8Size;
        StrCharToUTF8(codePoint, &utf8Size);
        nBytes += utf8Size;
    }

    ++nBytes; // add an extra byte for the null-terminator

    const char* const result = (char*)alloc(nBytes, userdata);
    if (!result) {
        StrEncodingInfo<char> info = { 0, 0, NULL };
        return info;
    }

    char* ptr = (char*)result;
    for (const u16* p = utf16Str; *p; ) {
        u32 size;
        const u32 codePoint = StrNextCharUTF16(p, &size);
        p += size;

        u32 utf8Size;
        u32 utf8Data = StrCharToUTF8(codePoint, &utf8Size);
        for (u32 i = 0; i < utf8Size; ++i) {
            u8 byte = (u8)(utf8Data & 0xFFU);
            *ptr++ = (char)byte;
            utf8Data >>= 8U;
        }
    }

    *ptr = 0; // null-terminate

    StrEncodingInfo<char> info;
    info.nCodePoints = nCodePoints;
    info.nBaseUnits = nBytes;
    info.str = (char*)result;
    return info;
}
