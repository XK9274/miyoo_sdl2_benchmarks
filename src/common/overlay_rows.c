#include "common/overlay_rows.h"
#include "common/overlay_internal.h"

#include <stdio.h>
#include <string.h>

#include "common/backend_probe.h"
#include "common/display_config.h"
#include "common/memory_opt.h"

void bench_overlay_configure(BenchOverlay *overlay,
                              const OverlayRowSpec *rows, int row_count,
                              const OverlayKeybind *keybinds, int keybind_count)
{
    if (!overlay) {
        return;
    }

    SDL_LockMutex(overlay->mutex);

    overlay->configured_row_count = SDL_clamp(row_count, 0, OVERLAY_ROWS_MAX);
    if (overlay->configured_row_count > 0) {
        rs_memcpy(overlay->configured_rows, rows,
                  (size_t)overlay->configured_row_count * sizeof(OverlayRowSpec));
    }

    overlay->configured_keybind_count = SDL_clamp(keybind_count, 0, OVERLAY_KEYBINDS_MAX);
    if (overlay->configured_keybind_count > 0) {
        rs_memcpy(overlay->configured_keybinds, keybinds,
                  (size_t)overlay->configured_keybind_count * sizeof(OverlayKeybind));
    }

    if (!overlay->row_registry_configured) {
        overlay->row_registry_configured = SDL_TRUE;
        overlay->width = bench_logical_w() / 3;
        overlay->height = bench_logical_h();
    }

    overlay->dirty = SDL_TRUE;
    SDL_CondSignal(overlay->cond);
    SDL_UnlockMutex(overlay->mutex);
}

void bench_overlay_update(BenchOverlay *overlay, const BenchMetrics *metrics,
                          const char *const *custom_values, int custom_value_count)
{
    if (!overlay || !metrics) {
        return;
    }

    SDL_LockMutex(overlay->mutex);

    overlay->latest_metrics = *metrics;
    overlay->has_metrics = SDL_TRUE;

    overlay->custom_value_count = SDL_clamp(custom_value_count, 0, OVERLAY_ROWS_MAX);
    for (int i = 0; i < overlay->custom_value_count; ++i) {
        const char *value = (custom_values && custom_values[i]) ? custom_values[i] : "";
        strncpy(overlay->custom_values[i], value, OVERLAY_CUSTOM_VALUE_LEN - 1);
        overlay->custom_values[i][OVERLAY_CUSTOM_VALUE_LEN - 1] = '\0';
    }

    rolling_chart_push(&overlay->fps_chart, (float)metrics->current_fps);
    rolling_chart_push(&overlay->frametime_chart, (float)metrics->frame_time_ms);

#ifdef DEBUG_BUILD
    SDL_bool needs_debug_stats = SDL_FALSE;
    for (int i = 0; i < overlay->configured_row_count; ++i) {
        switch (overlay->configured_rows[i].kind) {
            case OVERLAY_ROW_CMDQUEUE_TIME:
            case OVERLAY_ROW_PRESENT_TIME:
            case OVERLAY_ROW_FILL_TIME:
            case OVERLAY_ROW_COPY_TIME:
            case OVERLAY_ROW_GEOMETRY_TIME:
            case OVERLAY_ROW_LINES_TIME:
            case OVERLAY_ROW_MISC_TIME:
            case OVERLAY_ROW_GEOMETRY_STATS:
                needs_debug_stats = SDL_TRUE;
                break;
            default:
                break;
        }
    }
    if (needs_debug_stats) {
        overlay_debug_stats_poll(overlay->renderer, &overlay->debug_stats);
    }
#endif

    overlay->dirty = SDL_TRUE;
    SDL_CondSignal(overlay->cond);
    SDL_UnlockMutex(overlay->mutex);
}

