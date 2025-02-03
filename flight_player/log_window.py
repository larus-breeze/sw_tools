
from logger import Logger
from PyQt5 import QtWidgets, QtGui, QtCore


class LogWindow(QtWidgets.QPlainTextEdit):
    def __init__(self, parent, logger: Logger):
        super().__init__(parent)
        self._logger = logger
        self.setReadOnly(True)
        self._logger.log_info.connect(self._add_msg)

    def _add_msg(self, msg):
        cursor = self.textCursor()
        cursor.movePosition(QtGui.QTextCursor.End)
        self.setTextCursor(cursor)
        self.insertPlainText(msg + '\n')
