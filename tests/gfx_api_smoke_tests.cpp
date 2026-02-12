#include <cassert>
#include <cstdio>
#include <vector>

#include "g4f/g4f.h"
#include "g4f/g4f_camera.h"

static void fillChecker(std::vector<uint32_t>& out, int w, int h, int cell) {
    out.resize((size_t)w * (size_t)h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int a = ((x / cell) + (y / cell)) & 1;
            out[(size_t)y * (size_t)w + (size_t)x] = a ? 0xFFFFFFFFu : 0x303040FFu;
        }
    }
}

int main() {
    g4f_window_desc windowDesc{};
    windowDesc.title_utf8 = "g4f_gfx_api_smoke_tests";
    windowDesc.width = 360;
    windowDesc.height = 220;
    windowDesc.resizable = 0;

    g4f_ctx3d* ctx = g4f_ctx3d_create(&windowDesc);
    if (!ctx) {
        std::fprintf(stderr, "gfx_api_smoke_tests: g4f_ctx3d_create failed\n");
        return 1;
    }

    g4f_window* window = g4f_ctx3d_window(ctx);
    g4f_gfx* gfx = g4f_ctx3d_gfx(ctx);
    assert(window && gfx);

    g4f_gfx_set_vsync(gfx, 0);
    g4f_gfx_set_light_dir(gfx, -0.3f, -1.0f, -0.2f);
    g4f_gfx_set_light_colors(gfx, g4f_rgba_u32(255, 250, 240, 255), g4f_rgba_u32(35, 45, 70, 255));

    const int texW = 32, texH = 32;
    g4f_gfx_texture* dyn = g4f_gfx_texture_create_rgba8_dynamic(gfx, texW, texH);
    assert(dyn != nullptr);

    std::vector<uint32_t> checker;
    fillChecker(checker, texW, texH, 4);
    int ok = g4f_gfx_texture_update_rgba8(dyn, checker.data(), texW * 4);
    assert(ok == 1);

    g4f_gfx_material_unlit_desc mdesc{};
    mdesc.tintRgba = g4f_rgba_u32(255, 255, 255, 255);
    mdesc.texture = dyn;
    mdesc.alphaBlend = 0;
    mdesc.depthTest = 1;
    mdesc.depthWrite = 1;
    mdesc.cullMode = 0;
    g4f_gfx_material* unlit = g4f_gfx_material_create_unlit(gfx, &mdesc);
    g4f_gfx_material* lit = g4f_gfx_material_create_lit(gfx, &mdesc);
    assert(unlit && lit);

    g4f_gfx_mesh* cube = g4f_gfx_mesh_create_cube_p3n3uv2(gfx, 1.0f);
    g4f_gfx_mesh* plane = g4f_gfx_mesh_create_plane_xz_p3n3uv2(gfx, 3.0f, 2.0f);
    assert(cube && plane);

    g4f_camera_fps cam = g4f_camera_fps_default();
    cam.position = g4f_vec3{0.0f, 1.2f, -4.5f};

    double start = g4f_ctx3d_time(ctx);
    int frames = 0;
    while (g4f_ctx3d_poll(ctx)) {
        if (g4f_ctx3d_time(ctx) - start > 0.35 || frames++ > 120) {
            g4f_window_request_close(window);
        }

        // Should be safe even without focus.
        g4f_camera_fps_update(&cam, window, g4f_ctx3d_dt(ctx));

        g4f_frame3d_begin(ctx, g4f_rgba_u32(10, 10, 14, 255));

        int w = 0, h = 0;
        g4f_gfx_get_size(gfx, &w, &h);
        assert(w > 0 && h > 0);

        float t = (float)g4f_ctx3d_time(ctx);
        g4f_mat4 view = g4f_camera_fps_view(&cam);
        g4f_mat4 proj = g4f_camera_fps_proj(&cam, g4f_gfx_aspect(gfx));
        g4f_mat4 model = g4f_mat4_mul(g4f_mat4_rotation_y(t), g4f_mat4_rotation_x(t * 0.7f));
        g4f_mat4 mvp = g4f_mat4_mul(g4f_mat4_mul(model, view), proj);

        g4f_gfx_draw_mesh_xform(gfx, cube, lit, &model, &mvp);
        g4f_gfx_draw_mesh(gfx, plane, unlit, &mvp);
        g4f_gfx_draw_debug_cube(gfx, t);

        g4f_frame3d_end(ctx);
    }

    g4f_gfx_mesh_destroy(plane);
    g4f_gfx_mesh_destroy(cube);
    g4f_gfx_material_destroy(lit);
    g4f_gfx_material_destroy(unlit);
    g4f_gfx_texture_destroy(dyn);
    g4f_ctx3d_destroy(ctx);

    std::printf("gfx_api_smoke_tests: OK\n");
    return 0;
}
