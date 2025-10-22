from PyQt5 import QtCore, QtGui, QtWidgets

from player_ui import Ui_Form
from flight_data import FlightData
from can_bus.interface import CanInterface, CanFrames
from logger import Logger
from nmea import Nmea
from baro_widget import BaroWidget

# The player widget controls the playback of the larus flight data and displays the status

class Player(QtWidgets.QWidget):
    def __init__(self, parent: QtWidgets.QWidget, dir: str, logger: Logger, can_frames: CanFrames):
        """Initialize the player widget (parent is parent widget, dir is directory of mainwindow)"""
        super().__init__(parent)
        self.ui = Ui_Form()
        self.ui.setupUi(self)

        self.logger = logger
        self.data = FlightData()
        self.can = CanInterface(5005, logger, can_frames)
        self.nmea = Nmea(8881)
        self.file_open = False
        self.tick_cnt = 0
        self.is_running = False
        self.blink = False

        self.led = QtGui.QPixmap(dir + "/icons/green_led.png")
        self.no_led = QtGui.QPixmap(dir + "/icons/no_led.png")

        settings = QtCore.QSettings()

        self.ui.cbCan.addItem('UDP')
        self.ui.cbCan.currentTextChanged.connect(self.can.set_interface)
        if self.can.canbus_available():
            self.ui.cbCan.addItem('CAN')
            self.ui.cbCan.setCurrentText(settings.value('interface', 'CAN'))

        self.ui.cbNmea.addItem('UDP')

        self.ui.cbProtocol.addItem('legacy')
        self.ui.cbProtocol.addItem('new')
        self.ui.cbProtocol.currentTextChanged.connect(self.can.set_protocol)
        self.ui.cbProtocol.setCurrentText(settings.value('protocol', 'new'))

        self.ui.hsPlayerSpeed.valueChanged.connect(self.set_player_speed)

        self.ui.lbBlink.setPixmap(self.no_led)
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.tick_100ms)
        self.timer2 = QtCore.QTimer()
        self.timer2.timeout.connect(self.tick_1s)
        self.timer2.start(1000)
        self.set_player_speed()
        self.baroWidget = self.ui.baroWidget

        self.open_file()

    def start_flight_player(self):
        """Play the larus flight data"""
        self.timer.start(100)
        self.is_running = True
        self.blink = True

    def stop_flight_player(self):
        """Stop playing the data"""
        self.timer.stop()
        self.is_running = False
        self.ui.lbBlink.setPixmap(self.no_led)

    def tick_100ms(self):
        """This routine is call every 100ms and send the flight data through the canbus channel"""
        if self.file_open and self.is_running:
            self.data.tick()
            self.can.can_send_frames(self.data)

    def tick_1s(self):
        """This routine is called every second an actualizes the nmea data"""
        if self.file_open:
            self.ui.lbFlightTimeA.setText(str(self.data.time()))
            if self.is_running:
                self.nmea.send_data(self.data)
                if self.blink:
                    self.ui.lbBlink.setPixmap(self.led)
                else:
                    self.ui.lbBlink.setPixmap(self.no_led)
                self.blink = not self.blink
                rel_pos = self.data.get_relative()
                self.baroWidget.set_vline_rel(rel_pos)
                self.ui.lbIasA.setText(f"{self.data['ias']*3.6:3.0f}")
                self.ui.lbAltitudeA.setText(f"{-self.data['position_down']:4.0f}")

    def open_file(self):
        """Opens a file containing Larus flight data"""
        settings = QtCore.QSettings()
        fileName = settings.value("fileName", "") 

        if fileName != '':
            self.stop_flight_player()
            QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)

            try:
                self.data.from_file(fileName)
                self.file_open = True
            except:
                self.file_open = False
           
            if self.file_open:
                self.ui.lbFileNameA.setText(fileName.split('/')[-1])
                self.ui.lbDateA.setText(str(self.data.date_of_flight()))
                self.ui.lbStartRecordingA.setText(str(self.data.start_recording()))
                self.ui.lbStopRecordingA.setText(str(self.data.end_recording()))
                self.ui.verticalLayout.removeWidget(self.baroWidget)
                self.baroWidget = BaroWidget(self.data)
                self.baroWidget.posChanged.connect(self.set_player_pos)
                self.ui.verticalLayout.insertWidget(3, self.baroWidget)
                QtWidgets.QApplication.restoreOverrideCursor()
            else:
                self.ui.lbFileNameA.setText("")
                self.ui.lbDateA.setText("")
                self.ui.lbStartRecordingA.setText("")
                self.ui.lbStopRecordingA.setText("")
    
                QtWidgets.QApplication.restoreOverrideCursor()
                msgBox = QtWidgets.QMessageBox()
                msgBox.setIcon(QtWidgets.QMessageBox.Critical)
                msgBox.setText("Could not open file")
                msgBox.exec()

            self.set_player_pos(0.0)
            self.is_running = False

    def set_player_speed(self):
        """The playback rate of the player can be in the range of 0.1 to 10 times the original speed"""
        pos = self.ui.hsPlayerSpeed.value() # 1..20
        if pos < 10:
            speed = 0.1 * pos
        else:
            speed = (pos - 9) * 1.0
        self.ui.lbSpeed.setText(f"{speed:3.1f}")
        self.data.set_speed(speed)

    def set_player_pos(self, pos):
        """Sets the temporal position within the opened flight"""
        if self.file_open:
            self.data.set_relative(pos)
            self.ui.lbFlightTimeA.setText(str(self.data.time()))
