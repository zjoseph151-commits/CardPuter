from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"


def firmware_source_text(exclude_names=()):
    excluded_names = set(exclude_names)
    source_files = sorted(
        path
        for path in SRC.iterdir()
        if path.suffix in {".cpp", ".h"} and path.name not in excluded_names
    )
    return "\n".join(path.read_text(encoding="utf-8") for path in source_files)
