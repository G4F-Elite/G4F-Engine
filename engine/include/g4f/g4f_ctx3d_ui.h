#pragma once

#include "g4f.h"
#include "g4f_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

// Convenience wrapper for 3D apps that also need a 2D UI overlay:
// - owns: g4f_ctx3d + g4f_renderer (overlay) + g4f_ui
// - no editor; intended for menus/HUD/debug overlays
typedef struct g4f_ctx3d_ui g4f_ctx3d_ui;

g4f_ctx3d_ui* g4f_ctx3d_ui_create(const g4f_window_desc* windowDesc);
void g4f_ctx3d_ui_destroy(g4f_ctx3d_ui* ctx);

int g4f_ctx3d_ui_poll(g4f_ctx3d_ui* ctx);       // returns 0 when should close
double g4f_ctx3d_ui_time(const g4f_ctx3d_ui* ctx);
float g4f_ctx3d_ui_dt(const g4f_ctx3d_ui* ctx); // seconds since last poll

g4f_window* g4f_ctx3d_ui_window(g4f_ctx3d_ui* ctx);
g4f_gfx* g4f_ctx3d_ui_gfx(g4f_ctx3d_ui* ctx);
g4f_renderer* g4f_ctx3d_ui_renderer(g4f_ctx3d_ui* ctx);
g4f_ui* g4f_ctx3d_ui_ui(g4f_ctx3d_ui* ctx);

// 3D frame helpers (forward to g4f_frame3d_begin/end on the owned ctx3d).
void g4f_ctx3d_ui_frame3d_begin(g4f_ctx3d_ui* ctx, uint32_t clearRgba);
void g4f_ctx3d_ui_frame3d_end(g4f_ctx3d_ui* ctx);

// Overlay helpers (call after your 3D draw calls, before g4f_frame3d_end()).
void g4f_ctx3d_ui_overlay_begin(g4f_ctx3d_ui* ctx);
void g4f_ctx3d_ui_overlay_end(g4f_ctx3d_ui* ctx);

#ifdef __cplusplus
} // extern "C"
#endif
