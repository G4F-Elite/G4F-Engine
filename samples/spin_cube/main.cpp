#include <cstdio>

#include "g4f/g4f.h"
#include "g4f/g4f_ui.h"

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
    g4f_ui* uiState = g4f_ui_create();
    if (!uiState) {
        g4f_renderer_destroy(ui);
        g4f_ctx3d_destroy(ctx);
        return 1;
    }

    while (g4f_ctx3d_poll(ctx)) {
        g4f_window* window = g4f_ctx3d_window(ctx);
        if (g4f_key_pressed(window, G4F_KEY_ESCAPE)) g4f_window_request_close(window);

        g4f_frame3d_begin(ctx, g4f_rgba_u32(14, 14, 18, 255));
        g4f_gfx_draw_debug_cube(gfx, (float)g4f_ctx3d_time(ctx));

        g4f_renderer_begin(ui);
        g4f_ui_begin(uiState, ui, window);
        g4f_ui_panel_begin_scroll(uiState, "G4F SPIN CUBE", g4f_rect_f{24, 24, 420, 240});
        g4f_ui_text_wrapped(uiState, "D3D11 render + Direct2D UI overlay.\nWheel: scroll. UP/DOWN or W/S: focus. ENTER/SPACE: activate. LEFT/RIGHT or A/D: slider.", 16.0f);
        g4f_ui_layout_spacer(uiState, 6.0f);
        g4f_ui_separator(uiState);
        int show = 0;
        g4f_ui_checkbox_k(uiState, "checkbox (stored)", "show", 1, &show);
        float f = 0.0f;
        g4f_ui_slider_float_k(uiState, "slider (stored)", "slider", 0.42f, 0.0f, 1.0f, &f);
        g4f_ui_layout_spacer(uiState, 10.0f);
        g4f_ui_text_wrapped(uiState, "Long text to force scrolling. Long text to force scrolling. Long text to force scrolling. Long text to force scrolling. Long text to force scrolling.", 14.0f);
        g4f_ui_panel_end(uiState);
        g4f_ui_end(uiState);
        g4f_renderer_end(ui);

        g4f_frame3d_end(ctx);
    }

    g4f_ui_destroy(uiState);
    g4f_renderer_destroy(ui);
    g4f_ctx3d_destroy(ctx);
    return 0;
}
