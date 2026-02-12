#include <cstdio>
#include <cstring>

#include "g4f/g4f.h"

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

        int w = 0, h = 0;
        g4f_window_get_size(window, &w, &h);

        g4f_frame_begin(ctx, g4f_rgba_u32(10, 10, 12, 255));

        g4f_draw_text(renderer, "BACKROOMS: VOID SHIFT", 64, 54, 34.0f, g4f_rgba_u32(245, 230, 120, 255));
        g4f_draw_text(renderer, "menu smoke test (2D-only)", 66, 92, 16.0f, g4f_rgba_u32(190, 190, 205, 255));

        const float baseX = 80.0f;
        float y = 160.0f;
        for (int i = 0; i < (int)(sizeof(kItems) / sizeof(kItems[0])); i++) {
            bool isSel = (i == selected);
            uint32_t bg = isSel ? g4f_rgba_u32(40, 60, 120, 255) : g4f_rgba_u32(22, 22, 28, 255);
            uint32_t fg = isSel ? g4f_rgba_u32(255, 255, 255, 255) : g4f_rgba_u32(200, 200, 210, 255);
            g4f_draw_rect(renderer, g4f_rect_f{baseX, y, 420, 44}, bg);
            g4f_draw_rect_outline(renderer, g4f_rect_f{baseX, y, 420, 44}, 1.5f, g4f_rgba_u32(70, 70, 90, 255));
            g4f_draw_text(renderer, kItems[i].label, baseX + 16.0f, y + 10.0f, 18.0f, fg);
            y += 56.0f;
        }

        char help[256];
        std::snprintf(help, sizeof(help), "UP/DOWN: select   ENTER/SPACE: confirm   ESC: exit   Mouse: %.0f,%.0f",
                      g4f_mouse_x(window), g4f_mouse_y(window));
        g4f_draw_text(renderer, help, 64, (float)h - 40.0f, 14.0f, g4f_rgba_u32(160, 160, 180, 255));

        g4f_frame_end(ctx);
    }

    g4f_ctx_destroy(ctx);
    return 0;
}
