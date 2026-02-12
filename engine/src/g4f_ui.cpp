#include "../include/g4f/g4f_ui.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

static uint64_t fnv1a64(const void* data, size_t len) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hash = 1469598103934665603ull;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint64_t)bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint64_t hashString(uint64_t seed, const char* str) {
    if (!str) return seed;
    uint64_t h = fnv1a64(str, std::strlen(str));
    return seed ^ (h + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
}

static float clampFloat(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int pointInRect(float x, float y, g4f_rect_f r) {
    return x >= r.x && y >= r.y && x <= (r.x + r.w) && y <= (r.y + r.h);
}

static uint32_t blendAlpha(uint32_t rgba, uint8_t alphaMul) {
    uint8_t a = (uint8_t)(rgba & 0xFFu);
    uint32_t newA = (uint32_t)a * (uint32_t)alphaMul / 255u;
    return (rgba & 0xFFFFFF00u) | (newA & 0xFFu);
}

} // namespace

struct g4f_ui {
    g4f_renderer* renderer = nullptr;
    const g4f_window* window = nullptr;

    g4f_ui_theme theme{};
    g4f_ui_layout layout{};
    bool hasLayout = false;
    bool panelOpen = false;
    g4f_rect_f panelBounds{};
    g4f_rect_f panelInner{};

    std::vector<uint64_t> idStack;
    uint64_t frameSeed = 0;

    uint64_t hot = 0;
    uint64_t active = 0;
    uint64_t lastActive = 0;

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    int mouseDown = 0;
    int mousePressed = 0;
};

g4f_ui_theme g4f_ui_theme_dark(void) {
    g4f_ui_theme t{};
    t.panelBg = g4f_rgba_u32(20, 20, 26, 230);
    t.panelBorder = g4f_rgba_u32(70, 70, 90, 255);
    t.text = g4f_rgba_u32(245, 245, 255, 255);
    t.textMuted = g4f_rgba_u32(170, 170, 190, 255);
    t.itemBg = g4f_rgba_u32(30, 30, 38, 255);
    t.itemHover = g4f_rgba_u32(42, 42, 54, 255);
    t.itemActive = g4f_rgba_u32(52, 70, 140, 255);
    t.accent = g4f_rgba_u32(90, 120, 255, 255);
    return t;
}

g4f_ui* g4f_ui_create(void) {
    auto* ui = new g4f_ui();
    ui->theme = g4f_ui_theme_dark();
    ui->frameSeed = 0xC0DEF00DULL;
    ui->idStack.reserve(8);
    return ui;
}

void g4f_ui_destroy(g4f_ui* ui) {
    delete ui;
}

void g4f_ui_set_theme(g4f_ui* ui, const g4f_ui_theme* theme) {
    if (!ui || !theme) return;
    ui->theme = *theme;
}

void g4f_ui_begin(g4f_ui* ui, g4f_renderer* renderer, const g4f_window* window) {
    if (!ui) return;
    ui->renderer = renderer;
    ui->window = window;
    ui->hasLayout = false;

    ui->mouseX = window ? g4f_mouse_x(window) : 0.0f;
    ui->mouseY = window ? g4f_mouse_y(window) : 0.0f;
    ui->mouseDown = window ? g4f_mouse_down(window, G4F_MOUSE_BUTTON_LEFT) : 0;
    ui->mousePressed = window ? g4f_mouse_pressed(window, G4F_MOUSE_BUTTON_LEFT) : 0;

    ui->hot = 0;
    ui->lastActive = ui->active;
    if (!ui->mouseDown) ui->active = 0;
}

void g4f_ui_end(g4f_ui* ui) {
    if (!ui) return;
    ui->renderer = nullptr;
    ui->window = nullptr;
}

void g4f_ui_push_id(g4f_ui* ui, const char* id_utf8) {
    if (!ui) return;
    uint64_t seed = ui->idStack.empty() ? ui->frameSeed : ui->idStack.back();
    ui->idStack.push_back(hashString(seed, id_utf8));
}

void g4f_ui_pop_id(g4f_ui* ui) {
    if (!ui || ui->idStack.empty()) return;
    ui->idStack.pop_back();
}

void g4f_ui_layout_begin(g4f_ui* ui, g4f_ui_layout layout) {
    if (!ui) return;
    ui->layout = layout;
    ui->layout.cursorX = layout.bounds.x + layout.padding;
    ui->layout.cursorY = layout.bounds.y + layout.padding;
    ui->layout.itemW = (layout.itemW > 0.0f) ? layout.itemW : (layout.bounds.w - layout.padding * 2.0f);
    ui->hasLayout = true;
}

g4f_rect_f g4f_ui_layout_next(g4f_ui* ui, float height) {
    if (!ui || !ui->hasLayout) return g4f_rect_f{0, 0, 0, 0};
    float h = (height > 0.0f) ? height : ui->layout.defaultItemH;
    g4f_rect_f r{ui->layout.cursorX, ui->layout.cursorY, ui->layout.itemW, h};
    ui->layout.cursorY += h + ui->layout.spacing;
    return r;
}

void g4f_ui_layout_spacer(g4f_ui* ui, float height) {
    if (!ui || !ui->hasLayout) return;
    ui->layout.cursorY += height;
}

static uint64_t g4f_ui_make_id(g4f_ui* ui, const char* label) {
    uint64_t seed = ui->idStack.empty() ? ui->frameSeed : ui->idStack.back();
    return hashString(seed, label);
}

static void uiDrawItemBg(g4f_ui* ui, g4f_rect_f r, bool hovered, bool active) {
    uint32_t bg = ui->theme.itemBg;
    if (active) bg = ui->theme.itemActive;
    else if (hovered) bg = ui->theme.itemHover;
    g4f_draw_round_rect(ui->renderer, r, 10.0f, bg);
    g4f_draw_round_rect_outline(ui->renderer, r, 10.0f, 1.5f, ui->theme.panelBorder);
}

static int uiItemBehavior(g4f_ui* ui, uint64_t id, g4f_rect_f r) {
    int hovered = pointInRect(ui->mouseX, ui->mouseY, r);
    if (hovered) ui->hot = id;
    if (hovered && ui->mousePressed) ui->active = id;

    int clicked = 0;
    if (!ui->mouseDown && ui->lastActive == id && hovered) clicked = 1;
    return clicked;
}

int g4f_ui_label(g4f_ui* ui, const char* text_utf8, float size_px) {
    if (!ui || !ui->renderer || !text_utf8) return 0;
    g4f_rect_f r = g4f_ui_layout_next(ui, size_px + 10.0f);
    g4f_draw_text(ui->renderer, text_utf8, r.x, r.y, size_px, ui->theme.text);
    return 0;
}

void g4f_ui_panel_begin(g4f_ui* ui, const char* title_utf8, g4f_rect_f bounds) {
    if (!ui || !ui->renderer) return;
    ui->panelOpen = true;
    ui->panelBounds = bounds;

    const float radius = 14.0f;
    g4f_draw_round_rect(ui->renderer, bounds, radius, ui->theme.panelBg);
    g4f_draw_round_rect_outline(ui->renderer, bounds, radius, 2.0f, ui->theme.panelBorder);

    float innerPad = 16.0f;
    float titleH = (title_utf8 && title_utf8[0]) ? 34.0f : 0.0f;
    ui->panelInner = g4f_rect_f{
        bounds.x + innerPad,
        bounds.y + innerPad + titleH,
        bounds.w - innerPad * 2.0f,
        bounds.h - innerPad * 2.0f - titleH,
    };

    if (title_utf8 && title_utf8[0]) {
        g4f_draw_text(ui->renderer, title_utf8, bounds.x + innerPad, bounds.y + 10.0f, 18.0f, ui->theme.text);
    }

    g4f_clip_push(ui->renderer, ui->panelInner);
    g4f_ui_layout layout{};
    layout.bounds = ui->panelInner;
    layout.padding = 0.0f;
    layout.spacing = 10.0f;
    layout.itemW = ui->panelInner.w;
    layout.defaultItemH = 44.0f;
    g4f_ui_layout_begin(ui, layout);
}

void g4f_ui_panel_end(g4f_ui* ui) {
    if (!ui || !ui->renderer) return;
    if (!ui->panelOpen) return;
    g4f_clip_pop(ui->renderer);
    ui->panelOpen = false;
}

int g4f_ui_button(g4f_ui* ui, const char* label_utf8) {
    if (!ui || !ui->renderer || !label_utf8) return 0;
    g4f_rect_f r = g4f_ui_layout_next(ui, 0.0f);
    uint64_t id = g4f_ui_make_id(ui, label_utf8);
    int clicked = uiItemBehavior(ui, id, r);
    bool hovered = ui->hot == id;
    bool active = ui->active == id && ui->mouseDown;
    uiDrawItemBg(ui, r, hovered, active);
    g4f_draw_text(ui->renderer, label_utf8, r.x + 14.0f, r.y + 10.0f, 18.0f, ui->theme.text);
    return clicked;
}

int g4f_ui_checkbox(g4f_ui* ui, const char* label_utf8, int* value) {
    if (!ui || !ui->renderer || !label_utf8 || !value) return 0;
    g4f_rect_f r = g4f_ui_layout_next(ui, 0.0f);
    uint64_t id = g4f_ui_make_id(ui, label_utf8);
    int clicked = uiItemBehavior(ui, id, r);
    bool hovered = ui->hot == id;
    bool active = ui->active == id && ui->mouseDown;
    uiDrawItemBg(ui, r, hovered, active);

    g4f_rect_f box{r.x + 14.0f, r.y + 10.0f, 22.0f, 22.0f};
    g4f_draw_round_rect(ui->renderer, box, 6.0f, blendAlpha(ui->theme.panelBg, 255));
    g4f_draw_round_rect_outline(ui->renderer, box, 6.0f, 2.0f, ui->theme.accent);
    if (*value) {
        g4f_draw_rect(ui->renderer, g4f_rect_f{box.x + 5.0f, box.y + 5.0f, box.w - 10.0f, box.h - 10.0f}, ui->theme.accent);
    }

    g4f_draw_text(ui->renderer, label_utf8, r.x + 48.0f, r.y + 10.0f, 18.0f, ui->theme.text);

    if (clicked) {
        *value = (*value) ? 0 : 1;
        return 1;
    }
    return 0;
}

int g4f_ui_slider_float(g4f_ui* ui, const char* label_utf8, float* value, float minValue, float maxValue) {
    if (!ui || !ui->renderer || !label_utf8 || !value) return 0;
    g4f_rect_f r = g4f_ui_layout_next(ui, 0.0f);
    uint64_t id = g4f_ui_make_id(ui, label_utf8);

    int hovered = pointInRect(ui->mouseX, ui->mouseY, r);
    if (hovered) ui->hot = id;
    if (hovered && ui->mousePressed) ui->active = id;

    bool isActive = ui->active == id && ui->mouseDown;
    bool isHovered = ui->hot == id;
    uiDrawItemBg(ui, r, isHovered, isActive);

    g4f_rect_f track{r.x + 14.0f, r.y + 32.0f, r.w - 28.0f, 6.0f};
    g4f_draw_round_rect(ui->renderer, track, 3.0f, blendAlpha(ui->theme.panelBorder, 255));

    float t = 0.0f;
    if (maxValue > minValue) t = (*value - minValue) / (maxValue - minValue);
    t = clampFloat(t, 0.0f, 1.0f);

    if (isActive && maxValue > minValue) {
        float px = clampFloat((ui->mouseX - track.x) / track.w, 0.0f, 1.0f);
        float newValue = minValue + px * (maxValue - minValue);
        if (newValue != *value) {
            *value = newValue;
            t = px;
        }
    }

    g4f_rect_f fill{track.x, track.y, track.w * t, track.h};
    g4f_draw_round_rect(ui->renderer, fill, 3.0f, ui->theme.accent);

    float knobX = track.x + track.w * t;
    g4f_draw_round_rect(ui->renderer, g4f_rect_f{knobX - 7.0f, track.y - 6.0f, 14.0f, 18.0f}, 7.0f, ui->theme.accent);

    g4f_draw_text(ui->renderer, label_utf8, r.x + 14.0f, r.y + 8.0f, 16.0f, ui->theme.text);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f", (double)*value);
    float tw = 0.0f, th = 0.0f;
    g4f_measure_text(ui->renderer, buf, 16.0f, &tw, &th);
    g4f_draw_text(ui->renderer, buf, r.x + r.w - 14.0f - tw, r.y + 8.0f, 16.0f, ui->theme.textMuted);

    return isActive ? 1 : 0;
}
