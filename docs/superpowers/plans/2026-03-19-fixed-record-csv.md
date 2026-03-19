# Fixed Record CSV Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让上位机在点击“开始记录”时立即将日志写入项目根目录 `record.csv`，点击“停止记录”时直接完成保存且不再弹窗导出。

**Architecture:** 在 `Logger` 中加入固定路径文件写入能力，将 CSV 的创建、表头写入、逐行追加与关闭文件集中到日志服务里。界面层只负责提供固定路径、更新按钮文案和日志提示。

**Tech Stack:** Python 3, PySide6, pytest, csv, pathlib

---

## Chunk 1: Logger Fixed File Output

### Task 1: Add failing logger tests

**Files:**
- Create: `tests/test_logger.py`
- Modify: `app/services/logger.py`
- Test: `tests/test_logger.py`

- [ ] **Step 1: Write the failing test**

```python
def test_start_creates_record_csv_and_writes_rows(tmp_path):
    ...
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python3 -m pytest tests/test_logger.py -q`
Expected: FAIL because `Logger.start()` does not accept a path and does not write a file.

- [ ] **Step 3: Write minimal implementation**

```python
class Logger:
    def start(self, path):
        ...
```

- [ ] **Step 4: Run test to verify it passes**

Run: `python3 -m pytest tests/test_logger.py -q`
Expected: PASS

## Chunk 2: UI Fixed Path Wiring

### Task 2: Update main window recording flow

**Files:**
- Modify: `app/ui/main_window.py`
- Modify: `README.md`
- Test: `tests/test_logger.py`

- [ ] **Step 1: Update the UI strings and fixed path wiring**
- [ ] **Step 2: Remove the export dialog from stop-record**
- [ ] **Step 3: Run focused tests**

Run: `python3 -m pytest tests/test_logger.py tests/test_protocol.py -q`
Expected: PASS
