#!/usr/bin/env python3
"""
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Turns the benchmark CSVs into the report figures and a Markdown summary.

    python3 scripts/plot_results.py --bench results/bench_cpu.csv results/bench_gpu.csv \
        --fps results/fps_sweep.csv --validation results/validation.csv --out results/figures

Only the standard library and matplotlib are needed. Missing inputs are skipped.

Outputs (in --out):
    thread_scaling.png    CPU speedup vs threads (one line per agent count) + ideal
    step_time_vs_agents.png  CPU vs GPU step time vs agent count (log-log)
    frame_breakdown_<backend>.png  per-frame step / copy / fill / draw (FPS sweep)
    summary.md            tables for the report: speedup, CPU vs GPU, copy share,
                          largest count at 30/60 FPS, validation results
"""

import argparse
import csv
import os
import sys
from collections import defaultdict

try:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:  # still write the Markdown tables
    plt = None

# Fixed categorical order so a backend keeps the same colour in every figure.
SERIES = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#6250d6", "#e34948"]
BACKEND_COLOR = {"cpu": SERIES[0], "cuda": SERIES[1], "stub": SERIES[2]}
INK = "#0b0b0b"
INK_2 = "#52514e"
GRID = "#e4e3df"


def read_csv(paths):
    rows = []
    for p in paths or []:
        if not os.path.exists(p):
            print(f"skipping missing {p}", file=sys.stderr)
            continue
        with open(p, newline="") as f:
            rows.extend(csv.DictReader(f))
    return rows


def num(row, key, default=float("nan")):
    try:
        return float(row[key])
    except (KeyError, ValueError, TypeError):
        return default


