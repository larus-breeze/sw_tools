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

Config your Linux System
---

You can configure your Linux system to automatically recognize the CAN USB stick and start Flight Player with a single click. This can be done in 4 steps.

**1. Activate Can Adapter when inserting**

Create a file /usr/local/initialize_CAN.sh using sudo with following content

```
#!/bin/bash
ip link set can0 up type can bitrate 1000000
ifconfig can0 txqueuelen 1000
```

Create a file /etc/udev/rules.d/88-init_CAN.rules using sudo with the following content
```
SUBSYSTEM="usb", ACTION="add",ATRRS{idVendor}=="1d50",
ATTRS{idProduct}=="606f", RUN+="/usr/local/initialize_CAN.sh"
```

**2. Create a virtual Python environment and install the Python packages as described in the “Installation” chapter**

**3. Create a script to start the App**

Create a file "flight_player.sh" somewhere under the home directory with the following content. The file must be executable.

```
#!/usr/bin/bash

cd <path-to-larus-directory>/sw_tools/flight_player/
source .venv/bin/activate
python flight_player.py
```

**4. Create a Start Menu Entry**

Create a Start menu entry that activates the generated flight_player script when activated. You can use the icons/larus_breeze.png image file to identify the application.
