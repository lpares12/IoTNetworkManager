# IoT Network Manager

This repository contains a practical method for managing IoT devices connectivity. This example is made using ESP8266.

The system consists in a different set of states depending on the current ESP8266 status and if there are network credentials saved in flash memory. The following diagram shows the different states network manager can take and how they transition.

![Diagram of states and how they transition](https://image.ibb.co/gM5fJb/esp8266nm.png)

## Format of the credentials

The credentials must be set usnig TCP in the port specified at network_manager.c following the format:
*SSID,password\x03*

where \x03 is the hexadecimal represntation of the ASCII character END OF TEXT or ^C.

The file named credentials.file contains an example. And it cant be sent using the netcat command `cat credentials.file | netcat 192.168.4.1 8089`

Also note that the ESP8266 by default uses ip 192.168.4.1 when in AP mode.
