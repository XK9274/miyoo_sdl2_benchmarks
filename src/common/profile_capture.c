#include "common/profile_capture.h"

#include <string.h>

#include "common/backend_probe.h"
#include "common/driver_support.h"
#ifdef DEBUG_BUILD
#include "common/overlay_debug_stats.h"
#endif

void bench_profile_load(BenchProfileCapture *cap)
{
    if (!cap) {
        return;
    }
    memset(cap, 0, sizeof(*cap));

    const char *output_path = SDL_getenv(BENCH_ENV_PROFILE_OUTPUT_PATH);
    if (!output_path || output_path[0] == '\0') {
        return;
    }

    const char *tag = SDL_getenv(BENCH_ENV_PROFILE_TAG);
    SDL_strlcpy(cap->tag, tag ? tag : "untagged", sizeof(cap->tag));

    const char *duration_str = SDL_getenv(BENCH_ENV_PROFILE_DURATION_S);
    cap->duration_s = duration_str ? SDL_atof(duration_str) : BENCH_PROFILE_DEFAULT_DURATION_S;
    if (cap->duration_s <= 0.0) {
        cap->duration_s = BENCH_PROFILE_DEFAULT_DURATION_S;
    }

    cap->file = fopen(output_path, "w");
    if (!cap->file) {
        SDL_Log("bench_profile_load: failed to open '%s' for writing", output_path);
        return;
    }

    fprintf(cap->file,
            "tag,elapsed_s,frame,fps,avg_fps,min_fps,max_fps,frame_ms,min_frame_ms,max_frame_ms,"
            "draw_calls,vertices,triangles,geometry_batches,"
            "mem_alloc_bytes,mem_peak_bytes,resource_allocations,resource_deallocations,allocation_time_ms,"
            "scaling_operations,scaling_overhead_ms,pixel_operations,lock_unlock_overhead_ms,"
            "stage_input_ms,stage_camera_ms,stage_transform_ms,stage_clear_ms,stage_draw_ms,stage_overlay_ms,stage_present_ms,"
            "cpu_percent,ram_percent,ram_used_mb,mma_used_bytes,mma_max_bytes,thread_count,cpu_freq_mhz,"
            "vsync_verified_active,battery_percent,charging,input_source,joystick_event_count,keyboard_event_count,display_w,display_h,"
            "driver_fps,driver_cmdqueue_ms,driver_present_ms,driver_fill_ms,driver_copy_ms,driver_geometry_ms,driver_lines_ms,driver_misc_ms,driver_blits,driver_triangles,driver_spans\n");
    fflush(cap->file);

    cap->next_sample_ms = 0.0;
    cap->active = SDL_TRUE;
}

