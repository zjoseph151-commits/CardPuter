# Saved WiFi Feedback And Delete Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make saved WiFi actions explicit by showing result screens after save/delete and allow deleting saved SSID names from the Saved WiFi feature.

**Architecture:** Extend the existing single-file state machine with result and delete-confirm screens. Keep persistence in ESP32 Preferences/NVS and reuse the existing saved-name array plus `persistSavedWifiNames()` so delete updates the same storage as save.

**Tech Stack:** PlatformIO, Arduino framework, M5Cardputer, ESP32 `Preferences`, Python guard scripts.

---

### Task 1: Guard Test

**Files:**
- Modify: `tools/check_saved_wifi.py`

- [ ] **Step 1: Add expected tokens**

Add checks for `Screen::WifiSaveResult`, `Screen::SavedWifiDeleteConfirm`, `Screen::SavedWifiDeleteResult`, `deleteSavedWifiName(`, `renderSavedWifiDeleteConfirm()`, `renderSavedWifiDeleteResult()`, `Network saved`, `Network deleted`, and README notes for `D to delete`.

- [ ] **Step 2: Run the guard test to verify it fails**

Run: `python tools/check_saved_wifi.py`

Expected: FAIL because result and delete screens are not implemented yet.

### Task 2: Firmware Screens And Storage

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Add screens and helpers**

Add enum states for save result, delete confirmation, and delete result. Add helper prototypes for rendering those screens and deleting a saved name.

- [ ] **Step 2: Show save result**

After `saveWifiName(...)`, route to `Screen::WifiSaveResult`. OK/Enter returns to WiFi Scan.

- [ ] **Step 3: Delete with confirmation**

In `Saved WiFi`, pressing `D` opens the delete confirmation screen for the selected SSID. OK/Enter deletes it, persists storage, and opens the delete result screen. Backspace cancels to `Saved WiFi`.

### Task 3: Docs And Verification

**Files:**
- Modify: `README.md`
- Run: `python tools/check_saved_wifi.py`
- Run: `python tools/check_menu_structure.py`
- Run: `python tools/check_wifi_scroll.py`
- Run: `pio run`

- [ ] **Step 1: Update README navigation and checklist**

Document save result behavior and `D` delete from Saved WiFi.

- [ ] **Step 2: Verify guards and build**

Expected: all guard scripts pass and PlatformIO build succeeds.
