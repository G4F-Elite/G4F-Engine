#pragma once

#include <cstdint>

// Internal error helpers (thread-local).
// Public API: g4f_last_error / g4f_clear_error in g4f.h.

void g4f_set_last_error(const char* messageUtf8);
void g4f_set_last_errorf(const char* fmt, ...);

void g4f_set_last_win32_error(const char* contextUtf8, uint32_t win32Error);
void g4f_set_last_hresult_error(const char* contextUtf8, long hresult);

