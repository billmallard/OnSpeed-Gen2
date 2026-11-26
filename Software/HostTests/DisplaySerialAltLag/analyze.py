"""Reads the harness traces in <out>/ and reports the altitude lag of each sentence.

Exits nonzero if the change touched anything besides the ONSPEED PALT field and
its checksum, or if the ONSPEED lag is not gone.
"""
import csv
import statistics
import sys
from pathlib import Path

LEVEL_OFF = 123.0  # s: the climb ends at 120 s and its 3 s ramp at 123 s (harness.cpp)
PALT = {"ONSPEED": slice(15, 21), "G3X": slice(27, 33)}


def load(path):
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        for k in ("t_s", "truth_ft", "kalman_ft"):
            r[k] = float(r[k])
        r["sent_ft"] = int(r["sent_ft"])
    return rows


def at(rows, t):
    return min(rows, key=lambda r: abs(r["t_s"] - t))


def metrics(rows):
    climb = [r["truth_ft"] - r["sent_ft"] for r in rows if 100.0 <= r["t_s"] <= 120.0]
    level = [r["sent_ft"] for r in rows if 30.0 <= r["t_s"] <= 60.0]
    final = rows[-1]["truth_ft"]
    after = {d: final - at(rows, LEVEL_OFF + d)["sent_ft"] for d in (0, 5, 10, 20)}
    return statistics.mean(climb), after, statistics.pstdev(level)


def main(out):
    out = Path(out)
    ok = True
    traces = {(v, f): load(out / f"{v}_{f}.csv") for v in ("base", "new") for f in PALT}

    for fmt, field in PALT.items():
        base, new = traces["base", fmt], traces["new", fmt]
        if len(base) != len(new):
            print(f"{fmt}: sentence counts differ ({len(base)} vs {len(new)})")
            ok = False
            continue
        changed = set()
        for b, n in zip(base, new):
            sb, sn = b["sentence"], n["sentence"]
            changed |= {i for i in range(max(len(sb), len(sn))) if sb[i:i + 1] != sn[i:i + 1]}
        allowed = set(range(field.start, field.stop)) if fmt == "ONSPEED" else set()
        if fmt == "ONSPEED":
            allowed |= {len(base[0]["sentence"]) - 2, len(base[0]["sentence"]) - 1}  # checksum
        stray = sorted(changed - allowed)
        print(f"{fmt}: {len(base)} sentences, byte offsets changed: "
              f"{sorted(changed) if changed else 'none'}{'  STRAY ' + str(stray) if stray else ''}")
        ok &= not stray
        if fmt == "G3X" and changed:
            ok = False

    print()
    print("format   version  climb lag (ft)  behind at level-off +0/+5/+10/+20 s (ft)  level jitter (ft, 1 sd)")
    for (v, f), rows in sorted(traces.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        lag, after, jitter = metrics(rows)
        print(f"{f:8} {v:8} {lag:14.1f}  {after[0]:7.1f} {after[5]:7.1f} {after[10]:7.1f} {after[20]:7.1f}"
              f"            {jitter:5.2f}")
    k = traces["new", "ONSPEED"]
    klag = statistics.mean(r["truth_ft"] - r["kalman_ft"] for r in k if 100.0 <= r["t_s"] <= 120.0)
    print(f"\nKalman filter alone, climb lag: {klag:.1f} ft")

    new_lag = metrics(traces["new", "ONSPEED"])[0]
    if abs(new_lag) > 10.0:
        print(f"ONSPEED climb lag {new_lag:.1f} ft is not gone")
        ok = False
    print("\nPASS" if ok else "\nFAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