def style(ax, title, xlabel, ylabel):
    ax.set_title(title, loc="left", fontsize=12, color=INK, pad=10)
    ax.set_xlabel(xlabel, color=INK_2)
    ax.set_ylabel(ylabel, color=INK_2)
    ax.grid(True, color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(INK_2)
    ax.tick_params(colors=INK_2)


def mean_by(rows, keyfn, valkey):
    """Averages valkey over repeats, grouped by keyfn(row)."""
    acc = defaultdict(list)
    for r in rows:
        v = num(r, valkey)
        if v == v:  # not NaN
            acc[keyfn(r)].append(v)
    return {k: sum(v) / len(v) for k, v in acc.items()}


def thread_scaling(rows, out, md):
    cpu = [r for r in rows if r["backend"] == "cpu" and int(r["threads"]) > 0]
    if not cpu:
        return
    step = mean_by(cpu, lambda r: (int(r["agents"]), int(r["threads"])), "step_ms_mean")
    agents = sorted({a for a, _ in step})
    md.append("## CPU thread scaling\n")
    md.append("| agents | threads | step ms | speedup vs 1 thread | efficiency |")
    md.append("|---:|---:|---:|---:|---:|")

    if plt:
        fig, ax = plt.subplots(figsize=(7, 4.5), dpi=150)
    max_t = 1
    for i, a in enumerate(agents[:len(SERIES)]):
        ts = sorted(t for (aa, t) in step if aa == a)
        if 1 not in ts:
            continue
        base = step[(a, 1)]
        sp = [base / step[(a, t)] for t in ts]
        max_t = max(max_t, max(ts))
        for t, s in zip(ts, sp):
            md.append(f"| {a:,} | {t} | {step[(a, t)]:.3f} | {s:.2f}x | {100 * s / t:.0f}% |")
        if plt:
            ax.plot(ts, sp, marker="o", markersize=5, linewidth=2, color=SERIES[i], label=f"{a:,} agents")
    md.append("")
    if plt:
        ax.plot([1, max_t], [1, max_t], linestyle="--", linewidth=1, color=INK_2, label="ideal (linear)")
        style(ax, "CPU backend: speedup vs. worker threads", "threads", "speedup vs. 1 thread")
        ax.legend(frameon=False)
        fig.tight_layout()
        fig.savefig(os.path.join(out, "thread_scaling.png"))
        plt.close(fig)


def cpu_vs_gpu(rows, out, md):
    # CPU: use the best (fastest) thread count per agent count.
    best = {}
    for r in rows:
        b = r["backend"]
        if b == "stub":
            continue
        key = (b, int(r["agents"]))
        v = num(r, "step_ms_mean")
        if v == v and (key not in best or v < best[key][0]):
            best[key] = (v, num(r, "copy_ms_mean"), r["threads"])
    if not best:
        return
    backends = sorted({b for b, _ in best}, key=lambda b: list(BACKEND_COLOR).index(b) if b in BACKEND_COLOR else 99)
    agents = sorted({a for _, a in best})

    md.append("## CPU vs. GPU step time\n")
    md.append("| agents | " + " | ".join(f"{b} step ms" for b in backends) +
              (" | GPU speedup | GPU copy ms | copy / step |" if "cuda" in backends and "cpu" in backends else " |"))
    md.append("|---:|" + "---:|" * len(backends) + ("---:|---:|---:|" if "cuda" in backends and "cpu" in backends else ""))
    for a in agents:
        cells = [f"{best[(b, a)][0]:.3f}" if (b, a) in best else "-" for b in backends]
        line = f"| {a:,} | " + " | ".join(cells) + " |"
        if "cuda" in backends and "cpu" in backends and ("cuda", a) in best and ("cpu", a) in best:
            g, c = best[("cuda", a)], best[("cpu", a)]
            line += f" {c[0] / g[0]:.1f}x | {g[1]:.3f} | {100 * g[1] / g[0]:.0f}% |"
        md.append(line)
    md.append("")

    if plt:
        fig, ax = plt.subplots(figsize=(7, 4.5), dpi=150)
        for b in backends:
            xs = [a for a in agents if (b, a) in best]
            ax.plot(xs, [best[(b, a)][0] for a in xs], marker="o", markersize=5, linewidth=2,
                    color=BACKEND_COLOR.get(b, SERIES[3]), label=b.upper() if b != "cpu" else "CPU (best thread count)")
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xticks(agents)
        ax.set_xticklabels([f"{a // 1000}k" if a >= 1000 else str(a) for a in agents])
        ax.minorticks_off()
        for ms, label in ((1000 / 60, "60 FPS budget"), (1000 / 30, "30 FPS budget")):
            ax.axhline(ms, color=INK_2, linewidth=1, linestyle=":")
            ax.text(agents[0], ms * 1.05, label, color=INK_2, fontsize=8)
        style(ax, "Simulation step time vs. agent count", "agents", "step time (ms, log scale)")
        ax.legend(frameon=False)
        fig.tight_layout()
        fig.savefig(os.path.join(out, "step_time_vs_agents.png"))
        plt.close(fig)


def fps_sweep(rows, out, md):
    if not rows:
        return
    phases = [("step_ms_mean", "step"), ("copy_ms_mean", "GPU/backend -> host copy"),
              ("fill_ms_mean", "vertex fill"), ("draw_ms_mean", "draw + display")]
    by_backend = defaultdict(dict)
    for r in rows:  # later rows (re-runs) replace earlier ones
        by_backend[r["backend"]][int(r["agents"])] = r

    md.append("## Rendering: frame time breakdown (FPS sweep)\n")
    md.append("| backend | agents | FPS | frame ms | step | copy | fill | draw | copy % of frame |")
    md.append("|---|---:|---:|---:|---:|---:|---:|---:|---:|")
    best = {}
    for b, per in by_backend.items():
        for a in sorted(per):
            r = per[a]
            fps = num(r, "fps_mean")
            md.append(f"| {b} | {a:,} | {fps:.1f} | {num(r, 'frame_ms_mean'):.2f} | {num(r, 'step_ms_mean'):.2f} | "
                      f"{num(r, 'copy_ms_mean'):.2f} | {num(r, 'fill_ms_mean'):.2f} | {num(r, 'draw_ms_mean'):.2f} | "
                      f"{num(r, 'copy_pct_of_frame'):.1f}% |")
            for target in (30, 60):
                if fps >= target:
                    best[(b, target)] = max(best.get((b, target), 0), a)
    md.append("")
    md.append("**Largest tested agent count at the FPS target (mean FPS, rendering on):**\n")
    md.append("| backend | >= 30 FPS | >= 60 FPS |")
    md.append("|---|---:|---:|")
    for b in by_backend:
        c30 = f"{best[(b, 30)]:,}" if (b, 30) in best else "none tested"
        c60 = f"{best[(b, 60)]:,}" if (b, 60) in best else "none tested"
        md.append(f"| {b} | {c30} | {c60} |")
    md.append("")

    if not plt:
        return
    for b, per in by_backend.items():
        agents = sorted(per)
        labels = [f"{a // 1000}k" if a >= 1000 else str(a) for a in agents]
        fig, ax = plt.subplots(figsize=(7, 4.5), dpi=150)
        bottom = [0.0] * len(agents)
        for i, (key, name) in enumerate(phases):
            vals = [num(per[a], key, 0.0) for a in agents]
            ax.bar(labels, vals, bottom=bottom, color=SERIES[i], edgecolor="white", linewidth=1.5, label=name, width=0.6)
            bottom = [x + v for x, v in zip(bottom, vals)]
        for ms, label in ((1000 / 60, "60 FPS"), (1000 / 30, "30 FPS")):
            ax.axhline(ms, color=INK_2, linewidth=1, linestyle=":")
            ax.text(len(agents) - 0.5, ms * 1.02, label, color=INK_2, fontsize=8, ha="right")
        style(ax, f"Frame time breakdown - {b.upper()} backend", "agents", "ms per frame")
        ax.legend(frameon=False, fontsize=8)
        fig.tight_layout()
        fig.savefig(os.path.join(out, f"frame_breakdown_{b}.png"))
        plt.close(fig)


def validation(rows, md):
    if not rows:
        return
    md.append("## CPU vs. GPU agreement (reference metrics)\n")
    md.append("| reference | candidate | agents | steps | metric | reference | candidate | rel. diff | tolerance | result |")
    md.append("|---|---|---:|---:|---|---:|---:|---:|---:|---|")
    for r in rows:
        md.append(f"| {r['reference']} | {r['candidate']} | {int(r['agents']):,} | {r['steps']} | {r['metric']} | "
                  f"{num(r, 'reference_value'):.4f} | {num(r, 'candidate_value'):.4f} | "
                  f"{100 * num(r, 'rel_diff'):.2f}% | {100 * num(r, 'tolerance'):.0f}% | {r['pass']} |")
    md.append("")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bench", nargs="*", default=["results/bench.csv"])
    ap.add_argument("--fps", nargs="*", default=["results/fps_sweep.csv"])
    ap.add_argument("--validation", nargs="*", default=["results/validation.csv"])
    ap.add_argument("--out", default="results/figures")
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    if plt is None:
        print("matplotlib not found: writing tables only (pip install --user matplotlib)", file=sys.stderr)

    bench = read_csv(args.bench)
    md = ["# Results summary", "", "Generated by scripts/plot_results.py from the CSVs in results/.", ""]
    hosts = sorted({(r.get("host", "?"), r.get("device", "?"), r["backend"]) for r in bench})
    if hosts:
        md.append("Machines: " + "; ".join(f"{b} on {d} ({h})" for h, d, b in hosts))
        md.append("")
    thread_scaling(bench, args.out, md)
    cpu_vs_gpu(bench, args.out, md)
    fps_sweep(read_csv(args.fps), args.out, md)
    validation(read_csv(args.validation), md)

    path = os.path.join(args.out, "summary.md")
    with open(path, "w") as f:
        f.write("\n".join(md) + "\n")
    print(f"wrote {path} and figures in {args.out}")


if __name__ == "__main__":
    main()
