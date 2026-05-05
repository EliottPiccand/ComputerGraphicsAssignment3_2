from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageOps


ROOT_MARKER = "Assets"
SMOOTHNESS_SUFFIX = "MetallicSmoothness.png"
ROUGHNESS_SUFFIX = "MetallicRoughness.png"


def convert_texture(source_path: Path, destination_path: Path) -> None:
    image = Image.open(source_path)
    try:
        if image.mode not in ("RGB", "RGBA"):
            image = image.convert("RGBA")

        red, green, blue, *alpha = image.split()
        roughness = ImageOps.invert(green)

        if alpha:
            converted = Image.merge("RGBA", (red, roughness, blue, alpha[0]))
        else:
            converted = Image.merge("RGB", (red, roughness, blue))

        destination_path.parent.mkdir(parents=True, exist_ok=True)
        converted.save(destination_path)
    finally:
        image.close()


def patch_gltf_file(gltf_path: Path) -> int:
    text = gltf_path.read_text(encoding="utf-8")
    updated = text.replace(SMOOTHNESS_SUFFIX, ROUGHNESS_SUFFIX)
    if updated != text:
        gltf_path.write_text(updated, encoding="utf-8")
        return 1
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert metallic smoothness textures to metallic roughness.")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1], help="Repository root")
    args = parser.parse_args()

    root = args.root
    textures_root = root / "Assets" / "Textures"
    models_root = root / "Assets" / "Models"

    converted_count = 0
    patched_models = 0

    for source_path in textures_root.rglob(f"*{SMOOTHNESS_SUFFIX}"):
        destination_path = source_path.with_name(source_path.name.replace(SMOOTHNESS_SUFFIX, ROUGHNESS_SUFFIX))
        convert_texture(source_path, destination_path)
        converted_count += 1
        print(f"converted {source_path.relative_to(root)} -> {destination_path.relative_to(root)}")

    for gltf_path in models_root.rglob("*.gltf"):
        patched_models += patch_gltf_file(gltf_path)

    print(f"converted textures: {converted_count}")
    print(f"patched model files: {patched_models}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
