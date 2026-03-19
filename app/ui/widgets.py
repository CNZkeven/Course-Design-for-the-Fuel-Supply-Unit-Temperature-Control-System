from PySide6.QtCore import Qt
from PySide6.QtWidgets import QComboBox, QFrame, QLabel, QHBoxLayout, QVBoxLayout


class PortComboBox(QComboBox):
    def __init__(self, refresh_callback, parent=None):
        super().__init__(parent)
        self._refresh_callback = refresh_callback

    def showPopup(self) -> None:
        if self._refresh_callback:
            self._refresh_callback()
        super().showPopup()


class KpiCard(QFrame):
    def __init__(self, title: str, unit: str = "", parent=None):
        super().__init__(parent)
        self.setObjectName("kpiCard")
        self.title = QLabel(title)
        self.title.setObjectName("kpiTitle")
        self.value = QLabel("--")
        self.value.setObjectName("kpiValue")
        self.unit = QLabel(unit)
        self.unit.setObjectName("kpiUnit")

        value_row = QHBoxLayout()
        value_row.addWidget(self.value)
        value_row.addWidget(self.unit, alignment=Qt.AlignBottom)
        value_row.addStretch(1)

        layout = QVBoxLayout(self)
        layout.addWidget(self.title)
        layout.addLayout(value_row)

    def set_value(self, text: str) -> None:
        self.value.setText(text)


class StatusLight(QFrame):
    def __init__(self, label: str, parent=None):
        super().__init__(parent)
        self.setObjectName("statusLight")
        self.dot = QFrame()
        self.dot.setObjectName("statusDot")
        self.text = QLabel(label)
        self.text.setObjectName("statusText")

        layout = QHBoxLayout(self)
        layout.addWidget(self.dot)
        layout.addWidget(self.text)
        layout.addStretch(1)

    def set_state(self, active: bool, alarm: bool = False) -> None:
        if alarm:
            self.dot.setProperty("state", "alarm")
        else:
            self.dot.setProperty("state", "on" if active else "off")
        self.dot.style().unpolish(self.dot)
        self.dot.style().polish(self.dot)
