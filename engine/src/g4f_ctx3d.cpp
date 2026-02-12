#include "g4f_platform_win32.h"

#include "../include/g4f/g4f.h"

struct g4f_ctx3d {
    g4f_app* app = nullptr;
    g4f_window* window = nullptr;
    g4f_gfx* gfx = nullptr;

    double timeSeconds = 0.0;
    double lastTimeSeconds = 0.0;
    float dtSeconds = 0.0f;
};

g4f_ctx3d* g4f_ctx3d_create(const g4f_window_desc* windowDesc) {
    if (!windowDesc) return nullptr;
    g4f_app_desc appDesc{};

    auto* ctx = new g4f_ctx3d();
    ctx->app = g4f_app_create(&appDesc);
    if (!ctx->app) {
        g4f_ctx3d_destroy(ctx);
        return nullptr;
    }

    ctx->window = g4f_window_create(ctx->app, windowDesc);
    if (!ctx->window) {
        g4f_ctx3d_destroy(ctx);
        return nullptr;
    }

    ctx->gfx = g4f_gfx_create(ctx->window);
    if (!ctx->gfx) {
        g4f_ctx3d_destroy(ctx);
        return nullptr;
    }

    ctx->timeSeconds = g4f_time_seconds(ctx->app);
    ctx->lastTimeSeconds = ctx->timeSeconds;
    ctx->dtSeconds = 1.0f / 60.0f;
    return ctx;
}

void g4f_ctx3d_destroy(g4f_ctx3d* ctx) {
    if (!ctx) return;
    if (ctx->gfx) g4f_gfx_destroy(ctx->gfx);
    if (ctx->window) g4f_window_destroy(ctx->window);
    if (ctx->app) g4f_app_destroy(ctx->app);
    delete ctx;
}

int g4f_ctx3d_poll(g4f_ctx3d* ctx) {
    if (!ctx || !ctx->window || !ctx->app) return 0;
    int alive = g4f_window_poll(ctx->window);

    ctx->timeSeconds = g4f_time_seconds(ctx->app);
    double rawDt = ctx->timeSeconds - ctx->lastTimeSeconds;
    ctx->lastTimeSeconds = ctx->timeSeconds;

    if (rawDt <= 0.0 || rawDt > 0.5) rawDt = 1.0 / 60.0;
    ctx->dtSeconds = (float)rawDt;
    return alive;
}

double g4f_ctx3d_time(const g4f_ctx3d* ctx) {
    return ctx ? ctx->timeSeconds : 0.0;
}

float g4f_ctx3d_dt(const g4f_ctx3d* ctx) {
    return ctx ? ctx->dtSeconds : 0.0f;
}

g4f_window* g4f_ctx3d_window(g4f_ctx3d* ctx) {
    return ctx ? ctx->window : nullptr;
}

g4f_gfx* g4f_ctx3d_gfx(g4f_ctx3d* ctx) {
    return ctx ? ctx->gfx : nullptr;
}

void g4f_frame3d_begin(g4f_ctx3d* ctx, uint32_t clearRgba) {
    if (!ctx || !ctx->gfx) return;
    g4f_gfx_begin(ctx->gfx, clearRgba);
}

void g4f_frame3d_end(g4f_ctx3d* ctx) {
    if (!ctx || !ctx->gfx) return;
    g4f_gfx_end(ctx->gfx);
}

