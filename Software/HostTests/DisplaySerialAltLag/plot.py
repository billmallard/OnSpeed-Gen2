"""Plots the ONSPEED sentence's altitude, before and after, from <out>/ into <png>."""
import csv
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402


def load(path):
    with open(path, newline="") as f:
        return [(float(r["t_s"]), float(r["truth_ft"]), int(r["sent_ft"])) for r in csv.DictReader(f)]


def main(out, png):
    base = load(f"{out}/base_ONSPEED.csv")
    new = load(f"{out}/new_ONSPEED.csv")
    blue, orange, ink, muted = "#2a78d6", "#eb6834", "#1a1a19", "#8a8980"

    fig, (top, bot) = plt.subplots(2, 1, figsize=(9, 6.5), sharex=True, dpi=120,
                                   gridspec_kw={"height_ratios": [3, 2]})
    for ax in (top, bot):
        ax.grid(True, color="#e4e3dc", linewidth=0.8)
        for s in ("top", "right"):
            ax.spines[s].set_visible(False)
        for s in ("left", "bottom"):
            ax.spines[s].set_color(muted)
        ax.tick_params(colors=muted, labelcolor=ink)
        ax.axvspan(60, 123, color="#f1f0ea", zorder=0)

    t = [r[0] for r in base]
    top.plot(t, [r[1] for r in base], color=muted, linewidth=2, linestyle="--", label="True pressure altitude")
    top.plot(t, [r[2] for r in base], color=orange, linewidth=2, label="Sent, before (be1481c)")
    top.plot([r[0] for r in new], [r[2] for r in new], color=blue, linewidth=2, label="Sent, after")
    top.set_ylabel("Altitude in the ONSPEED sentence (ft)", color=ink)
    top.set_title("Climb at 1000 fpm from 3000 ft, then level off: the sentence before and after",
                  color=ink, loc="left", fontsize=11)
    top.legend(frameon=False, loc="upper left", labelcolor=ink)
    top.text(91, 3100, "climb", color=muted, ha="center")

    bot.axhline(0, color=muted, linewidth=1)
    bot.plot(t, [r[2] - r[1] for r in base], color=orange, linewidth=2)
    bot.plot([r[0] for r in new], [r[2] - r[1] for r in new], color=blue, linewidth=2)
    bot.set_ylabel("Sent minus true (ft)", color=ink)
    bot.set_xlabel("Time (s)", color=ink)
    bot.text(110, -150, "before: 174 ft low", color=ink, ha="right")
    bot.text(170, 12, "after: within 2 ft", color=ink, ha="right")
    bot.set_xlim(40, 180)

    fig.tight_layout()
    fig.savefig(png)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
