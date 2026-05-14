#!/usr/bin/env python3
"""
Compare seL4 fastpath tracepoint breakdown between Unicore and SMP.

This version is intentionally strict about data source validity:
  - It scans the log for complete seL4_Call + seL4_ReplyRecv sequences only.
  - A valid sequence is exactly:
        0..35, 16,17,18,19
    where:
        0..15   = fastpath_call body
        16..19  = switchToThread_fp reached from fastpath_call
        20..35  = fastpath_reply_recv body
        16..19  = switchToThread_fp reached from fastpath_reply_recv
  - Extra 16..19 emitted by the kernel scheduler/preemption path are ignored
    unless they appear at the exact expected position inside a valid sequence.
  - Broken / partial / interleaved candidates are dropped.

Final output:
  - call_mean_comparison.png
  - replyrecv_mean_comparison.png
  - terminal print of overall average Call+ReplyRecv cost for Unicore and SMP
"""

from __future__ import annotations

import argparse
import math
import re
from dataclasses import dataclass
from pathlib import Path
from typing import List, Sequence, Tuple

import matplotlib.pyplot as plt
import numpy as np

TRACE_RE = re.compile(
    r"tracepoint\s+id\s*=\s*(0x[0-9a-fA-F]+|\d+)\s+duration\s*=\s*(0x[0-9a-fA-F]+|\d+)"
)

EXPECTED_IDS: List[int] = list(range(36)) + [16, 17, 18, 19]
CALL_KEYS: List[Tuple[str, int]] = [("call", i) for i in list(range(16)) + [16, 17, 18, 19]]
REPLY_KEYS: List[Tuple[str, int]] = [("replyrecv", i) for i in list(range(20, 36)) + [16, 17, 18, 19]]

CALL_LABELS = {
    0: "0 msgInfo/fault",
    1: "1 mi check",
    2: "2 lookup EP",
    3: "3 get dest",
    4: "4 hw debug",
    5: "5 dest vtable",
    6: "6 ASID map",
    7: "7 prio check",
    8: "8 grant check",
    9: "9 domain/MCS",
    10: "10 affinity",
    11: "11 dequeue dest",
    12: "12 block caller",
    13: "13 donate SC/reply",
    14: "14 copy MRs",
    15: "15 dest running",
    16: "16 vcpu switch",
    17: "17 HW ASID read",
    18: "18 switch rootVSpace",
    19: "19 FPU/current",
}

REPLY_LABELS = {
    20: "20 msgInfo/fault",
    21: "21 lookup EP",
    22: "22 lookup reply",
    23: "23 ntfn/EP state",
    24: "24 validate reply",
    25: "25 caller fault",
    26: "26 caller vtable",
    27: "27 ASID map",
    28: "28 prio/domain/MCS",
    29: "29 affinity",
    30: "30 block receiver",
    31: "31 enqueue EP",
    32: "32 donate SC/reply",
    33: "33 exception path",
    34: "34 copy MRs",
    35: "35 caller running",
    16: "16 vcpu switch",
    17: "17 HW ASID read",
    18: "18 switch rootVSpace",
    19: "19 FPU/current",
}


@dataclass(frozen=True)
class TraceEvent:
    trace_id: int
    cycles: int
    line_no: int


@dataclass(frozen=True)
class ValidSequence:
    events: List[TraceEvent]
    start_line: int
    end_line: int


def parse_int(s: str) -> int:
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def parse_trace_events(path: Path) -> List[TraceEvent]:
    events: List[TraceEvent] = []
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line_no, line in enumerate(f, start=1):
            m = TRACE_RE.search(line)
            if not m:
                continue
            trace_id = parse_int(m.group(1))
            cycles = parse_int(m.group(2))
            events.append(TraceEvent(trace_id=trace_id, cycles=cycles, line_no=line_no))
    return events


