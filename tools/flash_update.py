import argparse
import sys
import time
import zlib

try:
    import serial
except ImportError:
    sys.exit("This script needs pyserial: pip install pyserial")

MAGIC = b"UPDT"
ACK = 0x06
NACK = 0x15
CHUNK_SIZE = 256  # must match UPDATE_CHUNK_SIZE in bootloader/include/update.h

DEFAULT_BAUD = 115200


def read_one_byte(ser, timeout_s):
    """Read exactly one byte, or return None if it doesn't arrive in time."""
    ser.timeout = timeout_s
    data = ser.read(1)
    return data[0] if data else None


def send_magic_and_wait_for_ack(ser, attempt_window_s=3.0, retry_interval_s=0.1):
    """
    Repeatedly send the magic sequence for up to attempt_window_s seconds,
    since we don't know exactly when the bootloader's own listening window
    started relative to when this script got launched.
    """
    deadline = time.monotonic() + attempt_window_s
    while time.monotonic() < deadline:
        ser.write(MAGIC)
        ser.flush()
        byte = read_one_byte(ser, retry_interval_s)
        if byte == ACK:
            return True
        # NACK isn't expected here (bootloader only ACKs on a full magic
        # match), but bail out early if we somehow get one.
        if byte == NACK:
            return False
    return False


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("image", help="path to the raw app image (.bin)")
    parser.add_argument(
        "--port", required=True, help="serial port, e.g. /dev/ttyACM0 or COM5"
    )
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument(
        "--handshake-timeout",
        type=float,
        default=3.0,
        help="seconds to keep retrying the magic handshake",
    )
    parser.add_argument(
        "--erase-timeout",
        type=float,
        default=5.0,
        help="seconds to wait for the post-erase ACK",
    )
    parser.add_argument(
        "--chunk-timeout",
        type=float,
        default=2.0,
        help="seconds to wait for each per-chunk ACK/NACK",
    )
    args = parser.parse_args()

    with open(args.image, "rb") as f:
        image = f.read()

    image_size = len(image)
    image_crc = zlib.crc32(image) & 0xFFFFFFFF

    print(f"Image: {args.image} ({image_size} bytes, crc32=0x{image_crc:08X})")

    ser = serial.Serial(args.port, args.baud)

    print("Reset the board now. Waiting for bootloader handshake...")
    if not send_magic_and_wait_for_ack(ser, attempt_window_s=args.handshake_timeout):
        sys.exit('No handshake ACK - bootloader didn\'t respond to "UPDT" in time.')
    print("Handshake OK, bootloader is in update mode.")

    header = image_size.to_bytes(4, "little") + image_crc.to_bytes(4, "little")
    ser.write(header)
    ser.flush()

    byte = read_one_byte(ser, args.erase_timeout)
    if byte != ACK:
        sys.exit(f"Erase failed or timed out (got {byte!r}).")
    print("Erase complete, streaming image...")

    offset = 0
    while offset < image_size:
        chunk = image[offset : offset + CHUNK_SIZE]
        ser.write(chunk)
        ser.flush()

        byte = read_one_byte(ser, args.chunk_timeout)
        if byte is None:
            sys.exit(
                f"Timed out waiting for ACK at offset {offset}. "
                "Reset the board and rerun - flash can't be resumed mid-session."
            )
        if byte == NACK:
            is_last = (offset + len(chunk)) >= image_size
            if is_last:
                sys.exit(
                    "CRC mismatch reported by bootloader - image did not verify. "
                    "Reset the board and try again."
                )
            sys.exit(
                f"Bootloader NACKed the chunk at offset {offset} (flash write error). "
                "Reset the board and try again."
            )

        offset += len(chunk)
        print(f"\r  {offset}/{image_size} bytes", end="", flush=True)

    print("\nUpdate verified OK. Board is resetting.")


if __name__ == "__main__":
    main()
