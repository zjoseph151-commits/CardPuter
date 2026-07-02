# Environment Feature Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a live Environment screen for the M5Stack ENV III Unit on the Cardputer Grove I2C port.

**Architecture:** Keep the built-in Cardputer LCD as the control screen and add one non-blocking Environment feature. Use the M5Unit-ENV direct drivers for SHT30 temperature/humidity and QMP6988 pressure, initialize them defensively on the Grove I2C pins, and refresh readings about once per second.

**Tech Stack:** PlatformIO, Arduino framework, M5Cardputer, `Wire`, `M5Unit-ENV`.

---

### Task 1: Guard Test

**Files:**
- Create: `tools/check_environment_feature.py`

- [x] **Step 1: Write the failing guard**

Check for the PlatformIO dependency, ENV include, menu entry, screen state, Grove I2C pins, init/read/render helpers, and README wiring/test notes.

- [x] **Step 2: Run the guard**

Run: `python tools/check_environment_feature.py`
Expected: FAIL before firmware implementation because `m5stack/M5Unit-ENV` and `showEnvironment()` do not exist yet.

### Task 2: Firmware Implementation

**Files:**
- Modify: `platformio.ini`
- Modify: `src/main.cpp`

- [x] **Step 1: Add dependency and includes**

Add `m5stack/M5Unit-ENV`, `Wire.h`, and `M5UnitENV.h`.

- [x] **Step 2: Add Environment state**

Add `Screen::Environment`, menu label `Environment`, sensor globals for `SHT3X` and `QMP6988`, and status/readback fields.

- [x] **Step 3: Add defensive sensor init and reads**

Use Grove pins `ENV_I2C_SDA_PIN = 2` and `ENV_I2C_SCL_PIN = 1`, call `begin()` for both sensors, allow partial availability, and retry if the unit is not detected.

- [x] **Step 4: Add the screen renderer**

Show temperature in C/F, humidity in percent, pressure in hPa, altitude if available, and connected/not-found status. Use the existing canvas refresh pattern.

### Task 3: Docs And Verification

**Files:**
- Modify: `README.md`
- Run: all guard scripts
- Run: `python -m platformio run`

- [x] **Step 1: Update README**

Document the ENV III hardware, Grove wiring, the Environment feature, and first test checklist item.

- [x] **Step 2: Verify**

Run the new guard, all existing guards, and a PlatformIO build. Expected: all pass.
