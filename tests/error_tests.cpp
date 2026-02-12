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
