from dataclasses import dataclass


@dataclass
class Telemetry:
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
