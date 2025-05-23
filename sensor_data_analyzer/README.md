# Sensor Data Analyzer
Software-In-The-Loop Simulator for the Larus Glider Flight Sensor

* Eclipse CDT project for a headless program to **replay or process flight sensor data**

* The output can be either a binary file containing float32_t binary data (raw measurements plus derived data)

* The program can also be used to **feed data in real-time to a XCsoar instance** on the PC or over the netword (Port 8880 TCP) in **OpenVario NMEA** format.
* Optionally **CAN data** can be tunneled through a USART gateway (on /dev/ttyUSB0) to feed an external display. We use a modified AD57 from Air-Avionics.
* On Linux generic CAN output to the can0 network interface is also supported. Configure the CAN interface with:  

      sudo ip link set can0 up type can bitrate 1000000
      sudo ifconfig can0 txqueuelen 1000
 
This software needs a subproject [lib](https://github.com/larus-breeze/sw_algorithms_lib) that contains the **Larus** flight-computer algorithms to be SIL-tested using this software.

A Doxygen-generated documentation can be found [here](https://schaefer.eit.h-da.de/Larus_SIL/).

### How to use it: 
Import the project including the lib subproject using **Eclipse CDT** on a Linux machine, compile/link and run.

Or: Clone repository including the submodules using the **command line**: 

**Step one:** Get the files for the project including the "lib"-subproject:

      git clone --recurse-submodules https://github.com/larus-breeze/sw_tools
    
**Step two:** Within an empty workspace in eclipse do:
 
      File/Import_General/Existing Projects into Workspace

to set up your workspace with the Eclipse project for the Software in the loop SIL system.
The root within Eclipse shall point to the directory "sensor_data_analyzer".
"Search for nested projects" must be activated to resolve the library subproject.
After that you will be able to say "build all" to compile and link the project.


The program needs a **pair of files**  foo.f37 (or legacy .f50) and (same name) foo.EEPROM

The *.f37 file contains recorded flight data from the IMU, the pitot and static sensor and the (D-)GNSS
in binary float32_t format, recorded with 100Hz.

The EEPROM-file contains data describing the airplane and sensor, especially the rotation-angles
to rotate the sensor coordinate system into the airframe's coordinates.

Sample flight data can be found [here](https://schaefer.eit.h-da.de/Larus_SIL_testdata/)

**Usage** for an offline run to test the algorithms: 

    sensor_data_analyzer my_data_file.f37

**Usage** for a real- time run to provide data for XCsoar on the PC or a smartphone: 

    sensor_data_analyzer my_data_file.f37 3600
    
The "3600" - argument means: Go for real-time processing, skip the first flight hour and provide the output to a TCP-server on port 8880 on your PC.
  
The Doxygen-generated documentation of the simulator can be found [here](https://schaefer.eit.h-da.de/Larus_SIL/)


### How to use on Windows
- w64devkit recommended for compiling within Eclipse
- The MiniGW linker requires to link this lib  -lws2_32  
- In system_configuration.h  set #define _WIN32 1 

## Links
Supported binary data formats are defined here: [Formats](../analysis/dataformats.py)

The software is licensed under the GNU Public License V3.
