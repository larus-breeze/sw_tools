#!/user/bin/env python3
import traceback, sys
from PyQt6.QtWidgets import *
from PyQt6.QtCore import *

from larus_to_df import Larus2Df
from plot_essentials import *
class WorkerSignals(QObject):
    finished = pyqtSignal()
    error = pyqtSignal(tuple)
    result = pyqtSignal(object)
    progress = pyqtSignal(int)

class Worker(QRunnable):
    def __init__(self, fn, *args, **kwargs):
        super().__init__()
        self.fn = fn
        self.args = args
        self.kwargs = kwargs
        self.signals = WorkerSignals()
        self.kwargs['progress_callback'] = self.signals.progress

    @pyqtSlot()
    def run(self):
        try:
            result = self.fn(*self.args, **self.kwargs)
        except:
            traceback.print_exc()
            exc_type, value = sys.exc_info()[:2]
            self.signals.error.emit((exc_type, value, traceback.format_exc()))
        finally:
            self.signals.finished.emit()


class Window(QWidget):
    df = None
    fileButtonHandle = None
    magButtonHandle = None
    ahrsButtonHandle = None
    csvButtonHandle = None
    sourceFile = None
    waitingWidget = None
    worker = None
    threadpool = QThreadPool()

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Larus Analyzer")

        self.fileButtonHandle = QPushButton(text="Open Larus Logfile", parent=self)
        self.fileButtonHandle.clicked.connect(self.open_dialog)

        self.magButtonHandle = QPushButton(text="Plot Mag", parent=self)
        self.magButtonHandle.clicked.connect(self.plot_mag)
        self.magButtonHandle.setEnabled(False)

        self.ahrsButtonHandle = QPushButton(text="Plot AHRS", parent=self)
        self.ahrsButtonHandle.clicked.connect(self.plot_ahrs)
        self.ahrsButtonHandle.setEnabled(False)

        self.csvButtonHandle = QPushButton(text="Export csv", parent=self)
        self.csvButtonHandle.clicked.connect(self.export_csv)
        self.csvButtonHandle.setEnabled(False)

        self.fileButtonHandle.setFixedSize(300, 60)
        self.magButtonHandle.setFixedSize(300, 60)
        self.ahrsButtonHandle.setFixedSize(300, 60)
        self.csvButtonHandle.setFixedSize(300, 60)
        layout = QVBoxLayout()
        layout.addWidget(self.fileButtonHandle)
        layout.addWidget(self.magButtonHandle)
        layout.addWidget(self.ahrsButtonHandle)
        layout.addWidget(self.csvButtonHandle)
        self.setLayout(layout)

    def disable_buttons(self):
        self.fileButtonHandle.setEnabled(False)
        self.magButtonHandle.setEnabled(False)
        self.ahrsButtonHandle.setEnabled(False)
        self.csvButtonHandle.setEnabled(False)

    def enable_buttons(self):
        self.fileButtonHandle.setEnabled(True)
        self.magButtonHandle.setEnabled(True)
        self.ahrsButtonHandle.setEnabled(True)
        self.csvButtonHandle.setEnabled(True)

    def execute_this_fn(self, progress_callback):
        self.df = Larus2Df(self.sourceFile).get_df()

    def thread_complete(self):
        self.waitingWidget.close()
        self.enable_buttons()

    @pyqtSlot()
    def open_dialog(self):
        file = QFileDialog.getOpenFileName(
            self,
            "Select a Larus File",
            QDir.homePath(),
            "Larus raw File (*.f37);; Larus Processed File (*.f114)",
        )
        self.sourceFile = file[0]

        self.waitingWidget = QDialog()
        self.waitingWidget.setFixedSize(200,100)
        self.waitingWidget.setWindowTitle("Loading data")

        # Create a label with a message
        label = QLabel("Please wait...")

        dialog_layout = QVBoxLayout()
        dialog_layout.addWidget(label)
        self.waitingWidget.setLayout(dialog_layout)
        self.waitingWidget.setDisabled(True)

        worker = Worker(self.execute_this_fn)  # Any other args, kwargs are passed to the run function
        worker.signals.result.connect(print)
        worker.signals.finished.connect(self.thread_complete)
        self.threadpool.start(worker)

        self.disable_buttons()
        self.waitingWidget.show()
        print("Loaded file {} in dataframe".format(self.sourceFile))

    @pyqtSlot()
    def plot_mag(self):
        if self.df is not None:
            print(self.df)
        plot_mag(self.df)

    def plot_ahrs(self):
        if self.df is not None:
            print(self.df)
        plot_wind(self.df)

    def export_csv(self):
        destination_file = QFileDialog.getSaveFileName(
            self,
            "Select a Larus File",
            "{}.csv".format(self.sourceFile),
            "Larus Processed File (*.f114)",
        )
        print(destination_file[0])
        self.df.to_csv(destination_file[0])


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = Window()
    window.show()
    sys.exit(app.exec())