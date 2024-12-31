Larus Flight Player
===================

The Larus Flight Player is a small Python tool for playing Larus flight recordings.

![flight_player](https://github.com/user-attachments/assets/0c8368e6-fc0d-484c-b3a2-e363310241fb)

Installation
------------

An executable python 3 installation is required. Besides the standard library the following modules are required:

```
$ pip3 install python-can pandas pyqt5 pyqtcharts numpy pyqtgraph
```

Usage
-----

The tool can be used under Linux and Windows. 

```
$ python3 mainwindow.py
```

It is possible to analyze different fxx files of the sensor box (*.f37, *.f110, *.f114, ...). Some of the data is
automatically converted into readable and analyzable formats. With large files, this can take quite a while until 
the app opens.

In the lower part, the flight is displayed in the form of a barogram. You can navigate and zoom in this display. 
A mouse click determines what exactly is played. The red line shows where the player is currently working. The output 
of the CAN packets is done either via UDP port 5005 or an active CAN bus adapter. The output of the NMEA data is done 
via UDP port 8881.

Configure a generic CAN output to the can0 network interface on Linux systems via:  

      sudo ip link set can0 up type can bitrate 1000000
      sudo ifconfig can0 txqueuelen 1000
