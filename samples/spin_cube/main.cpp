#include <cstdio>
#include <vector>

#include "g4f/g4f.h"
#include "g4f/g4f_camera.h"
#include "g4f/g4f_ui.h"

static g4f_gfx_mesh* createUvCube(g4f_gfx* gfx) {
    // 24 vertices (4 per face) so each face has its own normal/uv.
    const g4f_gfx_vertex_p3n3uv2 v[] = {
        // -Z
        {-1,-1,-1, 0,0,-1, 0,1}, {+1,-1,-1, 0,0,-1, 1,1}, {+1,+1,-1, 0,0,-1, 1,0}, {-1,+1,-1, 0,0,-1, 0,0},
        // +Z
        {-1,-1,+1, 0,0,+1, 0,1}, {-1,+1,+1, 0,0,+1, 0,0}, {+1,+1,+1, 0,0,+1, 1,0}, {+1,-1,+1, 0,0,+1, 1,1},
        // -Y
        {-1,-1,-1, 0,-1,0, 0,1}, {-1,-1,+1, 0,-1,0, 0,0}, {+1,-1,+1, 0,-1,0, 1,0}, {+1,-1,-1, 0,-1,0, 1,1},
        // +Y
        {-1,+1,-1, 0,+1,0, 0,1}, {+1,+1,-1, 0,+1,0, 1,1}, {+1,+1,+1, 0,+1,0, 1,0}, {-1,+1,+1, 0,+1,0, 0,0},
        // +X
        {+1,-1,-1, +1,0,0, 0,1}, {+1,-1,+1, +1,0,0, 1,1}, {+1,+1,+1, +1,0,0, 1,0}, {+1,+1,-1, +1,0,0, 0,0},
        // -X
        {-1,-1,-1, -1,0,0, 1,1}, {-1,+1,-1, -1,0,0, 1,0}, {-1,+1,+1, -1,0,0, 0,0}, {-1,-1,+1, -1,0,0, 0,1},
    };
    const uint16_t idx[] = {
        0,1,2, 0,2,3,
        4,5,6, 4,6,7,
        8,9,10, 8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23,
    };
    return g4f_gfx_mesh_create_p3n3uv2(gfx, v, (int)(sizeof(v) / sizeof(v[0])), idx, (int)(sizeof(idx) / sizeof(idx[0])));
}

static g4f_gfx_texture* createCheckerTexture(g4f_gfx* gfx, int w, int h) {
    std::vector<uint32_t> pixels((size_t)w * (size_t)h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int cx = (x / 8) & 1;
            int cy = (y / 8) & 1;
            int on = (cx ^ cy) & 1;
            uint32_t c = on ? g4f_rgba_u32(240, 240, 255, 255) : g4f_rgba_u32(40, 40, 55, 255);
            pixels[(size_t)y * (size_t)w + (size_t)x] = c;
        }
    }
    return g4f_gfx_texture_create_rgba8(gfx, w, h, pixels.data(), w * 4);
}

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
    g4f_gfx_mesh* cube = createUvCube(gfx);
    g4f_gfx_texture* checker = createCheckerTexture(gfx, 64, 64);
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
        int ww = 0, wh = 0;
        g4f_window_get_size(window, &ww, &wh);
        float aspect = (wh > 0) ? ((float)ww / (float)wh) : 1.0f;
        g4f_mat4 proj = g4f_camera_fps_proj(&cam, aspect);
        g4f_mat4 view = g4f_camera_fps_view(&cam);
        g4f_mat4 rot = g4f_mat4_mul(g4f_mat4_rotation_y(t * 0.8f), g4f_mat4_rotation_x(t * 0.5f));
        g4f_mat4 mvp = g4f_mat4_mul(g4f_mat4_mul(rot, view), proj);
        g4f_gfx_draw_mesh(gfx, cube, mtl, &mvp);

        g4f_renderer_begin(ui);
        g4f_ui_begin(uiState, ui, window);
        g4f_ui_panel_begin_scroll(uiState, "G4F SPIN CUBE", g4f_rect_f{24, 24, 420, 240});
        g4f_ui_text_wrapped(uiState, "D3D11 render + Direct2D UI overlay.\nHold RMB: capture mouse + FPS camera (WASD, Space/Ctrl). Wheel: scroll. UP/DOWN or W/S: focus. TAB: cycle focus. ENTER/SPACE: activate. LEFT/RIGHT or A/D: slider.", 16.0f);
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
