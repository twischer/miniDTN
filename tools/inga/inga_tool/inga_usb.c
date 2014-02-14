/* Reset INGA nodes and configure their FTDI
 *
 * (C) 2011 Daniel Willmann <daniel@totalueberwachung.de>
 * (C) 2014 Sven Hesse <drmccoy@drmccoy.de>
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <libudev.h>
#include <string.h>

#include "inga_usb.h"

#define VERBOSE(args...)\
	if (verbose)\
		fprintf(stderr, ##args)

static void inga_usb_resolve(struct inga_usb_config_t *cfg, int verbose);

#if FTDI_VERSION == 0
/* Code specific to libftdi 0.x / libusb-0.1 */

#include <usb.h>

struct inga_usb_device_t {
	struct usb_device *usbdev;
};

struct inga_usb_device_t *inga_usb_find_device(struct inga_usb_config_t *cfg, int verbose)
{
	int rc;
	struct usb_bus *bus;
	struct inga_usb_device_t *usb;
	struct usb_dev_handle *ufd;
	long int busnum;
	char *bus_end, buf[256];

	inga_usb_resolve(cfg, verbose);

	usb_init();
	usb_find_busses();
	usb_find_devices();

	bus = usb_get_busses();

	usb = (struct inga_usb_device_t *) malloc(sizeof(struct inga_usb_device_t));
	memset(usb, 0, sizeof(struct inga_usb_device_t));

	/* Iterate over all devices in all busses and see if one matches */
	for (;bus != NULL; bus = bus->next) {
		for (usb->usbdev = bus->devices; usb->usbdev != NULL; usb->usbdev = usb->usbdev->next) {

			/* Check if busnumber and devicenumber match */
			if ((cfg->busnum != 0) && (cfg->devnum != 0)) {
				if (usb->usbdev->devnum != cfg->devnum)
					continue;

				/* Convert the bus number */
				busnum = strtol(usb->usbdev->bus->dirname, &bus_end, 10);
				if (bus_end[0] == 0 && usb->usbdev->bus->dirname[0] != 0) {
					if (busnum != cfg->busnum)
						continue;
				}
				VERBOSE("Found USB device matching %i:%i\n", cfg->busnum, cfg->devnum);
			}

			/* Check if USB serial matches */
			if (cfg->device_serial) {
				ufd = usb_open(usb->usbdev);
				if (!ufd)
					continue;
				if (!usb->usbdev->descriptor.iSerialNumber) {
					usb_close(ufd);
					continue;
				}
				rc = usb_get_string_simple(ufd, usb->usbdev->descriptor.iSerialNumber, buf, sizeof(buf));
				usb_close(ufd);

				if (rc <= 0 || strcmp(buf, cfg->device_serial))
					continue;

				VERBOSE("Found USB serial %s on USB device %s:%i\n", cfg->device_serial, usb->usbdev->bus->dirname, usb->usbdev->devnum);
			}

			VERBOSE("All requirements match, using %i:%i as target device\n", atoi(usb->usbdev->bus->dirname), usb->usbdev->devnum);
			return usb;
		}
	}

	inga_usb_free_device(usb);
	return NULL;
}

int inga_usb_ftdi_reset(struct ftdi_context *ftdi)
{
	if (ftdi_usb_reset(ftdi) < 0)
		return -1;

	if (usb_reset(ftdi->usb_dev) < 0)
		return -1;

	return 0;
}

int inga_usb_ftdi_close(struct ftdi_context *ftdi)
{
	return ftdi_usb_close(ftdi);
}

void inga_usb_free_device(struct inga_usb_device_t *usb) {
	free(usb);
}

int inga_usb_ftdi_read_eeprom(struct ftdi_context *ftdi, unsigned char *eeprom)
{
	return ftdi_read_eeprom(ftdi, eeprom);
}

int inga_usb_ftdi_write_eeprom(struct ftdi_context *ftdi, unsigned char *eeprom)
{
	return ftdi_write_eeprom(ftdi, eeprom);
}

int inga_usb_ftdi_eeprom_decode(struct ftdi_eeprom *eeprom, unsigned char *output, int size)
{
	return ftdi_eeprom_decode(eeprom, output, size);
}

int inga_usb_ftdi_eeprom_build(struct ftdi_eeprom *eeprom, unsigned char *output)
{
	return ftdi_eeprom_build(eeprom, output);
}

#elif FTDI_VERSION == 1
/* Code specific to libftdi 1.x / libusb-1.x */

#include <libusb.h>

struct inga_usb_device_t {
	libusb_context *usbctx;

	struct libusb_device *usbdev;
};

struct inga_usb_device_t *inga_usb_find_device(struct inga_usb_config_t *cfg, int verbose)
{
	struct inga_usb_device_t *usb;
	libusb_device **dev_list = NULL;
	ssize_t dev_count, i;

	inga_usb_resolve(cfg, verbose);

	usb = (struct inga_usb_device_t *) malloc(sizeof(struct inga_usb_device_t));
	memset(usb, 0, sizeof(struct inga_usb_device_t));

	libusb_init(&usb->usbctx);

	dev_count = libusb_get_device_list(usb->usbctx, &dev_list);
	if (dev_count < 0) {
		fprintf(stderr, "Failed get USB device list\n");
		exit(EXIT_FAILURE);
	}