static SDL_bool overlay_rows_format_standard(OverlayRowKind kind, const BenchMetrics *metrics,
                                             const OverlayDebugStats *dbg, char *buf, size_t buf_len)
{
#ifndef DEBUG_BUILD
    (void)dbg;
#endif
    switch (kind) {
        case OVERLAY_ROW_FPS:
            snprintf(buf, buf_len, "FPS %.1f", metrics->current_fps);
            return SDL_TRUE;
        case OVERLAY_ROW_FRAME_TIME:
            snprintf(buf, buf_len, "Frame %.2f ms", metrics->frame_time_ms);
            return SDL_TRUE;
        case OVERLAY_ROW_DRAW_CALLS:
            snprintf(buf, buf_len, "Draws %llu", (unsigned long long)metrics->draw_calls);
            return SDL_TRUE;
        case OVERLAY_ROW_VERTICES:
            snprintf(buf, buf_len, "Verts %llu", (unsigned long long)metrics->vertices_rendered);
            return SDL_TRUE;
        case OVERLAY_ROW_TRIANGLES:
            snprintf(buf, buf_len, "Tris %llu", (unsigned long long)metrics->triangles_rendered);
            return SDL_TRUE;
        case OVERLAY_ROW_TEXTURE_SWITCHES:
            snprintf(buf, buf_len, "Tex swaps %llu", (unsigned long long)metrics->texture_switches);
            return SDL_TRUE;
        case OVERLAY_ROW_CPU_PERCENT:
            snprintf(buf, buf_len, "CPU %.0f%%", (double)bench_backend_probe_cpu_percent());
            return SDL_TRUE;
        case OVERLAY_ROW_RAM_USAGE:
            snprintf(buf, buf_len, "RAM %.1f%%", (double)bench_backend_probe_ram_percent());
            return SDL_TRUE;
        case OVERLAY_ROW_MMA_USAGE: {
            Uint32 used = 0, max = 0;
            bench_backend_probe_mma_pool(&used, &max);
            snprintf(buf, buf_len, "MMA %.1f / %.1f MB", used / (1024.0 * 1024.0), max / (1024.0 * 1024.0));
            return SDL_TRUE;
        }
        case OVERLAY_ROW_MEMORY:
            snprintf(buf, buf_len, "Mem %.1f/%.1fMB . Allocs %llu",
                     metrics->memory_allocated_bytes / (1024.0 * 1024.0),
                     metrics->memory_peak_bytes / (1024.0 * 1024.0),
                     (unsigned long long)metrics->resource_allocations);
            return SDL_TRUE;
        case OVERLAY_ROW_RESOURCE_OPS:
            snprintf(buf, buf_len, "Scale %llu . Pixel %llu ops",
                     (unsigned long long)metrics->scaling_operations,
                     (unsigned long long)metrics->pixel_operations);
            return SDL_TRUE;
        case OVERLAY_ROW_TIMING_OVERHEAD:
            if (metrics->lock_unlock_overhead_ms <= 0.0 && metrics->scaling_overhead_ms <= 0.0 &&
                metrics->allocation_time_ms <= 0.0) {
                return SDL_FALSE;
            }
            snprintf(buf, buf_len, "Lock %.2f Scale %.2f Alloc %.2fms",
                     metrics->lock_unlock_overhead_ms, metrics->scaling_overhead_ms,
                     metrics->allocation_time_ms);
            return SDL_TRUE;
#ifdef DEBUG_BUILD
        case OVERLAY_ROW_CMDQUEUE_TIME:
            if (!dbg->have_timing) return SDL_FALSE;
            snprintf(buf, buf_len, "CmdQ %.2fms", dbg->cmdqueue_ms);
            return SDL_TRUE;
        case OVERLAY_ROW_PRESENT_TIME:
            if (!dbg->have_timing) return SDL_FALSE;
            snprintf(buf, buf_len, "Present %.2fms", dbg->present_ms);
            return SDL_TRUE;
        case OVERLAY_ROW_FILL_TIME:
            if (!dbg->have_timing) return SDL_FALSE;
            snprintf(buf, buf_len, "Fill %.2fms", dbg->fill_ms);
            return SDL_TRUE;
        case OVERLAY_ROW_COPY_TIME:
            if (!dbg->have_timing) return SDL_FALSE;
            snprintf(buf, buf_len, "Copy %.2fms", dbg->copy_ms);
            return SDL_TRUE;
        case OVERLAY_ROW_GEOMETRY_TIME:
            if (!dbg->have_timing) return SDL_FALSE;
            snprintf(buf, buf_len, "Geom %.2fms", dbg->geometry_ms);
            return SDL_TRUE;
        case OVERLAY_ROW_LINES_TIME:
            if (!dbg->have_timing) return SDL_FALSE;
            snprintf(buf, buf_len, "Lines %.2fms", dbg->lines_ms);
            return SDL_TRUE;
        case OVERLAY_ROW_MISC_TIME:
            if (!dbg->have_timing) return SDL_FALSE;
            snprintf(buf, buf_len, "Misc %.2fms", dbg->misc_ms);
            return SDL_TRUE;
        case OVERLAY_ROW_GEOMETRY_STATS:
            if (!dbg->have_geometry) return SDL_FALSE;
            snprintf(buf, buf_len, "Tri %llu Spans %llu Px %llu",
                     (unsigned long long)dbg->triangles, (unsigned long long)dbg->spans,
                     (unsigned long long)dbg->span_pixels);
            return SDL_TRUE;
#else
        case OVERLAY_ROW_CMDQUEUE_TIME:
        case OVERLAY_ROW_PRESENT_TIME:
        case OVERLAY_ROW_FILL_TIME:
        case OVERLAY_ROW_COPY_TIME:
        case OVERLAY_ROW_GEOMETRY_TIME:
        case OVERLAY_ROW_LINES_TIME:
        case OVERLAY_ROW_MISC_TIME:
        case OVERLAY_ROW_GEOMETRY_STATS:
            return SDL_FALSE;
#endif
        case OVERLAY_ROW_CUSTOM:
        default:
            return SDL_FALSE;
    }
}

