#ifndef COMMON_BACKEND_PROBE_H
#define COMMON_BACKEND_PROBE_H

#include <SDL2/SDL.h>

/* System-wide MMA heap accounting, read fresh each call from
 * /proc/mi_modules/mi_sys_mma/mma_heap_name0. Leaves both outputs at 0 if
 * the file isn't present (e.g. running off-device). */
void bench_backend_probe_mma_pool(Uint32 *out_used_bytes, Uint32 *out_max_bytes);

/* Live OS thread count for this process, from /proc/self/status. Returns 0
 * if the file can't be read. */
Uint32 bench_backend_probe_thread_count(void);

/* Current CPU clock speed in MHz from cpufreq sysfs, or 0 if unavailable. */
Uint32 bench_backend_probe_cpu_freq_mhz(void);

/* This process's CPU usage (% of one core) since the previous call to this
 * function, from /proc/self/stat. Returns 0 on the first call. */
float bench_backend_probe_cpu_percent(void);

/* This process's resident memory as a percentage of total system RAM
 * (SDL_GetSystemRAM), from /proc/self/status VmRSS. */
float bench_backend_probe_ram_percent(void);

#endif /* COMMON_BACKEND_PROBE_H */
