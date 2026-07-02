# Voice Memos Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `Voice Memos` feature that records built-in mic audio to microSD as WAV files, lists saved memos, plays them through the built-in speaker, and deletes selected memos.

**Architecture:** Extend the existing `src/main.cpp` state machine with a `VoiceMemos` screen plus delete confirmation/result states. Use M5Cardputer mic/speaker APIs from the official examples, SD/SPI with the Cardputer SD pins, and simple WAV files in `/memos` with automatic names like `memo001.wav`.

**Tech Stack:** PlatformIO, Arduino framework, M5Cardputer/M5Unified audio APIs, ESP32 Arduino `SD` and `SPI`, Python guard scripts.

---

### Task 1: Guard Test

**Files:**
- Create: `tools/check_voice_memos.py`

- [ ] **Step 1: Write the failing guard**

Check for `#include <SPI.h>`, `#include <SD.h>`, `Screen::VoiceMemos`, `Voice Memos`, SD pin constants, WAV header type, `/memos`, `startVoiceMemoRecording()`, `stopVoiceMemoRecording()`, `playSelectedVoiceMemo()`, `deleteSelectedVoiceMemo()`, `M5Cardputer.Mic.record`, `M5Cardputer.Speaker.playRaw`, and README controls.

- [ ] **Step 2: Run the guard to verify it fails**

Run: `python tools/check_voice_memos.py`

Expected: FAIL because voice memos are not implemented yet.

### Task 2: Firmware Feature

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Add includes, constants, and menu item**

Add `SPI.h`, `SD.h`, SD pin constants from the official M5Cardputer example, voice memo limits, WAV header struct, memo file list arrays, and `Voice Memos` to the main menu.

- [ ] **Step 2: Add SD and memo-list helpers**

Add `initVoiceMemoSd()`, `scanVoiceMemos()`, `findNextVoiceMemoPath()`, and `renderVoiceMemos()`.

- [ ] **Step 3: Add recording helpers**

Add `startVoiceMemoRecording()`, `serviceVoiceMemoRecording()`, `stopVoiceMemoRecording()`, and `writeWavHeader()`. Recording should run in small chunks from loop and stop on `R`, max duration, or error.

- [ ] **Step 4: Add playback and delete helpers**

Add `playSelectedVoiceMemo()`, `deleteSelectedVoiceMemo()`, delete confirmation/result screens, and key handling. Stop mic before speaker playback and restart mic only when recording again.

### Task 3: Docs And Verification

**Files:**
- Modify: `README.md`
- Run: `python tools/check_voice_memos.py`
- Run existing guard scripts
- Run: `pio run`

- [ ] **Step 1: Update README**

Document SD card requirement, `Voice Memos`, `R` record/stop, arrows scroll, OK/Enter play, and `D` delete.

- [ ] **Step 2: Verify**

Expected: all guard scripts pass and PlatformIO build succeeds.
