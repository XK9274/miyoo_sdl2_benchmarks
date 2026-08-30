#include "common/math3d/mat4.h"

#include <math.h>

Mat4 bench_mat4_identity(void)
{
    Mat4 r;
    for (int i = 0; i < 16; i++) {
        r.m[i] = 0.0f;
    }
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4 bench_mat4_multiply(const Mat4 *a, const Mat4 *b)
{
    Mat4 r;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a->m[row * 4 + k] * b->m[k * 4 + col];
            }
            r.m[row * 4 + col] = sum;
        }
    }
    return r;
}

Mat4 bench_mat4_perspective(float fov_y_radians, float aspect, float near_plane, float far_plane)
{
    Mat4 r = bench_mat4_identity();
    const float f = 1.0f / tanf(fov_y_radians * 0.5f);
    const float range_inv = 1.0f / (near_plane - far_plane);

    for (int i = 0; i < 16; i++) {
        r.m[i] = 0.0f;
    }
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far_plane + near_plane) * range_inv;
    r.m[11] = 2.0f * far_plane * near_plane * range_inv;
    r.m[14] = -1.0f;
    return r;
}

Mat4 bench_mat4_look_at(Vec3 eye, Vec3 target, Vec3 up)
{
    const Vec3 forward = bench_vec3_normalize(bench_vec3_sub(target, eye));
    const Vec3 right = bench_vec3_normalize(bench_vec3_cross(forward, up));
    const Vec3 camera_up = bench_vec3_cross(right, forward);

    Mat4 r = bench_mat4_identity();
    r.m[0] = right.x;    r.m[1] = right.y;    r.m[2] = right.z;    r.m[3] = -bench_vec3_dot(right, eye);
    r.m[4] = camera_up.x; r.m[5] = camera_up.y; r.m[6] = camera_up.z; r.m[7] = -bench_vec3_dot(camera_up, eye);
    r.m[8] = -forward.x;  r.m[9] = -forward.y;  r.m[10] = -forward.z; r.m[11] = bench_vec3_dot(forward, eye);
    return r;
}

Vec4 bench_mat4_transform_point(const Mat4 *m, Vec3 p)
{
    Vec4 r;
    r.x = m->m[0] * p.x + m->m[1] * p.y + m->m[2] * p.z + m->m[3];
    r.y = m->m[4] * p.x + m->m[5] * p.y + m->m[6] * p.z + m->m[7];
    r.z = m->m[8] * p.x + m->m[9] * p.y + m->m[10] * p.z + m->m[11];
    r.w = m->m[12] * p.x + m->m[13] * p.y + m->m[14] * p.z + m->m[15];
    return r;
}

Vec3 bench_mat4_transform_vector(const Mat4 *m, Vec3 v)
{
    Vec3 r;
    r.x = m->m[0] * v.x + m->m[1] * v.y + m->m[2] * v.z;
    r.y = m->m[4] * v.x + m->m[5] * v.y + m->m[6] * v.z;
    r.z = m->m[8] * v.x + m->m[9] * v.y + m->m[10] * v.z;
    return r;
}
