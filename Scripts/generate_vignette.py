#!/usr/bin/env python3
"""Generate a red vignette overlay PNG for ship-hit feedback."""

from __future__ import annotations

import argparse
import math
import struct
import zlib
from pathlib import Path


def smoothstep(edge0: float, edge1: float, value: float) -> float:
	if edge0 == edge1:
		return 1.0 if value >= edge1 else 0.0

	t = (value - edge0) / (edge1 - edge0)
	t = max(0.0, min(1.0, t))
	return t * t * (3.0 - 2.0 * t)


def make_red_vignette_rgba(
	width: int,
	height: int,
	inner_radius: float,
	max_alpha: int,
	gamma: float,
	red: int,
	green: int,
	blue: int,
) -> bytes:
	cx = (width - 1) * 0.5
	cy = (height - 1) * 0.5
	inv_cx = 1.0 / max(cx, 1.0)
	inv_cy = 1.0 / max(cy, 1.0)

	rows = bytearray()
	for y in range(height):
		rows.append(0)  # PNG filter type 0 (none)
		ny = (y - cy) * inv_cy

		for x in range(width):
			nx = (x - cx) * inv_cx
			distance = math.sqrt(nx * nx + ny * ny)

			edge_strength = smoothstep(inner_radius, 1.0, distance)
			alpha = int(round((edge_strength**gamma) * max_alpha))
			alpha = max(0, min(255, alpha))

			rows.extend((red, green, blue, alpha))

	return bytes(rows)


def _png_chunk(chunk_type: bytes, data: bytes) -> bytes:
	length = struct.pack(">I", len(data))
	crc = struct.pack(">I", zlib.crc32(chunk_type + data) & 0xFFFFFFFF)
	return length + chunk_type + data + crc


def write_png_rgba(path: Path, width: int, height: int, rgba_scanlines: bytes) -> None:
	ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
	idat = zlib.compress(rgba_scanlines, level=9)

	png = bytearray()
	png.extend(b"\x89PNG\r\n\x1a\n")
	png.extend(_png_chunk(b"IHDR", ihdr))
	png.extend(_png_chunk(b"IDAT", idat))
	png.extend(_png_chunk(b"IEND", b""))

	path.parent.mkdir(parents=True, exist_ok=True)
	path.write_bytes(bytes(png))


def parse_args() -> argparse.Namespace:
	default_output = Path(__file__).resolve().parent.parent / "Assets" / "Textures" / "Effects" / "HitVignette.png"

	parser = argparse.ArgumentParser(description="Generate a red vignette overlay PNG.")
	parser.add_argument("--width", type=int, default=1280, help="Image width in pixels.")
	parser.add_argument("--height", type=int, default=720, help="Image height in pixels.")
	parser.add_argument("--output", type=Path, default=default_output, help="Output .png path.")
	parser.add_argument("--inner-radius", type=float, default=0.53, help="Radius where alpha starts rising.")
	parser.add_argument("--max-alpha", type=int, default=210, help="Maximum edge alpha [0..255].")
	parser.add_argument("--gamma", type=float, default=1.6, help="Falloff shaping factor.")
	parser.add_argument("--red", type=int, default=255, help="Red channel [0..255].")
	parser.add_argument("--green", type=int, default=18, help="Green channel [0..255].")
	parser.add_argument("--blue", type=int, default=18, help="Blue channel [0..255].")
	return parser.parse_args()


def main() -> int:
	args = parse_args()

	if args.width <= 0 or args.height <= 0:
		raise ValueError("width and height must be positive")
	if not (0 <= args.max_alpha <= 255):
		raise ValueError("max-alpha must be in [0, 255]")
	if not (0.0 <= args.inner_radius <= 1.5):
		raise ValueError("inner-radius should be in [0.0, 1.5]")
	if args.gamma <= 0.0:
		raise ValueError("gamma must be > 0")

	for channel_name in ("red", "green", "blue"):
		channel_value = getattr(args, channel_name)
		if not (0 <= channel_value <= 255):
			raise ValueError(f"{channel_name} must be in [0, 255]")

	scanlines = make_red_vignette_rgba(
		width=args.width,
		height=args.height,
		inner_radius=args.inner_radius,
		max_alpha=args.max_alpha,
		gamma=args.gamma,
		red=args.red,
		green=args.green,
		blue=args.blue,
	)

	output_path = args.output.resolve()
	write_png_rgba(output_path, args.width, args.height, scanlines)
	print(f"Wrote {args.width}x{args.height} vignette to: {output_path}")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
