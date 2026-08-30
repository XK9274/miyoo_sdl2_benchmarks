#ifndef COMMON_RENDER3D_CAMERA_H
#define COMMON_RENDER3D_CAMERA_H

#include "common/math3d/mat4.h"
#include "common/math3d/vec3.h"

/* +Y up (matches Blender's OBJ export), right-handed, CCW front-face
 * winding. Camera orbits a fixed target; the model itself never rotates. */

typedef struct {
    Vec3 target;
    float distance;
    float yaw_radians;   /* rotation around +Y; 0 = looking from +Z toward target */
    float pitch_radians; /* clamped to +/-89 degrees */
    float fov_y_radians;
    float near_plane;
    float far_plane;
} Camera3D;

/* distance is clamped to a small positive floor. */
void camera3d_init(Camera3D *camera, Vec3 target, float distance);

/* Call every frame to drive the auto-rotate turntable. */
void camera3d_auto_rotate(Camera3D *camera, float radians_per_second, float dt);

/* Manual joystick/keyboard override; pitch and distance are clamped internally. */
void camera3d_orbit(Camera3D *camera, float yaw_delta_radians, float pitch_delta_radians);
void camera3d_zoom(Camera3D *camera, float distance_delta);

Vec3 camera3d_eye_position(const Camera3D *camera);
Mat4 camera3d_view_matrix(const Camera3D *camera);
Mat4 camera3d_projection_matrix(const Camera3D *camera, float aspect_ratio);

#endif /* COMMON_RENDER3D_CAMERA_H */
