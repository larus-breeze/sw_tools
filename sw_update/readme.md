Software update of Larus components
===================================
General
-------
All Larus units have software included and it is therefore necessary that the software of the units is kept up to date. The software update should be easy to perform for the user. The possible input channels are not limited, they include SdCard, Can Bus, RS323 UART, WLAN and Bluetooth. The implementation is provided depending on the specific device.

The implementation effort within the end devices should be manageable. For this reason, a generic approach is chosen that requires little product-specific implementation. A software update always takes place in two steps:
- First, a suitable software image is copied into a free memory area of the device. It is the responsibility of the running unit software to organise this. Own bootloaders on the respective system are possible, but not mandatory.
- The current firmware is then replaced with the new firmware in a device.

Two software components are provided here that can be used generally:
- A generic Python script that can pack any update image. It can be used for any device.
- A copy function that replaces the current firmware with the new firmware. This component depends on the controller used, but not on the device type or hardware. The support of new controllers is easy to implement.

Update Procedure
----------------
To create an update option, the developer must **package the finished software together with some meta data and a copy routine**. The update image is packed using the Python script "pack_image.py". The included copy routine depends on the controller used and its memory map. The copy routine is not dependent on the specific device or application. It can be easily adapted to new controllers or memory constellations.
 
During the update process, the **running application** carries out the following points:
- Is the offered update trustworthy (Magic Number)?
- Is the offered update compatible with the hardware (HW release)?
- Does the software version offered make sense (SW release)? For example, it will not make sense to reinstall an identical software version.
- If all of the above points have been approved, the update is loaded and copied to a previously defined memory area.
- A copy routine is then started, which is also loaded into the memory area in question.
- **Please note:** The running application must ensure that the copy routine can complete its task. This means that the following conditions must be met before the call:
  - The watchdog must be inactive.
  - The MPU, if present, must be inactive.

The **copy routine** included in the update package performs the following:
- Is the offered update trustworthy (Magic Number)?
- Is the integrity of the update package ensured (CRC check)?
- Delete the current application.
- Copy the new application to the correct location.
- Restart the device.

Implementation in Detail
------------------------
The following diagram shows the image of the memory of a device after the running application has loaded the image and before the copy routine has been started. You can see in detail which memory areas and meta data are available. The absolute addresses are given as an example (stm32f407).

![memory_map](https://github.com/larus-breeze/sw_tools/blob/master/sw_update/assets/copy_stm32f407_1m.png)

This repository provides the following components:
- pack_image.py: Generally usable tool for creating update images
- stm32f407_1m: Implementation for the stm32f407 controller (copy app, example app)
- stm32h743_1m: Implementation for the stm32h743 controller (copy app, example app)

Some hints
- The example app and the copy app work with different memory regions (memory.x). Before the other app can be compiled, the build directory must be deleted with cargo clean. Otherwise the result is not usable.
- The copy app can be tested on its own by starting it in the debugger and stopping it at the beginning. Before the application is started, the finished image can be written over it (Openocd command "mon program image.bin \<address\>")
- The sample app can then be tested, again by stopping it after starting. Before it runs, the image must be reloaded (Openocd command "mon program image.bin \<address\>").