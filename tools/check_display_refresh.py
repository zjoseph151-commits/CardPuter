import re
from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")


def function_body(name):
    match = re.search(rf"^void\s+{name}\s*\([^)]*\)\s*\{{", SOURCE, re.MULTILINE)
    if not match:
        raise AssertionError(f"Could not find definition for {name}")

    body_start = SOURCE.index("{", match.start())
    depth = 0

    for index in range(body_start, len(SOURCE)):
        char = SOURCE[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return SOURCE[body_start:index]

    raise AssertionError(f"Could not find body for {name}")


def test_dynamic_screens_do_not_force_full_redraw():
    dynamic_screens = (
        "showBatteryInfo",
        "showSystemInfo",
        "showEnvironment",
        "showLevelTool",
    )

    offenders = [
        name for name in dynamic_screens if "drawHeader(" in function_body(name)
    ]

    assert offenders == [], (
        "Dynamic update screens should redraw only the content region, "
        f"but these still call drawHeader(): {', '.join(offenders)}"
    )


def test_content_canvas_allocation_is_guarded():
    for token in [
        "contentCanvasReady",
        "initContentCanvas()",
        "contentCanvas.setPsram(false)",
        "contentCanvas.createSprite(width, height) != nullptr",
        "CONTENT_CANVAS_FALLBACK_COLOR_DEPTH",
        "contentCanvas.getBuffer() == nullptr",
        "contentCanvas.pushSprite(&M5Cardputer.Display, 0, CONTENT_TOP)",
    ]:
        assert token in SOURCE, f"Missing guarded content canvas token: {token}"

    assert "contentCanvas.createSprite(" not in MAIN, (
        "setup() should use initContentCanvas() instead of unguarded sprite creation"
    )


if __name__ == "__main__":
    test_dynamic_screens_do_not_force_full_redraw()
    test_content_canvas_allocation_is_guarded()
    print("Display refresh checks passed.")
