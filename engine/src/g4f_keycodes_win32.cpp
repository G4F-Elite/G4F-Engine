#include "g4f_internal_win32.h"

#include "../include/g4f/g4f.h"

static int g4f_vk_to_key(WPARAM vk, LPARAM lparam) {
    const bool isExtended = (lparam & 0x01000000) != 0;
    const UINT scanCode = (UINT)((lparam >> 16) & 0xFF);

    switch ((UINT)vk) {
        case VK_ESCAPE: return G4F_KEY_ESCAPE;
        case VK_RETURN: return G4F_KEY_ENTER;
        case VK_TAB: return G4F_KEY_TAB;
        case VK_BACK: return G4F_KEY_BACKSPACE;
        case VK_INSERT: return G4F_KEY_INSERT;
        case VK_DELETE: return G4F_KEY_DELETE;
        case VK_RIGHT: return G4F_KEY_RIGHT;
        case VK_LEFT: return G4F_KEY_LEFT;
        case VK_DOWN: return G4F_KEY_DOWN;
        case VK_UP: return G4F_KEY_UP;
        case VK_PRIOR: return G4F_KEY_PAGE_UP;
        case VK_NEXT: return G4F_KEY_PAGE_DOWN;
        case VK_HOME: return G4F_KEY_HOME;
        case VK_END: return G4F_KEY_END;

        case VK_CAPITAL: return G4F_KEY_CAPS_LOCK;
        case VK_SCROLL: return G4F_KEY_SCROLL_LOCK;
        case VK_NUMLOCK: return G4F_KEY_NUM_LOCK;
        case VK_SNAPSHOT: return G4F_KEY_PRINT_SCREEN;
        case VK_PAUSE: return G4F_KEY_PAUSE;

        case VK_F1: return G4F_KEY_F1;
        case VK_F2: return G4F_KEY_F2;
        case VK_F3: return G4F_KEY_F3;
        case VK_F4: return G4F_KEY_F4;
        case VK_F5: return G4F_KEY_F5;
        case VK_F6: return G4F_KEY_F6;
        case VK_F7: return G4F_KEY_F7;
        case VK_F8: return G4F_KEY_F8;
        case VK_F9: return G4F_KEY_F9;
        case VK_F10: return G4F_KEY_F10;
        case VK_F11: return G4F_KEY_F11;
        case VK_F12: return G4F_KEY_F12;

        case VK_LSHIFT: return G4F_KEY_LEFT_SHIFT;
        case VK_RSHIFT: return G4F_KEY_RIGHT_SHIFT;
        case VK_LCONTROL: return G4F_KEY_LEFT_CONTROL;
        case VK_RCONTROL: return G4F_KEY_RIGHT_CONTROL;
        case VK_LMENU: return G4F_KEY_LEFT_ALT;
        case VK_RMENU: return G4F_KEY_RIGHT_ALT;
        case VK_APPS: return G4F_KEY_MENU;
        default: break;
    }

    if ((UINT)vk == VK_SHIFT) {
        UINT distinct = MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX);
        if (distinct == VK_LSHIFT) return G4F_KEY_LEFT_SHIFT;
        if (distinct == VK_RSHIFT) return G4F_KEY_RIGHT_SHIFT;
        return G4F_KEY_LEFT_SHIFT;
    }
    if ((UINT)vk == VK_CONTROL) return isExtended ? G4F_KEY_RIGHT_CONTROL : G4F_KEY_LEFT_CONTROL;
    if ((UINT)vk == VK_MENU) return isExtended ? G4F_KEY_RIGHT_ALT : G4F_KEY_LEFT_ALT;

    // Keypad mapping
    switch ((UINT)vk) {
        case VK_NUMPAD0: return G4F_KEY_KP_0;
        case VK_NUMPAD1: return G4F_KEY_KP_1;
        case VK_NUMPAD2: return G4F_KEY_KP_2;
        case VK_NUMPAD3: return G4F_KEY_KP_3;
        case VK_NUMPAD4: return G4F_KEY_KP_4;
        case VK_NUMPAD5: return G4F_KEY_KP_5;
        case VK_NUMPAD6: return G4F_KEY_KP_6;
        case VK_NUMPAD7: return G4F_KEY_KP_7;
        case VK_NUMPAD8: return G4F_KEY_KP_8;
        case VK_NUMPAD9: return G4F_KEY_KP_9;
        case VK_DECIMAL: return G4F_KEY_KP_DECIMAL;
        case VK_DIVIDE: return G4F_KEY_KP_DIVIDE;
        case VK_MULTIPLY: return G4F_KEY_KP_MULTIPLY;
        case VK_SUBTRACT: return G4F_KEY_KP_SUBTRACT;
        case VK_ADD: return G4F_KEY_KP_ADD;
        default: break;
    }

    // ASCII letters/digits/punct should map 1:1 for A-Z, 0-9, etc.
    if ((UINT)vk >= '0' && (UINT)vk <= '9') return (int)vk;
    if ((UINT)vk >= 'A' && (UINT)vk <= 'Z') return (int)vk;

    switch ((UINT)vk) {
        case VK_OEM_1: return G4F_KEY_SEMICOLON;
        case VK_OEM_PLUS: return G4F_KEY_EQUAL;
        case VK_OEM_COMMA: return G4F_KEY_COMMA;
        case VK_OEM_MINUS: return G4F_KEY_MINUS;
        case VK_OEM_PERIOD: return G4F_KEY_PERIOD;
        case VK_OEM_2: return G4F_KEY_SLASH;
        case VK_OEM_3: return G4F_KEY_GRAVE_ACCENT;
        case VK_OEM_4: return G4F_KEY_LEFT_BRACKET;
        case VK_OEM_5: return G4F_KEY_BACKSLASH;
        case VK_OEM_6: return G4F_KEY_RIGHT_BRACKET;
        case VK_OEM_7: return G4F_KEY_APOSTROPHE;
        default: break;
    }

    return G4F_KEY_UNKNOWN;
}

int g4f_win32_vk_to_g4f_key(WPARAM vk, LPARAM lparam) {
    return g4f_vk_to_key(vk, lparam);
}

