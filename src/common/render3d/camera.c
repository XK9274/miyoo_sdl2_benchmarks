#include "common/render3d/camera.h"

#include <math.h>

#define CAMERA3D_PI 3.14159265358979323846f
#define CAMERA3D_PITCH_LIMIT_RADIANS (89.0f * CAMERA3D_PI / 180.0f)
#define CAMERA3D_MIN_DISTANCE 0.05f
#define CAMERA3D_MAX_DISTANCE 1000.0f

static float camera3d_clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void camera3d_init(Camera3D *camera, Vec3 target, float distance)
{
    camera->target = target;
    camera->distance = camera3d_clampf(distance, CAMERA3D_MIN_DISTANCE, CAMERA3D_MAX_DISTANCE);
    camera->yaw_radians = 0.0f;
    camera->pitch_radians = 0.0f;
    camera->fov_y_radians = 60.0f * CAMERA3D_PI / 180.0f;
    camera->near_plane = 0.1f;
    camera->far_plane = 100.0f;
}

void camera3d_auto_rotate(Camera3D *camera, float radians_per_second, float dt)
{
    camera->yaw_radians += radians_per_second * dt;
}

void camera3d_orbit(Camera3D *camera, float yaw_delta_radians, float pitch_delta_radians)
{
    camera->yaw_radians += yaw_delta_radians;
    camera->pitch_radians = camera3d_clampf(camera->pitch_radians + pitch_delta_radians,
                                            -CAMERA3D_PITCH_LIMIT_RADIANS, CAMERA3D_PITCH_LIMIT_RADIANS);
}

void camera3d_zoom(Camera3D *camera, float distance_delta)
{
    camera->distance = camera3d_clampf(camera->distance + distance_delta,
                                       CAMERA3D_MIN_DISTANCE, CAMERA3D_MAX_DISTANCE);
}

Vec3 camera3d_eye_position(const Camera3D *camera)
{
    const float cos_pitch = cosf(camera->pitch_radians);
    const Vec3 offset = {
        camera->distance * cos_pitch * sinf(camera->yaw_radians),
        camera->distance * sinf(camera->pitch_radians),
        camera->distance * cos_pitch * cosf(camera->yaw_radians)
    };
    return bench_vec3_add(camera->target, offset);
}

Mat4 camera3d_view_matrix(const Camera3D *camera)
{
    const Vec3 eye = camera3d_eye_position(camera);
    const Vec3 up = {0.0f, 1.0f, 0.0f};
    return bench_mat4_look_at(eye, camera->target, up);
}

Mat4 camera3d_projection_matrix(const Camera3D *camera, float aspect_ratio)
{
    return bench_mat4_perspective(camera->fov_y_radians, aspect_ratio, camera->near_plane, camera->far_plane);
}
