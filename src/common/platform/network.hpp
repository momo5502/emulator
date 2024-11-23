#pragma once



template <typename Traits>
struct WSABUF
{
    ULONG len;
    EMULATOR_CAST(typename Traits::PVOID, CHAR*) buf;
};