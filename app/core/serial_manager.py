from PySide6.QtCore import QObject, Signal, QTimer
from PySide6.QtSerialPort import QSerialPort


class SerialManager(QObject):
    """基于 QSerialPort 的行协议串口管理器。

    设计目的：
    - 对 UI 层屏蔽 QtSerialPort 的底层细节
    - 统一处理文本协议的收发
    - 通过缓冲区实现“按行分帧”
    - 通过发送队列实现“分时下发多条命令”
    """

    line_received = Signal(str)
    status_changed = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._port = QSerialPort(self)
        # MCU 回传的是连续字节流，这里按字节缓存直到遇到 '\n' 再分帧。
        self._buffer = bytearray()
        self._port.readyRead.connect(self._on_ready_read)

        # 参数批量下发时使用队列 + 定时器，避免命令过于密集。
        self._send_queue: list[str] = []
        self._send_interval_ms = 0
        self._send_timer = QTimer(self)
        self._send_timer.setSingleShot(True)
        self._send_timer.timeout.connect(self._send_next)

    def connect_port(self, port_name: str, baud: int) -> bool:
        if self._port.isOpen():
            self._port.close()
        self._port.setPortName(port_name)
        # 串口参数与 STM32 端 uart_init() 保持一致：115200, 8N1, 无流控。
        self._port.setBaudRate(baud)
        self._port.setDataBits(QSerialPort.Data8)
        self._port.setParity(QSerialPort.NoParity)
        self._port.setStopBits(QSerialPort.OneStop)
        self._port.setFlowControl(QSerialPort.NoFlowControl)
        ok = self._port.open(QSerialPort.ReadWrite)
        self.status_changed.emit("connected" if ok else "failed")
        return ok

    def disconnect_port(self) -> None:
        if self._port.isOpen():
            self._port.close()
        self.status_changed.emit("disconnected")

    def send(self, text: str) -> None:
        if not self._port.isOpen():
            return
        self._port.write(text.encode("utf-8"))

    def send_batch(self, lines: list[str], interval_ms: int = 120) -> None:
        # MCU 端以主循环节拍处理命令，因此批量命令需要人为拉开间隔。
        if not self._port.isOpen():
            return
        self._send_interval_ms = max(0, interval_ms)
        self._send_queue = list(lines)
        self._send_next()

    def _send_next(self) -> None:
        if not self._port.isOpen():
            self._send_queue = []
            return
        if not self._send_queue:
            return
        text = self._send_queue.pop(0)
        self._port.write(text.encode("utf-8"))
        if self._send_queue:
            self._send_timer.start(self._send_interval_ms)

    def _on_ready_read(self) -> None:
        # readyRead 触发时并不保证只到达一整行，因此必须先累积到缓存区。
        self._buffer.extend(self._port.readAll().data())
        while b"\n" in self._buffer:
            line, _, rest = self._buffer.partition(b"\n")
            self._buffer = bytearray(rest)
            try:
                # MCU 文本行可能带 '\r'，strip() 后便于统一解析。
                decoded = line.decode("utf-8", errors="ignore").strip()
            except Exception:
                decoded = ""
            if decoded:
                self.line_received.emit(decoded)
