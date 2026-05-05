#!/usr/bin/env python3
"""
Apply a minimum roughness clamp to a metallic/roughness texture.

Usage:
    python clamp_roughness.py <input_image> <output_image> <min_roughness>

Example:
    python clamp_roughness.py texture.png output.png 0.3
"""

import sys
from pathlib import Path
import numpy as np
from PIL import Image


def clamp_roughness(input_path: str, output_path: str, min_roughness: float) -> None:
    """
    Clamp the roughness channel (G) of a metallic/roughness texture.
    
    Args:
        input_path: Path to input PNG/JPG texture
        output_path: Path to save modified texture
        min_roughness: Minimum roughness value (0.0 to 1.0)
    """
    # Validate input
    if not 0.0 <= min_roughness <= 1.0:
        raise ValueError(f"min_roughness must be between 0.0 and 1.0, got {min_roughness}")
    
    # Load image
    img = Image.open(input_path)
    
    # Convert to RGB(A) if needed
    if img.mode not in ('RGB', 'RGBA'):
        img = img.convert('RGBA')
    
    # Convert to numpy array
    arr = np.array(img, dtype=np.float32) / 255.0
    
    # Extract channels (assuming RGBA or RGB)
    # G channel = roughness, B channel = metallic
    if arr.shape[-1] == 4:
        r, g, b, a = arr[..., 0], arr[..., 1], arr[..., 2], arr[..., 3]
    else:
        r, g, b = arr[..., 0], arr[..., 1], arr[..., 2]
        a = None
    
    # Clamp roughness (G channel) to minimum
    g_clamped = np.maximum(g, min_roughness)
    
    # Reconstruct array
    if a is not None:
        result = np.stack([r, g_clamped, b, a], axis=-1)
    else:
        result = np.stack([r, g_clamped, b], axis=-1)
    
    # Convert back to 0-255 range
    result = np.clip(result * 255.0, 0, 255).astype(np.uint8)
    
    # Save
    output_img = Image.fromarray(result)
    output_img.save(output_path)
    
    print(f"Processed: {input_path} -> {output_path}")
    print(f"Min roughness applied: {min_roughness:.2f}")


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        print(f"\nError: Expected 3 arguments, got {len(sys.argv) - 1}")
        sys.exit(1)
    
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    
    try:
        min_roughness = float(sys.argv[3])
    except ValueError:
        print(f"Error: min_roughness must be a number, got '{sys.argv[3]}'")
        sys.exit(1)
    
    # Check input exists
    if not Path(input_path).exists():
        print(f"Error: Input file not found: {input_path}")
        sys.exit(1)
    
    clamp_roughness(input_path, output_path, min_roughness)


if __name__ == "__main__":
    main()