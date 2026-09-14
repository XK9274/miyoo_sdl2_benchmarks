#include "common/profile_capture.h"

#include <string.h>

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
            "tag,elapsed_s,frame,fps,avg_fps,min_fps,max_fps,frame_ms,draw_calls,vertices,"
            "triangles,mem_alloc_bytes,mem_peak_bytes,resource_allocations,resource_deallocations,"
            "stage_draw_ms,stage_present_ms\n");
    fflush(cap->file);

    cap->next_sample_ms = 0.0;
    cap->active = SDL_TRUE;
}

SDL_bool bench_profile_update(BenchProfileCapture *cap, const BenchMetrics *metrics)
{
    if (!cap || !cap->active || !metrics) {
        return SDL_FALSE;
    }

    if (metrics->accumulated_frame_time_ms >= cap->next_sample_ms) {
        fprintf(cap->file,
                "%s,%.3f,%llu,%.3f,%.3f,%.3f,%.3f,%.3f,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%.3f,%.3f\n",
                cap->tag,
                metrics->accumulated_frame_time_ms / 1000.0,
                (unsigned long long)metrics->frame_count,
                metrics->current_fps,
                metrics->avg_fps,
                metrics->min_fps,
                metrics->max_fps,
                metrics->frame_time_ms,
                (unsigned long long)metrics->draw_calls,
                (unsigned long long)metrics->vertices_rendered,
                (unsigned long long)metrics->triangles_rendered,
                (unsigned long long)metrics->memory_allocated_bytes,
                (unsigned long long)metrics->memory_peak_bytes,
                (unsigned long long)metrics->resource_allocations,
                (unsigned long long)metrics->resource_deallocations,
                metrics->stage_draw_ms,
                metrics->stage_present_ms);
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
