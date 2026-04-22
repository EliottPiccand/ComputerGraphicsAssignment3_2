from pathlib import Path

BASE_DIR = Path(__file__).parent.parent
SRC_DIR = BASE_DIR / "Src"
INCLUDE_DIR = BASE_DIR / "Include"

file_count = 0
line_count = 0

for header in INCLUDE_DIR.rglob("*.h"):
    line_count += len(header.read_text().splitlines())
    file_count += 1

for impl in SRC_DIR.rglob("*.cpp"):
    line_count += len(impl.read_text().splitlines())
    file_count += 1

print(f"{line_count} lines across {file_count} files")
