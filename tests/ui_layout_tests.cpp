#include <cassert>
#include <iostream>

#include "g4f/g4f_ui.h"

static void testLayoutNextAdvancesCursor() {
    g4f_ui* ui = g4f_ui_create();
    assert(ui != nullptr);

    g4f_ui_layout layout{};
    layout.bounds = g4f_rect_f{100, 50, 400, 300};
    layout.padding = 10.0f;
    layout.spacing = 5.0f;
    layout.itemW = 0.0f;
    layout.defaultItemH = 20.0f;
    g4f_ui_layout_begin(ui, layout);

    g4f_rect_f a = g4f_ui_layout_next(ui, 0.0f);
    g4f_rect_f b = g4f_ui_layout_next(ui, 0.0f);
    assert(a.x == 110.0f);
    assert(a.y == 60.0f);
    assert(a.w == 380.0f);
    assert(a.h == 20.0f);
    assert(b.y == a.y + a.h + 5.0f);

    g4f_ui_destroy(ui);
}

int main() {
    testLayoutNextAdvancesCursor();
    std::cout << "ui_layout_tests: OK\n";
    return 0;
}

