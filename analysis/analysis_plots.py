#!/user/bin/env python3
import sys
import matplotlib.pyplot as plt
import numpy as np
from process_to_df import ProcessLarus2Df
from PyQt6.QtWidgets import QMainWindow, QApplication, QPushButton, QFileDialog, QRadioButton
from PyQt6.QtCore import pyqtSlot
from PyQt6.QtWidgets import QApplication, QPushButton, QVBoxLayout, QWidget

def plot_mag(df):
    t = (df.index / 100.0 / 60.0).to_numpy()  # 100Hz ticks to minutes for the time axis
    print(df.columns)

    nav_ind_abs = np.sqrt(
        np.square(df['nav ind mag N']) + np.square(df['nav ind mag E']) + np.square(df['nav ind mag D']))

    # Plot the data
    figure, axis = plt.subplots()
    figure.suptitle("Induction in earth system: ")
    plt.autoscale(enable=True, axis='y')
    axis.grid()
    axis.set_xlabel('t [minutes]')

    axis.plot(t, nav_ind_abs.to_numpy(), "k", linewidth=1)
    axis.legend(["nav ind abs"], loc="lower left")
    par1 = axis.twinx()
    par1.plot(t, df['nav ind mag N'].to_numpy(), "r-", linewidth=0.5)
    par1.plot(t, df['nav ind mag E'].to_numpy(), "g-", linewidth=0.5)
    par1.plot(t, df['nav ind mag D'].to_numpy(), "b-", linewidth=0.5)

    minimum = df['nav ind mag N'].min()
    maximum = df['nav ind mag N'].max()
    if df['nav ind mag E'].min() < minimum:
        minimum = df['nav ind mag E'].min()
    if df['nav ind mag E'].max() > maximum:
        maximum = df['nav ind mag E'].max()
    if df['nav ind mag D'].min() < minimum:
        minimum = df['nav ind mag D'].min()
    if df['nav ind mag D'].max() > maximum:
        maximum = df['nav ind mag D'].max()
    par1.set_ylim(minimum - 0.5, maximum + 1.5)
    par1.legend(['nav ind mag N', 'nav ind mag E', 'nav ind mag D'], loc="lower right")
    plt.show()

class Window(QWidget):
    df = None
    buttonHandle = None

    def __init__(self):
        super().__init__()
        fileBtn = QPushButton(text="Open file dialog", parent=self)
        fileBtn.clicked.connect(self.open_dialog)

        magBtn = QPushButton(text="Plot Mag", parent=self)
        self.buttonHandle = magBtn
        magBtn.clicked.connect(self.plot_mag)
        magBtn.setEnabled(False)

        ahrsBtn = QPushButton(text="Plot AHRS", parent=self)
        ahrsBtn.setEnabled(False)


        fileBtn.setFixedSize(200, 60)
        magBtn.setFixedSize(200, 60)
        ahrsBtn.setFixedSize(200, 60)
        layout = QVBoxLayout()
        layout.addWidget(fileBtn)
        layout.addWidget(magBtn)
        layout.addWidget(ahrsBtn)
        self.setLayout(layout)

    @pyqtSlot()
    def open_dialog(self):
        file_name = QFileDialog.getOpenFileName(
            self,
            "Select a Larus File",
            "${HOME}",
            "Larus raw File (*.f37);; Larus Processed File (*.f114)",
        )

        self.df = ProcessLarus2Df(file_name[0]).get_df()
        print("Loaded file {} in dataframe".format(file_name[0]))
        self.buttonHandle.setEnabled(True)

    @pyqtSlot()
    def plot_mag(self):
        if self.df is not None:
            print(self.df)
        plot_mag(self.df)




if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = Window()
    window.show()
    sys.exit(app.exec())