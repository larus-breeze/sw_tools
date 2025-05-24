Installation CAN USB stick on Linux
=

A UDEV rule is required to automatically identify the USB stick. This must be copied to the directory /etc/udev/rules.d. The rule calls a bash script /usr/local/initialize_CAN.sh which must be located at the specified location with the release for execution.