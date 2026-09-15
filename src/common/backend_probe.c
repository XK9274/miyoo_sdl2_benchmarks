#define _POSIX_C_SOURCE 200809L

#include "common/backend_probe.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BENCH_MMA_HEAP_PROC "/proc/mi_modules/mi_sys_mma/mma_heap_name0"
#define BENCH_SELF_STATUS_PROC "/proc/self/status"
#define BENCH_SELF_STAT_PROC "/proc/self/stat"

/* cpuclock (no args) reads the SoC's real PLL register via /dev/mem --
 * the standard cpufreq sysfs nodes are capped at this device's stock
 * table max and never reflect an actual overclock above it. */
#define BENCH_CPUCLOCK_BIN "/mnt/SDCARD/.tmp_update/bin/cpuclock"
#define BENCH_CPU_FREQ_REFRESH_MS 2000

void bench_backend_probe_mma_pool(Uint32 *out_used_bytes, Uint32 *out_max_bytes)
{
    if (out_used_bytes) {
        *out_used_bytes = 0;
    }
    if (out_max_bytes) {
        *out_max_bytes = 0;
    }

    FILE *f = fopen(BENCH_MMA_HEAP_PROC, "r");
    if (!f) {
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char name[32];
        unsigned int base = 0, length = 0, avail = 0;
        if (sscanf(line, "%31s %x %x %x", name, &base, &length, &avail) == 4 &&
            strncmp(name, "mma_heap_name", 13) == 0) {
            if (out_max_bytes) {
                *out_max_bytes = length;
            }
            if (out_used_bytes) {
                *out_used_bytes = (length >= avail) ? (length - avail) : 0;
            }
            break;
        }
    }
    fclose(f);
}

Uint32 bench_backend_probe_thread_count(void)
{
    Uint32 threads = 0;
    bench_backend_probe_memory_and_threads(NULL, NULL, &threads);
    return threads;
}

Uint32 bench_backend_probe_cpu_freq_mhz(void)
{
    static Uint32 cached_mhz = 0;
    static Uint32 last_refresh_ms = 0;
    static SDL_bool checked_binary = SDL_FALSE;
    static SDL_bool binary_present = SDL_FALSE;

    if (!checked_binary) {
        checked_binary = SDL_TRUE;
        binary_present = (access(BENCH_CPUCLOCK_BIN, X_OK) == 0);
    }
    if (!binary_present) {
        return 0;
    }

    const Uint32 now_ms = SDL_GetTicks();
    if (cached_mhz != 0 && (now_ms - last_refresh_ms) < BENCH_CPU_FREQ_REFRESH_MS) {
        return cached_mhz;
    }

    FILE *p = popen(BENCH_CPUCLOCK_BIN, "r");
    if (!p) {
        return cached_mhz;
    }
    unsigned int mhz = 0;
    const int scanned = fscanf(p, "%u", &mhz);
    pclose(p);

    if (scanned == 1) {
        cached_mhz = mhz;
        last_refresh_ms = now_ms;
    }
    return cached_mhz;
}

float bench_backend_probe_cpu_percent(void)
{
    static unsigned long prev_ticks = 0;
    static Uint64 prev_counter = 0;

    FILE *f = fopen(BENCH_SELF_STAT_PROC, "r");
    if (!f) {
        return 0.0f;
    }

    unsigned long utime = 0, stime = 0;
    /* Field 14 (utime) / 15 (stime); skip the preceding fields including
     * the parenthesised, possibly-space-containing comm name. */
    const int scanned = fscanf(f,
        "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
        &utime, &stime);
    fclose(f);
    if (scanned != 2) {
        return 0.0f;
    }

    const unsigned long ticks = utime + stime;
    const Uint64 now = SDL_GetPerformanceCounter();

    float percent = 0.0f;
    if (prev_counter != 0) {
        const long clk_tck = sysconf(_SC_CLK_TCK);
        const double elapsed_s = (double)(now - prev_counter) / (double)SDL_GetPerformanceFrequency();
        if (clk_tck > 0 && elapsed_s > 0.0 && ticks >= prev_ticks) {
            const double cpu_s = (double)(ticks - prev_ticks) / (double)clk_tck;
            percent = (float)(cpu_s / elapsed_s * 100.0);
        }
    }

    prev_ticks = ticks;
    prev_counter = now;
    return percent;
}

float bench_backend_probe_ram_percent(void)
{
    float percent = 0.0f;
    bench_backend_probe_memory_and_threads(&percent, NULL, NULL);
    return percent;
}

float bench_backend_probe_ram_used_mb(void)
{
    float mb = 0.0f;
    bench_backend_probe_memory_and_threads(NULL, &mb, NULL);
    return mb;
}

void bench_backend_probe_memory_and_threads(float *out_ram_percent, float *out_ram_used_mb,
                                             Uint32 *out_thread_count)
{
    unsigned long rss_kb = 0;
    unsigned int threads = 0;

    FILE *f = fopen(BENCH_SELF_STATUS_PROC, "r");
    if (f) {
        char line[256];
        SDL_bool have_rss = SDL_FALSE, have_threads = SDL_FALSE;
        while ((!have_rss || !have_threads) && fgets(line, sizeof(line), f)) {
            if (!have_rss && sscanf(line, "VmRSS: %lu kB", &rss_kb) == 1) {
                have_rss = SDL_TRUE;
            } else if (!have_threads && sscanf(line, "Threads: %u", &threads) == 1) {
                have_threads = SDL_TRUE;
            }
        }
        fclose(f);
    }

    if (out_ram_percent) {
        const int total_mb = SDL_GetSystemRAM();
        *out_ram_percent = (total_mb > 0)
            ? (float)((double)rss_kb / ((double)total_mb * 1024.0) * 100.0)
            : 0.0f;
    }
    if (out_ram_used_mb) {
        *out_ram_used_mb = (float)(rss_kb / 1024.0);
    }
    if (out_thread_count) {
        *out_thread_count = threads;
    }
}
