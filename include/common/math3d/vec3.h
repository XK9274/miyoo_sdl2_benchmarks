#ifndef COMMON_MATH3D_VEC3_H
#define COMMON_MATH3D_VEC3_H

/* No SDL2 dependency -- shared with the OBJ/MTL loader, which must stay SDL-free. */

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vec4;

typedef struct {
    float u;
    float v;
} Vec2;

Vec3 bench_vec3_add(Vec3 a, Vec3 b);
Vec3 bench_vec3_sub(Vec3 a, Vec3 b);
Vec3 bench_vec3_scale(Vec3 v, float s);
float bench_vec3_dot(Vec3 a, Vec3 b);
Vec3 bench_vec3_cross(Vec3 a, Vec3 b);
float bench_vec3_length(Vec3 v);

/* Returns (0,0,0) for a zero-length input rather than dividing by zero. */
Vec3 bench_vec3_normalize(Vec3 v);

#endif /* COMMON_MATH3D_VEC3_H */
