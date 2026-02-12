#include <cstdio>
#include <cstring>
#include <vector>

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

static g4f_bitmap* createUiIcon(g4f_renderer* renderer) {
    const int width = 48;
    const int height = 48;
    std::vector<uint8_t> rgba((size_t)width * (size_t)height * 4);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int cx = (x / 8) & 1;
            int cy = (y / 8) & 1;
            int on = (cx ^ cy) & 1;
            uint8_t r = on ? 240 : 30;
            uint8_t g = on ? 240 : 30;
            uint8_t b = on ? 255 : 50;
            uint8_t a = 255;
            size_t idx = ((size_t)y * (size_t)width + (size_t)x) * 4;
            rgba[idx + 0] = r;
            rgba[idx + 1] = g;
            rgba[idx + 2] = b;
            rgba[idx + 3] = a;
        }
    }
    return g4f_bitmap_create_rgba8(renderer, width, height, rgba.data(), width * 4);
}

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

    g4f_bitmap* icon = nullptr;

    bool running = true;
    while (running && g4f_ctx_poll(ctx)) {
        g4f_window* window = g4f_ctx_window(ctx);
        g4f_renderer* renderer = g4f_ctx_renderer(ctx);
        if (g4f_key_pressed(window, G4F_KEY_ESCAPE)) running = false;

        g4f_frame_begin(ctx, g4f_rgba_u32(10, 10, 12, 255));
        int w = 0, h = 0;
        g4f_window_get_size(window, &w, &h);

        g4f_ui_begin(ui, renderer, window);
        g4f_ui_panel_begin_scroll(ui, "BACKROOMS: VOID SHIFT", g4f_rect_f{64, 48, 520, 420});
        g4f_ui_text_wrapped(ui, "menu smoke test (UI layer). Mouse wheel scroll is supported.", 16.0f);
        g4f_ui_layout_spacer(ui, 6.0f);

        if (!icon) icon = createUiIcon(renderer);
        if (icon) {
            g4f_ui_image(ui, icon, 96.0f, 1.0f);
            g4f_ui_layout_spacer(ui, 6.0f);
            g4f_ui_image_button(ui, "image button", icon, 56.0f, 1.0f);
            g4f_ui_layout_spacer(ui, 6.0f);
        }

        for (int i = 0; i < (int)(sizeof(kItems) / sizeof(kItems[0])); i++) {
            g4f_ui_push_id(ui, kItems[i].label);
            int clicked = g4f_ui_button(ui, kItems[i].label);
            g4f_ui_pop_id(ui);
            if (clicked) {
                if (std::strcmp(kItems[i].label, "QUIT") == 0) running = false;
            }
        }

        g4f_ui_layout_spacer(ui, 8.0f);
        int chk = 0;
        g4f_ui_checkbox_k(ui, "checkbox (stored)", "chk", 0, &chk);

        float vol = 0.0f;
        g4f_ui_slider_float_k(ui, "slider (stored)", "vol", 0.65f, 0.0f, 1.0f, &vol);

        char nameBuf[64];
        g4f_ui_input_text_k(ui, "player name (stored)", "player_name", "type here...", 48, nameBuf, (int)sizeof(nameBuf));

        g4f_ui_panel_end(ui);
        g4f_ui_end(ui);

        char help[256];
        std::snprintf(help, sizeof(help), "ESC: exit   Mouse: %.0f,%.0f   Wheel: %.2f",
                      g4f_mouse_x(window), g4f_mouse_y(window), g4f_mouse_wheel_delta(window));
        g4f_draw_text(renderer, help, 64, (float)h - 40.0f, 14.0f, g4f_rgba_u32(160, 160, 180, 255));

        g4f_frame_end(ctx);
    }

    g4f_bitmap_destroy(icon);
    g4f_ui_destroy(ui);
    g4f_ctx_destroy(ctx);
    return 0;
}
