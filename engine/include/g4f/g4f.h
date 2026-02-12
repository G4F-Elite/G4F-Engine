#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Public handles (opaque).
typedef struct g4f_app g4f_app;
typedef struct g4f_window g4f_window;
typedef struct g4f_renderer g4f_renderer;
typedef struct g4f_bitmap g4f_bitmap;
typedef struct g4f_ctx g4f_ctx;
typedef struct g4f_gfx g4f_gfx;
typedef struct g4f_ctx3d g4f_ctx3d;

typedef struct g4f_window_desc {
    const char* title_utf8;
    int width;
    int height;
    int resizable; // 0/1
} g4f_window_desc;

// Color packing: 0xRRGGBBAA
static inline uint32_t g4f_rgba_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a;
}

// GLFW-like key codes (subset + commonly used keys).
enum {
    G4F_KEY_UNKNOWN = -1,

    G4F_KEY_SPACE = 32,
    G4F_KEY_APOSTROPHE = 39,
    G4F_KEY_COMMA = 44,
    G4F_KEY_MINUS = 45,
    G4F_KEY_PERIOD = 46,
    G4F_KEY_SLASH = 47,
    G4F_KEY_0 = 48,
    G4F_KEY_1 = 49,
    G4F_KEY_2 = 50,
    G4F_KEY_3 = 51,
    G4F_KEY_4 = 52,
    G4F_KEY_5 = 53,
    G4F_KEY_6 = 54,
    G4F_KEY_7 = 55,
    G4F_KEY_8 = 56,
    G4F_KEY_9 = 57,
    G4F_KEY_SEMICOLON = 59,
    G4F_KEY_EQUAL = 61,
    G4F_KEY_A = 65,
    G4F_KEY_B = 66,
    G4F_KEY_C = 67,
    G4F_KEY_D = 68,
    G4F_KEY_E = 69,
    G4F_KEY_F = 70,
    G4F_KEY_G = 71,
    G4F_KEY_H = 72,
    G4F_KEY_I = 73,
    G4F_KEY_J = 74,
    G4F_KEY_K = 75,
    G4F_KEY_L = 76,
    G4F_KEY_M = 77,
    G4F_KEY_N = 78,
    G4F_KEY_O = 79,
    G4F_KEY_P = 80,
    G4F_KEY_Q = 81,
    G4F_KEY_R = 82,
    G4F_KEY_S = 83,
    G4F_KEY_T = 84,
    G4F_KEY_U = 85,
    G4F_KEY_V = 86,
    G4F_KEY_W = 87,
    G4F_KEY_X = 88,
    G4F_KEY_Y = 89,
    G4F_KEY_Z = 90,
    G4F_KEY_LEFT_BRACKET = 91,
    G4F_KEY_BACKSLASH = 92,
    G4F_KEY_RIGHT_BRACKET = 93,
    G4F_KEY_GRAVE_ACCENT = 96,

    G4F_KEY_ESCAPE = 256,
    G4F_KEY_ENTER = 257,
    G4F_KEY_TAB = 258,
    G4F_KEY_BACKSPACE = 259,
    G4F_KEY_INSERT = 260,
    G4F_KEY_DELETE = 261,
    G4F_KEY_RIGHT = 262,
    G4F_KEY_LEFT = 263,
    G4F_KEY_DOWN = 264,
    G4F_KEY_UP = 265,
    G4F_KEY_PAGE_UP = 266,
    G4F_KEY_PAGE_DOWN = 267,
    G4F_KEY_HOME = 268,
    G4F_KEY_END = 269,

    G4F_KEY_CAPS_LOCK = 280,
    G4F_KEY_SCROLL_LOCK = 281,
    G4F_KEY_NUM_LOCK = 282,
    G4F_KEY_PRINT_SCREEN = 283,
    G4F_KEY_PAUSE = 284,

    G4F_KEY_F1 = 290,
    G4F_KEY_F2 = 291,
    G4F_KEY_F3 = 292,
    G4F_KEY_F4 = 293,
    G4F_KEY_F5 = 294,
    G4F_KEY_F6 = 295,
    G4F_KEY_F7 = 296,
    G4F_KEY_F8 = 297,
    G4F_KEY_F9 = 298,
    G4F_KEY_F10 = 299,
    G4F_KEY_F11 = 300,
    G4F_KEY_F12 = 301,

    G4F_KEY_KP_0 = 320,
    G4F_KEY_KP_1 = 321,
    G4F_KEY_KP_2 = 322,
    G4F_KEY_KP_3 = 323,
    G4F_KEY_KP_4 = 324,
    G4F_KEY_KP_5 = 325,
    G4F_KEY_KP_6 = 326,
    G4F_KEY_KP_7 = 327,
    G4F_KEY_KP_8 = 328,
    G4F_KEY_KP_9 = 329,
    G4F_KEY_KP_DECIMAL = 330,
    G4F_KEY_KP_DIVIDE = 331,
    G4F_KEY_KP_MULTIPLY = 332,
    G4F_KEY_KP_SUBTRACT = 333,
    G4F_KEY_KP_ADD = 334,
    G4F_KEY_KP_ENTER = 335,
    G4F_KEY_KP_EQUAL = 336,

