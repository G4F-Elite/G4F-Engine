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

std::string g4f_wide_to_utf8(const wchar_t* wide) {
    if (!wide || wide[0] == L'\0') return std::string();
    int requiredBytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1, nullptr, 0, nullptr, nullptr);
    if (requiredBytes <= 0) return std::string();
    std::string utf8;
    utf8.resize((size_t)requiredBytes - 1);
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8.data(), requiredBytes, nullptr, nullptr);
    return utf8;
}
