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
typedef struct g4f_gfx_texture g4f_gfx_texture;
typedef struct g4f_gfx_material g4f_gfx_material;
typedef struct g4f_gfx_mesh g4f_gfx_mesh;

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

typedef struct g4f_vec3 {
    float x;
    float y;
    float z;
} g4f_vec3;

typedef struct g4f_mat4 {
    // Row-major 4x4 matrix (matches D3D/HLSL `row_major` usage in this engine).
    float m[16];
} g4f_mat4;

g4f_mat4 g4f_mat4_identity(void);
g4f_mat4 g4f_mat4_mul(g4f_mat4 a, g4f_mat4 b);
g4f_mat4 g4f_mat4_translation(float x, float y, float z);
g4f_mat4 g4f_mat4_rotation_x(float radians);
g4f_mat4 g4f_mat4_rotation_y(float radians);
g4f_mat4 g4f_mat4_rotation_z(float radians);
g4f_mat4 g4f_mat4_scale(float x, float y, float z);
g4f_mat4 g4f_mat4_perspective(float fovYRadians, float aspect, float zn, float zf);
g4f_mat4 g4f_mat4_look_at(g4f_vec3 eye, g4f_vec3 at, g4f_vec3 up);

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
void g4f_gfx_get_size(const g4f_gfx* gfx, int* width, int* height);
float g4f_gfx_aspect(const g4f_gfx* gfx);
void g4f_gfx_set_vsync(g4f_gfx* gfx, int enabled);

// Global lighting state (used by lit materials).
void g4f_gfx_set_light_dir(g4f_gfx* gfx, float x, float y, float z); // direction light travels
void g4f_gfx_set_light_colors(g4f_gfx* gfx, uint32_t lightRgba, uint32_t ambientRgba);

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

// 3D resources (created from code, no asset files).
g4f_gfx_texture* g4f_gfx_texture_create_rgba8(g4f_gfx* gfx, int width, int height, const void* rgbaPixels, int rowPitchBytes);
g4f_gfx_texture* g4f_gfx_texture_create_solid_rgba8(g4f_gfx* gfx, uint32_t rgba);
g4f_gfx_texture* g4f_gfx_texture_create_checker_rgba8(g4f_gfx* gfx, int width, int height, int cellSizePx, uint32_t rgbaA, uint32_t rgbaB);
void g4f_gfx_texture_destroy(g4f_gfx_texture* texture);

typedef struct g4f_gfx_material_unlit_desc {
    uint32_t tintRgba;            // multiplies output
    g4f_gfx_texture* texture;     // optional
    int alphaBlend;              // 0/1 (default 0)
    int depthTest;               // 0/1 (default 1)
    int depthWrite;              // 0/1 (default 1)
    int cullMode;                // 0 back (default), 1 none, 2 front
} g4f_gfx_material_unlit_desc;

g4f_gfx_material* g4f_gfx_material_create_unlit(g4f_gfx* gfx, const g4f_gfx_material_unlit_desc* desc);
g4f_gfx_material* g4f_gfx_material_create_lit(g4f_gfx* gfx, const g4f_gfx_material_unlit_desc* desc);
void g4f_gfx_material_destroy(g4f_gfx_material* material);
void g4f_gfx_material_set_tint_rgba(g4f_gfx_material* material, uint32_t rgba);
void g4f_gfx_material_set_texture(g4f_gfx_material* material, g4f_gfx_texture* texture);
void g4f_gfx_material_set_alpha_blend(g4f_gfx_material* material, int enabled);
void g4f_gfx_material_set_depth(g4f_gfx_material* material, int depthTest, int depthWrite);
void g4f_gfx_material_set_cull(g4f_gfx_material* material, int cullMode);

typedef struct g4f_gfx_vertex_p3n3uv2 {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
} g4f_gfx_vertex_p3n3uv2;

g4f_gfx_mesh* g4f_gfx_mesh_create_p3n3uv2(g4f_gfx* gfx, const g4f_gfx_vertex_p3n3uv2* vertices, int vertexCount, const uint16_t* indices, int indexCount);
g4f_gfx_mesh* g4f_gfx_mesh_create_cube_p3n3uv2(g4f_gfx* gfx, float halfExtent);
g4f_gfx_mesh* g4f_gfx_mesh_create_plane_xz_p3n3uv2(g4f_gfx* gfx, float halfExtent, float uvScale);
void g4f_gfx_mesh_destroy(g4f_gfx_mesh* mesh);
void g4f_gfx_draw_mesh(g4f_gfx* gfx, const g4f_gfx_mesh* mesh, const g4f_gfx_material* material, const g4f_mat4* mvp);
void g4f_gfx_draw_mesh_xform(g4f_gfx* gfx, const g4f_gfx_mesh* mesh, const g4f_gfx_material* material, const g4f_mat4* model, const g4f_mat4* mvp);

