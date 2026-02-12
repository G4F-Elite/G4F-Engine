#include <cassert>
#include <cmath>
#include <iostream>

#include "g4f/g4f.h"

static bool feq(float a, float b, float eps = 1e-5f) {
    return std::fabs(a - b) <= eps;
}

static void testIdentity() {
    g4f_mat4 m = g4f_mat4_identity();
    for (int i = 0; i < 16; i++) {
        float expected = (i % 5 == 0) ? 1.0f : 0.0f;
        assert(feq(m.m[i], expected));
    }
}

static void testMulIdentity() {
    g4f_mat4 a = g4f_mat4_translation(1.0f, 2.0f, 3.0f);
    g4f_mat4 i = g4f_mat4_identity();
    g4f_mat4 left = g4f_mat4_mul(a, i);
    g4f_mat4 right = g4f_mat4_mul(i, a);
    for (int k = 0; k < 16; k++) {
        assert(feq(left.m[k], a.m[k]));
        assert(feq(right.m[k], a.m[k]));
    }
}

static void testTranslationLayout() {
    g4f_mat4 t = g4f_mat4_translation(7.0f, -2.0f, 5.0f);
    assert(feq(t.m[12], 7.0f));
    assert(feq(t.m[13], -2.0f));
    assert(feq(t.m[14], 5.0f));
    assert(feq(t.m[15], 1.0f));
}

static void testScaleLayout() {
    g4f_mat4 s = g4f_mat4_scale(2.0f, 3.0f, 4.0f);
    assert(feq(s.m[0], 2.0f));
    assert(feq(s.m[5], 3.0f));
    assert(feq(s.m[10], 4.0f));
    assert(feq(s.m[15], 1.0f));
}

static void testRotationZShape() {
    g4f_mat4 r = g4f_mat4_rotation_z(1.2345f);
    assert(std::isfinite(r.m[0]));
    assert(std::isfinite(r.m[1]));
    assert(std::isfinite(r.m[4]));
    assert(std::isfinite(r.m[5]));
    assert(feq(r.m[10], 1.0f));
    assert(feq(r.m[15], 1.0f));
}

static void testPerspectiveShape() {
    g4f_mat4 p = g4f_mat4_perspective(70.0f * 3.14159265f / 180.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    assert(std::isfinite(p.m[0]));
    assert(std::isfinite(p.m[5]));
    assert(feq(p.m[11], 1.0f));
    assert(p.m[10] > 0.0f);
}

static void testLookAtOrthonormal() {
    g4f_vec3 eye{1.0f, 2.0f, -3.0f};
    g4f_vec3 at{1.0f, 2.0f, 0.0f};
    g4f_vec3 up{0.0f, 1.0f, 0.0f};
    g4f_mat4 v = g4f_mat4_look_at(eye, at, up);

    // Right = column0, Up = column1, Forward = -column2.
    g4f_vec3 right{v.m[0], v.m[4], v.m[8]};
    g4f_vec3 upVec{v.m[1], v.m[5], v.m[9]};
    g4f_vec3 forward{-v.m[2], -v.m[6], -v.m[10]};

    auto dot = [](g4f_vec3 a, g4f_vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; };
    auto len = [&](g4f_vec3 a) { return std::sqrt(dot(a, a)); };

    assert(feq(len(right), 1.0f, 1e-4f));
    assert(feq(len(upVec), 1.0f, 1e-4f));
    assert(feq(len(forward), 1.0f, 1e-4f));
    assert(std::fabs(dot(right, upVec)) < 1e-3f);
    assert(std::fabs(dot(right, forward)) < 1e-3f);
    assert(std::fabs(dot(upVec, forward)) < 1e-3f);
}

int main() {
    testIdentity();
    testMulIdentity();
    testTranslationLayout();
    testScaleLayout();
    testRotationZShape();
    testPerspectiveShape();
    testLookAtOrthonormal();
    std::cout << "math_tests: OK\n";
    return 0;
}
