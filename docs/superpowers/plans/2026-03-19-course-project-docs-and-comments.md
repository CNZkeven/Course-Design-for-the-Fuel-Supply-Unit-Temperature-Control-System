# Course Project Docs And Comments Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rewrite the project README into a course-project style document and add fuller explanatory comments to the firmware and host-side core files, with special emphasis on serial communication.

**Architecture:** Keep the existing STM32 firmware plus PySide6 host application structure, but improve the documentation layer so the project is easier to present in a class report and defend in oral questioning. Clarify the end-to-end data flow from ADC sampling to PID output to serial telemetry and host-side plotting.

**Tech Stack:** C on STM32F407, STM32 Standard Peripheral Library, PySide6, QtSerialPort, pyqtgraph, pytest

---

## Chunk 1: README Rewrite

### Task 1: Replace the existing README with a course-project oriented version

**Files:**
- Modify: `README.md`
- Reference: `任务要求.md`
- Reference: `code/USER/main.c`
- Reference: `app/core/protocol.py`
- Reference: `app/core/serial_manager.py`

- [ ] **Step 1: Outline the README sections**

Cover project background, requirements mapping, system architecture, control flow, serial protocol, key files, tech stack, run instructions, and experiment guidance.

- [ ] **Step 2: Rewrite the README**

Explain the design in report-friendly Chinese, with a dedicated serial communication section and honest wording for metrics that still require real experiments.

- [ ] **Step 3: Verify the README content**

Check that the document matches the actual source files and does not claim unverified experimental results.

## Chunk 2: Core Comment Improvements

### Task 2: Add fuller comments to firmware and host-side core files

**Files:**
- Modify: `code/USER/main.c`
- Modify: `code/USER/control_output.c`
- Modify: `code/USER/temp_conversion.c`
- Modify: `code/SYSTEM/usart/usart.c`
- Modify: `code/SYSTEM/usart/usart.h`
- Modify: `app/core/protocol.py`
- Modify: `app/core/serial_manager.py`
- Modify: `app/ui/main_window.py`

- [ ] **Step 1: Annotate firmware-side logic**

Add comments that explain ADC sampling, PID control, DAC scaling, LCD display refresh, USART command parsing, and the CRLF-based receive state machine.

- [ ] **Step 2: Annotate host-side serial pipeline**

Add comments that explain command formatting, line buffering, batch command pacing, telemetry parsing, and chart/log/record updates.

- [ ] **Step 3: Run lightweight verification**

Run Python syntax checks on the edited host files and review the edited C files for obvious syntax/comment issues.
