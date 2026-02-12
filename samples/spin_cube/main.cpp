#include <cstdio>

#include "g4f/g4f.h"

int main() {
    g4f_window_desc windowDesc{};
    windowDesc.title_utf8 = "G4F Engine - Spin Cube (3D bring-up)";
    windowDesc.width = 1280;
    windowDesc.height = 720;
    windowDesc.resizable = 1;

    g4f_ctx3d* ctx = g4f_ctx3d_create(&windowDesc);
    if (!ctx) {
        std::fprintf(stderr, "Failed to create g4f_ctx3d\n");
        return 1;
    }

    g4f_gfx* gfx = g4f_ctx3d_gfx(ctx);
    g4f_renderer* ui = g4f_renderer_create_for_gfx(gfx);
    if (!ui) {
        std::fprintf(stderr, "Failed to create UI renderer for gfx\n");
        g4f_ctx3d_destroy(ctx);
        return 1;
    }

    while (g4f_ctx3d_poll(ctx)) {
        g4f_window* window = g4f_ctx3d_window(ctx);
        if (g4f_key_pressed(window, G4F_KEY_ESCAPE)) g4f_window_request_close(window);

        g4f_frame3d_begin(ctx, g4f_rgba_u32(14, 14, 18, 255));
        g4f_gfx_draw_debug_cube(gfx, (float)g4f_ctx3d_time(ctx));

        g4f_renderer_begin(ui);
        g4f_draw_round_rect(ui, g4f_rect_f{24, 24, 420, 110}, 14.0f, g4f_rgba_u32(20, 20, 26, 220));
        g4f_draw_round_rect_outline(ui, g4f_rect_f{24, 24, 420, 110}, 14.0f, 2.0f, g4f_rgba_u32(70, 90, 180, 255));
        g4f_draw_text(ui, "D3D11 + 2D UI overlay", 44, 44, 20.0f, g4f_rgba_u32(245, 245, 255, 255));
        g4f_draw_text(ui, "ESC: quit", 44, 74, 16.0f, g4f_rgba_u32(200, 200, 210, 255));
        g4f_renderer_end(ui);

        g4f_frame3d_end(ctx);
    }

    g4f_renderer_destroy(ui);
    g4f_ctx3d_destroy(ctx);
    return 0;
}
