#include <cassert>
#include <cmath>
#include <cstdio>

#include "g4f/g4f.h"
#include "g4f/g4f_camera.h"

static bool isFiniteMat4(g4f_mat4 m) {
    for (float v : m.m) {
        if (!std::isfinite(v)) return false;
    }
    return true;
}

int main() {
    g4f_camera_fps cam = g4f_camera_fps_default();
    assert(std::isfinite(cam.yawRadians));
    assert(std::isfinite(cam.pitchRadians));
    assert(cam.fovYRadians > 0.1f);
    assert(cam.moveSpeed > 0.0f);
    assert(cam.lookSensitivity > 0.0f);

    g4f_mat4 v = g4f_camera_fps_view(&cam);
    g4f_mat4 p = g4f_camera_fps_proj(&cam, 16.0f / 9.0f);
    assert(isFiniteMat4(v));
    assert(isFiniteMat4(p));

    // Update should be safe even with a "neutral" window state.
    g4f_window_desc windowDesc{};
    windowDesc.title_utf8 = "g4f_camera_tests";
    windowDesc.width = 200;
    windowDesc.height = 140;
    windowDesc.resizable = 0;

    g4f_ctx* ctx = g4f_ctx_create(&windowDesc);
    assert(ctx != nullptr);
    g4f_window* window = g4f_ctx_window(ctx);
    assert(window != nullptr);

    // Poll a couple times so input state is initialized.
    for (int i = 0; i < 3 && g4f_ctx_poll(ctx); i++) {
        g4f_camera_fps_update(&cam, window, g4f_ctx_dt(ctx));
    }

    g4f_ctx_destroy(ctx);

    std::printf("camera_tests: OK\n");
    return 0;
}

