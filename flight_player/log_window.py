
from logger import Logger
from PyQt5 import QtWidgets, QtGui, QtCore
from details_ui import Ui_details
from can_bus.interface import CanFrames
from can_bus.can_frames import to_u8, to_u16, to_f32

CONFIG_ID = {
    "Volume": 0,
    "McCready": 1,
    "Water Ballast": 2,
    "Bugs": 3,
    "QNH": 4,
    "Pilot Weight": 5,
    "TC Climb Rate": 7,
    "TC Speed To Fly": 8,
    "Speed To Fly / Vario": 9,
}

class LogWindow(QtWidgets.QWidget):
    def __init__(self, parent, logger: Logger, can_frames: CanFrames):
        super().__init__(parent)
        self.ui = Ui_details()
        self.ui.setupUi(self)
        self._logger = logger
        self._can_frames = can_frames
        self.ui.plainTextEdit.setReadOnly(True)
        self._logger.log_info.connect(self._add_msg)
        self.ui.pbSend.clicked.connect(self._send)

    def _add_msg(self, msg):
        cursor = self.ui.plainTextEdit.textCursor()
        cursor.movePosition(QtGui.QTextCursor.End)
        self.ui.plainTextEdit.setTextCursor(cursor)
        self.ui.plainTextEdit.insertPlainText(msg + '\n')

    def _send(self):
        value = self.ui.dsValue.value()
        config_id = CONFIG_ID[self.ui.cbCommand.currentText()]

        data = to_u16(config_id)
        if config_id in (0, 9):
            data += to_u8(int(value)) + b"\00\00\00\00\00"
        else:
            data += b"\00\00" + to_f32(value)

        self._can_frames.add(0x522, data)

