#pragma once

template <typename Traits>
struct UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    EMULATOR_CAST(typename Traits::PVOID, char16_t*) Buffer;
};