def find_valid_sequences(events: Sequence[TraceEvent]) -> Tuple[List[ValidSequence], int, int]:
    seqs: List[ValidSequence] = []
    i = 0
    dropped_candidates = 0

    while i < len(events):
        if events[i].trace_id != EXPECTED_IDS[0]:
            i += 1
            continue

        j = 0
        start = i
        while i + j < len(events) and j < len(EXPECTED_IDS):
            if events[i + j].trace_id != EXPECTED_IDS[j]:
                break
            j += 1

        if j == len(EXPECTED_IDS):
            matched = list(events[start : start + len(EXPECTED_IDS)])
            seqs.append(
                ValidSequence(
                    events=matched,
                    start_line=matched[0].line_no,
                    end_line=matched[-1].line_no,
                )
            )
            i = start + len(EXPECTED_IDS)
        else:
            dropped_candidates += 1
            i = start + 1

    return seqs, dropped_candidates, len(events)


def values_for_keys(seqs: Sequence[ValidSequence], keys: Sequence[Tuple[str, int]]) -> np.ndarray:
    rows: List[List[float]] = []
    for seq in seqs:
        call_pos = {i: i for i in range(16)} | {16: 16, 17: 17, 18: 18, 19: 19}
        reply_pos = {i: i for i in range(20, 36)} | {16: 36, 17: 37, 18: 38, 19: 39}

        row: List[float] = []
        for phase, tid in keys:
            pos = call_pos[tid] if phase == "call" else reply_pos[tid]
            row.append(float(seq.events[pos].cycles))
        rows.append(row)

    if not rows:
        return np.empty((0, len(keys)), dtype=float)
    return np.asarray(rows, dtype=float)


def mean_std(values: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    if values.shape[0] == 0:
        raise ValueError("no valid iterations found")
    mean = values.mean(axis=0)
    ddof = 1 if values.shape[0] > 1 else 0
    std = values.std(axis=0, ddof=ddof)
    return mean, std


def pct_delta(unicore_mean: np.ndarray, smp_mean: np.ndarray) -> np.ndarray:
    out = np.full_like(unicore_mean, np.nan, dtype=float)
    nonzero = unicore_mean != 0
    out[nonzero] = (smp_mean[nonzero] - unicore_mean[nonzero]) / unicore_mean[nonzero] * 100.0
    return out


def annotate_percent(ax, xs, heights, pcts):
    ymax = max(float(np.max(heights)) if len(heights) else 0.0, 1.0)
    for x, h, pct in zip(xs, heights, pcts):
        text = "n/a" if math.isnan(float(pct)) else f"{pct:+.0f}%"
        ax.text(
            x,
            h + ymax * 0.035,
            text,
            ha="center",
            va="bottom",
            fontsize=8,
            rotation=90,
        )


def plot_mean_comparison(
    out_path: Path,
    title: str,
    labels: Sequence[str],
    unicore_mean: np.ndarray,
    unicore_std: np.ndarray,
    smp_mean: np.ndarray,
    smp_std: np.ndarray,
):
    n = len(labels)
    x = np.arange(n)
    width = 0.38
    pcts = pct_delta(unicore_mean, smp_mean)

    fig_width = max(14, n * 0.65)
    fig, ax = plt.subplots(figsize=(fig_width, 7.5))

    ax.bar(
        x - width / 2,
        unicore_mean,
        width,
        yerr=unicore_std,
        capsize=3,
        label="Unicore mean ± std",
    )
    ax.bar(
        x + width / 2,
        smp_mean,
        width,
        yerr=smp_std,
        capsize=3,
        label="SMP mean ± std",
    )

    top = np.maximum(unicore_mean + unicore_std, smp_mean + smp_std)
    annotate_percent(ax, x, top, pcts)

    ax.set_title(title)
    ax.set_ylabel("cycles")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=60, ha="right")
    ax.legend()
    ax.grid(axis="y", linestyle="--", linewidth=0.5, alpha=0.5)

    for tick_label, u_mean, s_mean in zip(ax.get_xticklabels(), unicore_mean, smp_mean):
        if u_mean > s_mean + 2:
            tick_label.set_color("red")
            tick_label.set_fontweight("bold")

    ymax = max(float(np.max(top)) if len(top) else 0.0, 1.0)
    ax.set_ylim(0, ymax * 1.28)

    fig.tight_layout()
    fig.savefig(out_path, dpi=180)
    plt.close(fig)


