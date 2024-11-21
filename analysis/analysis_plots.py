#!/user/bin/env python3
import sys
import time

import matplotlib.pyplot as plt
import numpy as np


from larus_to_df import Larus2Df
from plot_essentials import *

# PyQT6 imports
from PyQt6.QtWidgets import QMainWindow, QApplication, QPushButton, QFileDialog, QRadioButton, QProgressBar, QLabel, QMessageBox, QDialog, QSplashScreen
from PyQt6.QtCore import pyqtSlot, Qt
from PyQt6.QtGui import QMovie, QPixmap
from PyQt6.QtWidgets import QApplication, QPushButton, QVBoxLayout, QWidget



class Window(QWidget):
    df = None
    magButtonHandle = None
    ahrsButtonHandle = None
    csvButtonHandle = None
    sourceFile = None

    def __init__(self):
        super().__init__()
        fileBtn = QPushButton(text="Open file dialog", parent=self)
        fileBtn.clicked.connect(self.open_dialog)

        magBtn = QPushButton(text="Plot Mag", parent=self)
        self.magButtonHandle = magBtn
        magBtn.clicked.connect(self.plot_mag)
        magBtn.setEnabled(False)

        ahrsBtn = QPushButton(text="Plot AHRS", parent=self)
        self.ahrsButtonHandle = ahrsBtn
        ahrsBtn.clicked.connect(self.plot_ahrs)
        ahrsBtn.setEnabled(False)

        csvBtn = QPushButton(text="Export csv", parent=self)
        self.csvButtonHandle = csvBtn
        csvBtn.clicked.connect(self.export_csv)
        csvBtn.setEnabled(False)


        fileBtn.setFixedSize(200, 60)
        magBtn.setFixedSize(200, 60)
        ahrsBtn.setFixedSize(200, 60)
        csvBtn.setFixedSize(200, 60)
        layout = QVBoxLayout()
        layout.addWidget(fileBtn)
        layout.addWidget(magBtn)
        layout.addWidget(ahrsBtn)
        layout.addWidget(csvBtn)
        self.setLayout(layout)

    @pyqtSlot()
    def open_dialog(self):
        file = QFileDialog.getOpenFileName(
            self,
            "Select a Larus File",
            "${HOME}",
            "Larus raw File (*.f37);; Larus Processed File (*.f114)",
        )
        self.sourceFile = file[0]



        #w1 = QMessageBox.information(self, 'Loading data', 'Please wait ... ',
        #w1 = QMessageBox()
        #w1 = QProgressBar()
        #w1.information(self,'Loading data', "Please wait ...")


        #img = QPixmap("Loading.gif")
        #w1 = QSplashScreen(img)
        #w1.setWindowTitle("Loading data")




        #w1.setValue(50)



        #w1.setWindowFlags(w1.windowFlags())# | Qt.WindowStaysOnTopHint)

        #w1.setEnabled(True)
        #w1.setAutoFillBackground(True)
        #w1.setDetailedText('Hier jetzt?')
        #w1.setText('Please wait ...')
        #w1.setWindowTitle('Loading data')
        #w1.exec()
        #w1.open()
        #w1.show()
        # QApplication::processEvents()
        #TODO: load in thread and signal finish here.






        self.df = Larus2Df(self.sourceFile).get_df()
        #w1.close()
        print("Loaded file {} in dataframe".format(self.sourceFile))
        self.magButtonHandle.setEnabled(True)
        self.ahrsButtonHandle.setEnabled(True)
        self.csvButtonHandle.setEnabled(True)

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