    G4F_KEY_LEFT_SHIFT = 340,
    G4F_KEY_LEFT_CONTROL = 341,
    G4F_KEY_LEFT_ALT = 342,
    G4F_KEY_LEFT_SUPER = 343,
    G4F_KEY_RIGHT_SHIFT = 344,
    G4F_KEY_RIGHT_CONTROL = 345,
    G4F_KEY_RIGHT_ALT = 346,
    G4F_KEY_RIGHT_SUPER = 347,
    G4F_KEY_MENU = 348,
};

enum {
    G4F_MOUSE_BUTTON_LEFT = 0,
    G4F_MOUSE_BUTTON_RIGHT = 1,
    G4F_MOUSE_BUTTON_MIDDLE = 2,
};

typedef struct g4f_app_desc {
    int reserved;
} g4f_app_desc;

typedef struct g4f_rect_f {
    float x;
    float y;
    float w;
    float h;
} g4f_rect_f;

// Lifecycle / platform.
const char* g4f_version_string(void);
g4f_app* g4f_app_create(const g4f_app_desc* desc);
void g4f_app_destroy(g4f_app* app);

double g4f_time_seconds(const g4f_app* app);

// High-level context (simplest integration):
// - owns app + window + renderer
// - computes dt each poll
g4f_ctx* g4f_ctx_create(const g4f_window_desc* windowDesc);
void g4f_ctx_destroy(g4f_ctx* ctx);
int g4f_ctx_poll(g4f_ctx* ctx);       // returns 0 when should close
double g4f_ctx_time(const g4f_ctx* ctx);
float g4f_ctx_dt(const g4f_ctx* ctx); // seconds since last poll
g4f_window* g4f_ctx_window(g4f_ctx* ctx);
g4f_renderer* g4f_ctx_renderer(g4f_ctx* ctx);

void g4f_frame_begin(g4f_ctx* ctx, uint32_t clearRgba);
void g4f_frame_end(g4f_ctx* ctx);

// 3D (D3D11) — foundational API, no asset files:
// - shaders/materials/geometry are generated in code
g4f_gfx* g4f_gfx_create(g4f_window* window);
void g4f_gfx_destroy(g4f_gfx* gfx);
void g4f_gfx_begin(g4f_gfx* gfx, uint32_t clearRgba);
void g4f_gfx_end(g4f_gfx* gfx); // presents

// High-level 3D context (simplest 3D integration).
g4f_ctx3d* g4f_ctx3d_create(const g4f_window_desc* windowDesc);
void g4f_ctx3d_destroy(g4f_ctx3d* ctx);
int g4f_ctx3d_poll(g4f_ctx3d* ctx);       // returns 0 when should close
double g4f_ctx3d_time(const g4f_ctx3d* ctx);
float g4f_ctx3d_dt(const g4f_ctx3d* ctx); // seconds since last poll
g4f_window* g4f_ctx3d_window(g4f_ctx3d* ctx);
g4f_gfx* g4f_ctx3d_gfx(g4f_ctx3d* ctx);

void g4f_frame3d_begin(g4f_ctx3d* ctx, uint32_t clearRgba);
void g4f_frame3d_end(g4f_ctx3d* ctx);

// Minimal built-in 3D draw for early bring-up (procedural cube).
// Intended as a temporary scaffolding API during engine bootstrap.
void g4f_gfx_draw_debug_cube(g4f_gfx* gfx, float timeSeconds);

// Window.
g4f_window* g4f_window_create(g4f_app* app, const g4f_window_desc* desc);
void g4f_window_destroy(g4f_window* window);
int g4f_window_poll(g4f_window* window); // returns 0 when should close
void g4f_window_request_close(g4f_window* window);
void g4f_window_get_size(const g4f_window* window, int* width, int* height);

// Input (state is updated by g4f_window_poll()).
int g4f_key_down(const g4f_window* window, int key);
int g4f_key_pressed(const g4f_window* window, int key);
int g4f_mouse_down(const g4f_window* window, int button);
int g4f_mouse_pressed(const g4f_window* window, int button);
float g4f_mouse_x(const g4f_window* window);
float g4f_mouse_y(const g4f_window* window);
float g4f_mouse_wheel_delta(const g4f_window* window); // per-frame, resets on poll

// 2D Renderer.
g4f_renderer* g4f_renderer_create(g4f_window* window);
void g4f_renderer_destroy(g4f_renderer* renderer);
void g4f_renderer_begin(g4f_renderer* renderer);
void g4f_renderer_end(g4f_renderer* renderer);
void g4f_renderer_clear(g4f_renderer* renderer, uint32_t rgba);

void g4f_draw_rect(g4f_renderer* renderer, g4f_rect_f rect, uint32_t rgba);
void g4f_draw_rect_outline(g4f_renderer* renderer, g4f_rect_f rect, float thickness, uint32_t rgba);
void g4f_draw_line(g4f_renderer* renderer, float x1, float y1, float x2, float y2, float thickness, uint32_t rgba);
void g4f_draw_text(g4f_renderer* renderer, const char* text_utf8, float x, float y, float size_px, uint32_t rgba);

// Bitmaps.
g4f_bitmap* g4f_bitmap_load(g4f_renderer* renderer, const char* path_utf8);
void g4f_bitmap_destroy(g4f_bitmap* bitmap);
void g4f_draw_bitmap(g4f_renderer* renderer, const g4f_bitmap* bitmap, g4f_rect_f dst, float opacity);

#ifdef __cplusplus
} // extern "C"
#endif
