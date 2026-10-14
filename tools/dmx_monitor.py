#!/usr/bin/env python3
"""Read the DMX512 universe that spooky-arduino transmits, from the end of the
DMX path.

Wiring: a **bidirectional** USB-RS485 adapter sits at the end of the DMX line
(A/B to the pair, common to the adapter's ground) and shows up on the Mac as a
USB serial port. A CP2102/CH340 dongle with an auto-direction transceiver (for
example the MAX13487) works; a USB-to-DMX *controller* cable does not — the DSD
Tech SH-RS09B is FT232RL + MAX485 and receives nothing at all.

If you have no such adapter, skip this and use the fixture: flash the white
chase test pattern and watch the beam bar.

This script reads the adapter as a passive listener and prints the channel
values the firmware is putting on the bus.

It is a passive read, not a conformance tester. The firmware sends a full
512-channel universe, so every frame ends in a long run of zero channels; the
script resynchronises on that run and on the DMX start code, rather than relying
on the adapter reporting the break.

Usage:
    python3 tools/dmx_monitor.py --list
    python3 tools/dmx_monitor.py /dev/cu.wchusbserial1234
    python3 tools/dmx_monitor.py /dev/cu.wchusbserial1234 --once
    python3 tools/dmx_monitor.py /dev/cu.wchusbserial1234 --all
"""

import argparse
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("pyserial is required: python3 -m pip install pyserial")

DMX_BAUD = 250000
UNIVERSE = 512
FIXTURE_CHANNELS = 42

# Longest zero run that can appear inside a frame is the unused tail of the
# universe (channels 43-512). Anything at least this long is a frame boundary.
MIN_ZERO_RUN = 400

# Bytes of the unused universe tail used to validate a framed candidate.
TAIL_CHECK = 120
TAIL_ZERO_FRACTION = 0.9

# The firmware always drives the beam bar's horizontal channel to a value
# between its idle and ceiling positions, and the channel is preceded by the
# DMX start code. Together those identify a frame start even when a UART has
# emitted a stray non-zero byte for the break just before it.
BEAM_IDLE_MIN = 40
BEAM_CEILING_MAX = 120
START_CODE = 0

CHANNEL_NAMES = {
    1: "beam horizontal",
    2: "beam horiz speed",
    3: "beam effects",
    4: "beam effect speed",
    5: "beam master dim",
    6: "beam strobe",
}
for _bar in range(8):
    for _offset, _color in enumerate("RGBW"):
        CHANNEL_NAMES[7 + _bar * 4 + _offset] = f"BAR{_bar + 1} {_color}"
CHANNEL_NAMES.update(
    {
        39: "bubble output",
        40: "haze volume",
        41: "blower speed",
        42: "bubble cleaning",
    }
)


def open_port(port):
    return serial.Serial(
        port=port,
        baudrate=DMX_BAUD,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_TWO,
        timeout=0.5,
    )


def find_run_end(buffer, start=0):
    """Index of the first byte after a long zero run, at or after `start`."""
    run = 0
    for index in range(start, len(buffer)):
        if buffer[index] == 0:
            run += 1
        else:
            if run >= MIN_ZERO_RUN:
                return index
            run = 0
    return None


def tail_is_unused(buffer, start):
    """True when the channels after the fixture block are the unused zero tail.

    This is what makes a frame self-validating: the firmware sends a full
    512-channel universe but only drives channels 1-42, so everything after the
    fixture block is zero.
    """
    tail_start = start + FIXTURE_CHANNELS
    tail_end = tail_start + TAIL_CHECK
    if tail_end > len(buffer):
        return None  # not enough data yet
    zeros = sum(1 for byte in buffer[tail_start:tail_end] if byte == 0)
    return zeros >= TAIL_CHECK * TAIL_ZERO_FRACTION


def find_frame_start(buffer):
    """Index of channel 1 of the first frame in the buffer, or None.

    A UART that meets the DMX break reports a framing error and may emit stray
    bytes, and some take several bytes to resynchronise. So the anchor is the
    DMX start code followed by channel 1, validated by the unused zero tail -
    never the break itself. The exact idle-park value is tried first because the
    test patterns hold channel 1 there, which makes it unambiguous; a monitor
    watching moving effects falls back to the wider idle-to-ceiling window.
    """
    limit = len(buffer) - FIXTURE_CHANNELS - TAIL_CHECK
    if limit < 1:
        return None
    for exact in (True, False):
        for index in range(limit):
            if buffer[index] != START_CODE:
                continue
            ch1 = buffer[index + 1]
            if exact:
                if ch1 != BEAM_IDLE_MIN:
                    continue
            elif not BEAM_IDLE_MIN <= ch1 <= BEAM_CEILING_MAX:
                continue
            if tail_is_unused(buffer, index + 1):
                return index + 1
    return None


def frame_reader(ser, once, deadline=None):
    """Yield one 42-byte fixture frame each time a frame can be framed.

    In `once` mode it keeps reading until `deadline`: opening the port resets
    the board, so the first reads can be empty while it boots.
    """
    buffer = bytearray()
    while True:
        chunk = ser.read(max(1, ser.in_waiting or 1))
        if chunk:
            buffer.extend(chunk)
        elif once and (deadline is None or time.monotonic() > deadline):
            return

        while True:
            start = find_frame_start(buffer)
            if start is None:
                # Keep only a tail long enough to hold a frame plus its check.
                keep = 2 * (UNIVERSE + 1)
                if len(buffer) > 2 * keep:
                    del buffer[:-keep]
                break
            frame = bytes(buffer[start : start + FIXTURE_CHANNELS])
            del buffer[:start]
            yield frame
            if once:
                return


def print_frame(frame, show_all):
    non_zero = [(index + 1, value) for index, value in enumerate(frame) if value]
    print(f"--- frame: {len(non_zero)} non-zero fixture channels ---")
    for channel in range(1, FIXTURE_CHANNELS + 1):
        value = frame[channel - 1]
        if show_all or value:
            name = CHANNEL_NAMES.get(channel, "")
            print(f"  ch{channel:>3} {name:<18} {value:>3}")
    if not non_zero:
        print("  (all fixture channels are 0 - blackout)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", nargs="?", help="serial port of the DMX adapter")
    parser.add_argument("--list", action="store_true", help="list serial ports and exit")
    parser.add_argument("--once", action="store_true", help="print one frame and exit")
    parser.add_argument(
        "--all", action="store_true", help="print every fixture channel, zero or not"
    )
    args = parser.parse_args()

    if args.list:
        for info in list_ports.comports():
            print(f"{info.device}\t{info.description}")
        return 0

    if not args.port:
        parser.error("a serial port is required (use --list to find it)")

    try:
        ser = open_port(args.port)
    except serial.SerialException as error:
        sys.exit(f"could not open {args.port}: {error}")

    with ser:
        print(f"listening on {args.port} at {DMX_BAUD} baud, 8N2")
        if args.once:
            deadline = time.monotonic() + 6.0
            for frame in frame_reader(ser, once=True, deadline=deadline):
                print_frame(frame, args.all)
                return 0
            print("no complete DMX frame seen")
            return 1

        last_frame = None
        last_print = 0.0
        try:
            for frame in frame_reader(ser, once=False):
                now = time.monotonic()
                if frame != last_frame and now - last_print >= 0.2:
                    print_frame(frame, args.all)
                    last_frame = frame
                    last_print = now
        except KeyboardInterrupt:
            print("\nstopped")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
