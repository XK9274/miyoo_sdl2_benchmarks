#!/usr/bin/env python3
"""Diff two profiler run directories and flag fps regressions.

Usage: compare_profile_runs.py <baseline_run_dir> <candidate_run_dir> [--threshold-pct N]

Exits non-zero if any matched entry's avg fps dropped by more than the
threshold, so this can gate a manual before/after check while iterating on
the SDL2 backend.
"""

import argparse
import sys
from pathlib import Path

from profile_run import load_profile_run


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline_run_dir", type=Path)
    parser.add_argument("candidate_run_dir", type=Path)
    parser.add_argument("--threshold-pct", type=float, default=5.0,
                        help="flag an entry when avg fps drops by at least this percent (default 5.0)")
    parser.add_argument("--cpu-freq-threshold-pct", type=float, default=5.0,
                        help="flag an entry when min_cpu_freq_mhz drops by at least this percent (default 5.0)")
    parser.add_argument("--ram-growth-threshold-pct", type=float, default=5.0,
                        help="flag an entry when peak_ram_mb grows by at least this percent (default 5.0)")
    args = parser.parse_args()

    baseline_run = load_profile_run(args.baseline_run_dir)
    candidate_run = load_profile_run(args.candidate_run_dir)
    baseline = baseline_run.by_tag()
    candidate = candidate_run.by_tag()

    if (baseline_run.debug_build is not None and candidate_run.debug_build is not None
            and baseline_run.debug_build != candidate_run.debug_build):
        print("WARNING: comparing a debug build against a release build -- "
              "-Og vs -O2 codegen affects raw fps too, not just driver_* stats\n")

    common_tags = sorted(set(baseline) & set(candidate))
    only_baseline = sorted(set(baseline) - set(candidate))
    only_candidate = sorted(set(candidate) - set(baseline))

    header = f"{'tag':<10} {'entry':<32} {'baseline fps':>13} {'candidate fps':>14} {'delta %':>9}  status"
    print(header)
    print("-" * len(header))

    regressions = 0
    for tag in common_tags:
        b = baseline[tag]
        c = candidate[tag]
        if b.avg_fps <= 0:
            delta_pct = 0.0
        else:
            delta_pct = (c.avg_fps - b.avg_fps) / b.avg_fps * 100.0

        flags = []
        if delta_pct <= -args.threshold_pct:
            flags.append("FPS REGRESSION")

        if b.min_cpu_freq_mhz > 0 and c.min_cpu_freq_mhz > 0:
            freq_delta_pct = (c.min_cpu_freq_mhz - b.min_cpu_freq_mhz) / b.min_cpu_freq_mhz * 100.0
            if freq_delta_pct <= -args.cpu_freq_threshold_pct:
                flags.append("THROTTLE REGRESSION")

        if b.peak_ram_mb > 0:
            ram_delta_pct = (c.peak_ram_mb - b.peak_ram_mb) / b.peak_ram_mb * 100.0
            if ram_delta_pct >= args.ram_growth_threshold_pct:
                flags.append("RAM GROWTH")

        status = ", ".join(flags) if flags else "ok"
        if flags:
            regressions += 1

        print(
            f"{tag:<10} {c.entry_label[:32]:<32} {b.avg_fps:>13.2f} {c.avg_fps:>14.2f} "
            f"{delta_pct:>8.1f}%  {status}"
        )

    if only_baseline:
        print(f"\nOnly in baseline run: {', '.join(only_baseline)}")
    if only_candidate:
        print(f"Only in candidate run: {', '.join(only_candidate)}")

    print(f"\n{regressions} regression(s) at >= {args.threshold_pct}% avg fps drop")
    return 1 if regressions else 0


if __name__ == "__main__":
    sys.exit(main())
