"""上位机串口协议层。

STM32 下位机与 Python 上位机采用纯文本协议通信：

- MCU -> PC：每 100ms 上报一行 10 字段 CSV
- PC -> MCU：发送以 CRLF 结尾的控制命令

本文件只负责“协议格式”本身，不负责串口打开、收发时序和界面刷新。
"""

from .state import Telemetry


def is_telemetry_line(line: str) -> bool:
    # 合法遥测帧固定包含 10 个字段，因此应恰好出现 9 个逗号。
    return line.count(",") == 9


def parse_line(line: str) -> Telemetry:
    # 按 CSV 文本拆分字段，并去掉首尾空白符。
    parts = [p.strip() for p in line.split(",")]
    if len(parts) != 10:
        raise ValueError("invalid field count")
    # 字段顺序必须与 MCU printf 的输出顺序严格一致。
    temp, set_temp, p, i, d, dac, volt, run, alarm, limit_ = parts
    return Telemetry(
        temp=float(temp),
        set_temp=float(set_temp),
        p=float(p),
        i=float(i),
        d=float(d),
        dac=float(dac),
        volt=float(volt),
        run=int(run),
        alarm=int(alarm),
        limit=float(limit_),
    )


def build_command(cmd: str, value: float | None = None) -> str:
    # 下位机接收状态机以 CRLF 作为命令结束符，因此上位机必须统一补齐。
    if value is None:
        return f"{cmd}\r\n"
    return f"{cmd}:{value:.2f}\r\n"
