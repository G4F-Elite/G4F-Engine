#include "../include/g4f/g4f_ctx3d_ui.h"
#include "g4f_error_internal.h"

struct g4f_ctx3d_ui {
    g4f_ctx3d* ctx3d = nullptr;
    g4f_renderer* overlay = nullptr;
    g4f_ui* ui = nullptr;
};

g4f_ctx3d_ui* g4f_ctx3d_ui_create(const g4f_window_desc* windowDesc) {
    if (!windowDesc) {
        g4f_set_last_error("g4f_ctx3d_ui_create: windowDesc is null");
        return nullptr;
    }
    auto* ctx = new g4f_ctx3d_ui();

    ctx->ctx3d = g4f_ctx3d_create(windowDesc);
    if (!ctx->ctx3d) {
        if (!g4f_last_error()[0]) g4f_set_last_error("g4f_ctx3d_ui_create: g4f_ctx3d_create failed");
        g4f_ctx3d_ui_destroy(ctx);
        return nullptr;
    }

    g4f_gfx* gfx = g4f_ctx3d_gfx(ctx->ctx3d);
    ctx->overlay = g4f_renderer_create_for_gfx(gfx);
    if (!ctx->overlay) {
        if (!g4f_last_error()[0]) g4f_set_last_error("g4f_ctx3d_ui_create: g4f_renderer_create_for_gfx failed");
        g4f_ctx3d_ui_destroy(ctx);
        return nullptr;
    }

    ctx->ui = g4f_ui_create();
    if (!ctx->ui) {
        if (!g4f_last_error()[0]) g4f_set_last_error("g4f_ctx3d_ui_create: g4f_ui_create failed");
        g4f_ctx3d_ui_destroy(ctx);
        return nullptr;
    }

    return ctx;
}

void g4f_ctx3d_ui_destroy(g4f_ctx3d_ui* ctx) {
    if (!ctx) return;
    if (ctx->ui) g4f_ui_destroy(ctx->ui);
    if (ctx->overlay) g4f_renderer_destroy(ctx->overlay);
    if (ctx->ctx3d) g4f_ctx3d_destroy(ctx->ctx3d);
    delete ctx;
}

int g4f_ctx3d_ui_poll(g4f_ctx3d_ui* ctx) {
    if (!ctx || !ctx->ctx3d) return 0;
    return g4f_ctx3d_poll(ctx->ctx3d);
}

double g4f_ctx3d_ui_time(const g4f_ctx3d_ui* ctx) {
    if (!ctx || !ctx->ctx3d) return 0.0;
    return g4f_ctx3d_time(ctx->ctx3d);
}

float g4f_ctx3d_ui_dt(const g4f_ctx3d_ui* ctx) {
    if (!ctx || !ctx->ctx3d) return 0.0f;
    return g4f_ctx3d_dt(ctx->ctx3d);
}

g4f_window* g4f_ctx3d_ui_window(g4f_ctx3d_ui* ctx) {
    return (ctx && ctx->ctx3d) ? g4f_ctx3d_window(ctx->ctx3d) : nullptr;
}

g4f_gfx* g4f_ctx3d_ui_gfx(g4f_ctx3d_ui* ctx) {
    return (ctx && ctx->ctx3d) ? g4f_ctx3d_gfx(ctx->ctx3d) : nullptr;
}

g4f_renderer* g4f_ctx3d_ui_renderer(g4f_ctx3d_ui* ctx) {
    return ctx ? ctx->overlay : nullptr;
}

g4f_ui* g4f_ctx3d_ui_ui(g4f_ctx3d_ui* ctx) {
    return ctx ? ctx->ui : nullptr;
}

void g4f_ctx3d_ui_frame3d_begin(g4f_ctx3d_ui* ctx, uint32_t clearRgba) {
    if (!ctx || !ctx->ctx3d) return;
    g4f_frame3d_begin(ctx->ctx3d, clearRgba);
}

void g4f_ctx3d_ui_frame3d_end(g4f_ctx3d_ui* ctx) {
    if (!ctx || !ctx->ctx3d) return;
    g4f_frame3d_end(ctx->ctx3d);
}

void g4f_ctx3d_ui_overlay_begin(g4f_ctx3d_ui* ctx) {
    if (!ctx || !ctx->overlay || !ctx->ui) return;
    g4f_window* window = g4f_ctx3d_ui_window(ctx);
    g4f_renderer_begin(ctx->overlay);
    g4f_ui_begin(ctx->ui, ctx->overlay, window);
}

void g4f_ctx3d_ui_overlay_end(g4f_ctx3d_ui* ctx) {
    if (!ctx || !ctx->overlay || !ctx->ui) return;
    g4f_ui_end(ctx->ui);
    g4f_renderer_end(ctx->overlay);
}
