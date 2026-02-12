#include "../include/g4f/g4f_ui.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
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
    uint64_t focus = 0;

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float wheelDelta = 0.0f;
    int mouseDown = 0;
    int mousePressed = 0;

    int navDir = 0;      // -1 up, +1 down
    int navActivate = 0; // enter/space
    int navLeft = 0;
    int navRight = 0;
    int navTabDir = 0; // -1 shift+tab, +1 tab
    std::vector<uint64_t> navOrder;

    std::unordered_map<uint64_t, int> storeInt;
    std::unordered_map<uint64_t, float> storeFloat;
    std::unordered_map<uint64_t, std::string> storeString;

    uint64_t textActive = 0;

    struct ScrollState {
        uint64_t id;
        float scrollY;
        float maxY;
    };
    std::vector<ScrollState> scrollStates;
    ScrollState* currentScroll = nullptr;
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
    ui->scrollStates.reserve(8);
    ui->storeInt.reserve(64);
    ui->storeFloat.reserve(64);
    ui->storeString.reserve(32);
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
    ui->wheelDelta = window ? g4f_mouse_wheel_delta(window) : 0.0f;
    ui->mouseDown = window ? g4f_mouse_down(window, G4F_MOUSE_BUTTON_LEFT) : 0;
    ui->mousePressed = window ? g4f_mouse_pressed(window, G4F_MOUSE_BUTTON_LEFT) : 0;

    ui->navDir = 0;
    ui->navActivate = 0;
    ui->navLeft = 0;
    ui->navRight = 0;
    ui->navTabDir = 0;
    ui->navOrder.clear();

    if (window) {
        int up = g4f_key_pressed(window, G4F_KEY_UP) || g4f_key_pressed(window, G4F_KEY_W);
        int down = g4f_key_pressed(window, G4F_KEY_DOWN) || g4f_key_pressed(window, G4F_KEY_S);
        if (up && !down) ui->navDir = -1;
        if (down && !up) ui->navDir = +1;
        ui->navActivate = g4f_key_pressed(window, G4F_KEY_ENTER) || g4f_key_pressed(window, G4F_KEY_SPACE);
        ui->navLeft = g4f_key_pressed(window, G4F_KEY_LEFT) || g4f_key_pressed(window, G4F_KEY_A);
        ui->navRight = g4f_key_pressed(window, G4F_KEY_RIGHT) || g4f_key_pressed(window, G4F_KEY_D);

        int tab = g4f_key_pressed(window, G4F_KEY_TAB);
        if (tab) {
            int shift = g4f_key_down(window, G4F_KEY_LEFT_SHIFT) || g4f_key_down(window, G4F_KEY_RIGHT_SHIFT);
            ui->navTabDir = shift ? -1 : +1;
        }
    }

    ui->hot = 0;
    ui->lastActive = ui->active;
    if (!ui->mouseDown) ui->active = 0;
}