SDL_bool bench_profile_update(BenchProfileCapture *cap, const BenchMetrics *metrics, SDL_Renderer *renderer)
{
    if (!cap || !cap->active || !metrics) {
        return SDL_FALSE;
    }

    if (metrics->accumulated_frame_time_ms >= cap->next_sample_ms) {
        const float cpu_percent = bench_backend_probe_cpu_percent();
        float ram_percent, ram_used_mb;
        Uint32 thread_count;
        bench_backend_probe_memory_and_threads(&ram_percent, &ram_used_mb, &thread_count);
        Uint32 mma_used_bytes, mma_max_bytes;
        bench_backend_probe_mma_pool(&mma_used_bytes, &mma_max_bytes);
        const Uint32 cpu_freq_mhz = bench_backend_probe_cpu_freq_mhz();

        BenchDriverStatus status;
        bench_driver_get_status(&status);
        const int battery_percent = status.power_info_valid ? status.battery_percent : -1;

        double driver_fps = 0.0, driver_cmdqueue_ms = 0.0, driver_present_ms = 0.0,
               driver_fill_ms = 0.0, driver_copy_ms = 0.0, driver_geometry_ms = 0.0,
               driver_lines_ms = 0.0, driver_misc_ms = 0.0, driver_blits = 0.0;
        Uint64 driver_triangles = 0, driver_spans = 0;
#ifdef DEBUG_BUILD
        OverlayDebugStats dstats;
        SDL_zero(dstats);
        overlay_debug_stats_poll(renderer, &dstats);
        if (dstats.have_timing) {
            driver_fps = dstats.fps;
            driver_cmdqueue_ms = dstats.cmdqueue_ms;
            driver_present_ms = dstats.present_ms;
            driver_fill_ms = dstats.fill_ms;
            driver_copy_ms = dstats.copy_ms;
            driver_geometry_ms = dstats.geometry_ms;
            driver_lines_ms = dstats.lines_ms;
            driver_misc_ms = dstats.misc_ms;
            driver_blits = dstats.blits;
        }
        if (dstats.have_geometry) {
            driver_triangles = dstats.triangles;
            driver_spans = dstats.spans;
        }
#else
        (void)renderer;
#endif

        fprintf(cap->file,
                "%s,%.3f,%llu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,"
                "%llu,%llu,%llu,%llu,"
                "%llu,%llu,%llu,%llu,%.3f,"
                "%llu,%.3f,%llu,%.3f,"
                "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,"
                "%.3f,%.3f,%.3f,%u,%u,%u,%u,"
                "%d,%d,%d,%d,%u,%u,%d,%d,"
                "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%llu,%llu\n",
                cap->tag,
                metrics->accumulated_frame_time_ms / 1000.0,
                (unsigned long long)metrics->frame_count,
                metrics->current_fps,
                metrics->avg_fps,
                metrics->min_fps,
                metrics->max_fps,
                metrics->frame_time_ms,
                metrics->min_frame_time_ms,
                metrics->max_frame_time_ms,
                (unsigned long long)metrics->draw_calls,
                (unsigned long long)metrics->vertices_rendered,
                (unsigned long long)metrics->triangles_rendered,
                (unsigned long long)metrics->geometry_batches,
                (unsigned long long)metrics->memory_allocated_bytes,
                (unsigned long long)metrics->memory_peak_bytes,
                (unsigned long long)metrics->resource_allocations,
                (unsigned long long)metrics->resource_deallocations,
                metrics->allocation_time_ms,
                (unsigned long long)metrics->scaling_operations,
                metrics->scaling_overhead_ms,
                (unsigned long long)metrics->pixel_operations,
                metrics->lock_unlock_overhead_ms,
                metrics->stage_input_ms,
                metrics->stage_camera_ms,
                metrics->stage_transform_ms,
                metrics->stage_clear_ms,
                metrics->stage_draw_ms,
                metrics->stage_overlay_ms,
                metrics->stage_present_ms,
                cpu_percent,
                ram_percent,
                ram_used_mb,
                mma_used_bytes,
                mma_max_bytes,
                thread_count,
                cpu_freq_mhz,
                (int)status.vsync_verified_active,
                battery_percent,
                (int)status.charging,
                (int)status.input_source,
                status.joystick_event_count,
                status.keyboard_event_count,
                status.display_w,
                status.display_h,
                driver_fps,
                driver_cmdqueue_ms,
                driver_present_ms,
                driver_fill_ms,
                driver_copy_ms,
                driver_geometry_ms,
                driver_lines_ms,
                driver_misc_ms,
                driver_blits,
                (unsigned long long)driver_triangles,
                (unsigned long long)driver_spans);
        fflush(cap->file);
        cap->next_sample_ms = metrics->accumulated_frame_time_ms + BENCH_PROFILE_SAMPLE_INTERVAL_MS;
    }

    return metrics->accumulated_frame_time_ms >= cap->duration_s * 1000.0;
}

void bench_profile_shutdown(BenchProfileCapture *cap)
{
    if (!cap || !cap->file) {
        return;
    }
    fclose(cap->file);
    cap->file = NULL;
    cap->active = SDL_FALSE;
}
