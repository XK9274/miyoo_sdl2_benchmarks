#ifndef COMMON_MEMORY_OPT_H
#define COMMON_MEMORY_OPT_H

#include <stddef.h>
#include <string.h>

#ifdef __ARM_NEON
#include <neon.h>
#endif

static inline void *rs_memcpy(void *dst, const void *src, size_t len)
{
#ifdef __ARM_NEON
    return neon_memcpy(dst, src, len);
#else
    return memcpy(dst, src, len);
#endif
}

static inline void rs_memcpy_bytes(void *dst, const void *src, size_t len)
{
    (void)rs_memcpy(dst, src, len);
}

#endif /* COMMON_MEMORY_OPT_H */
