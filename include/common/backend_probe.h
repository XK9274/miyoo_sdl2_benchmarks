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

/* This process's CPU usage since the previous call to this function, from
 * /proc/self/stat utime+stime summed across all its threads -- can exceed
 * 100% when multiple threads run concurrently on separate cores. Returns 0
 * on the first call. */
float bench_backend_probe_cpu_percent(void);

/* This process's resident memory as a percentage of total system RAM
 * (SDL_GetSystemRAM), from /proc/self/status VmRSS. */
float bench_backend_probe_ram_percent(void);

/* This process's resident memory in MB, from /proc/self/status VmRSS. */
float bench_backend_probe_ram_used_mb(void);

/* Reads /proc/self/status once, deriving any of RAM%, RAM MB, and thread
 * count a caller wants from it -- pass NULL for outputs not needed. Cheaper
 * than calling the individual probes above when more than one is wanted for
 * the same sample. */
void bench_backend_probe_memory_and_threads(float *out_ram_percent, float *out_ram_used_mb,
                                             Uint32 *out_thread_count);

#endif /* COMMON_BACKEND_PROBE_H */
