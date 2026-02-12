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

    while (g4f_ctx3d_poll(ctx)) {
        g4f_window* window = g4f_ctx3d_window(ctx);
        g4f_gfx* gfx = g4f_ctx3d_gfx(ctx);
        if (g4f_key_pressed(window, G4F_KEY_ESCAPE)) g4f_window_request_close(window);

        g4f_frame3d_begin(ctx, g4f_rgba_u32(14, 14, 18, 255));
        g4f_gfx_draw_debug_cube(gfx, (float)g4f_ctx3d_time(ctx));
        g4f_frame3d_end(ctx);
    }

    g4f_ctx3d_destroy(ctx);
    return 0;
}

