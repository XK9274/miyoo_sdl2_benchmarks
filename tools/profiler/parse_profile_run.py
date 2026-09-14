#!/usr/bin/env python3
"""Summarize a profiler run directory pulled off the Miyoo device.

Usage: parse_profile_run.py <run_dir>
"""

import argparse
import sys
from pathlib import Path

from profile_run import load_profile_run


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", type=Path)
    args = parser.parse_args()

    run = load_profile_run(args.run_dir)

    header = f"{'tag':<10} {'entry':<32} {'samples':>7} {'avg fps':>8} {'min fps':>8} {'max fps':>8} {'max ms':>7}  status"
    print(f"Run: {run.run_dir}")
    print(header)
    print("-" * len(header))

    had_failure = False
    for entry in sorted(run.entries, key=lambda e: e.tag):
        status = "ok"
        if entry.run_log is None:
            status = "no run.log entry"
            had_failure = True
        elif entry.run_log.crashed:
            status = f"crashed (signal {entry.run_log.signal})"
            had_failure = True
        elif entry.run_log.exec_failed:
            status = "exec failed"
            had_failure = True
        elif entry.run_log.exit_code != 0:
            status = f"exit {entry.run_log.exit_code}"
            had_failure = True
        elif entry.sample_count == 0:
            status = "no samples"
            had_failure = True

        print(
            f"{entry.tag:<10} {entry.entry_label[:32]:<32} {entry.sample_count:>7} "
            f"{entry.avg_fps:>8.2f} {entry.min_fps:>8.2f} {entry.max_fps:>8.2f} "
            f"{entry.max_frame_ms:>7.2f}  {status}"
        )

    return 1 if had_failure else 0


if __name__ == "__main__":
    sys.exit(main())
