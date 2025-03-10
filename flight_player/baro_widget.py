from PyQt5 import QtCore, QtWidgets
import pyqtgraph as pg
from pyqtgraph import PlotWidget
import numpy as np

from flight_data import FlightData

class BaroWidget(PlotWidget):

    # Signals a change of the cursor by mouse click
    posChanged = QtCore.pyqtSignal(float)

    def __init__(self, data: FlightData, parent=None, figsize=(1024, 200)):
        """Initialize the BaroWidget"""
        PlotWidget.__init__(self, parent)

        self.setBackground("w")

        t, altitude = data.altitude_series()
        self.p1 = self.plot(t, altitude, pen=pg.mkPen('b'))

        self.pos_min = self.p1.dataRect().left()
        self.pos_max = self.p1.dataRect().right()

        self.vline = pg.InfiniteLine(angle=90, movable=False, pen=pg.mkPen('r'))
        self.addItem(self.vline)
        self.set_vline_abs(0)

        self.scene().sigMouseClicked.connect(self.mouse_clicked)

    def set_vline_abs(self, abs_pos):
        """Set the vline to an absolut value"""
        self.vline.setPos(self.clamp_pos(abs_pos))

    def set_vline_rel(self, rel_pos):
        """Set the vline to an relative value (0.0 ... 1.0)"""
        abs_pos = rel_pos * (self.pos_max - self.pos_min) 
        self.set_vline_abs(abs_pos)

    def clamp_pos(self, pos):
        """Limit the values to a valid range"""
        return max(self.pos_min, min(pos, self.pos_max))

    def mouse_clicked(self, event):
        """Process a mouse click"""
        event.accept()
        pos = event.scenePos()
        abs_pos = self.p1.getViewBox().mapSceneToView(pos).x()
        self.set_vline_abs(abs_pos)
        abs_pos = self.clamp_pos(abs_pos)
        rel_pos = (abs_pos - self.pos_min) / (self.pos_max - self.pos_min)
        self.posChanged.emit(rel_pos)
