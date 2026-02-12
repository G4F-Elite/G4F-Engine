#include <cassert>
#include <cstdio>

#include "g4f/g4f.h"
#include "g4f/g4f_ui.h"

int main() {
    g4f_window_desc windowDesc{};
    windowDesc.title_utf8 = "g4f_ui_widget_smoke_tests";
    windowDesc.width = 420;
    windowDesc.height = 260;
    windowDesc.resizable = 0;

    g4f_ctx* ctx = g4f_ctx_create(&windowDesc);
    if (!ctx) {
        std::fprintf(stderr, "ui_widget_smoke_tests: g4f_ctx_create failed\n");
        return 1;
    }

    g4f_window* window = g4f_ctx_window(ctx);
    g4f_renderer* renderer = g4f_ctx_renderer(ctx);
    assert(window && renderer);

    g4f_ui* ui = g4f_ui_create();
    assert(ui != nullptr);

    g4f_ui_theme theme = g4f_ui_theme_dark();
    g4f_ui_set_theme(ui, &theme);

    const uint32_t pixels[4] = {0xFFFFFFFFu, 0xFF0000FFu, 0x00FF00FFu, 0x0000FFFFu};
    g4f_bitmap* bmp = g4f_bitmap_create_rgba8(renderer, 2, 2, pixels, 2 * 4);
    assert(bmp != nullptr);

    double start = g4f_ctx_time(ctx);
    int frames = 0;
    while (g4f_ctx_poll(ctx)) {
        if (g4f_ctx_time(ctx) - start > 0.35 || frames++ > 120) {
            g4f_window_request_close(window);
        }

        g4f_frame_begin(ctx, g4f_rgba_u32(12, 12, 14, 255));

        g4f_ui_begin(ui, renderer, window);

        g4f_ui_push_id(ui, "root");

        g4f_ui_panel_begin(ui, "Panel", g4f_rect_f{12, 12, 396, 236});
        g4f_ui_layout layout{};
        layout.bounds = g4f_rect_f{20, 40, 380, 200};
        layout.padding = 8.0f;
        layout.spacing = 6.0f;
        layout.defaultItemH = 22.0f;
        g4f_ui_layout_begin(ui, layout);

        (void)g4f_ui_label(ui, "Label", 16.0f);
        g4f_ui_separator(ui);

        int v = 1;
        (void)g4f_ui_checkbox(ui, "Checkbox", &v);

        float f = 0.5f;
        (void)g4f_ui_slider_float(ui, "Slider", &f, 0.0f, 1.0f);

        (void)g4f_ui_button(ui, "Button");
        g4f_ui_tooltip(ui, "Tooltip text", 14.0f);

        g4f_ui_disabled_begin(ui, 1);
        (void)g4f_ui_button(ui, "Disabled");
        g4f_ui_disabled_end(ui);

        (void)g4f_ui_text_wrapped(
            ui,
            "Some wrapped UI text to exercise text layout and clipping.",
            14.0f
        );

        g4f_ui_image(ui, bmp, 48.0f, 1.0f);
        (void)g4f_ui_image_button(ui, "ImgBtn", bmp, 48.0f, 1.0f);

        int outV = 0;
        (void)g4f_ui_checkbox_k(ui, "KeyedCheckbox", "cb", 1, &outV);
        float outF = 0.0f;
        (void)g4f_ui_slider_float_k(ui, "KeyedSlider", "sl", 0.25f, 0.0f, 1.0f, &outF);

        char buf[64]{};
        (void)g4f_ui_input_text_k(ui, "Input", "in", "placeholder", 32, buf, (int)sizeof(buf));

        g4f_ui_panel_end(ui);

        g4f_ui_pop_id(ui);

        g4f_ui_end(ui);

        g4f_frame_end(ctx);
    }

    g4f_bitmap_destroy(bmp);
    g4f_ui_destroy(ui);
    g4f_ctx_destroy(ctx);

    std::printf("ui_widget_smoke_tests: OK\n");
    return 0;
}

