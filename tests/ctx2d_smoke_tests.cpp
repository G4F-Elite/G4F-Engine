#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

#include "g4f/g4f.h"

static bool isFiniteMat4(g4f_mat4 m) {
    for (float v : m.m) {
        if (!std::isfinite(v)) return false;
    }
    return true;
}

int main() {
    g4f_window_desc windowDesc{};
    windowDesc.title_utf8 = "g4f_ctx2d_smoke_tests";
    windowDesc.width = 360;
    windowDesc.height = 220;
    windowDesc.resizable = 0;

    g4f_ctx* ctx = g4f_ctx_create(&windowDesc);
    if (!ctx) {
        std::fprintf(stderr, "ctx2d_smoke_tests: g4f_ctx_create failed\n");
        return 1;
    }

    g4f_window* window = g4f_ctx_window(ctx);
    g4f_renderer* renderer = g4f_ctx_renderer(ctx);
    assert(window && renderer);

    // Create a tiny bitmap (in-memory) to exercise bitmap APIs.
    const int bmpW = 2;
    const int bmpH = 2;
    const uint32_t pixels[bmpW * bmpH] = {
        0xFFFFFFFFu, 0xFF0000FFu,
        0x00FF00FFu, 0x0000FFFFu,
    };
    g4f_bitmap* bmp = g4f_bitmap_create_rgba8(renderer, bmpW, bmpH, pixels, bmpW * 4);
    assert(bmp != nullptr);

    int gotW = 0, gotH = 0;
    g4f_bitmap_get_size(bmp, &gotW, &gotH);
    assert(gotW == bmpW && gotH == bmpH);

    g4f_window_set_title(window, "g4f_ctx2d_smoke_tests (running)");

    double start = g4f_ctx_time(ctx);
    int frames = 0;
    while (g4f_ctx_poll(ctx)) {
        if (g4f_ctx_time(ctx) - start > 0.35 || frames++ > 120) {
            g4f_window_request_close(window);
        }

        // Exercise input getters (no interaction required).
        (void)g4f_key_down(window, G4F_KEY_ESCAPE);
        (void)g4f_key_pressed(window, G4F_KEY_ESCAPE);
        (void)g4f_mouse_down(window, G4F_MOUSE_BUTTON_LEFT);
        (void)g4f_mouse_pressed(window, G4F_MOUSE_BUTTON_LEFT);
        (void)g4f_mouse_x(window);
        (void)g4f_mouse_y(window);
        (void)g4f_mouse_dx(window);
        (void)g4f_mouse_dy(window);
        (void)g4f_mouse_wheel_delta(window);
        (void)g4f_text_input_count(window);
        (void)g4f_window_focused(window);

        // Cursor toggles should be robust even without focus.
        g4f_window_set_cursor_visible(window, 1);
        g4f_window_set_cursor_captured(window, 0);
        (void)g4f_window_cursor_visible(window);
        (void)g4f_window_cursor_captured(window);

        // Basic math helpers used everywhere.
        g4f_mat4 I = g4f_mat4_identity();
        g4f_mat4 P = g4f_mat4_perspective(70.0f * 3.14159265f / 180.0f, 1.0f, 0.1f, 100.0f);
        g4f_mat4 M = g4f_mat4_mul(I, P);
        assert(isFiniteMat4(M));

        g4f_frame_begin(ctx, g4f_rgba_u32(14, 14, 18, 255));

        g4f_renderer_clear(renderer, g4f_rgba_u32(14, 14, 18, 255));
        g4f_draw_rect(renderer, g4f_rect_f{10, 10, 120, 60}, g4f_rgba_u32(40, 50, 70, 255));
        g4f_draw_rect_outline(renderer, g4f_rect_f{10, 10, 120, 60}, 2.0f, g4f_rgba_u32(220, 220, 220, 255));
        g4f_draw_round_rect(renderer, g4f_rect_f{140, 10, 120, 60}, 10.0f, g4f_rgba_u32(60, 80, 120, 255));
        g4f_draw_round_rect_outline(renderer, g4f_rect_f{140, 10, 120, 60}, 10.0f, 2.0f, g4f_rgba_u32(255, 255, 255, 255));
        g4f_draw_line(renderer, 10, 80, 260, 80, 2.0f, g4f_rgba_u32(200, 180, 80, 255));

        float tw = 0.0f, th = 0.0f;
        g4f_measure_text(renderer, "hello", 16.0f, &tw, &th);
        assert(std::isfinite(tw) && std::isfinite(th));
        g4f_draw_text(renderer, "hello g4f", 10.0f, 90.0f, 16.0f, g4f_rgba_u32(240, 240, 240, 255));

        g4f_rect_f clip{10, 120, 250, 70};
        g4f_clip_push(renderer, clip);
        g4f_draw_text_wrapped(
            renderer,
            "wrapped text wrapped text wrapped text wrapped text wrapped text",
            g4f_rect_f{12, 122, 246, 66},
            14.0f,
            g4f_rgba_u32(230, 230, 240, 255)
        );
        g4f_draw_bitmap(renderer, bmp, g4f_rect_f{220, 125, 32, 32}, 1.0f);
        g4f_clip_pop(renderer);

        g4f_frame_end(ctx);
    }

    g4f_bitmap_destroy(bmp);
    g4f_ctx_destroy(ctx);
    std::printf("ctx2d_smoke_tests: OK\n");
    return 0;
}

