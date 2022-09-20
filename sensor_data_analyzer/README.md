Software-In-The-Loop Simulator for the Larus Glider Flight Sensor
=================================================================

* Eclipse CDT project for a headless program to replay and process flight sensor data

* The output can be either a binary file containing float32_t binary data (raw measurements plus derived data)

* The program can also be used to feed data in real-time to a XCsoar instance on the PC or over the netword (Port 8880 TCP) in OpenVario NMEA format.
* Optionally CAN data can be tunneled through a USART gateway to feed an external display. We use a modified AD57 from air-avionics.

This software needs a subproject **"lib"** that contains the **Larus** variometer and wind measurement algorithms to be
tested using this software.

How to use it: Import the project including the lib subproject using Eclipse CDT on a Linux machine, compile and run.
