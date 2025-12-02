#!/user/bin/env python3
import os
import traceback, sys
from PyQt6.QtWidgets import *
from PyQt6.QtCore import *
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent))   # Add parent folder to make imports of parallel directory possible
from larus_data.larus_to_df import Larus2Df, check_if_larus_file
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
    rootDir = None
    app = None
    df = None
    fileButtonHandle = None
    magButtonHandle = None
    ahrsButtonHandle = None
    altitudeButtonHandle = None
    gnssButtonHandle = None
    attitudeHistButtonHandle = None
    csvButtonHandle = None
    infoButtonHandle = None
    sourceFile = None
    destinationCsvFile = None
    waitingWidget = None
    aboutWidget = None
    worker = None
    threadpool = QThreadPool()

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Larus Analyzer")

        self.fileButtonHandle = QPushButton(text="Open Larus Logfile", parent=self)
        self.fileButtonHandle.clicked.connect(self.open_dialog)

        self.magButtonHandle = QPushButton(text="Plot Magnetics", parent=self)
        self.magButtonHandle.clicked.connect(self.plot_mag)
        self.magButtonHandle.setEnabled(False)

        self.ahrsButtonHandle = QPushButton(text="Plot AHRS", parent=self)
        self.ahrsButtonHandle.clicked.connect(self.plot_ahrs)
        self.ahrsButtonHandle.setEnabled(False)

        self.altitudeButtonHandle = QPushButton(text="Plot Altitude", parent=self)
        self.altitudeButtonHandle.clicked.connect(self.plot_altitude)
        self.altitudeButtonHandle.setEnabled(False)

        self.gnssButtonHandle = QPushButton(text="Plot GNSS", parent=self)
        self.gnssButtonHandle.clicked.connect(self.plot_gnss)
        self.gnssButtonHandle.setEnabled(False)

        self.attitudeHistButtonHandle = QPushButton(text="Plot Attitude Histogram", parent=self)
        self.attitudeHistButtonHandle.clicked.connect(self.plot_attitude_hist)
        self.attitudeHistButtonHandle.setEnabled(False)

        self.csvButtonHandle = QPushButton(text="Export csv", parent=self)
        self.csvButtonHandle.clicked.connect(self.export_csv)
        self.csvButtonHandle.setEnabled(False)

        self.infoButtonHandle = QPushButton(text="About", parent=self)
        self.infoButtonHandle.clicked.connect(self.show_about_widget)
        self.infoButtonHandle.setEnabled(True)


        self.fileButtonHandle.setFixedSize(300, 60)
        self.magButtonHandle.setFixedSize(300, 60)
        self.ahrsButtonHandle.setFixedSize(300, 60)
        self.altitudeButtonHandle.setFixedSize(300, 60)
        self.gnssButtonHandle.setFixedSize(300, 60)
        self.attitudeHistButtonHandle.setFixedSize(300,60)
        self.csvButtonHandle.setFixedSize(300, 60)
        self.infoButtonHandle.setFixedSize(300,60)

        layout = QVBoxLayout()
        layout.addWidget(self.fileButtonHandle)
        layout.addWidget(self.magButtonHandle)
        layout.addWidget(self.ahrsButtonHandle)
        layout.addWidget(self.altitudeButtonHandle)
        layout.addWidget(self.gnssButtonHandle)
        layout.addWidget(self.attitudeHistButtonHandle)
        layout.addWidget(self.csvButtonHandle)
        layout.addWidget(self.infoButtonHandle)
        self.setLayout(layout)

    def disable_buttons(self):
        self.fileButtonHandle.setEnabled(False)
        self.magButtonHandle.setEnabled(False)
        self.ahrsButtonHandle.setEnabled(False)
        self.altitudeButtonHandle.setEnabled(False)
        self.gnssButtonHandle.setEnabled(False)
        self.attitudeHistButtonHandle.setEnabled(False)
        self.csvButtonHandle.setEnabled(False)

    def enable_buttons(self):
        self.fileButtonHandle.setEnabled(True)
        self.magButtonHandle.setEnabled(True)
        self.ahrsButtonHandle.setEnabled(True)
        self.altitudeButtonHandle.setEnabled(True)
        self.gnssButtonHandle.setEnabled(True)
        self.attitudeHistButtonHandle.setEnabled(True)
        self.csvButtonHandle.setEnabled(True)

    def execute_open_data(self, progress_callback):
        self.df = Larus2Df(self.sourceFile).get_df()
        print("Loaded file {} in dataframe".format(self.sourceFile))

    def execute_export_csv(self, progress_callback):
        self.df.to_csv(self.destinationCsvFile)
        print("Stored csv file {}".format(self.destinationCsvFile))

    def thread_complete(self):
        self.waitingWidget.close()
        self.enable_buttons()

    @pyqtSlot()
    def open_dialog(self):
        if self.rootDir is None:
            self.rootDir = QDir.homePath()

        print("Root: {}".format(self.rootDir))
        source_file = QFileDialog.getOpenFileName(
            self,
            caption="Select a Larus File",
            directory=self.rootDir,
            filter="Larus raw File (*.f37)",
        )
        # Set dir so that the file manager opens again here
        print("Return value: {}".format(source_file))
        self.rootDir = source_file[0].replace(os.path.basename(source_file[0]),"")

        if check_if_larus_file(source_file[0]):
            self.sourceFile = source_file[0]
            self.waitingWidget = QDialog()  #QSplashScreen() #Splash displayed on wrong monitor
            self.waitingWidget.setFixedSize(200,80)
            self.waitingWidget.setWindowTitle("Loading data")

            # Create a label with a message
            label = QLabel("Please wait...")

            dialog_layout = QVBoxLayout()
            dialog_layout.addWidget(label)
            self.waitingWidget.setLayout(dialog_layout)

            worker = Worker(self.execute_open_data)  # Any other args, kwargs are passed to the run function
            worker.signals.result.connect(print)
            worker.signals.finished.connect(self.thread_complete)
            self.threadpool.start(worker)

            self.disable_buttons()
            self.waitingWidget.show()


    @pyqtSlot()
    def plot_mag(self):
        plot_mag(self.df, self.sourceFile)

    def plot_ahrs(self):
        plot_ahrs(self.df, self.sourceFile)

    def plot_altitude(self):
        plot_altitude_speed(self.df, self.sourceFile)

    def plot_gnss(self):
        plot_gnss(self.df, self.sourceFile)

    def plot_attitude_hist(self):
        plot_attitude_histogram(self.df, self.sourceFile)

    def show_about_widget(self):
        self.aboutWidget = QDialog()  # QSplashScreen() #Splash displayed on wrong monitor
        self.aboutWidget.setFixedSize(250, 100)
        self.aboutWidget.setWindowTitle("About")
        # Create a label with a message
        label = QLabel(
            "Larus Analyzer \n Version 0.1.0 \n https://github.com/larus-breeze/ \n "
            "for sw_sensor release 0.5.0"
        )

        dialog_layout = QVBoxLayout()
        dialog_layout.addWidget(label)
        self.aboutWidget.setLayout(dialog_layout)
        self.aboutWidget.show()

    def export_csv(self):
        destination_file = QFileDialog.getSaveFileName(
            self,
            "Select a Larus File",
            "{}.csv".format(self.sourceFile),

        )
        self.destinationCsvFile = destination_file[0]
        self.waitingWidget = QDialog()
        self.waitingWidget.setFixedSize(200, 80)
        self.waitingWidget.setWindowTitle("Saving data")

        # Create a label with a message
        label = QLabel("Please wait!\nthis will take \nsome minutes")

        dialog_layout = QVBoxLayout()
        dialog_layout.addWidget(label)
        self.waitingWidget.setLayout(dialog_layout)

        worker = Worker(self.execute_export_csv)  # Any other args, kwargs are passed to the run function
        worker.signals.result.connect(print)
        worker.signals.finished.connect(self.thread_complete)
        self.threadpool.start(worker)

        self.disable_buttons()
        self.waitingWidget.show()




if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = Window()
    window.show()
    sys.exit(app.exec())