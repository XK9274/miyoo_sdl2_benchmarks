#include "common/math3d/vec3.h"

#include <math.h>

Vec3 bench_vec3_add(Vec3 a, Vec3 b)
{
    Vec3 r = {a.x + b.x, a.y + b.y, a.z + b.z};
    return r;
}

Vec3 bench_vec3_sub(Vec3 a, Vec3 b)
{
    Vec3 r = {a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}

Vec3 bench_vec3_scale(Vec3 v, float s)
{
    Vec3 r = {v.x * s, v.y * s, v.z * s};
    return r;
}

float bench_vec3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 bench_vec3_cross(Vec3 a, Vec3 b)
{
    Vec3 r = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return r;
}

float bench_vec3_length(Vec3 v)
{
    return sqrtf(bench_vec3_dot(v, v));
}

Vec3 bench_vec3_normalize(Vec3 v)
{
    const float len = bench_vec3_length(v);
    if (len < 1e-8f) {
        Vec3 zero = {0.0f, 0.0f, 0.0f};
        return zero;
    }
    return bench_vec3_scale(v, 1.0f / len);
}
