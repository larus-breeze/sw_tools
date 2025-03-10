Larus Flight Player
===================

The Larus Flight Player is a small Python tool for playing Larus flight recordings.

![flight_player](https://github.com/user-attachments/assets/0c8368e6-fc0d-484c-b3a2-e363310241fb)

Installation
------------

An executable python 3 installation is required. Besides the standard library some modules are required. First, 
we create a virtual environment and activate it. The corresponding packages are then installed:

```
$ python3 -m venv .venv
$ source .venv/bin/activate
(.venv) $ pip3 install python-can pandas pyqt5 pyqtgraph numpy
```

Usage
-----

The virtual environment must be activated if it is not already activated by the installation.

```
$ source .venv/bin/activate
(.venv) $ python flight_player.py
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

Alternatively, the USB stick can also be activated automatically when it is plugged in or started. To do this, create the file /etc/udev/rules.d/98-can-stick.rules

```
SUBSYSTEM==“usb”, ATTR{idVendor}==“1d50”, ATTR{idProduct}==“606f”, MODE="0666”
```
and the file /etc/systemd/network/80-can.network

```
[Match]
Name=can*

[CAN]
BitRate=1000000
```

Activating the changes 

```
$ systemctl restart systemd-networkd
```