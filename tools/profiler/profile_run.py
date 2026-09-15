"""Shared helpers for reading a profiler run directory pulled off the device.

A run directory (logs/profile/<run_id>/ on the device) contains:
  manifest.csv  -- one row per queued entry: tag,category,entry_label,bin_name,duration_s,csv_file
  run.log       -- one line per completed entry: tag=... exit_code=... crashed=... signal=... exec_failed=...
  <tag>.csv     -- raw per-frame samples for that entry, written by the device; the
                   last row holds that run's final avg/min/max fps and frame time.
"""

from __future__ import annotations

import csv
import re
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class RunLogEntry:
    exit_code: int
    crashed: bool
    signal: int
    exec_failed: bool


@dataclass
class EntrySummary:
    tag: str
    category: str
    entry_label: str
    bin_name: str
    duration_s: int
    sample_count: int
    avg_fps: float
    min_fps: float
    max_fps: float
    min_frame_ms: float
    max_frame_ms: float
    avg_cpu_percent: float
    peak_ram_mb: float
    peak_mma_used_bytes: int
    min_cpu_freq_mhz: float
    run_log: RunLogEntry | None = None


@dataclass
class ProfileRun:
    run_dir: Path
    entries: list[EntrySummary] = field(default_factory=list)
    debug_build: bool | None = None

    def by_tag(self) -> dict[str, EntrySummary]:
        return {e.tag: e for e in self.entries}


def _read_debug_build(run_dir: Path) -> bool | None:
    run_info_path = run_dir / "run_info.csv"
    if not run_info_path.exists():
        return None
    with run_info_path.open(newline="") as f:
        for row in csv.DictReader(f):
            return row.get("debug_build") == "1"
    return None


_RUN_LOG_LINE = re.compile(
    r"tag=(?P<tag>\S+) exit_code=(?P<exit_code>-?\d+) crashed=(?P<crashed>\d+) "
    r"signal=(?P<signal>-?\d+) exec_failed=(?P<exec_failed>\d+)"
)


def _read_run_log(run_dir: Path) -> dict[str, RunLogEntry]:
    log_path = run_dir / "run.log"
    result: dict[str, RunLogEntry] = {}
    if not log_path.exists():
        return result
    for line in log_path.read_text().splitlines():
        m = _RUN_LOG_LINE.match(line.strip())
        if not m:
            continue
        result[m.group("tag")] = RunLogEntry(
            exit_code=int(m.group("exit_code")),
            crashed=m.group("crashed") == "1",
            signal=int(m.group("signal")),
            exec_failed=m.group("exec_failed") == "1",
        )
    return result


def load_profile_run(run_dir: Path) -> ProfileRun:
    manifest_path = run_dir / "manifest.csv"
    if not manifest_path.exists():
        raise FileNotFoundError(f"no manifest.csv in {run_dir}")

    run_log = _read_run_log(run_dir)
    run = ProfileRun(run_dir=run_dir, debug_build=_read_debug_build(run_dir))

    with manifest_path.open(newline="") as f:
        for row in csv.DictReader(f):
            csv_path = run_dir / row["csv_file"]
            sample_count = 0
            avg_fps = min_fps = max_fps = 0.0
            min_frame_ms = max_frame_ms = 0.0
            peak_ram_mb = 0.0
            peak_mma_used_bytes = 0
            cpu_percent_sum = 0.0
            min_cpu_freq_mhz = 0.0
            if csv_path.exists():
                last_row = None
                with csv_path.open(newline="") as cf:
                    for sample in csv.DictReader(cf):
                        sample_count += 1
                        cpu_percent_sum += float(sample["cpu_percent"])
                        peak_ram_mb = max(peak_ram_mb, float(sample["ram_used_mb"]))
                        peak_mma_used_bytes = max(peak_mma_used_bytes, int(sample["mma_used_bytes"]))
                        # 0 means the cpufreq probe was unavailable on that sample,
                        # not a real throttle reading down to 0MHz.
                        sample_cpu_freq = float(sample["cpu_freq_mhz"])
                        if sample_cpu_freq > 0.0 and (min_cpu_freq_mhz == 0.0 or sample_cpu_freq < min_cpu_freq_mhz):
                            min_cpu_freq_mhz = sample_cpu_freq
                        last_row = sample
                if last_row:
                    avg_fps = float(last_row["avg_fps"])
                    min_fps = float(last_row["min_fps"])
                    max_fps = float(last_row["max_fps"])
                    min_frame_ms = float(last_row["min_frame_ms"])
                    max_frame_ms = float(last_row["max_frame_ms"])

            avg_cpu_percent = cpu_percent_sum / sample_count if sample_count else 0.0

            run.entries.append(
                EntrySummary(
                    tag=row["tag"],
                    category=row["category"],
                    entry_label=row["entry_label"],
                    bin_name=row["bin_name"],
                    duration_s=int(row["duration_s"]),
                    sample_count=sample_count,
                    avg_fps=avg_fps,
                    min_fps=min_fps,
                    max_fps=max_fps,
                    min_frame_ms=min_frame_ms,
                    max_frame_ms=max_frame_ms,
                    avg_cpu_percent=avg_cpu_percent,
                    peak_ram_mb=peak_ram_mb,
                    peak_mma_used_bytes=peak_mma_used_bytes,
                    min_cpu_freq_mhz=min_cpu_freq_mhz,
                    run_log=run_log.get(row["tag"]),
                )
            )

    return run
