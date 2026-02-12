#include <cassert>
#include <iostream>

#include "g4f/g4f.h"

static void testKeycodeValuesMatchGlfwLayout() {
    static_assert(G4F_KEY_ESCAPE == 256);
    static_assert(G4F_KEY_ENTER == 257);
    static_assert(G4F_KEY_TAB == 258);
    static_assert(G4F_KEY_BACKSPACE == 259);
    static_assert(G4F_KEY_LEFT_SHIFT == 340);
    static_assert(G4F_KEY_RIGHT_SHIFT == 344);
    static_assert(G4F_KEY_KP_8 == 328);
    static_assert(G4F_KEY_F10 == 299);
    static_assert(G4F_KEY_W == 'W');
    static_assert(G4F_KEY_1 == '1');
}

static void testRgbaPacking() {
    uint32_t c = g4f_rgba_u32(0x12, 0x34, 0x56, 0x78);
    assert(c == 0x12345678u);
}

int main() {
    testKeycodeValuesMatchGlfwLayout();
    testRgbaPacking();
    std::cout << "engine_keycodes_tests: OK\n";
    return 0;
}

