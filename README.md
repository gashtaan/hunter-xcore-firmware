# Alternative firmware for Hunter X-Core controller
This firmware is intended to be used in Hunter X-Core irrigation controllers that are using PIC18F86K90 as main MCU. As far I know at least unit revisions 2.10 and 3.11 are using this chip. The first revision is using Samsung MCU based on Zilog architecture, but as is their custom, it's completely proprietary. On the contrary the PIC microcontrollers are pretty well documented by Microchip, many thanks to them for it.

## Build
The project can be built in Microchip MPLAB X IDE.

Before the build, it's critical to configure correct unit revision by setting 2 or 3 in XCORE_VERSION macro in types.h file. It's important because Hunter changes the way how station triacs are driven - in revision 2 they are driven directly by MCU, but in revision 3 they are driven in **opposite logic** by additional transistors. If you choose wrong revision, after you power the unit up, all connected valves will be opened at the same time, so there is a risc of overloading the transformer. Also overcurrent protection is configured according the selected revision because of different power supply voltages. Check the revision on PCB, even better check PCB layout.

## Install
After the build, firmware can be uploaded into MCU via ICSP - all signals and power rail is accessible on PCB either on edge connector or pads that are accessible even from outside (they're located under the front sticker with X-CORE logo on it). Use the Pickit, bitbang it or if you have built the remote access module, you can upload firmware directly via remote interface.

## Remote access
The remote access module with ESP8266 can't be powered from the unit directly, because logic voltage regulation in X-Core unit is horribly inefficient. After the rectification from AC it's regulated down to 24V using zener diodes, then to **negative** -3.3 or -5V (depends on unit's revision) using linear voltage regulation wasting watts of energy in heat, just to power MCU that consumes mostly around 1mA. Powering ESP8266 which can consume 400mA at peak is simply impossible.

Because of that I choose the option to power the remote module from AC input, regulate to 3.3V with efficient switching regulator and connect to the unit via optocouplers. To communicate with MCU I use existing ICSP headers, this way I can do direct OTA updates of MCU firmware.

Firmware of remote module serves simple web-page that allows the user to start selected program, stations and adjust seasonal settings. It also allows update the current time of unit's RTC clock, which is also performed automatically at boot.
