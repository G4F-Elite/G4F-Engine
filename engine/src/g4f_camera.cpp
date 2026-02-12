#include "../include/g4f/g4f_camera.h"

#include <algorithm>
#include <cmath>

static float clampFloat(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static g4f_vec3 vec3Add(g4f_vec3 a, g4f_vec3 b) { return g4f_vec3{a.x + b.x, a.y + b.y, a.z + b.z}; }
static g4f_vec3 vec3Sub(g4f_vec3 a, g4f_vec3 b) { return g4f_vec3{a.x - b.x, a.y - b.y, a.z - b.z}; }
static g4f_vec3 vec3Mul(g4f_vec3 a, float s) { return g4f_vec3{a.x * s, a.y * s, a.z * s}; }

static float vec3Dot(g4f_vec3 a, g4f_vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

static g4f_vec3 vec3Cross(g4f_vec3 a, g4f_vec3 b) {
    return g4f_vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static g4f_vec3 vec3Normalize(g4f_vec3 v) {
    float len2 = vec3Dot(v, v);
    if (len2 <= 0.0f) return g4f_vec3{0.0f, 0.0f, 0.0f};
    float invLen = 1.0f / std::sqrt(len2);
    return g4f_vec3{v.x * invLen, v.y * invLen, v.z * invLen};
}

static g4f_vec3 fpsForward(float yaw, float pitch) {
    // Right-handed: +X right, +Y up, +Z forward.
    float cy = std::cos(yaw);
    float sy = std::sin(yaw);
    float cp = std::cos(pitch);
    float sp = std::sin(pitch);
    return vec3Normalize(g4f_vec3{sy * cp, sp, cy * cp});
}

g4f_camera_fps g4f_camera_fps_default(void) {
    g4f_camera_fps cam{};
    cam.position = g4f_vec3{0.0f, 0.0f, -4.0f};
    cam.yawRadians = 0.0f;
    cam.pitchRadians = 0.0f;
    cam.fovYRadians = 70.0f * 3.14159265f / 180.0f;
    cam.moveSpeed = 4.5f;
    cam.lookSensitivity = 0.0022f;
    return cam;
}

void g4f_camera_fps_update(g4f_camera_fps* cam, const g4f_window* window, float dtSeconds) {
    if (!cam || !window) return;
    if (dtSeconds < 0.0f) dtSeconds = 0.0f;
    if (dtSeconds > 0.25f) dtSeconds = 0.25f;

    float mx = g4f_mouse_dx(window);
    float my = g4f_mouse_dy(window);
    cam->yawRadians += mx * cam->lookSensitivity;
    cam->pitchRadians += my * cam->lookSensitivity;
    cam->pitchRadians = clampFloat(cam->pitchRadians, -1.55f, 1.55f);

    g4f_vec3 forward = fpsForward(cam->yawRadians, cam->pitchRadians);
    g4f_vec3 worldUp{0.0f, 1.0f, 0.0f};
    g4f_vec3 right = vec3Normalize(vec3Cross(worldUp, forward));

    float speed = cam->moveSpeed;
    if (g4f_key_down(window, G4F_KEY_LEFT_SHIFT) || g4f_key_down(window, G4F_KEY_RIGHT_SHIFT)) speed *= 2.0f;

    g4f_vec3 wish{0.0f, 0.0f, 0.0f};
    if (g4f_key_down(window, G4F_KEY_W)) wish = vec3Add(wish, forward);
    if (g4f_key_down(window, G4F_KEY_S)) wish = vec3Sub(wish, forward);
    if (g4f_key_down(window, G4F_KEY_D)) wish = vec3Add(wish, right);
    if (g4f_key_down(window, G4F_KEY_A)) wish = vec3Sub(wish, right);
    if (g4f_key_down(window, G4F_KEY_SPACE)) wish = vec3Add(wish, worldUp);
    if (g4f_key_down(window, G4F_KEY_LEFT_CONTROL) || g4f_key_down(window, G4F_KEY_RIGHT_CONTROL)) wish = vec3Sub(wish, worldUp);

    float len2 = vec3Dot(wish, wish);
    if (len2 > 0.0f) {
        wish = vec3Normalize(wish);
        cam->position = vec3Add(cam->position, vec3Mul(wish, speed * dtSeconds));
    }
}

g4f_mat4 g4f_camera_fps_view(const g4f_camera_fps* cam) {
    if (!cam) return g4f_mat4_identity();
    g4f_vec3 up{0.0f, 1.0f, 0.0f};
    g4f_vec3 f = fpsForward(cam->yawRadians, cam->pitchRadians);
    g4f_vec3 at = vec3Add(cam->position, f);
    return g4f_mat4_look_at(cam->position, at, up);
}

g4f_mat4 g4f_camera_fps_proj(const g4f_camera_fps* cam, float aspect) {
    float a = (aspect <= 0.0f) ? 1.0f : aspect;
    float fov = cam ? cam->fovYRadians : (70.0f * 3.14159265f / 180.0f);
    return g4f_mat4_perspective(fov, a, 0.1f, 100.0f);
}

