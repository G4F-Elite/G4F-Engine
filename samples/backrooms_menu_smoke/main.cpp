#include <cstdio>
#include <cstring>

#include "g4f/g4f.h"
#include "g4f/g4f_ui.h"

namespace {

struct MenuItem {
    const char* label;
};

constexpr MenuItem kItems[] = {
    {"START (SMOKE)"},
    {"SETTINGS (SMOKE)"},
    {"QUIT"},
};

} // namespace

int main() {
    g4f_window_desc windowDesc{};
    windowDesc.title_utf8 = "Backrooms - Menu Smoke (G4F)";
    windowDesc.width = 960;
    windowDesc.height = 540;
    windowDesc.resizable = 1;

    g4f_ctx* ctx = g4f_ctx_create(&windowDesc);
    if (!ctx) return 1;

    g4f_ui* ui = g4f_ui_create();
    if (!ui) { g4f_ctx_destroy(ctx); return 1; }

    int selected = 0;
    bool running = true;
    while (running && g4f_ctx_poll(ctx)) {
        g4f_window* window = g4f_ctx_window(ctx);
        g4f_renderer* renderer = g4f_ctx_renderer(ctx);
        if (g4f_key_pressed(window, G4F_KEY_ESCAPE)) running = false;
        if (g4f_key_pressed(window, G4F_KEY_UP)) selected = (selected + (int)(sizeof(kItems) / sizeof(kItems[0])) - 1) % (int)(sizeof(kItems) / sizeof(kItems[0]));
        if (g4f_key_pressed(window, G4F_KEY_DOWN)) selected = (selected + 1) % (int)(sizeof(kItems) / sizeof(kItems[0]));

        if (g4f_key_pressed(window, G4F_KEY_ENTER) || g4f_key_pressed(window, G4F_KEY_SPACE)) {
            if (std::strcmp(kItems[selected].label, "QUIT") == 0) running = false;
        }

        g4f_frame_begin(ctx, g4f_rgba_u32(10, 10, 12, 255));
        int w = 0, h = 0;
        g4f_window_get_size(window, &w, &h);

        g4f_ui_begin(ui, renderer, window);
        g4f_ui_layout layout{};
        layout.bounds = g4f_rect_f{64, 48, 520, 420};
        layout.padding = 18.0f;
        layout.spacing = 12.0f;
        layout.itemW = 420.0f;
        layout.defaultItemH = 44.0f;
        g4f_ui_layout_begin(ui, layout);

        g4f_ui_label(ui, "BACKROOMS: VOID SHIFT", 34.0f);
        g4f_ui_label(ui, "menu smoke test (UI layer)", 16.0f);
        g4f_ui_layout_spacer(ui, 10.0f);

        for (int i = 0; i < (int)(sizeof(kItems) / sizeof(kItems[0])); i++) {
            g4f_ui_push_id(ui, kItems[i].label);
            int clicked = g4f_ui_button(ui, kItems[i].label);
            g4f_ui_pop_id(ui);
            if (clicked) selected = i;
        }

        g4f_ui_layout_spacer(ui, 8.0f);
        int chk = (selected == 1) ? 1 : 0;
        g4f_ui_checkbox(ui, "dummy checkbox (state not persisted)", &chk);

        float vol = 0.65f;
        g4f_ui_slider_float(ui, "dummy slider", &vol, 0.0f, 1.0f);

        g4f_ui_end(ui);

        char help[256];
        std::snprintf(help, sizeof(help), "ESC: exit   Mouse: %.0f,%.0f   Wheel: %.2f",
                      g4f_mouse_x(window), g4f_mouse_y(window), g4f_mouse_wheel_delta(window));
        g4f_draw_text(renderer, help, 64, (float)h - 40.0f, 14.0f, g4f_rgba_u32(160, 160, 180, 255));

        g4f_frame_end(ctx);
    }

    g4f_ui_destroy(ui);
    g4f_ctx_destroy(ctx);
    return 0;
}
