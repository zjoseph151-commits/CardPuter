from pathlib import Path


SOURCE = Path("src/main.cpp").read_text()


def test_imu_screen_is_level_tool():
    assert "LevelTool" in SOURCE
    assert "showLevelTool()" in SOURCE
    assert "drawLevelCrosshair(" in SOURCE
    assert "drawLevelDot(" in SOURCE
    assert "LEVEL_DOT_SCALE_PIXELS" in SOURCE
    assert "LEVEL_TOLERANCE_PIXELS" in SOURCE
    assert "smoothedLevelX" in SOURCE
    assert "smoothedLevelY" in SOURCE
    assert '"LEVEL"' in SOURCE
    assert '"IMU Test"' not in SOURCE
    assert "showImuTest" not in SOURCE


if __name__ == "__main__":
    test_imu_screen_is_level_tool()
    print("Level tool checks passed.")
