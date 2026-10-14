#!/usr/bin/env python3
"""Name the effect that spooky-arduino is currently transmitting.

Reads the board's own USART - which carries the same byte stream that feeds the
MAX485 - so the DMX bus is left completely alone while a fixture is being
watched. Prints a timestamped line each time the inferred effect changes, with
the evidence it used, so a human can confirm it against what the fixture shows.

Usage:
    python3 tools/dmx_effects.py [/dev/cu.usbmodem11401] [--seconds N]
"""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is required: python3 -m pip install pyserial")

sys.path.insert(0, __file__.rsplit("/", 1)[0])
import dmx_monitor as m  # noqa: E402

BAR_COUNT = 8
COLOUR_BASE = 7
WINDOW_FRAMES = 40  # ~0.9 s of frames per classification

# The roster no longer sets a unique horizontal speed per effect, so the emitters
# a frame lights are the more useful signal - but several effects share a colour.
# The report therefore names the group a signature belongs to rather than
# claiming one effect, and leans on the strobe and the tilt speed to separate
# them: 200 with a hard strobe is blinder, 200 without is the jumping group,
# 100 is the drifting group, and a self-moving effect sets its own.
STROBE_ON = 150


def emitters_in(frame):
    """Which of R, G, B, W are lit anywhere on the fixture."""
    present = set()
    for bar in range(BAR_COUNT):
        base = COLOUR_BASE + bar * 4 - 1
        for offset, colour in enumerate("RGBW"):
            if frame[base + offset]:
                present.add(colour)
    return present


def describe(frame):
    """Name the group a frame's signature belongs to.

    Several effects share a colour, so this reports the group rather than
    claiming a specific effect. The strobe separates the blinder; the tilt speed
    separates the red drifting effects from the red self-moving ones.
    """
    present = emitters_in(frame)
    speed = frame[1]
    strobe = frame[5]
    if not present and not speed:
        return "blackout (disabled)"
    if strobe >= STROBE_ON:
        return "blinder (hard strobe)"
    if not present:
        return "dark between flashes (random strobe, lightning)"
    if "W" in present:
        return "white group (twinkle, random strobe, blinder, lightning)"
    if "G" in present:
        return "green group (ripple)"
    if len(present) > 1:
        return "multi-colour group (snake, colour roll)"
    if present == {"R"}:
        if speed >= 200:
            return "red self-moving group (shake, pendulum)"
        if speed:
            return "red drifting group (chase, wave, comet, fire, blood pulse)"
        return "red self-moving group (crawl)"
    return f"drifting group ({''.join(sorted(present))})"


def classify(frames):
    """Name the group the current look belongs to, over a window of frames.

    A window rather than a single frame, because the Uno's USB bridge drops and
    inserts bytes around the DMX break, so some frames arrive shifted; a window
    majority survives that.
    """
    votes = {}
    for frame in frames:
        votes.setdefault(describe(frame), []).append(frame)

    name = max(votes, key=lambda k: len(votes[k]))
    if len(votes[name]) < len(frames) * 0.3:
        return None, None, None, None, None  # too noisy to call

    group = votes[name]
    present = set()
    for frame in group:
        present |= emitters_in(frame)
    strobe = max(frame[5] for frame in group)
    speeds = sorted(frame[1] for frame in group)
    return (name, "".join(sorted(present)) or "dark", strobe,
            f"{speeds[0]}..{speeds[-1]}", group[0][1])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", nargs="?", default="/dev/cu.usbmodem11401")
    parser.add_argument("--seconds", type=float, default=180.0)
    args = parser.parse_args()

    ser = serial.Serial(args.port, 250000, bytesize=8, parity="N",
                        stopbits=2, timeout=0.3)
    print(f"reading {args.port}; the board resets when this opens, so power it on now",
          flush=True)
    time.sleep(2.5)

    start = time.monotonic()
    current = None
    window = []
    for frame in m.frame_reader(ser, once=False):
        window.append(frame)
        if len(window) < WINDOW_FRAMES:
            continue

        name, present, strobe, speed_range, speed = classify(window)
        now = time.monotonic() - start
        if name is not None and name != current:
            current = name
            print(f"t={now:6.1f}s  {name}  [emitters={present or 'dark'} "
                  f"strobe={strobe} speed={speed_range}]", flush=True)

        window = window[-(WINDOW_FRAMES // 2):]
        if now > args.seconds:
            break
    ser.close()
    print("capture finished", flush=True)


if __name__ == "__main__":
    main()
