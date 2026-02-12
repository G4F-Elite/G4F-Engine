#include <cassert>
#include <cstdio>
#include <cstring>

#include "g4f/g4f.h"

static void expectErrorContains(const char* needle) {
    const char* err = g4f_last_error();
    assert(err != nullptr);
    assert(err[0] != '\0');
    assert(std::strstr(err, needle) != nullptr);
}

int main() {
    g4f_clear_error();
    assert(g4f_last_error()[0] == '\0');

    // Null-arg validation.
    g4f_ctx* ctx = g4f_ctx_create(nullptr);
    assert(ctx == nullptr);
    expectErrorContains("g4f_ctx_create");

    g4f_clear_error();
    g4f_renderer* r = g4f_renderer_create(nullptr);
    assert(r == nullptr);
    expectErrorContains("g4f_renderer_create");

    g4f_clear_error();
    g4f_gfx* gfx = g4f_gfx_create(nullptr);
    assert(gfx == nullptr);
    expectErrorContains("g4f_gfx_create");

    // 2D bitmap validation.
    g4f_clear_error();
    g4f_bitmap* bitmap0 = g4f_bitmap_load(nullptr, "missing.png");
    assert(bitmap0 == nullptr);
    expectErrorContains("g4f_bitmap_load");

    g4f_clear_error();
    const uint32_t pixel = 0xFFFFFFFFu;
    g4f_bitmap* bitmap1 = g4f_bitmap_create_rgba8(nullptr, 1, 1, &pixel, 4);
    assert(bitmap1 == nullptr);
    expectErrorContains("g4f_bitmap_create_rgba8");

    // 3D resource validation.
    g4f_clear_error();
    g4f_gfx_texture* tex0 = g4f_gfx_texture_create_checker_rgba8(nullptr, 4, 4, 8, 0xFF0000FFu, 0xFFFFFFFFu);
    assert(tex0 == nullptr);
    expectErrorContains("g4f_gfx_texture_create_checker_rgba8");

    g4f_clear_error();
    g4f_gfx_material* material0 = g4f_gfx_material_create_unlit(nullptr, nullptr);
    assert(material0 == nullptr);
    expectErrorContains("g4f_gfx_material_create_unlit");

    g4f_clear_error();
    g4f_gfx_mesh* mesh0 = g4f_gfx_mesh_create_p3n3uv2(nullptr, nullptr, 0, nullptr, 0);
    assert(mesh0 == nullptr);
    expectErrorContains("g4f_gfx_mesh_create_p3n3uv2");

    g4f_clear_error();
    int updated0 = g4f_gfx_texture_update_rgba8(nullptr, &pixel, 4);
    assert(updated0 == 0);
    expectErrorContains("g4f_gfx_texture_update_rgba8");

    // Window create validation.
    g4f_app_desc appDesc{};
    g4f_app* app = g4f_app_create(&appDesc);
    assert(app != nullptr);

    g4f_clear_error();
    g4f_window* w0 = g4f_window_create(app, nullptr);
    assert(w0 == nullptr);
    expectErrorContains("g4f_window_create");

    g4f_app_destroy(app);

    std::printf("error_tests: OK\n");
    return 0;
}
