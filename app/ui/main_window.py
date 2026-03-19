import time
from pathlib import Path

from PySide6.QtCore import Qt
from PySide6.QtGui import QDoubleValidator
from PySide6.QtSerialPort import QSerialPortInfo
from PySide6.QtWidgets import (
    QComboBox, QFrame, QGridLayout, QHBoxLayout, QLabel, QLineEdit,
    QMainWindow, QPushButton, QTextEdit, QVBoxLayout, QWidget,
    QGraphicsDropShadowEffect
)

import pyqtgraph as pg

from app.core.protocol import parse_line, build_command, is_telemetry_line
from app.core.serial_manager import SerialManager
from app.services.logger import Logger
from app.services.settings import load_settings, save_settings
from app.ui.widgets import KpiCard, StatusLight, PortComboBox


class MainWindow(QMainWindow):
    """燃油供应单元温控上位机主界面。

    主窗口承担四类职责：
    - 串口连接与命令下发
    - 实时数据显示与状态指示
    - 温度/设定/输出曲线绘制
    - 实验记录保存
    """

    def __init__(self):
        super().__init__()
        self.setWindowTitle("燃油供应单元温度控制")
        self.resize(1200, 720)

        # 串口管理器负责收发行协议，Logger 负责把遥测写入 record.csv。
        self.serial = SerialManager(self)
        self.logger = Logger()
        self.settings_path = Path("settings.json")
        self.record_path = Path(__file__).resolve().parents[2] / "record.csv"

        self._build_ui()
        self._load_settings()
        self._wire_signals()

    def _build_ui(self) -> None:
        root = QWidget()
        self.setCentralWidget(root)

        main = QHBoxLayout(root)
        main.setContentsMargins(16, 16, 16, 16)
        main.setSpacing(16)

        # 左侧为控制区：串口、启停、参数和状态灯。
        left = QVBoxLayout()
        left.setSpacing(12)

        self.card_serial = self._card("串口连接", "serial")
        serial_layout = self.card_serial.layout()
        self.port_combo = PortComboBox(self._refresh_ports)
        self._refresh_ports()
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["115200", "9600", "19200", "57600"])
        self.baud_combo.setCurrentText("115200")
        self.btn_connect = QPushButton("连接")
        self.btn_disconnect = QPushButton("断开")
        self.btn_disconnect.setEnabled(False)
        serial_layout.addWidget(QLabel("端口"))
        serial_layout.addWidget(self.port_combo)
        serial_layout.addWidget(QLabel("波特率"))
        serial_layout.addWidget(self.baud_combo)
        serial_layout.addWidget(self.btn_connect)
        serial_layout.addWidget(self.btn_disconnect)

        self.card_control = self._card("运行控制", "control")
        control_layout = self.card_control.layout()
        self.btn_start = QPushButton("启动")
        self.btn_stop = QPushButton("停止")
        self.btn_start.setObjectName("btnStart")
        self.btn_stop.setObjectName("btnStop")
        control_layout.addWidget(self.btn_start)
        control_layout.addWidget(self.btn_stop)

        self.card_params = self._card("参数设置", "params")
        params_container = QWidget()
        params_layout = QGridLayout(params_container)
        self.input_set = QLineEdit()
        self.input_p = QLineEdit()
        self.input_i = QLineEdit()
        self.input_d = QLineEdit()
        self.input_limit = QLineEdit()
        val = QDoubleValidator(bottom=-9999, top=9999, decimals=2)
        for w in [self.input_set, self.input_p, self.input_i, self.input_d, self.input_limit]:
            w.setValidator(val)
        params_layout.addWidget(QLabel("设定温度"), 0, 0)
        params_layout.addWidget(self.input_set, 0, 1)
        params_layout.addWidget(QLabel("比例 P"), 1, 0)
        params_layout.addWidget(self.input_p, 1, 1)
        params_layout.addWidget(QLabel("积分 I"), 2, 0)
        params_layout.addWidget(self.input_i, 2, 1)
        params_layout.addWidget(QLabel("微分 D"), 3, 0)
        params_layout.addWidget(self.input_d, 3, 1)
        params_layout.addWidget(QLabel("超温阈值"), 4, 0)
        params_layout.addWidget(self.input_limit, 4, 1)
        self.btn_apply = QPushButton("下发到下位机")
        params_layout.addWidget(self.btn_apply, 5, 0, 1, 2)
        self.card_params.layout().addWidget(params_container)

        self.card_status = self._card("状态", "status")
        status_layout = self.card_status.layout()
        self.light_run = StatusLight("运行")
        self.light_alarm = StatusLight("报警")
        status_layout.addWidget(self.light_run)
        status_layout.addWidget(self.light_alarm)

        left.addWidget(self.card_serial)
        left.addWidget(self.card_control)
        left.addWidget(self.card_params)
        left.addWidget(self.card_status)
        left.addStretch(1)

        # 右侧为监控区：关键指标、曲线和日志。
        right = QVBoxLayout()
        right.setSpacing(12)

        kpi_row = QHBoxLayout()
        self.kpi_temp = KpiCard("当前温度", "℃")
        self.kpi_set = KpiCard("设定温度", "℃")
        self.kpi_volt = KpiCard("输出电压", "V")
        self.kpi_dac = KpiCard("DAC", "")
        kpi_row.addWidget(self.kpi_temp)
        kpi_row.addWidget(self.kpi_set)
        kpi_row.addWidget(self.kpi_volt)
        kpi_row.addWidget(self.kpi_dac)

        header = QFrame()
        header.setObjectName("plotHeader")
        header_layout = QHBoxLayout(header)
        header_layout.setContentsMargins(6, 4, 6, 4)
        header_label = QLabel("温度曲线")
        header_label.setObjectName("plotTitle")
        header_layout.addWidget(header_label)
        header_layout.addStretch(1)

        self.plot = pg.PlotWidget()
        self.plot.setBackground("w")
        self.plot.addLegend()
        self.plot.showGrid(x=True, y=True, alpha=0.3)
        self.plot.setLabel("bottom", "时间", units="s")
        self.plot.setLabel("left", "温度 / 电压")
        self.plot.getAxis("left").setTextPen(pg.mkPen("#1f2937"))
        self.plot.getAxis("bottom").setTextPen(pg.mkPen("#1f2937"))
        self.plot.getAxis("left").setPen(pg.mkPen("#cbd5e1"))
        self.plot.getAxis("bottom").setPen(pg.mkPen("#cbd5e1"))
        self.curve_temp = self.plot.plot(pen=pg.mkPen("#16a34a", width=2), name="温度")
        self.curve_set = self.plot.plot(pen=pg.mkPen("#2563eb", width=2), name="设定")
        self.curve_out = self.plot.plot(pen=pg.mkPen("#f59e0b", width=2), name="输出")

        self.log = QTextEdit()
        self.log.setReadOnly(True)

        record_row = QHBoxLayout()
        self.btn_record_start = QPushButton("开始记录")
        self.btn_record_stop = QPushButton("停止记录")
        self.btn_record_stop.setEnabled(False)
        record_row.addWidget(self.btn_record_start)
        record_row.addWidget(self.btn_record_stop)

        right.addLayout(kpi_row)
        right.addWidget(header)
        right.addWidget(self.plot, 1)
        right.addLayout(record_row)
        right.addWidget(self.log, 0)

        main.addLayout(left, 0)
        main.addLayout(right, 1)

        self._apply_style()

        # 曲线数据缓存。_t0 记录本次实验首帧时刻，横轴统一换算为相对秒数。
        self.data_x = []
        self.data_temp = []
        self.data_set = []
        self.data_out = []
        self._t0 = None

        self._apply_shadow(self.card_serial)
        self._apply_shadow(self.card_control)
        self._apply_shadow(self.card_params)
        self._apply_shadow(self.card_status)

    def _card(self, title: str, card_type: str | None = None) -> QFrame:
        frame = QFrame()
        frame.setObjectName("card")
        if card_type:
            frame.setProperty("type", card_type)
        layout = QVBoxLayout(frame)
        label = QLabel(title)
        label.setObjectName("cardTitle")
        layout.addWidget(label)
        return frame

    def _apply_style(self) -> None:
        style_path = Path("assets/style.qss")
        if style_path.exists():
            self.setStyleSheet(style_path.read_text(encoding="utf-8"))

    def _apply_shadow(self, widget: QWidget) -> None:
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(18)
        shadow.setOffset(0, 4)
        shadow.setColor(Qt.gray)
        widget.setGraphicsEffect(shadow)

    def _refresh_ports(self) -> None:
        current = self.port_combo.currentText()
        self.port_combo.clear()
        for info in QSerialPortInfo.availablePorts():
            self.port_combo.addItem(info.portName())
        if current:
            idx = self.port_combo.findText(current)
            if idx >= 0:
                self.port_combo.setCurrentIndex(idx)

    def _load_settings(self) -> None:
        # 上次保存的参数可在重新打开上位机时自动恢复，方便重复实验。
        if self.settings_path.exists():
            data = load_settings(self.settings_path)
        else:
            data = {"set_temp": 60.0, "p": 50.0, "i": 0.1, "d": 0.0, "limit": 75.0}
        self.input_set.setText(str(data["set_temp"]))
        self.input_p.setText(str(data["p"]))
        self.input_i.setText(str(data["i"]))
        self.input_d.setText(str(data["d"]))
        self.input_limit.setText(str(data["limit"]))

    def _save_settings(self) -> None:
        data = {
            "set_temp": float(self.input_set.text() or 0),
            "p": float(self.input_p.text() or 0),
            "i": float(self.input_i.text() or 0),
            "d": float(self.input_d.text() or 0),
            "limit": float(self.input_limit.text() or 0),
        }
        save_settings(self.settings_path, data)

    def _wire_signals(self) -> None:
        # UI 操作统一绑定到对应槽函数，便于把“界面事件”和“串口业务”分离。
        self.btn_connect.clicked.connect(self._on_connect)
        self.btn_disconnect.clicked.connect(self._on_disconnect)
        self.btn_start.clicked.connect(self._on_start)
        self.btn_stop.clicked.connect(lambda: self.serial.send(build_command("STOP")))
        self.btn_apply.clicked.connect(self._apply_params)

        self.btn_record_start.clicked.connect(self._record_start)
        self.btn_record_stop.clicked.connect(self._record_stop)

        self.serial.line_received.connect(self._on_line)
        self.serial.status_changed.connect(self._on_status)

    def _on_connect(self) -> None:
        port = self.port_combo.currentText()
        baud = int(self.baud_combo.currentText())
        if self.serial.connect_port(port, baud):
            self.btn_connect.setEnabled(False)
            self.btn_disconnect.setEnabled(True)
            self.log.append(f"已连接: {port}@{baud}")
        else:
            self.log.append("连接失败")

    def _on_disconnect(self) -> None:
        self.serial.disconnect_port()
        self.btn_connect.setEnabled(True)
        self.btn_disconnect.setEnabled(False)
        self.log.append("已断开")

    def _on_start(self) -> None:
        # 每次重新开始实验时清空旧曲线，避免不同实验过程叠在一起。
        self._t0 = None
        self.data_x.clear()
        self.data_temp.clear()
        self.data_set.clear()
        self.data_out.clear()
        self.serial.send(build_command("START"))

    def _apply_params(self) -> None:
        # 无论是否连上 MCU，都先把当前参数保存到本地。
        self._save_settings()
        if not self.serial._port.isOpen():
            self.log.append("参数已本地保存（未连接）")
            return
        cmds = [
            build_command("TEMP", float(self.input_set.text() or 0)),
            build_command("P", float(self.input_p.text() or 0)),
            build_command("I", float(self.input_i.text() or 0)),
            build_command("D", float(self.input_d.text() or 0)),
            build_command("LIMIT", float(self.input_limit.text() or 0)),
        ]
        # MCU 采用按行命令解析，主循环节拍约 100ms，因此批量下发时要留间隔。
        self.serial.send_batch(cmds, interval_ms=120)

    def _record_start(self) -> None:
        try:
            self.logger.start(self.record_path)
        except OSError as exc:
            self.log.append(f"开始记录失败: {self.record_path} ({exc})")
            return
        self.btn_record_start.setEnabled(False)
        self.btn_record_stop.setEnabled(True)
        self.log.append(f"开始记录，保存到: {self.record_path}")

    def _record_stop(self) -> None:
        try:
            self.logger.stop()
        except OSError as exc:
            self.log.append(f"停止记录失败: {self.record_path} ({exc})")
            return
        self.btn_record_start.setEnabled(True)
        self.btn_record_stop.setEnabled(False)
        self.log.append(f"记录已保存: {self.record_path}")

    def _on_line(self, line: str) -> None:
        # 非遥测帧通常是 MCU 的提示信息，例如 "Start heating"、"Unknown command"。
        if not is_telemetry_line(line):
            self.log.append(line)
            return
        try:
            data = parse_line(line)
        except Exception:
            self.log.append(f"解析失败: {line}")
            return

        # 把一帧串口遥测同步更新到 KPI、状态灯、记录器和曲线图。
        self.kpi_temp.set_value(f"{data.temp:.2f}")
        self.kpi_set.set_value(f"{data.set_temp:.2f}")
        self.kpi_volt.set_value(f"{data.volt:.3f}")
        self.kpi_dac.set_value(f"{data.dac:.0f}")
        self.light_run.set_state(bool(data.run))
        self.light_alarm.set_state(False, alarm=bool(data.alarm))

        self.logger.add(data)

        now = time.monotonic()
        if self._t0 is None:
            # 首帧作为时间零点，后续所有点都用相对时间显示，更适合实验分析。
            self._t0 = now
        elapsed = now - self._t0

        self.data_x.append(elapsed)
        self.data_temp.append(data.temp)
        self.data_set.append(data.set_temp)
        self.data_out.append(data.volt)

        self.curve_temp.setData(self.data_x, self.data_temp)
        self.curve_set.setData(self.data_x, self.data_set)
        self.curve_out.setData(self.data_x, self.data_out)

    def _on_status(self, status: str) -> None:
        self.log.append(f"串口状态: {status}")
