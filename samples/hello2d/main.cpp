#include <cstdio>

#include "g4f/g4f.h"

int main() {
    g4f_window_desc windowDesc{};
    windowDesc.title_utf8 = "G4F Engine - Hello 2D";
    windowDesc.width = 1280;
    windowDesc.height = 720;
    windowDesc.resizable = 1;

    g4f_ctx* ctx = g4f_ctx_create(&windowDesc);
    if (!ctx) {
        std::fprintf(stderr, "Failed to create g4f_ctx\n");
        return 1;
    }

    double fpsAcc = 0.0;
    int fpsFrames = 0;
    double fpsValue = 0.0;

    while (g4f_ctx_poll(ctx)) {
        g4f_window* window = g4f_ctx_window(ctx);
        g4f_renderer* renderer = g4f_ctx_renderer(ctx);
        if (g4f_key_pressed(window, G4F_KEY_ESCAPE)) g4f_window_request_close(window);

        double dt = (double)g4f_ctx_dt(ctx);

        fpsAcc += dt;
        fpsFrames += 1;
        if (fpsAcc >= 0.25) {
            fpsValue = (double)fpsFrames / fpsAcc;
            fpsAcc = 0.0;
            fpsFrames = 0;
        }

        g4f_frame_begin(ctx, g4f_rgba_u32(18, 18, 22, 255));

        int w = 0, h = 0;
        g4f_window_get_size(window, &w, &h);

        g4f_draw_rect(renderer, g4f_rect_f{40, 40, 420, 180}, g4f_rgba_u32(32, 45, 88, 255));
        g4f_draw_rect_outline(renderer, g4f_rect_f{40, 40, 420, 180}, 2.0f, g4f_rgba_u32(90, 120, 255, 255));

        char text[256];
        std::snprintf(text, sizeof(text), "Window: %dx%d\nMouse: %.0f, %.0f\nWheel: %.2f\nFPS: %.1f",
                      w, h, g4f_mouse_x(window), g4f_mouse_y(window), g4f_mouse_wheel_delta(window), fpsValue);
        g4f_draw_text(renderer, text, 60, 60, 18.0f, g4f_rgba_u32(235, 235, 245, 255));

        g4f_draw_line(renderer, 40, (float)h - 60, (float)w - 40, (float)h - 60, 2.0f, g4f_rgba_u32(70, 70, 90, 255));
        g4f_draw_text(renderer, "Press ESC to exit", 60, (float)h - 50, 16.0f, g4f_rgba_u32(200, 200, 210, 255));

        g4f_frame_end(ctx);
    }

    g4f_ctx_destroy(ctx);
    return 0;
}