void g4f_ui_end(g4f_ui* ui) {
    if (!ui) return;

    if (ui->textActive != 0 && ui->mousePressed && ui->focus != ui->textActive) {
        ui->textActive = 0;
    }

    // Apply keyboard navigation after all widgets registered.
    if (!ui->navOrder.empty()) {
        if (ui->focus == 0) ui->focus = ui->navOrder.front();
        if (ui->navDir != 0 && ui->textActive == 0) {
            int n = (int)ui->navOrder.size();
            int idx = -1;
            for (int i = 0; i < n; i++) {
                if (ui->navOrder[(size_t)i] == ui->focus) { idx = i; break; }
            }
            if (idx < 0) ui->focus = ui->navOrder.front();
            else {
                int next = (idx + ui->navDir + n) % n;
                ui->focus = ui->navOrder[(size_t)next];
            }
        }

        if (ui->navTabDir != 0) {
            int n = (int)ui->navOrder.size();
            int idx = -1;
            for (int i = 0; i < n; i++) {
                if (ui->navOrder[(size_t)i] == ui->focus) { idx = i; break; }
            }
            if (idx < 0) ui->focus = ui->navOrder.front();
            else {
                int next = (idx + ui->navTabDir + n) % n;
                ui->focus = ui->navOrder[(size_t)next];
            }
            ui->textActive = 0;
        }
    }

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
    float scrollY = ui->currentScroll ? ui->currentScroll->scrollY : 0.0f;
    g4f_rect_f r{ui->layout.cursorX, ui->layout.cursorY - scrollY, ui->layout.itemW, h};
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

int g4f_ui_store_get_i(g4f_ui* ui, const char* key_utf8, int defaultValue) {
    if (!ui || !key_utf8) return defaultValue;
    uint64_t id = g4f_ui_make_id(ui, key_utf8);
    auto it = ui->storeInt.find(id);
    if (it == ui->storeInt.end()) {
        ui->storeInt.emplace(id, defaultValue);
        return defaultValue;
    }
    return it->second;
}

void g4f_ui_store_set_i(g4f_ui* ui, const char* key_utf8, int value) {
    if (!ui || !key_utf8) return;
    uint64_t id = g4f_ui_make_id(ui, key_utf8);
    ui->storeInt[id] = value;
}

float g4f_ui_store_get_f(g4f_ui* ui, const char* key_utf8, float defaultValue) {
    if (!ui || !key_utf8) return defaultValue;
    uint64_t id = g4f_ui_make_id(ui, key_utf8);
    auto it = ui->storeFloat.find(id);
    if (it == ui->storeFloat.end()) {
        ui->storeFloat.emplace(id, defaultValue);
        return defaultValue;
    }
    return it->second;
}

void g4f_ui_store_set_f(g4f_ui* ui, const char* key_utf8, float value) {
    if (!ui || !key_utf8) return;
    uint64_t id = g4f_ui_make_id(ui, key_utf8);
    ui->storeFloat[id] = value;
}

static std::string& uiStoreStringRef(g4f_ui* ui, const char* key_utf8, const char* defaultValue) {
    uint64_t id = g4f_ui_make_id(ui, key_utf8);
    auto it = ui->storeString.find(id);
    if (it == ui->storeString.end()) {
        auto [insertIt, inserted] = ui->storeString.emplace(id, defaultValue ? defaultValue : "");
        return insertIt->second;
    }
    return it->second;
}

static size_t uiUtf8PrevBoundary(const std::string& s, size_t pos) {
    if (pos == 0) return 0;
    size_t i = (pos > s.size()) ? s.size() : pos;
    i--;
    while (i > 0 && ((uint8_t)s[i] & 0xC0u) == 0x80u) i--;
    return i;
}

static size_t uiUtf8NextBoundary(const std::string& s, size_t pos) {
    size_t i = (pos > s.size()) ? s.size() : pos;
    if (i >= s.size()) return s.size();
    i++;
    while (i < s.size() && ((uint8_t)s[i] & 0xC0u) == 0x80u) i++;
    return i;
}

static size_t uiUtf8ClampBoundary(const std::string& s, size_t pos) {
    size_t i = (pos > s.size()) ? s.size() : pos;
    while (i > 0 && i < s.size() && ((uint8_t)s[i] & 0xC0u) == 0x80u) i--;
    return i;
}

static void uiEraseRange(std::string& s, size_t a, size_t b) {
    if (a > b) std::swap(a, b);
    if (a > s.size()) a = s.size();
    if (b > s.size()) b = s.size();
    if (a == b) return;
    s.erase(a, b - a);
}

static size_t uiFindPrevWordBoundary(const std::string& s, size_t caret) {
    size_t i = caret;
    while (i > 0) {
        size_t prev = uiUtf8PrevBoundary(s, i);
        unsigned char ch = (unsigned char)s[prev];
        if (ch > ' ') break;
        i = prev;
    }
    while (i > 0) {
        size_t prev = uiUtf8PrevBoundary(s, i);
        unsigned char ch = (unsigned char)s[prev];
        if (ch <= ' ') break;
        i = prev;
    }
    return i;
}

static size_t uiFindNextWordBoundary(const std::string& s, size_t caret) {
    size_t i = caret;
    while (i < s.size()) {
        size_t next = uiUtf8NextBoundary(s, i);
        unsigned char ch = (unsigned char)s[i];
        if (ch > ' ') break;
        i = next;
    }
    while (i < s.size()) {
        size_t next = uiUtf8NextBoundary(s, i);
        unsigned char ch = (unsigned char)s[i];
        if (ch <= ' ') break;
        i = next;
    }
    return i;
}

static void uiUtf8AppendCodepoint(std::string& s, uint32_t cp) {
    if (cp <= 0x7Fu) {
        s.push_back((char)cp);
        return;
    }
    if (cp <= 0x7FFu) {
        s.push_back((char)(0xC0u | ((cp >> 6) & 0x1Fu)));
        s.push_back((char)(0x80u | (cp & 0x3Fu)));
        return;
    }
    if (cp <= 0xFFFFu) {
        s.push_back((char)(0xE0u | ((cp >> 12) & 0x0Fu)));
        s.push_back((char)(0x80u | ((cp >> 6) & 0x3Fu)));
        s.push_back((char)(0x80u | (cp & 0x3Fu)));
        return;
    }
    s.push_back((char)(0xF0u | ((cp >> 18) & 0x07u)));
    s.push_back((char)(0x80u | ((cp >> 12) & 0x3Fu)));
    s.push_back((char)(0x80u | ((cp >> 6) & 0x3Fu)));
    s.push_back((char)(0x80u | (cp & 0x3Fu)));
}

static void uiUtf8TrimToMaxBytes(std::string& s, int maxBytes) {
    if (maxBytes < 0) return;
    if ((int)s.size() <= maxBytes) return;
    size_t end = (size_t)maxBytes;
    while (end > 0 && ((uint8_t)s[end] & 0xC0u) == 0x80u) end--;
    s.resize(end);
}

static std::vector<size_t> uiUtf8Boundaries(const std::string& s) {
    std::vector<size_t> boundaries;
    boundaries.reserve(s.size() + 1);
    boundaries.push_back(0);
    size_t pos = 0;
    while (pos < s.size()) {
        pos = uiUtf8NextBoundary(s, pos);
        boundaries.push_back(pos);
    }
    if (boundaries.empty() || boundaries.back() != s.size()) boundaries.push_back(s.size());
    return boundaries;
}

static size_t uiCaretFromX(g4f_renderer* renderer, const std::string& s, float fontSizePx, float x) {
    if (!renderer) return 0;
    if (s.empty()) return 0;
    if (x <= 0.0f) return 0;

    float fullW = 0.0f, fullH = 0.0f;
    g4f_measure_text(renderer, s.c_str(), fontSizePx, &fullW, &fullH);
    if (x >= fullW) return s.size();

    std::vector<size_t> boundaries = uiUtf8Boundaries(s);
    if (boundaries.size() <= 1) return 0;

    auto widthAt = [&](size_t boundary) -> float {
        if (boundary == 0) return 0.0f;
        std::string left = s.substr(0, boundary);
        float w = 0.0f, h = 0.0f;
        g4f_measure_text(renderer, left.c_str(), fontSizePx, &w, &h);
        return w;
    };

    int lo = 0;
    int hi = (int)boundaries.size() - 1;
    while (hi - lo > 1) {
        int mid = (lo + hi) / 2;
        float w = widthAt(boundaries[(size_t)mid]);
        if (w < x) lo = mid;
        else hi = mid;
    }

    float wLo = widthAt(boundaries[(size_t)lo]);
    float wHi = widthAt(boundaries[(size_t)hi]);
    if ((x - wLo) <= (wHi - x)) return boundaries[(size_t)lo];
    return boundaries[(size_t)hi];
}

static uint64_t uiDeriveId(uint64_t base, uint64_t salt) {
    return base ^ (salt + 0x9e3779b97f4a7c15ull + (base << 6) + (base >> 2));
}

static g4f_ui::ScrollState* uiFindOrCreateScroll(g4f_ui* ui, uint64_t id) {
    for (auto& s : ui->scrollStates) {
        if (s.id == id) return &s;
    }
    ui->scrollStates.push_back(g4f_ui::ScrollState{id, 0.0f, 0.0f});
    return &ui->scrollStates.back();
}

static void uiDrawItemBg(g4f_ui* ui, g4f_rect_f r, bool hovered, bool active, bool focused) {
    uint32_t bg = ui->theme.itemBg;
    if (active) bg = ui->theme.itemActive;
    else if (hovered) bg = ui->theme.itemHover;
    g4f_draw_round_rect(ui->renderer, r, 10.0f, bg);
    uint32_t border = focused ? ui->theme.accent : ui->theme.panelBorder;
    g4f_draw_round_rect_outline(ui->renderer, r, 10.0f, 1.5f, border);
}

static int uiItemBehavior(g4f_ui* ui, uint64_t id, g4f_rect_f r) {
    int hovered = pointInRect(ui->mouseX, ui->mouseY, r);
    if (hovered) ui->hot = id;
    if (hovered && ui->mousePressed) { ui->active = id; ui->focus = id; }

    int clicked = 0;
    if (!ui->mouseDown && ui->lastActive == id && hovered) clicked = 1;
    return clicked;
}

static void uiRegisterFocusable(g4f_ui* ui, uint64_t id) {
    ui->navOrder.push_back(id);
    if (ui->focus == 0) ui->focus = id;
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

void g4f_ui_panel_begin_scroll(g4f_ui* ui, const char* title_utf8, g4f_rect_f bounds) {
    if (!ui) return;
    uint64_t id = g4f_ui_make_id(ui, title_utf8 ? title_utf8 : "panel");
    ui->currentScroll = uiFindOrCreateScroll(ui, id);

    g4f_ui_panel_begin(ui, title_utf8, bounds);

    if (ui->currentScroll && ui->wheelDelta != 0.0f && pointInRect(ui->mouseX, ui->mouseY, ui->panelInner)) {
        const float scrollSpeed = 34.0f;
        ui->currentScroll->scrollY = ui->currentScroll->scrollY - ui->wheelDelta * scrollSpeed;
        if (ui->currentScroll->scrollY < 0.0f) ui->currentScroll->scrollY = 0.0f;
        if (ui->currentScroll->scrollY > ui->currentScroll->maxY) ui->currentScroll->scrollY = ui->currentScroll->maxY;
    }
}

void g4f_ui_panel_end(g4f_ui* ui) {
    if (!ui || !ui->renderer) return;
    if (!ui->panelOpen) return;
    if (ui->currentScroll) {
        float contentH = ui->layout.cursorY - ui->panelInner.y;
        if (contentH > 0.0f) contentH -= ui->layout.spacing;
        ui->currentScroll->maxY = contentH > ui->panelInner.h ? (contentH - ui->panelInner.h) : 0.0f;
        if (ui->currentScroll->scrollY < 0.0f) ui->currentScroll->scrollY = 0.0f;
        if (ui->currentScroll->scrollY > ui->currentScroll->maxY) ui->currentScroll->scrollY = ui->currentScroll->maxY;
        ui->currentScroll = nullptr;
    }
    g4f_clip_pop(ui->renderer);
    ui->panelOpen = false;
}

int g4f_ui_button(g4f_ui* ui, const char* label_utf8) {
    if (!ui || !ui->renderer || !label_utf8) return 0;
    g4f_rect_f r = g4f_ui_layout_next(ui, 0.0f);
    uint64_t id = g4f_ui_make_id(ui, label_utf8);
    uiRegisterFocusable(ui, id);
    int clicked = uiItemBehavior(ui, id, r);
    bool hovered = ui->hot == id;
    bool active = ui->active == id && ui->mouseDown;
    bool focused = ui->focus == id;
    uiDrawItemBg(ui, r, hovered, active, focused);
    g4f_draw_text(ui->renderer, label_utf8, r.x + 14.0f, r.y + 10.0f, 18.0f, ui->theme.text);
    if (focused && ui->navActivate) clicked = 1;
    return clicked;
}

int g4f_ui_checkbox(g4f_ui* ui, const char* label_utf8, int* value) {
    if (!ui || !ui->renderer || !label_utf8 || !value) return 0;
    g4f_rect_f r = g4f_ui_layout_next(ui, 0.0f);
    uint64_t id = g4f_ui_make_id(ui, label_utf8);
    uiRegisterFocusable(ui, id);
    int clicked = uiItemBehavior(ui, id, r);
    bool hovered = ui->hot == id;
    bool active = ui->active == id && ui->mouseDown;
    bool focused = ui->focus == id;
    uiDrawItemBg(ui, r, hovered, active, focused);

    g4f_rect_f box{r.x + 14.0f, r.y + 10.0f, 22.0f, 22.0f};
    g4f_draw_round_rect(ui->renderer, box, 6.0f, blendAlpha(ui->theme.panelBg, 255));
    g4f_draw_round_rect_outline(ui->renderer, box, 6.0f, 2.0f, ui->theme.accent);
    if (*value) {
        g4f_draw_rect(ui->renderer, g4f_rect_f{box.x + 5.0f, box.y + 5.0f, box.w - 10.0f, box.h - 10.0f}, ui->theme.accent);
    }

    g4f_draw_text(ui->renderer, label_utf8, r.x + 48.0f, r.y + 10.0f, 18.0f, ui->theme.text);

    if (focused && ui->navActivate) clicked = 1;
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
    uiRegisterFocusable(ui, id);

    int hovered = pointInRect(ui->mouseX, ui->mouseY, r);
    if (hovered) ui->hot = id;
    if (hovered && ui->mousePressed) { ui->active = id; ui->focus = id; }

    bool isActive = ui->active == id && ui->mouseDown;
    bool isHovered = ui->hot == id;
    bool focused = ui->focus == id;
    uiDrawItemBg(ui, r, isHovered, isActive, focused);

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

    if (focused && maxValue > minValue) {
        float step = 0.05f;
        if (ui->window) {
            int shift = g4f_key_down(ui->window, G4F_KEY_LEFT_SHIFT) || g4f_key_down(ui->window, G4F_KEY_RIGHT_SHIFT);
            if (shift) step = 0.01f;
        }
        if (ui->navLeft) *value = clampFloat(*value - step * (maxValue - minValue), minValue, maxValue);
        if (ui->navRight) *value = clampFloat(*value + step * (maxValue - minValue), minValue, maxValue);
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

int g4f_ui_checkbox_k(g4f_ui* ui, const char* label_utf8, const char* key_utf8, int defaultValue, int* outValue) {
    if (!ui || !label_utf8 || !key_utf8) return 0;
    int v = g4f_ui_store_get_i(ui, key_utf8, defaultValue);
    int changed = g4f_ui_checkbox(ui, label_utf8, &v);
    if (changed) g4f_ui_store_set_i(ui, key_utf8, v);
    if (outValue) *outValue = v;
    return changed;
}

int g4f_ui_slider_float_k(g4f_ui* ui, const char* label_utf8, const char* key_utf8, float defaultValue, float minValue, float maxValue, float* outValue) {
    if (!ui || !label_utf8 || !key_utf8) return 0;
    float v = g4f_ui_store_get_f(ui, key_utf8, defaultValue);
    int changed = g4f_ui_slider_float(ui, label_utf8, &v, minValue, maxValue);
    if (changed) g4f_ui_store_set_f(ui, key_utf8, v);
    if (outValue) *outValue = v;
    return changed;
}

int g4f_ui_input_text_k(g4f_ui* ui, const char* label_utf8, const char* key_utf8, const char* placeholder_utf8, int maxBytes, char* out_utf8, int out_cap) {
    if (out_utf8 && out_cap > 0) out_utf8[0] = '\0';
    if (!ui || !ui->renderer || !label_utf8 || !key_utf8) return 0;
    if (maxBytes < 1) maxBytes = 1;

    std::string& value = uiStoreStringRef(ui, key_utf8, "");
    if ((int)value.size() > maxBytes) value.resize((size_t)maxBytes);

    g4f_rect_f r = g4f_ui_layout_next(ui, 0.0f);
    uint64_t id = g4f_ui_make_id(ui, key_utf8);
    uiRegisterFocusable(ui, id);

    int clicked = uiItemBehavior(ui, id, r);
    bool focused = ui->focus == id;
    bool active = ui->textActive == id;
    if (clicked) ui->textActive = id;
    if (focused && ui->navActivate) ui->textActive = id;
    if (!focused && ui->navActivate && ui->textActive == id) ui->textActive = 0;
    if (ui->window && g4f_key_pressed(ui->window, G4F_KEY_ESCAPE) && ui->textActive == id) ui->textActive = 0;

    bool hovered = ui->hot == id;
    uiDrawItemBg(ui, r, hovered, ui->active == id && ui->mouseDown, focused || active);

    // Label
    g4f_draw_text(ui->renderer, label_utf8, r.x + 14.0f, r.y + 8.0f, 16.0f, ui->theme.text);

    // Input box area
    g4f_rect_f box{r.x + 14.0f, r.y + 28.0f, r.w - 28.0f, 14.0f};

    int changed = 0;
    uint64_t caretKey = uiDeriveId(id, 0xCACE7710u);
    uint64_t anchorKey = uiDeriveId(id, 0xA11C0B1Eu);
    uint64_t dragKey = uiDeriveId(id, 0xD2A611A5u);
    // Caret/selection are stored as UTF-8 byte indices.
    auto itCaret = ui->storeInt.find(caretKey);
    auto itAnchor = ui->storeInt.find(anchorKey);
    if (itCaret == ui->storeInt.end()) ui->storeInt.emplace(caretKey, (int)value.size());
    if (itAnchor == ui->storeInt.end()) ui->storeInt.emplace(anchorKey, (int)value.size());
    if (ui->storeInt.find(dragKey) == ui->storeInt.end()) ui->storeInt.emplace(dragKey, 0);
    itCaret = ui->storeInt.find(caretKey);
    itAnchor = ui->storeInt.find(anchorKey);
    size_t caret = itCaret != ui->storeInt.end() ? uiUtf8ClampBoundary(value, (size_t)itCaret->second) : value.size();
    size_t anchor = itAnchor != ui->storeInt.end() ? uiUtf8ClampBoundary(value, (size_t)itAnchor->second) : caret;

    if (!ui->mouseDown) ui->storeInt[dragKey] = 0;

    // Mouse caret + drag selection (inside the input box).
    if (ui->window && ui->mousePressed) {
        g4f_rect_f boxHit{box.x, box.y - 4.0f, box.w, 22.0f};
        if (pointInRect(ui->mouseX, ui->mouseY, boxHit)) {
            int shift = g4f_key_down(ui->window, G4F_KEY_LEFT_SHIFT) || g4f_key_down(ui->window, G4F_KEY_RIGHT_SHIFT);
            ui->textActive = id;
            ui->storeInt[dragKey] = 1;
            float localX = ui->mouseX - (box.x + 6.0f);
            caret = uiCaretFromX(ui->renderer, value, 16.0f, localX);
            if (!shift) anchor = caret;
        }
    }

    if (ui->window && ui->textActive == id && ui->storeInt[dragKey] && ui->active == id && ui->mouseDown) {
        float localX = ui->mouseX - (box.x + 6.0f);
        caret = uiCaretFromX(ui->renderer, value, 16.0f, localX);
    }

    active = ui->textActive == id;

    g4f_draw_round_rect(ui->renderer, g4f_rect_f{box.x, box.y - 4.0f, box.w, 22.0f}, 8.0f, blendAlpha(ui->theme.panelBg, 255));
    g4f_draw_round_rect_outline(ui->renderer, g4f_rect_f{box.x, box.y - 4.0f, box.w, 22.0f}, 8.0f, 1.5f, (active ? ui->theme.accent : ui->theme.panelBorder));

    if (active && ui->window) {
        int ctrl = g4f_key_down(ui->window, G4F_KEY_LEFT_CONTROL) || g4f_key_down(ui->window, G4F_KEY_RIGHT_CONTROL);
        int shift = g4f_key_down(ui->window, G4F_KEY_LEFT_SHIFT) || g4f_key_down(ui->window, G4F_KEY_RIGHT_SHIFT);

        bool hasSelection = anchor != caret;
        size_t selA = hasSelection ? std::min(anchor, caret) : caret;
        size_t selB = hasSelection ? std::max(anchor, caret) : caret;

        if (ctrl && g4f_key_pressed(ui->window, G4F_KEY_A)) {
            anchor = 0;
            caret = value.size();
        }

        if (g4f_key_pressed(ui->window, G4F_KEY_HOME)) {
            caret = 0;
            if (!shift) anchor = caret;
        }
        if (g4f_key_pressed(ui->window, G4F_KEY_END)) {
            caret = value.size();
            if (!shift) anchor = caret;
        }

        if (g4f_key_pressed(ui->window, G4F_KEY_LEFT)) {
            caret = ctrl ? uiFindPrevWordBoundary(value, caret) : uiUtf8PrevBoundary(value, caret);
            if (!shift) anchor = caret;
        }
        if (g4f_key_pressed(ui->window, G4F_KEY_RIGHT)) {
            caret = ctrl ? uiFindNextWordBoundary(value, caret) : uiUtf8NextBoundary(value, caret);
            if (!shift) anchor = caret;
        }

        if (ctrl && g4f_key_pressed(ui->window, G4F_KEY_C)) {
            if (hasSelection) {
                std::string slice = value.substr(selA, selB - selA);
                g4f_clipboard_set_utf8(ui->window, slice.c_str());
            } else {
                g4f_clipboard_set_utf8(ui->window, value.c_str());
            }
        }
        if (ctrl && g4f_key_pressed(ui->window, G4F_KEY_X)) {
            if (hasSelection) {
                std::string slice = value.substr(selA, selB - selA);
                g4f_clipboard_set_utf8(ui->window, slice.c_str());
                uiEraseRange(value, selA, selB);
                caret = selA;
                anchor = caret;
                changed = 1;
            } else {
                g4f_clipboard_set_utf8(ui->window, value.c_str());
                if (!value.empty()) { value.clear(); caret = 0; anchor = 0; changed = 1; }
            }
        }
        if (ctrl && g4f_key_pressed(ui->window, G4F_KEY_V)) {
            char clip[2048];
            int got = g4f_clipboard_get_utf8(ui->window, clip, (int)sizeof(clip));
            if (got > 0) {
                if (hasSelection) {
                    uiEraseRange(value, selA, selB);
                    caret = selA;
                    anchor = caret;
                }
                value.insert(caret, clip, (size_t)got);
                caret += (size_t)got;
                anchor = caret;
                uiUtf8TrimToMaxBytes(value, maxBytes);
                caret = uiUtf8ClampBoundary(value, caret);
                anchor = caret;
                changed = 1;
            }
        }

        if (g4f_key_pressed(ui->window, G4F_KEY_ENTER)) {
            ui->textActive = 0;
        }
        if (g4f_key_pressed(ui->window, G4F_KEY_DELETE)) {
            if (hasSelection) {
                uiEraseRange(value, selA, selB);
                caret = selA;
                anchor = caret;
                changed = 1;
            } else if (caret < value.size()) {
                size_t next = ctrl ? uiFindNextWordBoundary(value, caret) : uiUtf8NextBoundary(value, caret);
                uiEraseRange(value, caret, next);
                changed = 1;
            }
        }
        if (g4f_key_pressed(ui->window, G4F_KEY_BACKSPACE) && !value.empty()) {
            if (hasSelection) {
                uiEraseRange(value, selA, selB);
                caret = selA;
                anchor = caret;
                changed = 1;
            } else if (caret > 0) {
                size_t prev = ctrl ? uiFindPrevWordBoundary(value, caret) : uiUtf8PrevBoundary(value, caret);
                uiEraseRange(value, prev, caret);
                caret = prev;
                anchor = caret;
                changed = 1;
            }
        }
        int count = g4f_text_input_count(ui->window);
        for (int i = 0; i < count; i++) {
            uint32_t cp = g4f_text_input_codepoint(ui->window, i);
            if (cp == '\r' || cp == '\n' || cp == '\t') continue;
            if (cp < 32) continue;
            if (hasSelection) {
                uiEraseRange(value, selA, selB);
                caret = selA;
                anchor = caret;
                hasSelection = false;
            }
            std::string before = value;
            std::string insert;
            uiUtf8AppendCodepoint(insert, cp);
            value.insert(caret, insert);
            caret += insert.size();
            anchor = caret;
            if ((int)value.size() > maxBytes) {
                value = before;
                caret = uiUtf8ClampBoundary(value, caret);
                anchor = caret;
            } else {
                changed = 1;
            }
        }
    }

    // Store caret/anchor
    ui->storeInt[caretKey] = (int)caret;
    ui->storeInt[anchorKey] = (int)anchor;

    g4f_clip_push(ui->renderer, g4f_rect_f{box.x + 2.0f, box.y - 3.0f, box.w - 4.0f, 20.0f});

    if (!value.empty() && active && anchor != caret) {
        size_t selA = std::min(anchor, caret);
        size_t selB = std::max(anchor, caret);
        std::string left = value.substr(0, selA);
        std::string mid = value.substr(selA, selB - selA);
        float lw = 0.0f, lh = 0.0f, mw = 0.0f, mh = 0.0f;
        g4f_measure_text(ui->renderer, left.c_str(), 16.0f, &lw, &lh);
        g4f_measure_text(ui->renderer, mid.c_str(), 16.0f, &mw, &mh);
        g4f_draw_rect(ui->renderer, g4f_rect_f{box.x + 6.0f + lw, box.y - 1.0f, mw, 18.0f}, blendAlpha(ui->theme.accent, 120));
    }

    const char* drawText = value.empty() ? (placeholder_utf8 ? placeholder_utf8 : "") : value.c_str();
    uint32_t drawColor = value.empty() ? ui->theme.textMuted : ui->theme.text;
    g4f_draw_text(ui->renderer, drawText, box.x + 6.0f, box.y - 2.0f, 16.0f, drawColor);

    if (active) {
        std::string left = value.substr(0, caret);
        float lw = 0.0f, lh = 0.0f;
        g4f_measure_text(ui->renderer, left.c_str(), 16.0f, &lw, &lh);
        float cx = box.x + 6.0f + lw;
        g4f_draw_line(ui->renderer, cx, box.y - 1.0f, cx, box.y + 16.0f, 1.5f, ui->theme.text);
    }
    g4f_clip_pop(ui->renderer);

    if (out_utf8 && out_cap > 0) {
        std::snprintf(out_utf8, (size_t)out_cap, "%s", value.c_str());
    }
    return changed;
}

int g4f_ui_text_wrapped(g4f_ui* ui, const char* text_utf8, float size_px) {
    if (!ui || !ui->renderer || !text_utf8) return 0;
    float tw = 0.0f, th = 0.0f;
    g4f_measure_text_wrapped(ui->renderer, text_utf8, size_px, ui->layout.itemW, 10000.0f, &tw, &th);
    if (th < size_px) th = size_px;
    g4f_rect_f r = g4f_ui_layout_next(ui, th + 6.0f);
    g4f_draw_text_wrapped(ui->renderer, text_utf8, r, size_px, ui->theme.textMuted);
    return 0;
}

void g4f_ui_separator(g4f_ui* ui) {
    if (!ui || !ui->renderer) return;
    g4f_rect_f r = g4f_ui_layout_next(ui, 10.0f);
    float y = r.y + r.h * 0.5f;
    g4f_draw_line(ui->renderer, r.x, y, r.x + r.w, y, 1.5f, blendAlpha(ui->theme.panelBorder, 255));
}
