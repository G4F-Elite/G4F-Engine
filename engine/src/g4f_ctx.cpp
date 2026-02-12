#include "g4f_platform_win32.h"
#include "g4f_error_internal.h"

#include "../include/g4f/g4f.h"

#include <algorithm>

struct g4f_ctx {
    g4f_app* app = nullptr;
    g4f_window* window = nullptr;
    g4f_renderer* renderer = nullptr;

    double timeSeconds = 0.0;
    double lastTimeSeconds = 0.0;
    float dtSeconds = 0.0f;
};

g4f_ctx* g4f_ctx_create(const g4f_window_desc* windowDesc) {
    if (!windowDesc) {
        g4f_set_last_error("g4f_ctx_create: windowDesc is null");
        return nullptr;
    }
    g4f_app_desc appDesc{};

    auto* ctx = new g4f_ctx();
    ctx->app = g4f_app_create(&appDesc);
    if (!ctx->app) {
        if (!g4f_last_error()[0]) g4f_set_last_error("g4f_ctx_create: g4f_app_create failed");
        g4f_ctx_destroy(ctx);
        return nullptr;
    }

    ctx->window = g4f_window_create(ctx->app, windowDesc);
    if (!ctx->window) {
        if (!g4f_last_error()[0]) g4f_set_last_error("g4f_ctx_create: g4f_window_create failed");
        g4f_ctx_destroy(ctx);
        return nullptr;
    }

    ctx->renderer = g4f_renderer_create(ctx->window);
    if (!ctx->renderer) {
        if (!g4f_last_error()[0]) g4f_set_last_error("g4f_ctx_create: g4f_renderer_create failed");
        g4f_ctx_destroy(ctx);
        return nullptr;
    }

    ctx->timeSeconds = g4f_time_seconds(ctx->app);
    ctx->lastTimeSeconds = ctx->timeSeconds;
    ctx->dtSeconds = 1.0f / 60.0f;
    return ctx;
}

void g4f_ctx_destroy(g4f_ctx* ctx) {
    if (!ctx) return;
    if (ctx->renderer) g4f_renderer_destroy(ctx->renderer);
    if (ctx->window) g4f_window_destroy(ctx->window);
    if (ctx->app) g4f_app_destroy(ctx->app);
    delete ctx;
}

int g4f_ctx_poll(g4f_ctx* ctx) {
    if (!ctx || !ctx->window || !ctx->app) return 0;
    int alive = g4f_window_poll(ctx->window);

    ctx->timeSeconds = g4f_time_seconds(ctx->app);
    double rawDt = ctx->timeSeconds - ctx->lastTimeSeconds;
    ctx->lastTimeSeconds = ctx->timeSeconds;

    if (rawDt <= 0.0 || rawDt > 0.5) rawDt = 1.0 / 60.0;
    ctx->dtSeconds = (float)rawDt;
    return alive;
}

double g4f_ctx_time(const g4f_ctx* ctx) {
    return ctx ? ctx->timeSeconds : 0.0;
}

float g4f_ctx_dt(const g4f_ctx* ctx) {
    return ctx ? ctx->dtSeconds : 0.0f;
}

g4f_window* g4f_ctx_window(g4f_ctx* ctx) {
    return ctx ? ctx->window : nullptr;
}

g4f_renderer* g4f_ctx_renderer(g4f_ctx* ctx) {
    return ctx ? ctx->renderer : nullptr;
}

void g4f_frame_begin(g4f_ctx* ctx, uint32_t clearRgba) {
    if (!ctx || !ctx->renderer) return;
    g4f_renderer_begin(ctx->renderer);
    g4f_renderer_clear(ctx->renderer, clearRgba);
}

void g4f_frame_end(g4f_ctx* ctx) {
    if (!ctx || !ctx->renderer) return;
    g4f_renderer_end(ctx->renderer);
}
