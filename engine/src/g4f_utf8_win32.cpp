#include "g4f_internal_win32.h"

#include <string>

std::wstring g4f_utf8_to_wide(const char* utf8) {
    if (!utf8 || utf8[0] == '\0') return std::wstring();

    int requiredChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, nullptr, 0);
    if (requiredChars <= 0) return std::wstring();

    std::wstring wide;
    wide.resize((size_t)requiredChars - 1);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide.data(), requiredChars);
    return wide;
}

