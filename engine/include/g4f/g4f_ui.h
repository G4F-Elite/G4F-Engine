#pragma once

#include "g4f.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct g4f_ui g4f_ui;

typedef struct g4f_ui_theme {
    uint32_t panelBg;
    uint32_t panelBorder;
    uint32_t text;
    uint32_t textMuted;
    uint32_t itemBg;
    uint32_t itemHover;
    uint32_t itemActive;
    uint32_t accent;
} g4f_ui_theme;

typedef struct g4f_ui_layout {
    g4f_rect_f bounds;
    float cursorX;
    float cursorY;
    float padding;
    float spacing;
    float itemW;
    float defaultItemH;
} g4f_ui_layout;

g4f_ui* g4f_ui_create(void);
void g4f_ui_destroy(g4f_ui* ui);

void g4f_ui_set_theme(g4f_ui* ui, const g4f_ui_theme* theme);
g4f_ui_theme g4f_ui_theme_dark(void);

void g4f_ui_begin(g4f_ui* ui, g4f_renderer* renderer, const g4f_window* window);
void g4f_ui_end(g4f_ui* ui);

void g4f_ui_push_id(g4f_ui* ui, const char* id_utf8);
void g4f_ui_pop_id(g4f_ui* ui);

// Simple per-UI storage (keyed by current ID stack + key string).
int g4f_ui_store_get_i(g4f_ui* ui, const char* key_utf8, int defaultValue);
void g4f_ui_store_set_i(g4f_ui* ui, const char* key_utf8, int value);
float g4f_ui_store_get_f(g4f_ui* ui, const char* key_utf8, float defaultValue);
void g4f_ui_store_set_f(g4f_ui* ui, const char* key_utf8, float value);

// Layout
void g4f_ui_layout_begin(g4f_ui* ui, g4f_ui_layout layout);
g4f_rect_f g4f_ui_layout_next(g4f_ui* ui, float height);
void g4f_ui_layout_spacer(g4f_ui* ui, float height);

// Widgets (immediate-mode).
// Return 1 if activated/toggled/changed.
int g4f_ui_label(g4f_ui* ui, const char* text_utf8, float size_px);
void g4f_ui_panel_begin(g4f_ui* ui, const char* title_utf8, g4f_rect_f bounds);
void g4f_ui_panel_end(g4f_ui* ui);
void g4f_ui_panel_begin_scroll(g4f_ui* ui, const char* title_utf8, g4f_rect_f bounds);

// Disabled-state stack (use to grey out and block interactions).
void g4f_ui_disabled_begin(g4f_ui* ui, int disabled);
void g4f_ui_disabled_end(g4f_ui* ui);

int g4f_ui_button(g4f_ui* ui, const char* label_utf8);
int g4f_ui_checkbox(g4f_ui* ui, const char* label_utf8, int* value);
int g4f_ui_slider_float(g4f_ui* ui, const char* label_utf8, float* value, float minValue, float maxValue);
int g4f_ui_text_wrapped(g4f_ui* ui, const char* text_utf8, float size_px);
void g4f_ui_separator(g4f_ui* ui);

// Tooltip for the last submitted item (shows when hovered).
void g4f_ui_tooltip(g4f_ui* ui, const char* text_utf8, float size_px);

// Image helpers (bitmaps can be loaded from file or generated in code).
void g4f_ui_image(g4f_ui* ui, const g4f_bitmap* bitmap, float height, float opacity);
int g4f_ui_image_button(g4f_ui* ui, const char* label_utf8, const g4f_bitmap* bitmap, float height, float opacity);

// Keyed widgets (values persist inside g4f_ui store).
int g4f_ui_checkbox_k(g4f_ui* ui, const char* label_utf8, const char* key_utf8, int defaultValue, int* outValue);
int g4f_ui_slider_float_k(g4f_ui* ui, const char* label_utf8, const char* key_utf8, float defaultValue, float minValue, float maxValue, float* outValue);
int g4f_ui_input_text_k(g4f_ui* ui, const char* label_utf8, const char* key_utf8, const char* placeholder_utf8, int maxBytes, char* out_utf8, int out_cap);

#ifdef __cplusplus
} // extern "C"
#endif
