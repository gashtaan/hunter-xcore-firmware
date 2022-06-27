# Alternative firmware for Hunter X-Core controller
This firmware is intended to be used in Hunter X-Core irrigation controllers that are using PIC18F86K90 as main MCU. As far I know at least unit revisions 2.10 and 3.11 are using this chip. The first revision is using Samsung MCU based on Zilog architecture, but as is their custom, it's completely proprietary. On the contrary the PIC microcontrollers are pretty well documented by Microchip, many thanks to them for it.

## Build
The project can be built in Microchip MPLAB X IDE.

Before the build, it's critical to configure correct unit revision by setting 2 or 3 in XCORE_VERSION macro in types.h file. It's important because Hunter changes the way how station triacs are driven - in revision 2 they are driven directly by MCU, but in revision 3 they are driven in **opposite logic** by additional transistors. If you choose wrong revision, after you power the unit up, all connected valves will be opened at the same time, so there is a risc of overloading the transformer. Also overcurrent protection is configured according the selected revision because of different power supply voltages. Check the revision on PCB, even better check PCB layout.

## Install
After the build, firmware can be uploaded into MCU via ICSP - all signals and power rail is accessible on PCB either on edge connector or pads that are accessible even from outside (they're located under the front sticker with X-CORE logo on it). Use the Pickit, bitbang it or if you have built the remote access module, you can upload firmware directly via remote interface.
