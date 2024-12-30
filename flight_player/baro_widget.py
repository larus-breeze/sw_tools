from PyQt5 import QtCore, QtWidgets
import pyqtgraph as pg
from pyqtgraph import PlotWidget
import numpy as np

from flight_data import FlightData

class BaroWidget(PlotWidget):

    setPos = QtCore.pyqtSignal(float)

    def __init__(self, data: FlightData, parent=None, figsize=(1024, 200)):
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

    def clear(self):
        pass

    def set_vline_abs(self, abs_pos):
        self.vline.setPos(self.clip_pos(abs_pos))

    def set_vline_rel(self, rel_pos):
        abs_pos = rel_pos * (self.pos_max - self.pos_min) 
        self.set_vline_abs(abs_pos)

    def to_relative_pos(self, abs_pos):
        abs_pos = self.clip_pos(abs_pos)
        rel_pos = (abs_pos - self.pos_min) / (self.pos_max - self.pos_min)
        return rel_pos

    def clip_pos(self, pos):
        if pos < self.pos_min:
            pos = self.pos_min
        if pos > self.pos_max:
            pos = self.pos_max
        return pos

    def mouse_clicked(self, event):
        event.accept()
        pos = event.scenePos()
        abs_pos = self.p1.getViewBox().mapSceneToView(pos).x()
        self.set_vline_abs(abs_pos)
        self.setPos.emit(self.to_relative_pos(abs_pos))
