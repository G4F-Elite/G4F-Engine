#include "../include/g4f/g4f.h"

#include <cmath>

static g4f_vec3 vec3Sub(g4f_vec3 a, g4f_vec3 b) {
    return g4f_vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

static float vec3Dot(g4f_vec3 a, g4f_vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static g4f_vec3 vec3Cross(g4f_vec3 a, g4f_vec3 b) {
    return g4f_vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

static g4f_vec3 vec3Normalize(g4f_vec3 v) {
    float len2 = vec3Dot(v, v);
    if (len2 <= 0.0f) return g4f_vec3{0.0f, 0.0f, 0.0f};
    float invLen = 1.0f / std::sqrt(len2);
    return g4f_vec3{v.x * invLen, v.y * invLen, v.z * invLen};
}

g4f_mat4 g4f_mat4_identity(void) {
    g4f_mat4 out{};
    out.m[0] = 1.0f;
    out.m[5] = 1.0f;
    out.m[10] = 1.0f;
    out.m[15] = 1.0f;
    return out;
}

g4f_mat4 g4f_mat4_mul(g4f_mat4 a, g4f_mat4 b) {
    g4f_mat4 out{};
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            out.m[r * 4 + c] =
                a.m[r * 4 + 0] * b.m[0 * 4 + c] +
                a.m[r * 4 + 1] * b.m[1 * 4 + c] +
                a.m[r * 4 + 2] * b.m[2 * 4 + c] +
                a.m[r * 4 + 3] * b.m[3 * 4 + c];
        }
    }
    return out;
}

g4f_mat4 g4f_mat4_rotation_y(float radians) {
    g4f_mat4 out = g4f_mat4_identity();
    float c = std::cos(radians);
    float s = std::sin(radians);
    out.m[0] = c;
    out.m[2] = s;
    out.m[8] = -s;
    out.m[10] = c;
    return out;
}

g4f_mat4 g4f_mat4_rotation_x(float radians) {
    g4f_mat4 out = g4f_mat4_identity();
    float c = std::cos(radians);
    float s = std::sin(radians);
    out.m[5] = c;
    out.m[6] = -s;
    out.m[9] = s;
    out.m[10] = c;
    return out;
}

g4f_mat4 g4f_mat4_translation(float x, float y, float z) {
    g4f_mat4 out = g4f_mat4_identity();
    out.m[12] = x;
    out.m[13] = y;
    out.m[14] = z;
    return out;
}

g4f_mat4 g4f_mat4_perspective(float fovYRadians, float aspect, float zn, float zf) {
    g4f_mat4 out{};
    float yScale = 1.0f / std::tan(fovYRadians * 0.5f);
    float xScale = yScale / aspect;
    out.m[0] = xScale;
    out.m[5] = yScale;
    out.m[10] = zf / (zf - zn);
    out.m[11] = 1.0f;
    out.m[14] = (-zn * zf) / (zf - zn);
    return out;
}

g4f_mat4 g4f_mat4_look_at(g4f_vec3 eye, g4f_vec3 at, g4f_vec3 up) {
    // Row-vector convention:
    // p_view = p_world * View, so translation is in last row, and axes are in columns.
    g4f_vec3 forward = vec3Normalize(vec3Sub(at, eye));
    g4f_vec3 right = vec3Normalize(vec3Cross(forward, up));
    g4f_vec3 upVec = vec3Cross(right, forward);

    g4f_mat4 out = g4f_mat4_identity();
    out.m[0] = right.x;
    out.m[4] = right.y;
    out.m[8] = right.z;

    out.m[1] = upVec.x;
    out.m[5] = upVec.y;
    out.m[9] = upVec.z;

    out.m[2] = -forward.x;
    out.m[6] = -forward.y;
    out.m[10] = -forward.z;

    out.m[12] = -vec3Dot(right, eye);
    out.m[13] = -vec3Dot(upVec, eye);
    out.m[14] = vec3Dot(forward, eye);
    return out;
}
