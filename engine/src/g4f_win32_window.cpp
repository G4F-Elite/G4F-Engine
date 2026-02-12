#include "g4f_platform_win32.h"

#include <algorithm>
#include <cstring>

namespace {

static void win32ApplyCursorCapture(g4f_window* window, bool wantCaptured) {
    if (!window || !window->state.hwnd) return;
    if (window->state.cursorCaptured == wantCaptured) return;
    window->state.cursorCaptured = wantCaptured;

    if (wantCaptured) {
        if (!window->state.rawMouseEnabled) {
            RAWINPUTDEVICE rid{};
            rid.usUsagePage = 0x01;
            rid.usUsage = 0x02;
            rid.dwFlags = RIDEV_INPUTSINK;
            rid.hwndTarget = window->state.hwnd;
            if (RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
                window->state.rawMouseEnabled = true;
                window->state.rawMouseDx = 0.0f;
                window->state.rawMouseDy = 0.0f;
            }
        }

        if (!window->state.cursorHidden) {
            while (ShowCursor(FALSE) >= 0) {}
            window->state.cursorHidden = true;
        }

        RECT rc{};
        GetClientRect(window->state.hwnd, &rc);
        POINT tl{rc.left, rc.top};
        POINT br{rc.right, rc.bottom};
        ClientToScreen(window->state.hwnd, &tl);
        ClientToScreen(window->state.hwnd, &br);
        RECT clip{tl.x, tl.y, br.x, br.y};
        ClipCursor(&clip);

        int cx = (rc.left + rc.right) / 2;
        int cy = (rc.top + rc.bottom) / 2;
        POINT p{cx, cy};
        ClientToScreen(window->state.hwnd, &p);
        window->state.ignoreNextMouseMove = true;
        SetCursorPos(p.x, p.y);
        window->state.mouseX = (float)cx;
        window->state.mouseY = (float)cy;
        window->state.prevMouseX = (float)cx;
        window->state.prevMouseY = (float)cy;
        window->state.mouseDx = 0.0f;
        window->state.mouseDy = 0.0f;
    } else {
        ClipCursor(nullptr);
        if (window->state.cursorHidden) {
            while (ShowCursor(TRUE) < 0) {}
            window->state.cursorHidden = false;
        }
        window->state.rawMouseEnabled = false;
        window->state.rawMouseDx = 0.0f;
        window->state.rawMouseDy = 0.0f;
        window->state.mouseDx = 0.0f;
        window->state.mouseDy = 0.0f;
        window->state.prevMouseX = window->state.mouseX;
        window->state.prevMouseY = window->state.mouseY;
    }
}

LRESULT CALLBACK g4f_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* windowState = (g4f::win32::WindowState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE: {
            auto* cs = (CREATESTRUCTW*)lparam;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
        case WM_CLOSE: {
            if (windowState) windowState->shouldClose = true;
            return 0;
        }
        case WM_SETFOCUS: {
            if (windowState) windowState->focused = true;
            return 0;
        }
        case WM_KILLFOCUS: {
            if (windowState) windowState->focused = false;
            return 0;
        }
        case WM_SIZE: {
            if (windowState) {
                windowState->width = LOWORD(lparam);
                windowState->height = HIWORD(lparam);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (windowState) {
                if (windowState->ignoreNextMouseMove) {
                    windowState->ignoreNextMouseMove = false;
                    return 0;
                }
                windowState->mouseX = (float)GET_X_LPARAM(lparam);
                windowState->mouseY = (float)GET_Y_LPARAM(lparam);
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            if (windowState) {
                int delta = GET_WHEEL_DELTA_WPARAM(wparam);
                windowState->wheelDelta += (float)delta / (float)WHEEL_DELTA;
            }
            return 0;
        }
        case WM_INPUT: {
            if (!windowState || !windowState->cursorCaptured || !windowState->rawMouseEnabled) break;
            UINT size = 0;
            GetRawInputData((HRAWINPUT)lparam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
            if (size == 0) return 0;
            windowState->rawInputBuf.resize((size_t)size);
            if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, windowState->rawInputBuf.data(), &size, sizeof(RAWINPUTHEADER)) != size) return 0;
            RAWINPUT* ri = (RAWINPUT*)windowState->rawInputBuf.data();
            if (ri->header.dwType == RIM_TYPEMOUSE) {
                windowState->rawMouseDx += (float)ri->data.mouse.lLastX;
                windowState->rawMouseDy += (float)ri->data.mouse.lLastY;
            }
            return 0;
        }
        case WM_CHAR: {
            if (!windowState) break;
            uint32_t codepoint = 0;
            uint16_t wc = (uint16_t)wparam;
            if (wc >= 0xD800 && wc <= 0xDBFF) {
                windowState->pendingHighSurrogate = wc;
                return 0;
            }
            if (wc >= 0xDC00 && wc <= 0xDFFF) {
                uint16_t hi = windowState->pendingHighSurrogate;
                windowState->pendingHighSurrogate = 0;
                if (hi >= 0xD800 && hi <= 0xDBFF) {
                    codepoint = 0x10000u + (((uint32_t)(hi - 0xD800u) << 10) | (uint32_t)(wc - 0xDC00u));
                } else {
                    codepoint = (uint32_t)wc;
                }
            } else {
                windowState->pendingHighSurrogate = 0;
                codepoint = (uint32_t)wc;
            }
            if (windowState->textInputCount < (int)(sizeof(windowState->textInput) / sizeof(windowState->textInput[0]))) {
                windowState->textInput[windowState->textInputCount++] = codepoint;
            }
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP: {
            if (!windowState) break;
            int button = -1;
            bool isDown = false;
            if (msg == WM_LBUTTONDOWN) { button = G4F_MOUSE_BUTTON_LEFT; isDown = true; }
            if (msg == WM_LBUTTONUP) { button = G4F_MOUSE_BUTTON_LEFT; isDown = false; }
            if (msg == WM_RBUTTONDOWN) { button = G4F_MOUSE_BUTTON_RIGHT; isDown = true; }
            if (msg == WM_RBUTTONUP) { button = G4F_MOUSE_BUTTON_RIGHT; isDown = false; }
            if (msg == WM_MBUTTONDOWN) { button = G4F_MOUSE_BUTTON_MIDDLE; isDown = true; }
            if (msg == WM_MBUTTONUP) { button = G4F_MOUSE_BUTTON_MIDDLE; isDown = false; }
            if (button >= 0 && button < (int)windowState->mouseDown.size()) {
                if (isDown && !windowState->mouseDown[(size_t)button]) windowState->mousePressed[(size_t)button] = 1;
                windowState->mouseDown[(size_t)button] = isDown ? 1 : 0;
            }
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            if (!windowState) break;
            bool isDown = (msg == WM_KEYDOWN) || (msg == WM_SYSKEYDOWN);
            int key = g4f_win32_vk_to_g4f_key(wparam, lparam);
            if (key >= 0 && key < g4f::win32::kKeyStateCount) {
                if (isDown && !windowState->keyDown[(size_t)key]) windowState->keyPressed[(size_t)key] = 1;
                windowState->keyDown[(size_t)key] = isDown ? 1 : 0;
            }
            return 0;
        }
        default: break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

} // namespace

const char* g4f_version_string(void) {
    return "g4f-engine/0.1 (win64)";
}

g4f_app* g4f_app_create(const g4f_app_desc* /*desc*/) {
    auto* app = new g4f_app();
    app->state.hinstance = GetModuleHandleW(nullptr);

    LARGE_INTEGER freq{};
    QueryPerformanceFrequency(&freq);
    app->state.qpcFreq = (uint64_t)freq.QuadPart;
    app->state.qpcStart = g4f_qpc_now();

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = g4f_wndproc;
    wc.hInstance = app->state.hinstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"G4FEngineWindow";
    app->state.wndClass = RegisterClassExW(&wc);

    return app;
}

void g4f_app_destroy(g4f_app* app) {
    if (!app) return;
    if (app->state.wndClass) UnregisterClassW(L"G4FEngineWindow", app->state.hinstance);
    CoUninitialize();
    delete app;
}

double g4f_time_seconds(const g4f_app* app) {
    if (!app) return 0.0;
    uint64_t now = g4f_qpc_now();
    uint64_t elapsed = now - app->state.qpcStart;
    return g4f_qpc_seconds(elapsed, app->state.qpcFreq);
}

g4f_window* g4f_window_create(g4f_app* app, const g4f_window_desc* desc) {
    if (!app || !desc) return nullptr;
    if (!app->state.wndClass) return nullptr;

    auto* window = new g4f_window();
    window->app = app;
    window->state.width = std::max(64, desc->width);
    window->state.height = std::max(64, desc->height);

    std::wstring title = g4f_utf8_to_wide(desc->title_utf8 ? desc->title_utf8 : "G4F");

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!desc->resizable) style &= ~WS_THICKFRAME;

    RECT rect{0, 0, (LONG)window->state.width, (LONG)window->state.height};
    AdjustWindowRect(&rect, style, FALSE);

    HWND hwnd = CreateWindowExW(
        0,
        L"G4FEngineWindow",
        title.c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        app->state.hinstance,
        &window->state
    );
    window->state.hwnd = hwnd;

    if (!hwnd) {
        delete window;
        return nullptr;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Initialize mouse position to avoid large deltas on first poll.
    POINT p{};
    if (GetCursorPos(&p) && ScreenToClient(hwnd, &p)) {
        window->state.mouseX = (float)p.x;
        window->state.mouseY = (float)p.y;
        window->state.prevMouseX = window->state.mouseX;
        window->state.prevMouseY = window->state.mouseY;
    }
    return window;
}

void g4f_window_destroy(g4f_window* window) {
    if (!window) return;
    if (window->state.hwnd) DestroyWindow(window->state.hwnd);
    delete window;
}

void g4f_window_request_close(g4f_window* window) {
    if (!window) return;
    window->state.shouldClose = true;
}

void g4f_window_get_size(const g4f_window* window, int* width, int* height) {
    if (!window) return;
    if (width) *width = window->state.width;
    if (height) *height = window->state.height;
}

void g4f_window_set_title(g4f_window* window, const char* title_utf8) {
    if (!window || !window->state.hwnd) return;
    std::wstring title = g4f_utf8_to_wide(title_utf8 ? title_utf8 : "");
    SetWindowTextW(window->state.hwnd, title.c_str());
}

int g4f_window_poll(g4f_window* window) {
    if (!window) return 0;
    window->state.keyPressed.fill(0);
    window->state.mousePressed.fill(0);
    window->state.mouseDx = 0.0f;
    window->state.mouseDy = 0.0f;
    window->state.wheelDelta = 0.0f;
    window->state.textInputCount = 0;
    window->state.pendingHighSurrogate = 0;
    window->state.rawMouseDx = 0.0f;
    window->state.rawMouseDy = 0.0f;

    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (!window->state.focused && window->state.cursorCaptured) {
        win32ApplyCursorCapture(window, false);
    }

    // Per-frame mouse delta. Prefer WM_INPUT raw deltas when captured.
    if (window->state.cursorCaptured && window->state.hwnd) {
        if (window->state.rawMouseEnabled) {
            window->state.mouseDx = window->state.rawMouseDx;
            window->state.mouseDy = window->state.rawMouseDy;
            window->state.prevMouseX = window->state.mouseX;
            window->state.prevMouseY = window->state.mouseY;
        } else {
            RECT rc{};
            GetClientRect(window->state.hwnd, &rc);
            int cx = (rc.left + rc.right) / 2;
            int cy = (rc.top + rc.bottom) / 2;

            float dx = window->state.mouseX - (float)cx;
            float dy = window->state.mouseY - (float)cy;
            window->state.mouseDx = dx;
            window->state.mouseDy = dy;

            POINT p{cx, cy};
            ClientToScreen(window->state.hwnd, &p);
            window->state.ignoreNextMouseMove = true;
            SetCursorPos(p.x, p.y);
            window->state.mouseX = (float)cx;
            window->state.mouseY = (float)cy;
            window->state.prevMouseX = (float)cx;
            window->state.prevMouseY = (float)cy;
        }
    } else {
        window->state.mouseDx = window->state.mouseX - window->state.prevMouseX;
        window->state.mouseDy = window->state.mouseY - window->state.prevMouseY;
        window->state.prevMouseX = window->state.mouseX;
        window->state.prevMouseY = window->state.mouseY;
    }

    return window->state.shouldClose ? 0 : 1;
}

int g4f_key_down(const g4f_window* window, int key) {
    if (!window || key < 0 || key >= g4f::win32::kKeyStateCount) return 0;
    return window->state.keyDown[(size_t)key] ? 1 : 0;
}

int g4f_key_pressed(const g4f_window* window, int key) {
    if (!window || key < 0 || key >= g4f::win32::kKeyStateCount) return 0;
    return window->state.keyPressed[(size_t)key] ? 1 : 0;
}

int g4f_mouse_down(const g4f_window* window, int button) {
    if (!window || button < 0 || button >= (int)window->state.mouseDown.size()) return 0;
    return window->state.mouseDown[(size_t)button] ? 1 : 0;
}

int g4f_mouse_pressed(const g4f_window* window, int button) {
    if (!window || button < 0 || button >= (int)window->state.mousePressed.size()) return 0;
    return window->state.mousePressed[(size_t)button] ? 1 : 0;
}

float g4f_mouse_x(const g4f_window* window) {
    return window ? window->state.mouseX : 0.0f;
}

float g4f_mouse_y(const g4f_window* window) {
    return window ? window->state.mouseY : 0.0f;
}

float g4f_mouse_dx(const g4f_window* window) {
    return window ? window->state.mouseDx : 0.0f;
}

float g4f_mouse_dy(const g4f_window* window) {
    return window ? window->state.mouseDy : 0.0f;
}

float g4f_mouse_wheel_delta(const g4f_window* window) {
    return window ? window->state.wheelDelta : 0.0f;
}

int g4f_text_input_count(const g4f_window* window) {
    return window ? window->state.textInputCount : 0;
}

uint32_t g4f_text_input_codepoint(const g4f_window* window, int index) {
    if (!window) return 0;
    if (index < 0 || index >= window->state.textInputCount) return 0;
    return window->state.textInput[(size_t)index];
}

int g4f_clipboard_get_utf8(const g4f_window* window, char* out_utf8, int out_cap) {
    if (out_utf8 && out_cap > 0) out_utf8[0] = '\0';
    if (!window || !out_utf8 || out_cap <= 1) return 0;

    if (!OpenClipboard(window->state.hwnd)) return 0;
    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (!data) { CloseClipboard(); return 0; }
    const wchar_t* wide = (const wchar_t*)GlobalLock(data);
    if (!wide) { CloseClipboard(); return 0; }

    std::string utf8 = g4f_wide_to_utf8(wide);
    GlobalUnlock(data);
    CloseClipboard();

    if (utf8.empty()) return 0;
    int toCopy = (int)utf8.size();
    if (toCopy > out_cap - 1) toCopy = out_cap - 1;
    std::memcpy(out_utf8, utf8.data(), (size_t)toCopy);
    out_utf8[toCopy] = '\0';
    return toCopy;
}

int g4f_clipboard_set_utf8(const g4f_window* window, const char* text_utf8) {
    if (!window || !text_utf8) return 0;
    std::wstring wide = g4f_utf8_to_wide(text_utf8);
    if (wide.empty()) wide = L"";

    if (!OpenClipboard(window->state.hwnd)) return 0;
    EmptyClipboard();

    size_t bytes = (wide.size() + 1) * sizeof(wchar_t);
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hmem) { CloseClipboard(); return 0; }
    void* dst = GlobalLock(hmem);
    if (!dst) { GlobalFree(hmem); CloseClipboard(); return 0; }
    std::memcpy(dst, wide.c_str(), bytes);
    GlobalUnlock(hmem);

    HANDLE res = SetClipboardData(CF_UNICODETEXT, hmem);
    CloseClipboard();
    if (!res) { GlobalFree(hmem); return 0; }
    return 1;
}

void g4f_window_set_cursor_captured(g4f_window* window, int captured) {
    win32ApplyCursorCapture(window, captured ? true : false);
}

int g4f_window_cursor_captured(const g4f_window* window) {
    return (window && window->state.cursorCaptured) ? 1 : 0;
}

int g4f_window_focused(const g4f_window* window) {
    return (window && window->state.focused) ? 1 : 0;
}
