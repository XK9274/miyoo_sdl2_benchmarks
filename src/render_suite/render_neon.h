#ifndef RENDER_SUITE_RENDER_NEON_H
#define RENDER_SUITE_RENDER_NEON_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define RS_HAS_NEON 1
#else
#define RS_HAS_NEON 0
#endif

static inline void rs_neon_copy_u32(uint32_t *dst, const uint32_t *src, size_t count)
{
#if RS_HAS_NEON
    size_t i = 0;
    for (; i + 16 <= count; i += 16) {
        uint32x4_t v0 = vld1q_u32(src + i);
        uint32x4_t v1 = vld1q_u32(src + i + 4);
        uint32x4_t v2 = vld1q_u32(src + i + 8);
        uint32x4_t v3 = vld1q_u32(src + i + 12);
        vst1q_u32(dst + i, v0);
        vst1q_u32(dst + i + 4, v1);
        vst1q_u32(dst + i + 8, v2);
        vst1q_u32(dst + i + 12, v3);
    }
    for (; i + 4 <= count; i += 4) {
        uint32x4_t v = vld1q_u32(src + i);
        vst1q_u32(dst + i, v);
    }
    for (; i < count; ++i) {
        dst[i] = src[i];
    }
#else
    if (count) {
        memcpy(dst, src, count * sizeof(uint32_t));
    }
#endif
}

static inline void rs_neon_fill_u32(uint32_t *dst, uint32_t value, size_t count)
{
#if RS_HAS_NEON
    size_t i = 0;
    uint32x4_t v = vdupq_n_u32(value);
    for (; i + 4 <= count; i += 4) {
        vst1q_u32(dst + i, v);
    }
    for (; i < count; ++i) {
        dst[i] = value;
    }
#else
    for (size_t i = 0; i < count; ++i) {
        dst[i] = value;
    }
#endif
}

#endif /* RENDER_SUITE_RENDER_NEON_H */
