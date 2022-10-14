Software-In-The-Loop Simulator for the Larus Glider Flight Sensor
=================================================================

* Eclipse CDT project for a headless program to **replay or process flight sensor data**

* The output can be either a binary file containing float32_t binary data (raw measurements plus derived data)

* The program can also be used to **feed data in real-time to a XCsoar instance** on the PC or over the netword (Port 8880 TCP) in **OpenVario NMEA** format.
* Optionally **CAN data** can be tunneled through a USART gateway (on /dev/ttyUSB0) to feed an external display. We use a modified AD57 from Air-Avionics.

This software needs a subproject [lib](https://github.com/larus-breeze/sw_sensor_algorithms) that contains the **Larus** flight-computer algorithms to be SIL-tested using this software.

# How to use it: 
Import the project including the lib subproject using **Eclipse CDT** on a Linux machine, compile/link and run.

Or: Clone repository including the submodules using the **command line**: 

##Step one: Get the files for the project including the "lib"-subproject:
    git clone --recurse-submodules https://github.com/larus-breeze/SIL_flight_sensor_emulator
##Step two: 
	Within an empty workspace in eclipse do: "File/Import_General/Existing Projects into Workspace"
to set up your workspace with the Eclipse project for the SIL system.
    
The program needs a **pair of files**  foo.f50 and (same name)  foo.f50.EEPROM

The *.f50 file contains recorded flight data from the IMU, the pitot and static sensor and the (D-)GNSS
in binary float32_t format, recorded with 100Hz.

The EEPROM-file contains data describing the airplane and sensor, especially the rotation-angles
to rotate the sensor coordinate system into the airframe's coordinates.

Sample flight data can be found [here](https://schaefer.eit.h-da.de/Larus_SIL_testdata/)

**Usage** for an offline run to test the algorithms: 

    sensor_data_analyzer my_data_file.f50

**Usage** for a real- time run to provide data for XCsoar on the PC or a smartphone: 

    sensor_data_analyzer my_data_file.f50 3600
    
The "3600" - argument means: Go for real-time processing, skip the first flight hour and provide the output to a TCP-server on port 8880 on your PC.
  
The Doxygen-generated documentation of the simulator can be found [here](https://schaefer.eit.h-da.de/Larus_SIL/)

The sorftware is licensed under the GNU Public License V3.