#include "../include/g4f/g4f.h"

#include "g4f_error_internal.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {

thread_local char g_lastError[1024]{};

static void setLastErrorString(const char* messageUtf8) {
    if (!messageUtf8 || !messageUtf8[0]) {
        g_lastError[0] = '\0';
        return;
    }
    std::snprintf(g_lastError, sizeof(g_lastError), "%s", messageUtf8);
    g_lastError[sizeof(g_lastError) - 1] = '\0';
}

} // namespace

extern "C" const char* g4f_last_error(void) {
    return g_lastError;
}

extern "C" void g4f_clear_error(void) {
    g_lastError[0] = '\0';
}

void g4f_set_last_error(const char* messageUtf8) {
    setLastErrorString(messageUtf8);
}

void g4f_set_last_errorf(const char* fmt, ...) {
    if (!fmt || !fmt[0]) {
        g_lastError[0] = '\0';
        return;
    }
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(g_lastError, sizeof(g_lastError), fmt, args);
    va_end(args);
    g_lastError[sizeof(g_lastError) - 1] = '\0';
}

void g4f_set_last_win32_error(const char* contextUtf8, uint32_t win32Error) {
    if (contextUtf8 && contextUtf8[0]) {
        g4f_set_last_errorf("%s (win32=%lu)", contextUtf8, (unsigned long)win32Error);
    } else {
        g4f_set_last_errorf("win32 error %lu", (unsigned long)win32Error);
    }
}

void g4f_set_last_hresult_error(const char* contextUtf8, long hresult) {
    if (contextUtf8 && contextUtf8[0]) {
        g4f_set_last_errorf("%s (hr=0x%08lX)", contextUtf8, (unsigned long)hresult);
    } else {
        g4f_set_last_errorf("HRESULT 0x%08lX", (unsigned long)hresult);
    }
}

