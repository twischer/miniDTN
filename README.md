The INGA sensor platform on Contiki-OS.
=======================================

## INGA

[![Build Status](https://jenkins.ibr.cs.tu-bs.de/jenkins/buildStatus/icon?job=inga--develop--cooja)](http://jenkins.ibr.cs.tu-bs.de/jenkins/view/INGA/job/inga--develop--compile/)

INGA is an Open Source Wireless Sensor Node for many different applications. 
INGA was developed at IBR as Inexpensive Node for General Applications and became part of many projects.

Current release version: 2.6-20131107

### Requirements
Required packages to be able to programm with recent linux (Debian/Ubuntu) systems:
`make pkg-config avr-libc binutils-avr gcc-avr avrdude libusb-dev libftdi-dev libpopt-dev libudev1 libudev-dev`

To gain write access (to flash INGA) even as regular user, a custom udev rule will change the group of INGAs USB device file every time you connect your INGA. To setup this, simply copy `examples/inga/99-inga-usb.rules` into `/etc/udev/rules.d/`(depending on your configuration you need to modify the group names). This will also make INGA nodes available as `/dev/inga/node-$ID`.

### Flashing process
This is a simple example how compiling and flashing works with contiki:

	$ cd examples/hello-world/
	$ make TARGET=inga savetarget
	$ make hello-world.upload
	$ make login

For more information see the INGA project website:

[www.ibr.cs.tu-bs.de/projects/inga/](https://www.ibr.cs.tu-bs.de/projects/inga/index.xml?lang=en).

##Contiki

[![Build Status](https://secure.travis-ci.org/contiki-os/contiki.png)](http://travis-ci.org/contiki-os/contiki)

Contiki is an open source operating system that runs on tiny low-power
microcontrollers and makes it possible to develop applications that
make efficient use of the hardware while providing standardized
low-power wireless communication for a range of hardware platforms.

Contiki is used in numerous commercial and non-commercial systems,
such as city sound monitoring, street lights, networked electrical
power meters, industrial monitoring, radiation monitoring,
construction site monitoring, alarm systems, remote house monitoring,
and so on.

For more information, see the Contiki website:

[http://contiki-os.org](http://contiki-os.org)