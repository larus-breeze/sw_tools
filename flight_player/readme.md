Larus Flight Player
===================

The Larus Flight Player is a small Python tool for playing Larus flight recordings.

![Screenshot](https://github.com/larus-breeze/sw_emulator_sil/blob/master/flight_player/screenshot.png)

Installation
------------

An executable python 3 installation is required. Besides the standard library the following modules are required:

```
$ pip3 install matplotlib python-can pandas pyqt5 numpy
```

Usage
-----

The tool can be used under Linux and Windows. 

First, the Larus flight record (\*.f37) must be translated to a displayable format (\*.f110, \*.f114) using the [sensor_data_anlayser](https://github.com/larus-breeze/sw_emulator_sil).

After starting the tool, a displayable file can be opened (Menu/Open). 
```
$ python3 mainwindow.py
```
Loading a flight can take some time. After loading, the barogram of the flight is displayed. With the slider behind "Emulation Time" the playback position in the flight can be changed. With the arrow the playback can be started, with the double bar the playback can be stopped.

The output of the CAN packets is done either via UDP port 5005 or an active CAN bus adapter. The output of the NMEA data is done via UDP port 8881.

Configure a generic CAN output to the can0 network interface on Linux systems via:  

      sudo ip link set can0 up type can bitrate 1000000
      sudo ifconfig can0 txqueuelen 1000
