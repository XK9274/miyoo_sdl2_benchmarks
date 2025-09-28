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

# Profiler & Bench Mode
- [ ] Implement common profiler that records per-scene metrics (avg/min/max FPS, frame time, draw calls) and outputs structured data.
- [ ] Add unified `--bench` launch flag respected by all benchmarks (disable input, auto-cycle scenes, collate metrics).
- [ ] Extend render suite auto-bench output to emit per-scene JSON/CSV for the profiler consumer.
- [ ] Update software/double-buffer benchmarks to honour bench flag (auto-run stress levels, collect metrics, ignore manual input).
- [ ] Mark space game and audio bench as interactive: if bench flag is set, skip launching them and log "interactive test skipped".
- [ ] Ensure launch scripts understand interactive vs automated entries and report combined summary at the end.
