#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <windowsx.h>

#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>

#include <stdint.h>

static inline void g4f_safe_release(IUnknown** ptr) {
    if (ptr && *ptr) {
        (*ptr)->Release();
        *ptr = nullptr;
    }
}

static inline uint64_t g4f_qpc_now(void) {
    LARGE_INTEGER value{};
    QueryPerformanceCounter(&value);
    return (uint64_t)value.QuadPart;
}

static inline double g4f_qpc_seconds(uint64_t ticks, uint64_t freq) {
    if (freq == 0) return 0.0;
    return (double)ticks / (double)freq;
}
