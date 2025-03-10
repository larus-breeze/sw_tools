from PyQt5 import QtCore


class Logger(QtCore.QObject):
    log_info = QtCore.pyqtSignal(str)

    def __init__(self):
        super(QtCore.QObject, self).__init__()

    def info(self, msg: str):
        self.log_info.emit(msg)

