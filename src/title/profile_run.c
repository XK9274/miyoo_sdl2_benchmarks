#define _POSIX_C_SOURCE 200809L

#include "title/profile_run.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "common/profile_capture.h"

/* Suites with no per-frame loop to time (dialog probes, etc). Add a bin_name
 * here to keep a future non-benchmark suite out of the profiler entirely. */
static const char *const kProfilerExcludedBins[] = {
    "sdl2_messagebox_probe",
    "sdl2_space_bench",
};

SDL_bool title_entry_is_profiler_excluded(const TitleSuiteEntry *entry)
{
    if (!entry || !entry->bin_name) {
        return SDL_TRUE;
    }
    for (size_t i = 0; i < SDL_arraysize(kProfilerExcludedBins); i++) {
        if (strcmp(entry->bin_name, kProfilerExcludedBins[i]) == 0) {
            return SDL_TRUE;
        }
    }
    return SDL_FALSE;
}

int title_profile_flatten_entries(const TitleState *state, TitleProfileQueueItem *out, int max)
{
    if (!state) {
        return 0;
    }
    int count = 0;
    for (int c = 0; c < TITLE_CATEGORY_COUNT - 1; c++) {
        const TitleCategory *category = &state->categories[c];
        for (int e = 0; e < category->entry_count; e++) {
            if (title_entry_is_profiler_excluded(&category->entries[e])) {
                continue;
            }
            if (out && count < max) {
                out[count].category = c;
                out[count].entry = e;
            }
            count++;
        }
    }
    return count;
}

static SDL_bool title_profile_mkdir_p(const char *path)
{
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s", path);
    for (char *p = buf + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(buf, 0755);
            *p = '/';
        }
    }
    return (mkdir(buf, 0755) == 0 || errno == EEXIST) ? SDL_TRUE : SDL_FALSE;
}

static void title_profile_run_dir(const TitleState *state, char *out_dir, size_t out_size)
{
    char bin_dir[PATH_MAX];
    if (!title_get_bin_dir(bin_dir, sizeof(bin_dir))) {
        out_dir[0] = '\0';
        return;
    }
    snprintf(out_dir, out_size, "%s/../logs/profile/%s", bin_dir, state->profile_run_id);
}

SDL_bool title_profile_run_begin(TitleState *state)
{
    if (!state) {
        return SDL_FALSE;
    }

    TitleProfileQueueItem flat[TITLE_PROFILE_MAX_QUEUE];
    const int total = title_profile_flatten_entries(state, flat, TITLE_PROFILE_MAX_QUEUE);

    state->profile_queue_count = 0;
    for (int i = 0; i < total; i++) {
        if (state->profile_selected[flat[i].category][flat[i].entry]) {
            state->profile_queue[state->profile_queue_count++] = flat[i];
        }
    }
    state->profile_queue_index = 0;
    if (state->profile_queue_count == 0) {
        return SDL_FALSE;
    }

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    strftime(state->profile_run_id, sizeof(state->profile_run_id), "%Y%m%d_%H%M%S", &tm_now);

    char run_dir[PATH_MAX];
    title_profile_run_dir(state, run_dir, sizeof(run_dir));
    if (run_dir[0] == '\0' || !title_profile_mkdir_p(run_dir)) {
        return SDL_FALSE;
    }

    char manifest_path[PATH_MAX];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.csv", run_dir);
    FILE *manifest = fopen(manifest_path, "w");
    if (!manifest) {
        return SDL_FALSE;
    }
    fprintf(manifest, "tag,category,entry_label,bin_name,duration_s,csv_file\n");
    for (int i = 0; i < state->profile_queue_count; i++) {
        const TitleProfileQueueItem *item = &state->profile_queue[i];
        const TitleCategory *category = &state->categories[item->category];
        const TitleSuiteEntry *entry = &category->entries[item->entry];
        fprintf(manifest, "c%d_e%d,%s,%s,%s,%d,c%d_e%d.csv\n",
                item->category, item->entry, category->label, entry->label, entry->bin_name,
                state->profile_duration_s, item->category, item->entry);
    }
    fclose(manifest);

    return SDL_TRUE;
}

SDL_bool title_profile_run_step(TitleState *state, TitleContext *ctx)
{
    if (!state || !ctx || state->profile_queue_index >= state->profile_queue_count) {
        return SDL_FALSE;
    }

    const TitleProfileQueueItem *item = &state->profile_queue[state->profile_queue_index];
    const TitleSuiteEntry *entry = &state->categories[item->category].entries[item->entry];

    char run_dir[PATH_MAX];
    title_profile_run_dir(state, run_dir, sizeof(run_dir));

    char tag[32];
    snprintf(tag, sizeof(tag), "c%d_e%d", item->category, item->entry);

    char output_path[PATH_MAX];
    char run_log_path[PATH_MAX];
    snprintf(output_path, sizeof(output_path), "%s/%s.csv", run_dir, tag);
    snprintf(run_log_path, sizeof(run_log_path), "%s/run.log", run_dir);

    TitleProfileLaunchParams params;
    params.tag = tag;
    params.duration_s = state->profile_duration_s;
    params.output_path = output_path;

    TitleLaunchResult result;
    const SDL_bool reinit_ok = title_launch_suite_ex(state, entry, ctx, &params, &result);

    FILE *log = fopen(run_log_path, "a");
    if (log) {
        fprintf(log, "tag=%s exit_code=%d crashed=%d signal=%d exec_failed=%d\n",
                tag, result.exit_code, result.crashed, result.signal_number, result.exec_failed);
        fclose(log);
    }

    state->profile_queue_index++;
    return reinit_ok;
}

const TitleSuiteEntry *title_profile_run_current_entry(const TitleState *state)
{
    if (!state || state->profile_queue_index < 0 || state->profile_queue_index >= state->profile_queue_count) {
        return NULL;
    }
    const TitleProfileQueueItem *item = &state->profile_queue[state->profile_queue_index];
    return &state->categories[item->category].entries[item->entry];
}
