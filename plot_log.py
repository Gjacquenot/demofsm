#!/usr/bin/env python3
"""Plot log.csv (step,speed,state) and save PNG.

Usage:
    python3 plot_log.py [log.csv] -o log.png
"""
import argparse
import sys
import csv

import numpy as np
import matplotlib.pyplot as plt


def main():
    p = argparse.ArgumentParser(description="Plot step vs speed from CSV and save PNG")
    p.add_argument('csv', nargs='?', default='log.csv', help='Input CSV file (default: log.csv)')
    p.add_argument('-o', '--out', default='log.png', help='Output PNG file (default: log.png)')
    p.add_argument('--dpi', type=int, default=150, help='Output DPI')
    args = p.parse_args()

    steps = []
    speeds = []
    states = []

    try:
        with open(args.csv, newline='') as fh:
            reader = csv.reader(fh)
            first = next(reader, None)
            if first is None:
                print(f"No data in {args.csv}", file=sys.stderr)
                return 3

            def is_number(s):
                try:
                    float(s)
                    return True
                except Exception:
                    return False

            if not is_number(first[0]):
                # header present, continue with remaining rows
                pass
            else:
                # first row is data
                row = first
                if len(row) >= 2:
                    try:
                        steps.append(float(row[0]))
                        speeds.append(float(row[1]))
                        states.append(row[2].strip() if len(row) > 2 else "")
                    except ValueError:
                        pass

            for row in reader:
                if not row:
                    continue
                if len(row) < 2:
                    continue
                try:
                    steps.append(float(row[0]))
                    speeds.append(float(row[1]))
                    states.append(row[2].strip() if len(row) > 2 else "")
                except ValueError:
                    continue

    except Exception as e:
        print(f"Failed to read {args.csv}: {e}", file=sys.stderr)
        return 2

    if len(steps) == 0:
        print(f"No numeric data found in {args.csv}", file=sys.stderr)
        return 3

    steps = np.array(steps)
    speeds = np.array(speeds)

    # plt.style.use('seaborn-darkgrid')
    fig, ax = plt.subplots(figsize=(10, 4))

    # Shade contiguous state regions
    if states:
        import matplotlib.patches as mpatches
        # define colors for known states (tweak as needed)
        color_map = {
            'Idle': '#1f77b4',
            'Running': '#2ca02c',
            'Error': '#d62728'
        }
        # compute typical step width
        if len(steps) > 1:
            dx = float(np.mean(np.diff(steps)))
        else:
            dx = 1.0

        seg_start = 0
        cur_state = states[0]
        for i in range(1, len(states)):
            if states[i] != cur_state:
                start_x = steps[seg_start]
                end_x = steps[i-1] + 0.5 * dx
                color = color_map.get(cur_state, '#888888')
                ax.axvspan(start_x, end_x, facecolor=color, alpha=0.25)
                seg_start = i
                cur_state = states[i]
        # last segment
        start_x = steps[seg_start]
        end_x = steps[-1] + 0.5 * dx
        color = color_map.get(cur_state, '#888888')
        ax.axvspan(start_x, end_x, facecolor=color, alpha=0.25)

        # legend patches
        patches = [mpatches.Patch(color=c, label=s) for s,c in color_map.items()]
        ax.legend(handles=patches, loc='upper left')

    ax.plot(steps, speeds, '-', linewidth=1)
    ax.set_xlabel('Step')
    ax.set_ylabel('Speed')
    ax.set_title('Motor speed over steps')
    ax.grid(True)
    plt.tight_layout()

    try:
        fig.savefig(args.out, dpi=args.dpi)
    except Exception as e:
        print(f"Failed to write {args.out}: {e}", file=sys.stderr)
        return 5

    print(f"Wrote {args.out}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
