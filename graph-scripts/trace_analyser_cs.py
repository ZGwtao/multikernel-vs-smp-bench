#!/usr/bin/env python3
"""
seL4 microkernel critical section overhead analysis script

Tracepoint definitions:
  id=0: seL4_Call      critical section overhead
  id=1: seL4_ReplyRecv critical section overhead
"""

import re
import sys
import argparse
from pathlib import Path


# Match lines of the form: ppc*_client: tracepoint id = 0x... \t duration = 0x...
LOG_PATTERN = re.compile(
    r"ppc(?:_\w+)?_client:\s+tracepoint\s+id\s*=\s*(0x[0-9a-fA-F]+)\s+"
    r"duration\s*=\s*(0x[0-9a-fA-F]+)"
)

ID_LABELS = {
    0: "seL4_Call      (id=0)",
    1: "seL4_ReplyRecv (id=1)",
}


def parse_log(text: str) -> list[tuple[int, int]]:
    """Extract a list of (id, duration) tuples from raw log text."""
    entries = []
    for m in LOG_PATTERN.finditer(text):
        tp_id    = int(m.group(1), 16)
        duration = int(m.group(2), 16)
        entries.append((tp_id, duration))
    return entries


def group_entries(entries: list[tuple[int, int]]) -> dict[int, list[int]]:
    """Group duration values by tracepoint id."""
    groups: dict[int, list[int]] = {}
    for tp_id, duration in entries:
        groups.setdefault(tp_id, []).append(duration)
    return groups


def stats(values: list[int]) -> dict:
    if not values:
        return {"count": 0}
    n        = len(values)
    total    = sum(values)
    mean     = total / n
    sorted_v = sorted(values)
    mid      = n // 2
    median   = sorted_v[mid] if n % 2 else (sorted_v[mid - 1] + sorted_v[mid]) / 2
    variance = sum((x - mean) ** 2 for x in values) / n
    std      = variance ** 0.5
    return {
        "count":  n,
        "min":    sorted_v[0],
        "max":    sorted_v[-1],
        "mean":   mean,
        "median": median,
        "std":    std,
        "total":  total,
    }


def print_stats(label: str, values: list[int]):
    print(f"\n{'=' * 60}")
    print(f"  {label}  (samples: {len(values)})")
    print(f"{'=' * 60}")
    if not values:
        print("  No valid samples.")
        return

    s = stats(values)
    print(f"    count  : {s['count']}")
    print(f"    min    : {s['min']}")
    print(f"    max    : {s['max']}")
    print(f"    mean   : {s['mean']:.2f}")
    print(f"    median : {s['median']:.1f}")
    print(f"    std    : {s['std']:.2f}")
    print(f"    total  : {s['total']}")

    # Per-sample detail table (only when sample count is manageable)
    if len(values) <= 500:
        print(f"\n  {'#':>4}  {'duration':>12}")
        print(f"  {'-'*4}  {'-'*12}")
        for i, v in enumerate(values, 1):
            print(f"  {i:>4}  {v:>12}")
    else:
        print(f"\n  (More than 500 samples — detail table skipped)")


def main():
    parser = argparse.ArgumentParser(
        description="seL4 microkernel critical section overhead analysis"
    )
    parser.add_argument(
        "logfile",
        nargs="?",
        help="Path to the log file (reads from stdin if omitted)"
    )
    parser.add_argument(
        "--show-unknown",
        action="store_true",
        help="Print entries with unrecognised tracepoint ids"
    )
    args = parser.parse_args()

    if args.logfile:
        text = Path(args.logfile).read_text(encoding="utf-8", errors="replace")
    else:
        text = sys.stdin.read()

    entries = parse_log(text)
    print(f"Parsed {len(entries)} tracepoint entries.")

    groups  = group_entries(entries)
    unknown = {k: v for k, v in groups.items() if k not in ID_LABELS}

    for tp_id, label in sorted(ID_LABELS.items()):
        print_stats(label, groups.get(tp_id, []))

    if unknown:
        print(f"\n  ⚠  Entries with unrecognised ids: "
              f"{sum(len(v) for v in unknown.values())}")
        if args.show_unknown:
            for tp_id, vals in sorted(unknown.items()):
                for v in vals:
                    print(f"    id={tp_id}  duration={v:#x} ({v})")
    else:
        print("\n  ✓ All entries matched known tracepoint ids.")


if __name__ == "__main__":
    main()