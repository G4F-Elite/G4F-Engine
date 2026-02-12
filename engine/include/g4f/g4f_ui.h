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

// Layout
void g4f_ui_layout_begin(g4f_ui* ui, g4f_ui_layout layout);
g4f_rect_f g4f_ui_layout_next(g4f_ui* ui, float height);
void g4f_ui_layout_spacer(g4f_ui* ui, float height);

// Widgets (immediate-mode).
// Return 1 if activated/toggled/changed.
int g4f_ui_label(g4f_ui* ui, const char* text_utf8, float size_px);
void g4f_ui_panel_begin(g4f_ui* ui, const char* title_utf8, g4f_rect_f bounds);
void g4f_ui_panel_end(g4f_ui* ui);
int g4f_ui_button(g4f_ui* ui, const char* label_utf8);
int g4f_ui_checkbox(g4f_ui* ui, const char* label_utf8, int* value);
int g4f_ui_slider_float(g4f_ui* ui, const char* label_utf8, float* value, float minValue, float maxValue);

#ifdef __cplusplus
} // extern "C"
#endif
