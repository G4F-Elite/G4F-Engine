#pragma once

#include "g4f.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct g4f_camera_fps {
    g4f_vec3 position;
    float yawRadians;   // rotation around +Y
    float pitchRadians; // rotation around +X

    float fovYRadians;
    float moveSpeed;       // units/sec
    float lookSensitivity; // radians per pixel
} g4f_camera_fps;

g4f_camera_fps g4f_camera_fps_default(void);
void g4f_camera_fps_update(g4f_camera_fps* cam, const g4f_window* window, float dtSeconds);

g4f_mat4 g4f_camera_fps_view(const g4f_camera_fps* cam);
g4f_mat4 g4f_camera_fps_proj(const g4f_camera_fps* cam, float aspect);

#ifdef __cplusplus
} // extern "C"
#endif

