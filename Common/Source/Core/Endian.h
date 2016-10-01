#ifndef CORE_ENDIAN_H
#define CORE_ENDIAN_H

#include "Types.h"

inline u16 EndianSwap16(u16 n)
{
    return (n << 8) | (n >> 8);
}

inline u32 EndianSwap32(u32 n)
{
    return ((n & 0xFF000000) >> 24) |
           ((n & 0x00FF0000) >> 8) |
           ((n & 0x0000FF00) << 8) |
           ((n & 0x000000FF) << 24);
}

inline u64 EndianSwap64(u64 n)
{
    n = (n & 0x00000000FFFFFFFF) << 32 | (n & 0xFFFFFFFF00000000) >> 32;
    n = (n & 0x0000FFFF0000FFFF) << 16 | (n & 0xFFFF0000FFFF0000) >> 16;
    n = (n & 0x00FF00FF00FF00FF) << 8  | (n & 0xFF00FF00FF00FF00) >> 8;
    return n;
}

union EndianU32F32 {
    u32 asU32;
    f32 asF32;
};

inline f32 EndianSwapFloat32(f32 n)
{
    EndianU32F32 u;
    u.asF32 = n;
    u.asU32 = EndianSwap32(u.asU32);
    return u.asF32;
}

inline u16 EndianSwapLE16(u16 n)
{
#ifdef ENDIAN_BIG
    return EndianSwap16(n);
#else
    return n;
#endif
}

inline u32 EndianSwapLE32(u32 n)
{
#ifdef ENDIAN_BIG
    return EndianSwap32(n);
#else
    return n;
#endif
}

inline u64 EndianSwapLE64(u64 n)
{
#ifdef ENDIAN_BIG
    return EndianSwap64(n);
#else
    return n;
#endif
}


inline f32 EndianSwapLEFloat32(f32 n)
{
#ifdef ENDIAN_BIG
    return EndianSwapF32(n);
#else
    return n;
#endif
}

#endif // CORE_ENDIAN_H
