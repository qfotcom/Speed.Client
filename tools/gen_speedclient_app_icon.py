#!/usr/bin/env python3
"""Bake SpeedClient.ico + Android mipmap launcher icons from windows/SpeedClientAppIcon.svg."""
from __future__ import annotations

import io
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SVG = ROOT / "windows" / "SpeedClientAppIcon.svg"
OUT_ICO = ROOT / "windows" / "SpeedClient.ico"
ANDROID_RES = ROOT / "android" / "res"
MASTER = 2048

ICO_SIZES = [
    (256, 256),
    (128, 128),
    (96, 96),
    (64, 64),
    (48, 48),
    (40, 40),
    (32, 32),
    (24, 24),
    (20, 20),
    (16, 16),
]

# launcher icon size per density bucket (px)
ANDROID_LAUNCHER = {
    "mipmap-mdpi": 48,
    "mipmap-hdpi": 72,
    "mipmap-xhdpi": 96,
    "mipmap-xxhdpi": 144,
    "mipmap-xxxhdpi": 192,
}


def _rasterize_svg() -> Image.Image:
    import cairosvg

    png = cairosvg.svg2png(url=str(SVG), output_width=MASTER, output_height=MASTER)
    return Image.open(io.BytesIO(png)).convert("RGBA")


def _write_launcher(master: Image.Image, name: str) -> None:
    for folder, px in ANDROID_LAUNCHER.items():
        out_dir = ANDROID_RES / folder
        out_dir.mkdir(parents=True, exist_ok=True)
        resized = master.resize((px, px), Image.Resampling.LANCZOS)
        resized.save(out_dir / f"{name}.png", format="PNG", optimize=True)


def main() -> None:
    if not SVG.is_file():
        raise SystemExit(f"Missing master SVG: {SVG}")

    master = _rasterize_svg()
    OUT_ICO.parent.mkdir(parents=True, exist_ok=True)
    master.save(OUT_ICO, format="ICO", sizes=ICO_SIZES, bitmap_format="bmp")
    _write_launcher(master, "ic_launcher")
    _write_launcher(master, "ic_launcher_round")
    print(f"Wrote {OUT_ICO}")
    print(f"Wrote Android mipmaps under {ANDROID_RES}")


if __name__ == "__main__":
    main()
