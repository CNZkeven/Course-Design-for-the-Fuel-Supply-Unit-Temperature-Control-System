from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, TextIO
import csv


@dataclass
class Record:
    timestamp: str
    temp: float
    set_temp: float
    p: float
    i: float
    d: float
    dac: float
    volt: float
    run: int
    alarm: int
    limit: float


@dataclass
class Logger:
    records: list[Record] = field(default_factory=list)
    recording: bool = False
    file_path: Path | None = field(default=None, init=False)
    _file: TextIO | None = field(default=None, init=False, repr=False)
    _writer: Any = field(default=None, init=False, repr=False)

    HEADER = [
        "timestamp", "temp", "set_temp", "p", "i", "d",
        "dac", "volt", "run", "alarm", "limit"
    ]

    def start(self, path: str | Path) -> None:
        self.stop()
        self.records.clear()
        self.file_path = Path(path)
        self.file_path.parent.mkdir(parents=True, exist_ok=True)
        self._file = self.file_path.open("w", newline="", encoding="utf-8")
        self._writer = csv.writer(self._file)
        self._writer.writerow(self.HEADER)
        self._file.flush()
        self.recording = True

    def stop(self) -> None:
        self.recording = False
        if self._file is not None:
            self._file.flush()
            self._file.close()
        self._file = None
        self._writer = None

    def add(self, data) -> None:
        if not self.recording or self._file is None or self._writer is None:
            return
        ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        record = Record(ts, data.temp, data.set_temp, data.p, data.i, data.d,
                        data.dac, data.volt, data.run, data.alarm, data.limit)
        self.records.append(record)
        self._writer.writerow([
            record.timestamp, record.temp, record.set_temp, record.p, record.i, record.d,
            record.dac, record.volt, record.run, record.alarm, record.limit
        ])
        self._file.flush()

    def export_csv(self, path: str) -> None:
        with open(path, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(self.HEADER)
            for r in self.records:
                writer.writerow([
                    r.timestamp, r.temp, r.set_temp, r.p, r.i, r.d,
                    r.dac, r.volt, r.run, r.alarm, r.limit
                ])
