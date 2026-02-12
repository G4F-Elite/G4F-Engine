#include <cstdio>
#include "g4f/g4f.h"
#include "g4f/g4f_camera.h"
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
    g4f_gfx_mesh* cube = g4f_gfx_mesh_create_cube_p3n3uv2(gfx, 1.0f);
    g4f_gfx_texture* checker = g4f_gfx_texture_create_checker_rgba8(
        gfx,
        64,
        64,
        8,
        g4f_rgba_u32(240, 240, 255, 255),
        g4f_rgba_u32(40, 40, 55, 255)
    );
    g4f_gfx_material_unlit_desc mdesc{};
    mdesc.tintRgba = g4f_rgba_u32(255, 255, 255, 255);
    mdesc.texture = checker;
    g4f_gfx_material* mtl = g4f_gfx_material_create_unlit(gfx, &mdesc);
    if (!cube || !checker || !mtl) {
        std::fprintf(stderr, "Failed to create 3D resources\n");
        g4f_gfx_material_destroy(mtl);
        g4f_gfx_texture_destroy(checker);
        g4f_gfx_mesh_destroy(cube);
        g4f_ctx3d_destroy(ctx);
        return 1;
    }

    g4f_renderer* ui = g4f_renderer_create_for_gfx(gfx);
    if (!ui) {
        std::fprintf(stderr, "Failed to create UI renderer for gfx\n");
        g4f_gfx_material_destroy(mtl);
        g4f_gfx_texture_destroy(checker);
        g4f_gfx_mesh_destroy(cube);
        g4f_ctx3d_destroy(ctx);
        return 1;
    }
    g4f_ui* uiState = g4f_ui_create();
    if (!uiState) {
        g4f_renderer_destroy(ui);
        g4f_ctx3d_destroy(ctx);
        return 1;
    }

    g4f_camera_fps cam = g4f_camera_fps_default();

    while (g4f_ctx3d_poll(ctx)) {
        g4f_window* window = g4f_ctx3d_window(ctx);
        if (g4f_key_pressed(window, G4F_KEY_ESCAPE)) g4f_window_request_close(window);

        if (g4f_mouse_pressed(window, G4F_MOUSE_BUTTON_RIGHT)) g4f_window_set_cursor_captured(window, 1);
        if (!g4f_mouse_down(window, G4F_MOUSE_BUTTON_RIGHT)) g4f_window_set_cursor_captured(window, 0);

        g4f_camera_fps_update(&cam, window, g4f_ctx3d_dt(ctx));

        g4f_frame3d_begin(ctx, g4f_rgba_u32(14, 14, 18, 255));

        float t = (float)g4f_ctx3d_time(ctx);
        float aspect = g4f_gfx_aspect(gfx);
        g4f_mat4 proj = g4f_camera_fps_proj(&cam, aspect);
        g4f_mat4 view = g4f_camera_fps_view(&cam);
        g4f_mat4 rot = g4f_mat4_mul(g4f_mat4_rotation_y(t * 0.8f), g4f_mat4_rotation_x(t * 0.5f));
        g4f_mat4 mvp = g4f_mat4_mul(g4f_mat4_mul(rot, view), proj);
        g4f_gfx_draw_mesh(gfx, cube, mtl, &mvp);

        g4f_renderer_begin(ui);
        g4f_ui_begin(uiState, ui, window);
        g4f_ui_panel_begin_scroll(uiState, "G4F SPIN CUBE", g4f_rect_f{24, 24, 420, 240});
        g4f_ui_text_wrapped(uiState, "D3D11 render + Direct2D UI overlay.\nHold RMB: capture mouse + FPS camera (WASD, Space/Ctrl). Alt-Tab releases capture.\nWheel: scroll. UP/DOWN or W/S: focus. TAB: cycle focus. ENTER/SPACE: activate. LEFT/RIGHT or A/D: slider.", 16.0f);
        g4f_ui_layout_spacer(uiState, 6.0f);
        g4f_ui_separator(uiState);
        int show = 0;
        g4f_ui_checkbox_k(uiState, "checkbox (stored)", "show", 1, &show);
        float f = 0.0f;
        g4f_ui_slider_float_k(uiState, "slider (stored)", "slider", 0.42f, 0.0f, 1.0f, &f);
        uint32_t tint = g4f_rgba_u32((uint8_t)(80 + 175 * f), (uint8_t)(140 + 115 * f), 255, 255);
        g4f_gfx_material_set_tint_rgba(mtl, tint);
        char textBuf[64];
        g4f_ui_input_text_k(uiState, "text input (stored)", "text", "hello...", 48, textBuf, (int)sizeof(textBuf));
        g4f_ui_layout_spacer(uiState, 10.0f);
        g4f_ui_text_wrapped(uiState, "Long text to force scrolling. Long text to force scrolling. Long text to force scrolling. Long text to force scrolling. Long text to force scrolling.", 14.0f);
        g4f_ui_panel_end(uiState);
        g4f_ui_end(uiState);
        g4f_renderer_end(ui);

        g4f_frame3d_end(ctx);
    }

    g4f_ui_destroy(uiState);
    g4f_renderer_destroy(ui);
    g4f_gfx_material_destroy(mtl);
    g4f_gfx_texture_destroy(checker);
    g4f_gfx_mesh_destroy(cube);
    g4f_ctx3d_destroy(ctx);
    return 0;
}
