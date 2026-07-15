from pathlib import Path
import re


SOURCE = Path("src/main.cpp").read_text()


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
        "showRfScanner",
        "showLevelTool",
    )

    offenders = [
        name for name in dynamic_screens if "drawHeader(" in function_body(name)
    ]

    assert offenders == [], (
        "Dynamic update screens should redraw only the content region, "
        f"but these still call drawHeader(): {', '.join(offenders)}"
    )


if __name__ == "__main__":
    test_dynamic_screens_do_not_force_full_redraw()
    print("Display refresh checks passed.")
