# Vorgehen:
# 1. Can USB Stick einstecken
# 2. dieses Skript aufrufen
# 3. ./sensor_data_analyzer <logdata_file.f37> <Zeitversatz in Sekunden>
# 4. telnet localhost 8880
#
sudo ip link set can0 up type can bitrate 1000000
sudo ifconfig can0 txqueuelen 1000
