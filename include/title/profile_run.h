#ifndef TITLE_PROFILE_RUN_H
#define TITLE_PROFILE_RUN_H

#include "title/launcher.h"
#include "title/state.h"

/* True when a suite shouldn't appear in the profiler's selection screen or
 * run queue (e.g. a dialog probe with no per-frame loop to time). */
SDL_bool title_entry_is_profiler_excluded(const TitleSuiteEntry *entry);

/* Flattens every eligible (category, entry) pair -- skipping the trailing
 * Quit category and any profiler-excluded entry -- into out[], in menu
 * order. Returns the count written, capped at max. */
int title_profile_flatten_entries(const TitleState *state, TitleProfileQueueItem *out, int max);

/* Builds the run queue from the current selection, creates the on-device run
 * directory under logs/profile/<run_id>/, and writes its manifest. Returns
 * SDL_FALSE if the directory or manifest couldn't be written. */
SDL_bool title_profile_run_begin(TitleState *state);

/* Launches the queued entry at profile_queue_index (blocking fork/exec/wait,
 * env vars from the profiler contract), appends its outcome to the run log,
 * and advances the index. Returns SDL_FALSE once the queue is exhausted. */
SDL_bool title_profile_run_step(TitleState *state, TitleContext *ctx);

const TitleSuiteEntry *title_profile_run_current_entry(const TitleState *state);

#endif /* TITLE_PROFILE_RUN_H */
