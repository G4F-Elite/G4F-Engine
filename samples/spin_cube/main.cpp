#include <cstdio>
#include <vector>
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
    g4f_gfx_mesh* floorMesh = g4f_gfx_mesh_create_plane_xz_p3n3uv2(gfx, 12.0f, 6.0f);
    const int checkerW = 128;
    const int checkerH = 128;
    const int checkerCellPx = 16;
    std::vector<uint32_t> checkerPixels((size_t)checkerW * (size_t)checkerH);

    g4f_gfx_texture* checker = g4f_gfx_texture_create_rgba8_dynamic(gfx, checkerW, checkerH);
    if (checker) {
        for (int y = 0; y < checkerH; y++) {
            for (int x = 0; x < checkerW; x++) {
                int cell = ((x / checkerCellPx) ^ (y / checkerCellPx)) & 1;
                checkerPixels[(size_t)y * (size_t)checkerW + (size_t)x] = cell ? g4f_rgba_u32(240, 240, 255, 255) : g4f_rgba_u32(40, 40, 55, 255);
            }
        }
        g4f_gfx_texture_update_rgba8(checker, checkerPixels.data(), checkerW * 4);
    }

    g4f_gfx_material_unlit_desc mdesc{};
    mdesc.tintRgba = g4f_rgba_u32(255, 255, 255, 255);
    mdesc.texture = checker;
    mdesc.alphaBlend = 0;
    mdesc.depthTest = 1;
    mdesc.depthWrite = 1;
    mdesc.cullMode = 0;
    g4f_gfx_material* mtlUnlit = g4f_gfx_material_create_unlit(gfx, &mdesc);
    g4f_gfx_material* mtlLit = g4f_gfx_material_create_lit(gfx, &mdesc);
    if (!cube || !floorMesh || !checker || !mtlUnlit || !mtlLit) {
        std::fprintf(stderr, "Failed to create 3D resources\n");
        g4f_gfx_material_destroy(mtlUnlit);
        g4f_gfx_material_destroy(mtlLit);
        g4f_gfx_texture_destroy(checker);
        g4f_gfx_mesh_destroy(floorMesh);
        g4f_gfx_mesh_destroy(cube);
        g4f_ctx3d_destroy(ctx);
        return 1;
    }

    g4f_renderer* ui = g4f_renderer_create_for_gfx(gfx);
    if (!ui) {
        std::fprintf(stderr, "Failed to create UI renderer for gfx\n");
        g4f_gfx_material_destroy(mtlUnlit);
        g4f_gfx_material_destroy(mtlLit);
        g4f_gfx_texture_destroy(checker);
        g4f_gfx_mesh_destroy(floorMesh);
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

        // Read UI store (from previous frame) to drive 3D before drawing the overlay UI.
        float slider = g4f_ui_store_get_f(uiState, "slider", 0.42f);
        int alpha = g4f_ui_store_get_i(uiState, "alpha", 0);
        int depthTest = g4f_ui_store_get_i(uiState, "depthTest", 1);
        int depthWrite = g4f_ui_store_get_i(uiState, "depthWrite", 1);
        int cullNone = g4f_ui_store_get_i(uiState, "cullNone", 0);
        int lit = g4f_ui_store_get_i(uiState, "lit", 1);
        int vsync = g4f_ui_store_get_i(uiState, "vsync", 1);
        float sx = g4f_ui_store_get_f(uiState, "sx", 1.0f);
        float sy = g4f_ui_store_get_f(uiState, "sy", 1.0f);
        float sz = g4f_ui_store_get_f(uiState, "sz", 1.0f);
        g4f_gfx_set_vsync(gfx, vsync);

        uint32_t tint = g4f_rgba_u32((uint8_t)(80 + 175 * slider), (uint8_t)(140 + 115 * slider), 255, 255);
        g4f_gfx_material_set_tint_rgba(mtlUnlit, tint);
        g4f_gfx_material_set_tint_rgba(mtlLit, tint);
        g4f_gfx_material_set_alpha_blend(mtlUnlit, alpha);
        g4f_gfx_material_set_alpha_blend(mtlLit, alpha);
        if (alpha) {
            g4f_gfx_material_set_tint_rgba(mtlUnlit, (tint & 0xFFFFFF00u) | 150u);
            g4f_gfx_material_set_tint_rgba(mtlLit, (tint & 0xFFFFFF00u) | 150u);
        }
        g4f_gfx_material_set_depth(mtlUnlit, depthTest, depthWrite);
        g4f_gfx_material_set_depth(mtlLit, depthTest, depthWrite);
        g4f_gfx_material_set_cull(mtlUnlit, cullNone ? 1 : 0);
        g4f_gfx_material_set_cull(mtlLit, cullNone ? 1 : 0);

        g4f_frame3d_begin(ctx, g4f_rgba_u32(14, 14, 18, 255));

        float t = (float)g4f_ctx3d_time(ctx);
        if (checker) {
            int phase = (int)(t * 10.0f);
            for (int y = 0; y < checkerH; y++) {
                for (int x = 0; x < checkerW; x++) {
                    int cell = (((x + phase) / checkerCellPx) ^ ((y + phase) / checkerCellPx)) & 1;
                    checkerPixels[(size_t)y * (size_t)checkerW + (size_t)x] = cell ? g4f_rgba_u32(240, 240, 255, 255) : g4f_rgba_u32(40, 40, 55, 255);
                }
            }
            g4f_gfx_texture_update_rgba8(checker, checkerPixels.data(), checkerW * 4);
        }
        float aspect = g4f_gfx_aspect(gfx);
        g4f_mat4 proj = g4f_camera_fps_proj(&cam, aspect);
        g4f_mat4 view = g4f_camera_fps_view(&cam);
        g4f_mat4 rot = g4f_mat4_mul(g4f_mat4_rotation_y(t * 0.8f), g4f_mat4_rotation_x(t * 0.5f));
        g4f_mat4 model = g4f_mat4_mul(g4f_mat4_scale(sx, sy, sz), rot);
        g4f_mat4 mvp = g4f_mat4_mul(g4f_mat4_mul(model, view), proj);

        // Light travels in this direction (world space).
        g4f_gfx_set_light_dir(gfx, -0.4f, -1.0f, -0.2f);
        g4f_gfx_set_light_colors(gfx, g4f_rgba_u32(255, 250, 240, 255), g4f_rgba_u32(40, 50, 70, 255));

        g4f_mat4 floorModel = g4f_mat4_translation(0.0f, -1.3f, 0.0f);
        g4f_mat4 floorMvp = g4f_mat4_mul(g4f_mat4_mul(floorModel, view), proj);
        g4f_gfx_material_set_cull(mtlLit, 0);
        g4f_gfx_material_set_depth(mtlLit, 1, 1);
        g4f_gfx_draw_mesh_xform(gfx, floorMesh, mtlLit, &floorModel, &floorMvp);

        g4f_gfx_draw_mesh_xform(gfx, cube, lit ? mtlLit : mtlUnlit, &model, &mvp);

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
        int alphaUi = 0;
        g4f_ui_checkbox_k(uiState, "alpha blend (stored)", "alpha", 0, &alphaUi);
        int depthTestUi = 0;
        int depthWriteUi = 0;
        g4f_ui_checkbox_k(uiState, "depth test (stored)", "depthTest", 1, &depthTestUi);
        g4f_ui_disabled_begin(uiState, !depthTestUi);
        g4f_ui_checkbox_k(uiState, "depth write (stored)", "depthWrite", 1, &depthWriteUi);
        g4f_ui_tooltip(uiState, "Depth write is only meaningful when depth test is enabled.", 14.0f);
        g4f_ui_disabled_end(uiState);
        int cullNoneUi = 0;
        g4f_ui_checkbox_k(uiState, "cull none (stored)", "cullNone", 0, &cullNoneUi);
        int litUi = 0;
        g4f_ui_checkbox_k(uiState, "lit shading (stored)", "lit", 1, &litUi);
        int vsyncUi = 0;
        g4f_ui_checkbox_k(uiState, "vsync (stored)", "vsync", 1, &vsyncUi);
        g4f_ui_tooltip(uiState, "VSync syncs Present() to the display (less tearing, more latency).", 14.0f);
        g4f_ui_slider_float_k(uiState, "scale X", "sx", 1.0f, 0.25f, 3.0f, &sx);
        g4f_ui_slider_float_k(uiState, "scale Y", "sy", 1.0f, 0.25f, 3.0f, &sy);
        g4f_ui_slider_float_k(uiState, "scale Z", "sz", 1.0f, 0.25f, 3.0f, &sz);
        char titleBuf[96];
        int titleChanged = g4f_ui_input_text_k(uiState, "window title", "title", "", 80, titleBuf, (int)sizeof(titleBuf));
        if (titleChanged || titleBuf[0] != '\0') {
            char composed[160];
            if (titleBuf[0] != '\0') std::snprintf(composed, sizeof(composed), "%s - %s", "G4F Engine", titleBuf);
            else std::snprintf(composed, sizeof(composed), "%s", "G4F Engine - Spin Cube (3D bring-up)");
            g4f_window_set_title(window, composed);
        }
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
    g4f_gfx_material_destroy(mtlUnlit);
    g4f_gfx_material_destroy(mtlLit);
    g4f_gfx_texture_destroy(checker);
    g4f_gfx_mesh_destroy(floorMesh);
    g4f_gfx_mesh_destroy(cube);
    g4f_ctx3d_destroy(ctx);
    return 0;
}