int overlay_rows_render_data(const BenchOverlay *snap, SDL_Surface *surface, TTF_Font *font,
                             int panel_w, int y, int row_height)
{
    const int text_w = panel_w - 2 * OVERLAY_EDGE_PAD;
    int custom_index = 0;
    char buf[128];

    for (int i = 0; i < snap->configured_row_count; ++i) {
        const OverlayRowSpec *spec = &snap->configured_rows[i];
        const SDL_Rect bounds = {OVERLAY_EDGE_PAD, y, text_w, row_height};

        if (spec->kind == OVERLAY_ROW_CUSTOM) {
            const char *value = (custom_index < snap->custom_value_count) ?
                                 snap->custom_values[custom_index] : "";
            custom_index++;
            if (spec->custom_label) {
                snprintf(buf, sizeof(buf), spec->custom_label, value);
                overlay_draw_text_line(surface, font, bounds, spec->alignment, spec->color, buf);
            }
            y += row_height;
            continue;
        }

        if (overlay_rows_format_standard(spec->kind, &snap->latest_metrics, &snap->debug_stats, buf, sizeof(buf))) {
            overlay_draw_text_line(surface, font, bounds, spec->alignment, spec->color, buf);
        }
        y += row_height;
    }

    return y;
}

int overlay_rows_render_keybinds(const BenchOverlay *snap, SDL_Surface *surface, TTF_Font *font,
                                 int panel_w, int y, int row_height)
{
    if (snap->configured_keybind_count <= 0) {
        return y;
    }

    const SDL_Color divider_color = {255, 255, 255, 36};
    SDL_Rect divider = {OVERLAY_EDGE_PAD, y, panel_w - 2 * OVERLAY_EDGE_PAD, 1};
    SDL_FillRect(surface, &divider, SDL_MapRGBA(surface->format, divider_color.r, divider_color.g,
                                                divider_color.b, divider_color.a));
    y += row_height / 4;

    const SDL_Color key_color = {240, 194, 94, 255};
    const SDL_Color action_color = {201, 198, 188, 255};

    for (int i = 0; i < snap->configured_keybind_count; ++i) {
        const OverlayKeybind *kb = &snap->configured_keybinds[i];
        char line[96];
        snprintf(line, sizeof(line), "%s", kb->label ? kb->label : "");
        const SDL_Rect key_bounds = {OVERLAY_EDGE_PAD, y, 56, row_height};
        overlay_draw_text_line(surface, font, key_bounds, 0, key_color, line);

        const SDL_Rect action_bounds = {OVERLAY_EDGE_PAD + 60, y,
                                        panel_w - 2 * OVERLAY_EDGE_PAD - 60, row_height};
        overlay_draw_text_line(surface, font, action_bounds, 0, action_color, kb->action ? kb->action : "");
        y += row_height;
    }

    return y;
}
