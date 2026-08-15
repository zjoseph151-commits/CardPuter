# External Display Revisit Plan

Date: 2026-08-15

## Goal

Revisit Priority #7 without reintroducing the external OLED feature until the hardware path is deliberate and safe.

The built-in Cardputer LCD remains the primary UI display. Any external display should be optional, secondary, and gracefully ignored when it is not connected.

## Current Context

- The 2.42 inch SSD1309 OLED experiment was removed from active firmware.
- The U8g2 dependency was removed from `platformio.ini`.
- The previous OLED Test menu item and firmware helpers were removed.
- Using G8/G9 for an external I2C OLED caused arrow-key failures.
- G8/G9 are the Cardputer Adv internal I2C bus and should not be used directly for external I2C modules.
- Moving the OLED to Grove G2/G1 worked during the earlier test.
- The ENV III Unit currently uses Grove G2/G1, so sharing Grove needs a deliberate hardware plan.

## Safety Decision

No active external display code should be added until the hardware plan is approved.

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
- OLED reserved for PaHub channel 1.
- Built-in Cardputer LCD remains the primary UI.

Rejected direction:

- Do not use G8/G9 directly for external I2C.

## Implementation Sequence

### Task 1: Lock The Inactive State

- [x] Keep U8g2 out of `platformio.ini`.
- [x] Keep active OLED firmware tokens out of source.
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
- [x] Keep U8g2 and OLED firmware inactive.
- [ ] Hardware-test ENV III through PaHub channel 0.
- [ ] Verify keyboard navigation with the PaHub attached.

### Task 4: Add Only An OLED Proof Of Life

Only after Task 3 is hardware-tested:

- [ ] Add the smallest possible display dependency and initialization path.
- [ ] Add a simple proof-of-life screen before building a real secondary-display feature.
- [ ] Show a graceful "not found" or disabled state if the external display is missing.
- [ ] Keep Backspace/menu navigation working on the built-in display.
- [ ] Verify arrow keys after wiring.
- [ ] Verify ENV III still works if the chosen plan shares Grove.

## Acceptance Checks

Priority #7 planning is complete enough when:

- `tools/check_external_display_revisit.py` passes.
- `tools/check_oled_test.py` still passes.
- `platformio.ini` still has no U8g2 dependency.
- Firmware source still has no active OLED Test screen.
- README, notes, TODO, and this plan all warn not to use G8/G9 directly for external I2C.
- The PaHub foundation keeps direct Grove ENV III working when the hub is absent.
- The next OLED firmware step is blocked on ENV III and keyboard hardware testing through the PaHub.
