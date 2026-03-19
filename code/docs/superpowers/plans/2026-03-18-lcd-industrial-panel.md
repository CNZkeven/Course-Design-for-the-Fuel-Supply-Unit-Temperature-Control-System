# LCD Industrial Panel Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Re-layout the STM32 LCD monitoring screen into an industrial control panel without changing PID, serial command, ADC, or DAC behavior.

**Architecture:** Keep all control logic in `USER/main.c` unchanged in behavior and refactor only the display layer. Add a small set of LCD drawing and numeric formatting helpers in `USER/main.c` so the screen can be redrawn with clear sections, larger primary values, and explicit status presentation.

**Tech Stack:** STM32F4 C firmware, ALIENTEK LCD driver, CMake/Ninja build.

---

### Task 1: Map the new display structure

**Files:**
- Modify: `USER/main.c`

- [ ] Define screen regions: top status bar, main temperature block, left telemetry box, right PID box.
- [ ] Introduce layout constants so coordinates are grouped instead of scattered literals.
- [ ] Keep all drawing in `USER/main.c` to match existing code structure.

### Task 2: Add minimal display helpers

**Files:**
- Modify: `USER/main.c`

- [ ] Add tiny helpers for labels, boxed sections, fixed-format numeric output, and status rendering.
- [ ] Use existing LCD APIs only (`LCD_ShowString`, `LCD_ShowxNum`, `LCD_DrawRectangle`, `LCD_Fill`, `LCD_DrawLine`).
- [ ] Preserve ASCII-only text to stay compatible with the bundled font tables.

### Task 3: Replace the old linear page with the new industrial layout

**Files:**
- Modify: `USER/main.c`

- [ ] Replace startup static labels with a dedicated screen initialization function.
- [ ] Update `display_values()` and `dac_output()` so values render into the new regions with fixed alignment and units.
- [ ] Add status text for `READY`, `RUN`, and `ALARM`, plus ADC channel and limit display.

### Task 4: Verify firmware build

**Files:**
- Modify: `USER/main.c`

- [ ] Run the project build command.
- [ ] Fix any warnings or errors introduced by the UI refactor.
- [ ] Report verification results precisely.
