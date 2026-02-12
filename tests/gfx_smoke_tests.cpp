#include <cassert>
#include <cstdio>

#include "g4f/g4f.h"
#include "g4f/g4f_ctx3d_ui.h"

int main() {
    g4f_window_desc windowDesc{};
    windowDesc.title_utf8 = "g4f_gfx_smoke_tests";
    windowDesc.width = 320;
    windowDesc.height = 180;
    windowDesc.resizable = 0;

    g4f_ctx3d_ui* ctx = g4f_ctx3d_ui_create(&windowDesc);
    if (!ctx) {
        std::fprintf(stderr, "gfx_smoke_tests: failed to create ctx\n");
        return 1;
    }

    g4f_window* window = g4f_ctx3d_ui_window(ctx);
    g4f_gfx* gfx = g4f_ctx3d_ui_gfx(ctx);
    assert(window && gfx);

    g4f_gfx_texture* tex = g4f_gfx_texture_create_checker_rgba8(
        gfx,
        32,
        32,
        4,
        g4f_rgba_u32(255, 255, 255, 255),
        g4f_rgba_u32(30, 30, 40, 255)
    );
    g4f_gfx_material_unlit_desc mdesc{};
    mdesc.tintRgba = g4f_rgba_u32(255, 255, 255, 255);
    mdesc.texture = tex;
    mdesc.alphaBlend = 0;
    mdesc.depthTest = 1;
    mdesc.depthWrite = 1;
    mdesc.cullMode = 0;
    g4f_gfx_material* mtl = g4f_gfx_material_create_lit(gfx, &mdesc);
    g4f_gfx_mesh* cube = g4f_gfx_mesh_create_cube_p3n3uv2(gfx, 1.0f);
    if (!tex || !mtl || !cube) {
        std::fprintf(stderr, "gfx_smoke_tests: failed to create resources\n");
        g4f_gfx_mesh_destroy(cube);
        g4f_gfx_material_destroy(mtl);
        g4f_gfx_texture_destroy(tex);
        g4f_ctx3d_ui_destroy(ctx);
        return 1;
    }

    // Run a short loop (auto-closes, no manual interaction required).
    double start = g4f_ctx3d_ui_time(ctx);
    while (g4f_ctx3d_ui_poll(ctx)) {
        if (g4f_ctx3d_ui_time(ctx) - start > 0.35) {
            g4f_window_request_close(window);
        }

        g4f_ctx3d_ui_frame3d_begin(ctx, g4f_rgba_u32(10, 10, 14, 255));

        float t = (float)g4f_ctx3d_ui_time(ctx);
        g4f_mat4 proj = g4f_mat4_perspective(70.0f * 3.14159265f / 180.0f, g4f_gfx_aspect(gfx), 0.1f, 100.0f);
        g4f_mat4 view = g4f_mat4_translation(0.0f, 0.0f, 4.0f);
        g4f_mat4 rot = g4f_mat4_mul(g4f_mat4_rotation_y(t), g4f_mat4_rotation_x(t * 0.7f));
        g4f_mat4 mvp = g4f_mat4_mul(g4f_mat4_mul(rot, view), proj);

        g4f_gfx_set_light_dir(gfx, -0.3f, -1.0f, -0.2f);
        g4f_gfx_set_light_colors(gfx, g4f_rgba_u32(255, 250, 240, 255), g4f_rgba_u32(30, 40, 60, 255));
        g4f_gfx_draw_mesh_xform(gfx, cube, mtl, &rot, &mvp);

        // Quick overlay path smoke (no UI widgets required).
        g4f_ctx3d_ui_overlay_begin(ctx);
        g4f_ctx3d_ui_overlay_end(ctx);

        g4f_ctx3d_ui_frame3d_end(ctx);
    }

    g4f_gfx_mesh_destroy(cube);
    g4f_gfx_material_destroy(mtl);
    g4f_gfx_texture_destroy(tex);
    g4f_ctx3d_ui_destroy(ctx);

    std::printf("gfx_smoke_tests: OK\n");
    return 0;
}

