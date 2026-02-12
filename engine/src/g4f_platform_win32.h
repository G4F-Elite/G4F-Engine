#pragma once

#include "g4f_internal_win32.h"

#include "../include/g4f/g4f.h"

#include <array>
#include <string>

std::wstring g4f_utf8_to_wide(const char* utf8);
std::string g4f_wide_to_utf8(const wchar_t* wide);
int g4f_win32_vk_to_g4f_key(WPARAM vk, LPARAM lparam);

namespace g4f::win32 {

constexpr int kKeyStateCount = 512;
constexpr int kMouseStateCount = 8;

struct WindowState {
    HWND hwnd = nullptr;
    bool shouldClose = false;

    int width = 0;
    int height = 0;

    std::array<uint8_t, kKeyStateCount> keyDown{};
    std::array<uint8_t, kKeyStateCount> keyPressed{};
    std::array<uint8_t, kMouseStateCount> mouseDown{};
    std::array<uint8_t, kMouseStateCount> mousePressed{};
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float wheelDelta = 0.0f;

    // Text input for the current poll/frame (Unicode code points).
    uint32_t textInput[64]{};
    int textInputCount = 0;
    uint16_t pendingHighSurrogate = 0;
};

struct AppState {
    HINSTANCE hinstance = nullptr;
    uint64_t qpcFreq = 0;
    uint64_t qpcStart = 0;
    ATOM wndClass = 0;
};

} // namespace g4f::win32

struct g4f_app {
    g4f::win32::AppState state;
};

struct g4f_window {
    g4f_app* app = nullptr;
    g4f::win32::WindowState state;
};
