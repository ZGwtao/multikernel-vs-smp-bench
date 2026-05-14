#!/usr/bin/env python3
"""
seL4 microkernel lock overhead analysis script

Tracepoint definitions:
  id=0: seL4_Call      lock-acquire overhead
  id=1: any syscall    lock-release overhead (paired with the immediately preceding entry)
  id=2: seL4_ReplyRecv lock-acquire overhead
"""

import re
import sys
import argparse
from pathlib import Path


# Match lines of the form: ppc_client: tracepoint id = 0x... \t duration = 0x...
# LOG_PATTERN = re.compile(
#     r"ppc_client:\s+tracepoint\s+id\s*=\s*(0x[0-9a-fA-F]+)\s+"
#     r"duration\s*=\s*(0x[0-9a-fA-F]+)"
# )
LOG_PATTERN = re.compile(
    r"ppc(?:_\w+)?_client:\s+tracepoint\s+id\s*=\s*(0x[0-9a-fA-F]+)\s+"
    r"duration\s*=\s*(0x[0-9a-fA-F]+)"
)


def parse_log(text: str) -> list[tuple[int, int]]:
    """Extract a list of (id, duration) tuples from raw log text."""
    entries = []
    for m in LOG_PATTERN.finditer(text):
        tp_id = int(m.group(1), 16)
        duration = int(m.group(2), 16)
        entries.append((tp_id, duration))
    return entries


def pair_entries(entries: list[tuple[int, int]]):
    """
    Scan entries in order and pair each id=1 (lock-release) record with
    the immediately preceding non-id=1 record (lock-acquire).

    Returns:
        sel4_call_pairs      : list of (acquire, release) duration tuples
        sel4_replyrecv_pairs : list of (acquire, release) duration tuples
        unpaired             : list of entries that could not be paired
    """
    sel4_call_pairs = []
    sel4_replyrecv_pairs = []
    unpaired = []

    prev = None  # the last unmatched non-id=1 entry

    for tp_id, duration in entries:
        if tp_id == 1:
            # Lock-release: pair with prev
            if prev is None:
                unpaired.append((tp_id, duration))
            else:
                prev_id, prev_dur = prev
                if prev_id == 0:
                    sel4_call_pairs.append((prev_dur, duration))
                elif prev_id == 2:
                    sel4_replyrecv_pairs.append((prev_dur, duration))
                else:
                    # Consecutive id=1 entries or unexpected id
                    unpaired.append((tp_id, duration))
                prev = None  # consumed
        else:
            # Lock-acquire: if the previous acquire was never paired, mark it unpaired
            if prev is not None:
                unpaired.append(prev)
            prev = (tp_id, duration)

    # Any trailing acquire entry left without a matching release
    if prev is not None:
        unpaired.append(prev)

    return sel4_call_pairs, sel4_replyrecv_pairs, unpaired


def stats(values: list[int]) -> dict:
    if not values:
        return {"count": 0}
    n = len(values)
    total = sum(values)
    mean = total / n
    sorted_v = sorted(values)
    mid = n // 2
    median = sorted_v[mid] if n % 2 else (sorted_v[mid - 1] + sorted_v[mid]) / 2
    variance = sum((x - mean) ** 2 for x in values) / n
    std = variance ** 0.5
    return {
        "count": n,
        "min": sorted_v[0],
        "max": sorted_v[-1],
        "mean": mean,
        "median": median,
        "std": std,
        "total": total,
    }


def print_stats(label: str, pairs: list[tuple[int, int]]):
    print(f"\n{'=' * 60}")
    print(f"  {label}  (samples: {len(pairs)})")
    print(f"{'=' * 60}")
    if not pairs:
        print("  No valid samples.")
        return

    acquires = [a for a, _ in pairs]
    releases = [r for _, r in pairs]
    totals   = [a + r for a, r in pairs]

    for tag, values in [("acquire", acquires),
                        ("release", releases),
                        ("acquire+release", totals)]:
        s = stats(values)
        print(f"\n  [{tag}]")
        print(f"    count  : {s['count']}")
        print(f"    min    : {s['min']}")
        print(f"    max    : {s['max']}")
        print(f"    mean   : {s['mean']:.2f}")
        print(f"    median : {s['median']:.1f}")
        print(f"    std    : {s['std']:.2f}")
        print(f"    total  : {s['total']}")

    # Per-sample detail table (only when sample count is manageable)
    if len(pairs) <= 50:
        print(f"\n  {'#':>4}  {'acquire':>10}  {'release':>10}  {'total':>10}")
        print(f"  {'-'*4}  {'-'*10}  {'-'*10}  {'-'*10}")
        for i, (a, r) in enumerate(pairs, 1):
            print(f"  {i:>4}  {a:>10}  {r:>10}  {a+r:>10}")
    else:
        print(f"\n  (More than 50 samples — detail table skipped)")


def main():
    parser = argparse.ArgumentParser(
        description="seL4 microkernel lock overhead analysis"
    )
    parser.add_argument(
        "logfile",
        nargs="?",
        help="Path to the log file (reads from stdin if omitted)"
    )
    parser.add_argument(
        "--show-unpaired",
        action="store_true",
        help="Print tracepoint entries that could not be paired"
    )
    args = parser.parse_args()

    if args.logfile:
        text = Path(args.logfile).read_text(encoding="utf-8", errors="replace")
    else:
        text = sys.stdin.read()

    entries = parse_log(text)
    print(f"Parsed {len(entries)} tracepoint entries.")

    sel4_call_pairs, sel4_replyrecv_pairs, unpaired = pair_entries(entries)

    print_stats("seL4_Call      (id=0 + id=1)", sel4_call_pairs)
    print_stats("seL4_ReplyRecv (id=2 + id=1)", sel4_replyrecv_pairs)

    if unpaired:
        print(f"\n  ⚠  {len(unpaired)} unpaired entries found")
        if args.show_unpaired:
            for tp_id, dur in unpaired:
                print(f"    id={tp_id}  duration={dur:#x} ({dur})")
    else:
        print("\n  ✓ All entries successfully paired.")


if __name__ == "__main__":
    main()