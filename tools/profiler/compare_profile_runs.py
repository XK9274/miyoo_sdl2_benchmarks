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
    args = parser.parse_args()

    baseline = load_profile_run(args.baseline_run_dir).by_tag()
    candidate = load_profile_run(args.candidate_run_dir).by_tag()

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

        status = "ok"
        if delta_pct <= -args.threshold_pct:
            status = "REGRESSION"
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
