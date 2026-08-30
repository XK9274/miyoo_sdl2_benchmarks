#ifndef COMMON_MATH3D_MAT4_H
#define COMMON_MATH3D_MAT4_H

#include "common/math3d/vec3.h"

/* No SDL2 dependency -- shared with the OBJ/MTL loader, which must stay
 * SDL-free. Row-major storage: m[row*4 + col]; points transform as column
 * vectors (p' = M*p). */
typedef struct {
    float m[16];
} Mat4;

Mat4 bench_mat4_identity(void);
Mat4 bench_mat4_multiply(const Mat4 *a, const Mat4 *b); /* a * b: b applied first */

/* Standard right-handed perspective projection (OpenGL-style clip space). */
Mat4 bench_mat4_perspective(float fov_y_radians, float aspect, float near_plane, float far_plane);

/* Standard right-handed view matrix; camera looks down -Z afterward. */
Mat4 bench_mat4_look_at(Vec3 eye, Vec3 target, Vec3 up);

Vec4 bench_mat4_transform_point(const Mat4 *m, Vec3 p);   /* w = 1 */

/* Ignores translation (w=0); only valid for orthogonal (pure-rotation,
 * no-scale) matrices. */
Vec3 bench_mat4_transform_vector(const Mat4 *m, Vec3 v);

#endif /* COMMON_MATH3D_MAT4_H */