def overall_sequence_cost(seqs: Sequence[ValidSequence]) -> np.ndarray:
    return np.asarray(
        [sum(ev.cycles for ev in seq.events) for seq in seqs],
        dtype=float,
    )


def scalar_mean_std(values: np.ndarray) -> Tuple[float, float]:
    if len(values) == 0:
        raise ValueError("no valid iterations found")
    mean = float(values.mean())
    ddof = 1 if len(values) > 1 else 0
    std = float(values.std(ddof=ddof))
    return mean, std


def compare(unicore_log: Path, smp_log: Path, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)

    u_events = parse_trace_events(unicore_log)
    s_events = parse_trace_events(smp_log)

    u_seqs, u_dropped, u_total = find_valid_sequences(u_events)
    s_seqs, s_dropped, s_total = find_valid_sequences(s_events)

    if not u_seqs:
        raise SystemExit(f"no valid complete Call+ReplyRecv sequence found in {unicore_log}")
    if not s_seqs:
        raise SystemExit(f"no valid complete Call+ReplyRecv sequence found in {smp_log}")

    u_call = values_for_keys(u_seqs, CALL_KEYS)
    s_call = values_for_keys(s_seqs, CALL_KEYS)
    u_reply = values_for_keys(u_seqs, REPLY_KEYS)
    s_reply = values_for_keys(s_seqs, REPLY_KEYS)

    u_call_mean, u_call_std = mean_std(u_call)
    s_call_mean, s_call_std = mean_std(s_call)
    u_reply_mean, u_reply_std = mean_std(u_reply)
    s_reply_mean, s_reply_std = mean_std(s_reply)

    u_overall = overall_sequence_cost(u_seqs)
    s_overall = overall_sequence_cost(s_seqs)

    u_overall_mean, u_overall_std = scalar_mean_std(u_overall)
    s_overall_mean, s_overall_std = scalar_mean_std(s_overall)

    if u_overall_mean != 0:
        overall_pct = (s_overall_mean - u_overall_mean) / u_overall_mean * 100.0
    else:
        overall_pct = float("nan")

    call_labels = [CALL_LABELS[tid] for _, tid in CALL_KEYS]
    reply_labels = [REPLY_LABELS[tid] for _, tid in REPLY_KEYS]

    plot_mean_comparison(
        out_dir / "call_mean_comparison.png",
        "seL4_Call tracepoint mean comparison: SMP vs Unicore",
        call_labels,
        u_call_mean,
        u_call_std,
        s_call_mean,
        s_call_std,
    )

    plot_mean_comparison(
        out_dir / "replyrecv_mean_comparison.png",
        "seL4_ReplyRecv tracepoint mean comparison: SMP vs Unicore",
        reply_labels,
        u_reply_mean,
        u_reply_std,
        s_reply_mean,
        s_reply_std,
    )

    print("Overall average cost per valid Call+ReplyRecv sequence:")
    print(
        f"  unicore: mean={u_overall_mean:.2f} cycles, "
        f"std={u_overall_std:.2f} cycles, samples={len(u_overall)}"
    )
    print(
        f"  smp:     mean={s_overall_mean:.2f} cycles, "
        f"std={s_overall_std:.2f} cycles, samples={len(s_overall)}"
    )
    print(f"  delta:   {overall_pct:+.2f}%")
    print()

    print("Trace source filtering:")
    print(f"  unicore: total_tracepoints={u_total}, valid_sequences={len(u_seqs)}, dropped_candidates={u_dropped}")
    print(f"  smp:     total_tracepoints={s_total}, valid_sequences={len(s_seqs)}, dropped_candidates={s_dropped}")
    print("Output:")
    print(f"  {out_dir / 'call_mean_comparison.png'}")
    print(f"  {out_dir / 'replyrecv_mean_comparison.png'}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Compare valid seL4_Call+ReplyRecv tracepoint sequences between Unicore and SMP logs."
    )
    parser.add_argument("unicore_log", type=Path)
    parser.add_argument("smp_log", type=Path)
    parser.add_argument("-o", "--out", type=Path, default=Path("trace_compare_out"))
    args = parser.parse_args()
    compare(args.unicore_log, args.smp_log, args.out)


if __name__ == "__main__":
    main()