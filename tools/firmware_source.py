from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"


def firmware_source_text():
    source_files = sorted(
        path for path in SRC.iterdir() if path.suffix in {".cpp", ".h"}
    )
    return "\n".join(path.read_text(encoding="utf-8") for path in source_files)
