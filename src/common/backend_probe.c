#define _POSIX_C_SOURCE 200809L

#include "common/backend_probe.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BENCH_MMA_HEAP_PROC "/proc/mi_modules/mi_sys_mma/mma_heap_name0"
#define BENCH_SELF_STATUS_PROC "/proc/self/status"
#define BENCH_SELF_STAT_PROC "/proc/self/stat"
#define BENCH_CPU_FREQ_PROC "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"

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
    FILE *f = fopen(BENCH_SELF_STATUS_PROC, "r");
    if (!f) {
        return 0;
    }

    char line[256];
    unsigned int threads = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "Threads: %u", &threads) == 1) {
            break;
        }
    }
    fclose(f);
    return threads;
}

Uint32 bench_backend_probe_cpu_freq_mhz(void)
{
    FILE *f = fopen(BENCH_CPU_FREQ_PROC, "r");
    if (!f) {
        return 0;
    }

    unsigned int khz = 0;
    const int scanned = fscanf(f, "%u", &khz);
    fclose(f);
    return (scanned == 1) ? (khz / 1000) : 0;
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
    FILE *f = fopen(BENCH_SELF_STATUS_PROC, "r");
    if (!f) {
        return 0.0f;
    }

    char line[256];
    unsigned long rss_kb = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmRSS: %lu kB", &rss_kb) == 1) {
            break;
        }
    }
    fclose(f);

    const int total_mb = SDL_GetSystemRAM();
    if (total_mb <= 0) {
        return 0.0f;
    }
    return (float)((double)rss_kb / ((double)total_mb * 1024.0) * 100.0);
}
