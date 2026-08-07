# Render Suite Optimisation TODO

- [ ] Profile each render suite scene on target ARMv7 hardware (perf counters, frame time, CPU usage).
- [x] Geometry scene: cache tessellated cube mesh and reuse precomputed rotation data.
- [ ] Geometry scene: migrate vertex rotation and star-field updates to NEON SIMD batches.
- [x] Lines scene: replace per-line `sinf`/`cosf` calls with LUT-backed wave evaluation and trim draw overhead.
- [x] Memory scene: reuse streaming texture buffers, remove per-frame malloc/free, and add NEON-backed upload paths.
- [x] Pixels scene: reuse streaming textures, avoid per-frame creation, and NEON-copy pixel data.
- [ ] Scaling scene: pre-render gradient/shape content to textures and use NEON to build colour ramps.
- [x] Space game: batch anomaly rendering and replace per-point trig with cached geometry.
- [x] Space game: cache enemy hull rotations and reduce draw call count.
- [ ] Texture scene: investigate batching sprite copies and using NEON to animate offsets/colour modulation.
- [x] Integrate NEON intrinsics behind capability checks and provide fallback scalar paths.
- [ ] Add automated performance regression benchmarks for the dual-core device.

# Rendering Bugs
- [ ] Overlay HUD flickers on some suites. Investigated with throttled hot-path diagnostics (`common/overlay.c` present path + `MMIYOO_RenderPresent`) on-device across render_suite (control), double_buf, space_bench, audio_bench:
  - **space_bench (suite 5): root-caused.** `src/space_bench/overlay.c`'s `draw_hud_line` uses its own per-line `HudLineCache` and calls `SDL_DestroyTexture`+`SDL_CreateTexture` every single frame any line's text differs from last frame -- for live stats (FPS/score/etc.) that's every frame. This is real GPU texture churn on the fenced MI_GFX pipeline mid-frame, the likely flicker cause. Common `src/common/overlay.c` avoids this correctly (single streaming texture, `SDL_UpdateTexture`, never destroyed/recreated except on renderer change) -- port space_bench's HUD to the same streaming-texture pattern instead of the destroy/recreate-per-line one.
  - **double_buf (suite 4): no anomaly found.** Texture created once, stable for the whole run, matched the non-flickering control closely. If it's still visibly flickering, the cause isn't in the overlay present path or texture lifecycle -- look elsewhere (scene-side, e.g. `db_render_backdrop`/particle draw ordering).
  - **audio_bench (suite 6): inconclusive, needs a clean retest.** First attempt bypassed `launch.sh`'s audioserver-stop step (ran the binary directly over SSH), so `MI_AO_SetPubAttr` failed and the loop stalled on that instead of exercising the overlay path at all. Also note: running `/mnt/SDCARD/.tmp_update/script/stop_audioserver.sh` standalone over SSH kills the SSH session as a side effect (device itself is fine, reconnects OK) -- always drive audio_bench through `launch.sh` (or the full suite) for testing, never that script in isolation.
  - **Still flickering after the above on a full clean run** -- the space_bench texture-churn finding is real but evidently not the whole story (or not yet fixed/tested in isolation). User's live suspicion: possibly a memory issue rather than (or in addition to) the texture-recreate churn -- worth checking MI_SYS/MMA allocation pressure and fragmentation during setup/steady-state next, not just the overlay's own texture lifecycle. Picking back up later.

# Space Game GL Effects
- [x] Add small, cheap GL-based effects to the space game (plasma bolts/projectiles, pickups, thumper) plus shield effect and thumper effects. Use small FBO sizes to keep it cheap on this hardware. Store the GL setup/context centrally (shared helper, not duplicated per-effect) rather than each effect standing up its own GL state -- reuse the pattern from `render_suite_gl`'s GL context handling where sensible.
  - Implemented via new `src/common/gl_effect.c`/`.h` (shared refcounted GLES2 context + FBO target + shader-compile helper, extracted from `render_suite_gl`'s pattern) and `src/space_bench/gl_effects.c`/`.h` (4 small shaders: bolt/pickup/thumper/shield). Each effect renders once per frame only when something on screen needs it (`SpaceGLEffectsFrameInput` active-flags), blitted many times per-instance via `SDL_SetTextureColorMod`/`AlphaMod` for cheap per-instance colour/alpha variation.
  - An anomaly laser-beam GL effect was also tried (stretched/rotated beam texture) but reverted at user request -- back to the original thin-line + helix rendering in `render/anomaly.c`.
  - Added `space_gl_effects_warmup()`: shader *compilation* alone didn't prevent a first-use stutter (GL drivers defer real work -- FBO bind, first `glReadPixels`/texture upload -- to the first actual draw). Warmup now forces one real draw per effect during the loading screen instead of on whichever gameplay frame first needs it.
  - Loading screen: added `BENCH_LOADING_STYLE_SHIP` to the shared `common/loading_screen.c` (reusable by other benches) -- a slow Z-axis-spinning orange wireframe ship silhouette, using the same bar/percentage/text rendering every other suite's loading screen already uses. `space_bench` shows this while state inits and effects compile/warm up, plus a explicit 2s continuously-presented hold before gameplay starts.

# Profiler & Bench Mode
- [ ] Implement common profiler that records per-scene metrics (avg/min/max FPS, frame time, draw calls) and outputs structured data.
- [ ] Add unified `--bench` launch flag respected by all benchmarks (disable input, auto-cycle scenes, collate metrics).
- [ ] Extend render suite auto-bench output to emit per-scene JSON/CSV for the profiler consumer.
- [ ] Update software/double-buffer benchmarks to honour bench flag (auto-run stress levels, collect metrics, ignore manual input).
- [ ] Mark space game and audio bench as interactive: if bench flag is set, skip launching them and log "interactive test skipped".
- [ ] Ensure launch scripts understand interactive vs automated entries and report combined summary at the end.
