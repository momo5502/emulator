#pragma once

#include <cstdint>


using BYTE      = std::uint8_t;
using WORD      = std::uint16_t;
using DWORD     = std::uint32_t;
using DWORD64   = std::uint64_t;

using CHAR      = BYTE;
using UCHAR = unsigned char;

using BOOLEAN   = bool;
using CSHORT    = short;
using USHORT    = WORD;
using LONG      = std::int32_t;
using LONGLONG  = std::int64_t;
using ULONGLONG = DWORD64;

#define DUMMYSTRUCTNAME

#define TRUE  true
#define FALSE false

typedef union _ULARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        DWORD HighPart;
    } s;
    ULONGLONG QuadPart;
} ULARGE_INTEGER;

typedef union _LARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        LONG HighPart;
    } s;
    LONGLONG QuadPart;
} LARGE_INTEGER;


#ifdef OS_WINDOWS

#else

using ULONG     = DWORD;

#endif