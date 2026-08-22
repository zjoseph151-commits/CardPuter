# External Display Revisit Plan

Date: 2026-08-15

## Goal

Revisit Priority #7 by adding external OLED support only after the hardware path is deliberate and safe.

The built-in Cardputer LCD remains the primary UI display. Any external display should be optional, secondary, and gracefully ignored when it is not connected.

## Current Context

- The 2.42 inch SSD1309 OLED experiment was removed from active firmware after the first pin-conflict test.
- The U8g2 dependency and OLED Test were later reintroduced only after the PaHub hardware path was chosen.
- Using G8/G9 for an external I2C OLED caused arrow-key failures.
- G8/G9 are the Cardputer Adv internal I2C bus and should not be used directly for external I2C modules.
- Moving the OLED to Grove G2/G1 worked during the earlier test.
- The ENV III Unit currently uses Grove G2/G1, so sharing Grove needs a deliberate hardware plan.

## Safety Decision

The proof-of-life screen came first so the hardware path could be verified before building useful display behavior.

User confirmed the OLED Test proof-of-life works on Cardputer hardware on 2026-08-15.

The next approved step is an OLED Status Dashboard foundation: the built-in LCD remains the main control screen, while the SSD1309 OLED becomes a small glance/status display on PaHub channel 1.

User confirmed the OLED Status Dashboard and Environment-after-dashboard behavior are working on 2026-08-22. Priority #7 is complete enough; future OLED refinements belong under Priority #8.

Chosen path: M5Stack Unit PaHub v2.1.

Hardware details:

- Product: M5Stack Unit PaHub v2.1, SKU U040-B-V21.
- I2C mux chip: PCA9548AP.
- Default I2C address: `0x70`.
- Address range: `0x70` through `0x77` using the onboard DIP switch.
- Six extended Grove ports.

Planned wiring:

- Cardputer Grove G2/G1 to Unit PaHub v2.1 input.
- Leave the PaHub DIP switch at default address `0x70` for the first test.
- ENV III on PaHub channel 0.
- SSD1309 OLED on PaHub channel 1.
- Built-in Cardputer LCD remains the primary UI.

Rejected direction:

- Do not use G8/G9 directly for external I2C.

## Implementation Sequence

### Task 1: Lock The Inactive State

- [x] Kept OLED inactive until the PaHub hardware path was chosen.
- [x] Keep README, notes, and TODO warnings about G8/G9.
- [x] Add a Priority #7 guard so these constraints are checked automatically.

### Task 2: Choose The Hardware Path

- [x] Decide whether the OLED will use Grove alone, Grove through an I2C expansion path, or another interface.
- [x] Confirm whether ENV III must remain connected at the same time.
- [x] Document exact wiring before adding OLED firmware code.

### Task 3: Add PaHub Foundation

- [x] Detect the PaHub at `0x70`.
- [x] Select ENV III on PaHub channel 0 before Environment initialization and reads.
- [x] Fall back to direct Grove when the PaHub is not connected.
- [x] Hardware-test ENV III through PaHub channel 0.
- [x] Verify keyboard navigation with the PaHub attached.

### Task 4: Add Only An OLED Proof Of Life

- [x] Add the smallest possible display dependency and initialization path.
- [x] Add a simple proof-of-life screen before building a real secondary-display feature.
- [x] Show a graceful "not found" or disabled state if the external display is missing.
- [x] Keep Backspace/menu navigation working on the built-in display.
- [x] OLED Test proof-of-life screen was added.
- [x] Select PaHub channel 1 before OLED probe/init/draw.
- [x] Probe `0x3C` and `0x3D`.
- [x] Draw a simple SSD1309 proof pattern with text, address, counter, border, and moving marker.
- [x] Hardware-test OLED Test with the SSD1309 on PaHub channel 1.
- [x] Verify arrow keys after wiring.
- [x] Verify ENV III still works if the chosen plan shares Grove.

### Task 5: OLED Status Dashboard Foundation

- [x] Centralize OLED dashboard drawing in `src/oled_test.cpp`.
- [x] Keep OLED Test as a diagnostics/proof-of-life screen.
- [x] Add `serviceOledStatusDashboard()` so normal screens refresh the OLED about once per second from `loop()`.
- [x] Add `renderOledStatusDashboard()` for the compact five-line dashboard.
- [x] Add `setOledStatusLine(...)` and `clearOledStatusLine()` so features can provide short context later without knowing U8g2 details.
- [x] Show screen/mode, battery, Wi-Fi, MQTT when Pi Monitor is active or has been used, and compact feature context.
- [x] Cover Main Menu, Pi Monitor, Environment, Voice Memos, and RF Scan first.
- [x] Keep OLED on PaHub channel 1 and ENV III on PaHub channel 0.
- [x] Keep graceful behavior when PaHub or OLED is missing.
- [x] Hardware-test the OLED Status Dashboard across Main Menu, Pi Monitor, Environment, Voice Memos, and RF Scan.
- [x] Verify ENV III still reads on PaHub channel 0 after OLED dashboard updates.
- [x] Verify keyboard navigation still feels normal while the dashboard is refreshing.

## Acceptance Checks

Priority #7 OLED Status Dashboard foundation is complete enough when:

- `tools/check_external_display_revisit.py` passes.
- `tools/check_oled_test.py` still passes.
- `tools/check_oled_status_dashboard.py` passes.
- `platformio.ini` has the U8g2 dependency.
- Firmware source has centralized OLED Status Dashboard helpers.
- README, notes, TODO, and this plan all warn not to use G8/G9 directly for external I2C.
- The PaHub foundation keeps direct Grove ENV III working when the hub is absent.
- OLED Test works with the SSD1309 on PaHub channel 1.
- OLED Status Dashboard works with the SSD1309 on PaHub channel 1.
- Keyboard navigation still works with both modules connected.
- ENV III still works on PaHub channel 0 after OLED dashboard updates.

Result: complete enough on 2026-08-22 based on user hardware testing.