	/* Iterate over all devices and see if one matches */
	for (i = 0; i < dev_count; i++) {
		usb->usbdev = dev_list[i];

		if ((cfg->busnum != 0) && (cfg->devnum != 0)) {
			if ((libusb_get_bus_number(usb->usbdev) != cfg->busnum) || (libusb_get_device_address(usb->usbdev) != cfg->devnum))
				continue;

			VERBOSE("Found USB device matching %i:%i\n", cfg->busnum, cfg->devnum);
		}

		/* Check if USB serial matches */
		if (cfg->device_serial) {
			struct libusb_device_descriptor desc;
			libusb_device_handle *handle;
			char buf[256];
			int rc;

			if (libusb_get_device_descriptor(usb->usbdev, &desc) != 0)
				continue;

			if (!desc.iSerialNumber)
				continue;

			if (libusb_open(usb->usbdev, &handle) < 0)
				continue;

			rc = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, buf, sizeof(buf));
			libusb_close(handle);

			if (rc <= 0 || strcmp(buf, cfg->device_serial))
				continue;

			VERBOSE("Found USB serial %s on USB device %i:%i\n", cfg->device_serial,
					libusb_get_bus_number(usb->usbdev), libusb_get_device_address(usb->usbdev));
		}

		VERBOSE("All requirements match, using %i:%i as target device\n",
				libusb_get_bus_number(usb->usbdev), libusb_get_device_address(usb->usbdev));

		libusb_free_device_list(dev_list, 1);
		return usb;
	}

	libusb_free_device_list(dev_list, 1);
	inga_usb_free_device(usb);
	return NULL;
}

int inga_usb_ftdi_reset(struct ftdi_context *ftdi)
{
	if (ftdi_usb_reset(ftdi) < 0)
		return -1;

	if (libusb_reset_device(ftdi->usb_dev) < 1)
		return -1;

	return 0;
}

int inga_usb_ftdi_close(struct ftdi_context *ftdi)
{
	libusb_device *dev = NULL;
	int interface;

	/* Remember the USB device and ftdi_sio interface */
	if (ftdi->usb_dev) {
		dev = libusb_get_device(ftdi->usb_dev);
		interface = ftdi->interface;
	}

	/* Close the FTDI */
	if (ftdi_usb_close(ftdi) < 0)
		return -1;

	/* Reattach the ftdi_sio kernel module to the USB device */
	if (dev) {
		libusb_device_handle *handle = NULL;

		if (libusb_open(dev, &handle) >= 0) {
			libusb_attach_kernel_driver(handle, interface);
			libusb_close(handle);
		}
	}

	return 0;
}

void inga_usb_free_device(struct inga_usb_device_t *usb) {
	if (!usb)
		return;

	if (usb->usbctx)
		libusb_exit(usb->usbctx);

	free(usb);
}

int inga_usb_ftdi_read_eeprom(struct ftdi_context *ftdi, unsigned char *eeprom)
{
	return -1;
}

int inga_usb_ftdi_write_eeprom(struct ftdi_context *ftdi, unsigned char *eeprom)
{
	return -1;
}

int inga_usb_ftdi_eeprom_decode(struct ftdi_eeprom *eeprom, unsigned char *output, int size)
{
	return -1;
}

int inga_usb_ftdi_eeprom_build(struct ftdi_eeprom *eeprom, unsigned char *output)
{
	return -1;
}

#else
	#error Unrecognized libftdi version
#endif


/* Code common to libftdi 0.x / libusb-0.1 and libftdi 1.x / libusb-1.x */

/* Use udev to find the USB bus and device numbers a ttyUSB device is connected to */
static void inga_usb_resolve(struct inga_usb_config_t *cfg, int verbose)
{
	int rc;
	struct udev *udev;
	struct udev_device *dev;
	char *devpath;
	const char *devnum, *busnum;

	if (cfg->device_path) {
		udev = udev_new();
		if (!udev) {
			fprintf(stderr, "Failed to initialize udev\n");
			exit(EXIT_FAILURE);
		}

		rc = asprintf(&devpath, "/sys/class/tty/%s", basename(cfg->device_path));
		if (rc < 0) {
			fprintf(stderr, "Failed to generate sysfs path\n");
			exit(EXIT_FAILURE);
		}

		dev = udev_device_new_from_syspath(udev, devpath);
		free(devpath);
		if (!dev) {
			fprintf(stderr, "Failed get udev device\n");
			exit(EXIT_FAILURE);
		}

		while (dev) {
			busnum = udev_device_get_sysattr_value(dev, "busnum");
			devnum = udev_device_get_sysattr_value(dev, "devnum");

			if (busnum && devnum) {
				/* TODO: Check conversion errors */
				cfg->busnum = atoi(busnum);
				cfg->devnum = atoi(devnum);
				VERBOSE("Device %s resolved to USB device %i:%i\n", cfg->device_path, cfg->busnum, cfg->devnum);
				break;
			}

			dev = udev_device_get_parent(dev);
		}
		udev_unref(udev);

		if (cfg->busnum == 0 || cfg->devnum == 0) {
			fprintf(stderr, "Failed to resolve %s to USB device\n", cfg->device_path);
			exit(EXIT_FAILURE);
		}
	}
}

int inga_usb_ftdi_init(struct ftdi_context *ftdi)
{
	return ftdi_init(ftdi);
}

void inga_usb_ftdi_deinit(struct ftdi_context *ftdi)
{
	ftdi_deinit(ftdi);
}


int inga_usb_ftdi_open(struct ftdi_context *ftdi, struct inga_usb_device_t *usb)
{
	return ftdi_usb_open_dev(ftdi, usb->usbdev);
}

int inga_usb_ftdi_set_bitmode(struct ftdi_context *ftdi, unsigned char bitmask, unsigned char mode)
{
	return ftdi_set_bitmode(ftdi, bitmask, mode);
}


char *inga_usb_ftdi_get_error_string(struct ftdi_context *ftdi)
{
	return ftdi_get_error_string(ftdi);
}