// Window.
g4f_window* g4f_window_create(g4f_app* app, const g4f_window_desc* desc);
void g4f_window_destroy(g4f_window* window);
int g4f_window_poll(g4f_window* window); // returns 0 when should close
void g4f_window_request_close(g4f_window* window);
void g4f_window_get_size(const g4f_window* window, int* width, int* height);
void g4f_window_set_title(g4f_window* window, const char* title_utf8);

// Input (state is updated by g4f_window_poll()).
int g4f_key_down(const g4f_window* window, int key);
int g4f_key_pressed(const g4f_window* window, int key);
int g4f_mouse_down(const g4f_window* window, int button);
int g4f_mouse_pressed(const g4f_window* window, int button);
float g4f_mouse_x(const g4f_window* window);
float g4f_mouse_y(const g4f_window* window);
float g4f_mouse_dx(const g4f_window* window); // per-frame
float g4f_mouse_dy(const g4f_window* window); // per-frame
float g4f_mouse_wheel_delta(const g4f_window* window); // per-frame, resets on poll

// Cursor capture (useful for 3D camera).
// When enabled, cursor is hidden and movement becomes "relative" via per-frame dx/dy.
void g4f_window_set_cursor_captured(g4f_window* window, int captured);
int g4f_window_cursor_captured(const g4f_window* window);
int g4f_window_focused(const g4f_window* window);

// Text input (per-frame), filled by the OS (WM_CHAR). Values are Unicode code points.
int g4f_text_input_count(const g4f_window* window);
uint32_t g4f_text_input_codepoint(const g4f_window* window, int index);

// Clipboard (UTF-8).
// Returns number of bytes written (excluding '\0') or 0 if empty/failure.
int g4f_clipboard_get_utf8(const g4f_window* window, char* out_utf8, int out_cap);
int g4f_clipboard_set_utf8(const g4f_window* window, const char* text_utf8);

// 2D Renderer.
g4f_renderer* g4f_renderer_create(g4f_window* window);
void g4f_renderer_destroy(g4f_renderer* renderer);
void g4f_renderer_begin(g4f_renderer* renderer);
void g4f_renderer_end(g4f_renderer* renderer);
void g4f_renderer_clear(g4f_renderer* renderer, uint32_t rgba);

void g4f_draw_rect(g4f_renderer* renderer, g4f_rect_f rect, uint32_t rgba);
void g4f_draw_rect_outline(g4f_renderer* renderer, g4f_rect_f rect, float thickness, uint32_t rgba);
void g4f_draw_round_rect(g4f_renderer* renderer, g4f_rect_f rect, float radius, uint32_t rgba);
void g4f_draw_round_rect_outline(g4f_renderer* renderer, g4f_rect_f rect, float radius, float thickness, uint32_t rgba);
void g4f_draw_line(g4f_renderer* renderer, float x1, float y1, float x2, float y2, float thickness, uint32_t rgba);
void g4f_draw_text(g4f_renderer* renderer, const char* text_utf8, float x, float y, float size_px, uint32_t rgba);
void g4f_draw_text_wrapped(g4f_renderer* renderer, const char* text_utf8, g4f_rect_f bounds, float size_px, uint32_t rgba);
void g4f_measure_text(g4f_renderer* renderer, const char* text_utf8, float size_px, float* out_w, float* out_h);
void g4f_measure_text_wrapped(g4f_renderer* renderer, const char* text_utf8, float size_px, float max_w, float max_h, float* out_w, float* out_h);

// Clipping (useful for panels/scroll areas).
void g4f_clip_push(g4f_renderer* renderer, g4f_rect_f rect);
void g4f_clip_pop(g4f_renderer* renderer);

// Bitmaps.
g4f_bitmap* g4f_bitmap_load(g4f_renderer* renderer, const char* path_utf8);
g4f_bitmap* g4f_bitmap_create_rgba8(g4f_renderer* renderer, int width, int height, const void* rgbaPixels, int rowPitchBytes);
void g4f_bitmap_destroy(g4f_bitmap* bitmap);
void g4f_bitmap_get_size(const g4f_bitmap* bitmap, int* width, int* height);
void g4f_draw_bitmap(g4f_renderer* renderer, const g4f_bitmap* bitmap, g4f_rect_f dst, float opacity);

// Create a UI renderer that draws into the current D3D11 swapchain backbuffer.
// Use for 2D panels/menus overlay in 3D apps.
g4f_renderer* g4f_renderer_create_for_gfx(g4f_gfx* gfx);

#ifdef __cplusplus
} // extern "C"
#endif
