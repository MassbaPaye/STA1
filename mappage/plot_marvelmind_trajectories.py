#!/usr/bin/env python3
import argparse
import csv
from collections import defaultdict
from math import isnan
import sys

import matplotlib.pyplot as plt


def parse_log(path):
    """
    Parse a Marvelmind CSV log.
    Expects rows like:
    time_ms,?, ?, device_id, x, y, z, ... (12 columns observed), with some 'T...' metadata rows to skip.
    Returns: dict[device_id] -> list of (t_ms, x, y, z)
    """
    data = defaultdict(list)
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        reader = csv.reader(f)
        for row in reader:
            if not row:
                continue
            # Skip metadata lines starting with 'T...' (e.g., T2025_10_01__000003_250,...)
            if row[0].startswith("T"):
                continue

            # We expect numeric rows; be defensive about length
            # Observed structure from sample:
            #  0: timestamp (int ms)
            #  1: seq? (int)
            #  2: ???   (int)
            #  3: device id (int)  -> we use this
            #  4: x (float meters)
            #  5: y (float meters)
            #  6: z (float meters) -> optional / often 0
            try:
                t_ms = int(row[0])
                device_id = int(row[3])
                x = float(row[4])
                y = float(row[5])
                z = float(row[6]) if len(row) > 6 and row[6] != "" else float("nan")
            except (ValueError, IndexError):
                # Skip malformed rows
                continue

            # Filter out obviously invalid coordinates (e.g., all zeros everywhere may be placeholders)
            data[device_id].append((t_ms, x, y, z))
    return data


def pick_devices(data, limit=3):
    """
    Pick up to 'limit' devices that appear to have non-trivial movement (variance in x/y).
    Heuristic: total span in x or y greater than tiny epsilon.
    """
    candidates = []
    for did, samples in data.items():
        if not samples:
            continue
        xs = [s[1] for s in samples]
        ys = [s[2] for s in samples]
        span_x = (max(xs) - min(xs)) if xs else 0.0
        span_y = (max(ys) - min(ys)) if ys else 0.0
        movement_score = span_x + span_y
        candidates.append((movement_score, did))
    # Sort by movement descending
    candidates.sort(reverse=True)
    picked = [did for _, did in candidates[:limit]]
    return picked


def plot_trajectories(data, devices, title=None, save_path=None):
    """
    Plot X vs Y trajectories for the given device IDs on a single chart.
    """
    if not devices:
        print("No devices selected to plot.", file=sys.stderr)
        return

    plt.figure()  # single chart, no subplots
    for did in devices:
        if did not in data or not data[did]:
            continue
        # Sort by time to draw proper path
        samples = sorted(data[did], key=lambda s: s[0])
        xs = [s[1] for s in samples]
        ys = [s[2] for s in samples]
        plt.plot(xs, ys, label=f"Device {did}")
        # Optionally mark start/end
        try:
            plt.scatter(xs[:1], ys[:1], marker="o")
            plt.scatter(xs[-1:], ys[-1:], marker="x")
        except Exception:
            pass

    plt.xlabel("X (m)")
    plt.ylabel("Y (m)")
    if title:
        plt.title(title)
    plt.legend()
    plt.grid(True)
    try:
        plt.axis("equal")
    except Exception:
        # Fallback if equal aspect not possible
        pass

    if save_path:
        plt.savefig(save_path, bbox_inches="tight", dpi=150)
        print(f"Saved figure to {save_path}")
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(description="Plot trajectories of up to 3 Marvelmind devices from a CSV log.")
    parser.add_argument("csv_path", help="Path to the Marvelmind CSV log.")
    parser.add_argument("--devices", type=str, default="",
                        help="Comma-separated device IDs to plot (e.g., 10,12,13). Auto-selects up to 3 if omitted.")
    parser.add_argument("--limit", type=int, default=3, help="Max number of devices to auto-select if --devices not provided.")
    parser.add_argument("--save", type=str, default="",
                        help="Optional path to save the figure instead of showing it (e.g., trajectories.png).")
    parser.add_argument("--title", type=str, default="Marvelmind Trajectories",
                        help="Title for the plot.")
    args = parser.parse_args()

    data = parse_log(args.csv_path)

    if args.devices.strip():
        try:
            devices = [int(x.strip()) for x in args.devices.split(",") if x.strip()]
        except ValueError:
            print("Invalid --devices list. Use comma-separated integers, e.g., 10,12,13", file=sys.stderr)
            sys.exit(1)
    else:
        devices = pick_devices(data, limit=max(1, args.limit))

    save_path = args.save.strip() or None
    plot_trajectories(data, devices, title=args.title, save_path=save_path)


if __name__ == "__main__":
    main()
