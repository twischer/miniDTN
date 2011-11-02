/* Reset INGA nodes and configure their FTDI
 *
 * (C) 2011 Daniel Willmann <daniel@totalueberwachung.de>
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

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <libudev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ftdi.h>
#include <popt.h>
#include <usb.h>

enum {
	MODE_RESET,
	MODE_UPDATE_EEPROM,
};

struct config_t {
	int mode;
	char *device_path;
	char *device_serial;
	char *device_id;
	int busnum;
	int devnum;
	int verbose;
	/* EEPROM configuration */
	int eep_cbusio;
	uint16_t eep_vendor_id;
	uint16_t eep_product_id;
	char *eep_manuf;
	char *eep_prod;
	char *eep_serial;
};

#define VERBOSE(args...)\
	if (cfg->verbose)\
		fprintf(stderr, ##args)

void resolve_usb_device(struct config_t *cfg)
{
	int rc;
	struct udev *udev;
	struct udev_device *dev;
	char *devpath;
	const char *devnum, *busnum;

	rc = sprintf(devpath, "/sys/class/tty/%s", basename(cfg->device_path));
	if (rc < 0) {
		fprintf(stderr, "Failed to generate sysfs path\n");
		exit(EXIT_FAILURE);
	}

	if (cfg->device_path) {
		udev = udev_new();
		if (!udev) {
			fprintf(stderr, "Failed to initialize udev\n");
			exit(EXIT_FAILURE);
		}
		dev = udev_device_new_from_syspath(udev, devpath);
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

struct usb_device *find_matching_usb_dev(struct config_t *cfg)
{
	int rc;
	struct usb_bus *bus;
	struct usb_device *usbdev;
	struct usb_dev_handle *ufd;
	long int busnum;
	char *bus_end, buf[256];

	resolve_usb_device(cfg);

	usb_init();
	usb_find_busses();
	usb_find_devices();

	bus = usb_get_busses();

	/* Iterate over all devices in all busses and see if it matches */
	for (;bus != NULL; bus = bus->next) {
		for (usbdev = bus->devices; usbdev != NULL; usbdev = usbdev->next) {
			/* Check if busnumber and devicenumber match */
			if (cfg->busnum != 0 && cfg->devnum != 0) {
				if (usbdev->devnum != cfg->devnum)
					continue;
				/* Convert the bus number */
				busnum = strtol(usbdev->bus->dirname, &bus_end, 10);
				if (bus_end[0] == 0 && usbdev->bus->dirname[0] != 0) {
					if (busnum != cfg->busnum)
						continue;
				}
				VERBOSE("Found USB device matching %i:%i\n", cfg->busnum, cfg->devnum);
			}

			/* Check if USB serial matches */
			if (cfg->device_serial) {
				ufd = usb_open(usbdev);
				if (!ufd)
					continue;
				if (!usbdev->descriptor.iSerialNumber) {
					usb_close(ufd);
					continue;
				}
				rc = usb_get_string_simple(ufd, usbdev->descriptor.iSerialNumber, buf, sizeof(buf));
				usb_close(ufd);

				if (rc <= 0 || strcmp(buf, cfg->device_serial))
					continue;

				VERBOSE("Found USB serial %s on USB device %s:%i\n", cfg->device_serial, usbdev->bus->dirname, usbdev->devnum);
			}

			VERBOSE("All requirements match, using %i:%i as target device\n", atoi(usbdev->bus->dirname), usbdev->devnum);
			return usbdev;
		}
	}

	return NULL;
}

void inga_reset(struct config_t *cfg)
{
	int rc;
    struct ftdi_context ftdic;
    struct usb_device *usbdev;

    if (ftdi_init(&ftdic) < 0)
    {
        fprintf(stderr, "ftdi_init failed\n");
        exit(EXIT_FAILURE);
    }

    /* Find the USB device for the path */
    usbdev = find_matching_usb_dev(cfg);
    if (!usbdev) {
	    fprintf(stderr, "Could not find device\n");
	    exit(EXIT_FAILURE);
    }

    rc = ftdi_usb_open_dev(&ftdic, usbdev);
    if (rc < 0 && rc != -5)
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", rc, ftdi_get_error_string(&ftdic));
        exit(EXIT_FAILURE);
    }

    usbdev = usb_device(ftdic.usb_dev);

	printf("Resetting INGA node...");
	fflush(stdout);

    /* Set CBUS3 to output and high */
    ftdi_set_bitmode(&ftdic, 0x88, BITMODE_CBUS);

    sleep(1);

    /* Set CBUS3 to output and low */
    ftdi_set_bitmode(&ftdic, 0x80, BITMODE_CBUS);

	printf("done\n");

    ftdi_set_bitmode(&ftdic, 0, BITMODE_RESET);

out:

    if (cfg->verbose)
	    fprintf(stderr, "Resetting USB device\n");

    /* USB reset is necessary in order to get the serial device back */
    ftdi_usb_reset(&ftdic);
    usb_reset(ftdic.usb_dev);
    ftdi_usb_close(&ftdic);
    ftdi_deinit(&ftdic);
}

void inga_eeprom(struct config_t *cfg)
{
	int rc;
    struct ftdi_context ftdic;
    struct usb_device *usbdev;
    struct ftdi_eeprom eeprom;
    char buf[FTDI_DEFAULT_EEPROM_SIZE];

    if (ftdi_init(&ftdic) < 0)
    {
        fprintf(stderr, "ftdi_init failed\n");
        exit(EXIT_FAILURE);
    }

    /* Find the USB device for the path */
    usbdev = find_matching_usb_dev(cfg);
    if (!usbdev) {
	    fprintf(stderr, "Could not find device\n");
	    exit(EXIT_FAILURE);
    }

    rc = ftdi_usb_open_dev(&ftdic, usbdev);
    if (rc < 0 && rc != -5)
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", rc, ftdi_get_error_string(&ftdic));
        exit(EXIT_FAILURE);
    }

    usbdev = usb_device(ftdic.usb_dev);

	printf("Reading out EEPROM image...");
	fflush(stdout);
	rc = ftdi_read_eeprom(&ftdic, buf);
	if (rc < 0) {
		fprintf(stderr, "\nCould not read EEPROM: %i\n", rc);
		exit(EXIT_FAILURE);
	}
	printf("done\n");

	rc = ftdi_eeprom_decode(&eeprom, buf, FTDI_DEFAULT_EEPROM_SIZE);
	if (rc < 0) {
		fprintf(stderr, "Could not decode EEPROM: %i\n", rc);
		exit(EXIT_FAILURE);
	}
	/* decode fails to set the size which is needed to build an image again */
	eeprom.size = FTDI_DEFAULT_EEPROM_SIZE;

	if (eeprom.chip_type != ftdic.type) {
		/* CBUS has not been copied over in this case, we need to do that ourselves */
		int i, tmp;
		for (i=0;i<5;i++) {
			if (i%2 == 0)
				tmp = buf[0x14 + (i/2)];
			eeprom.cbus_function[i] = tmp & 0x0f;
			tmp = tmp >> 4;
		}
	}

	VERBOSE("\nEEPROM config:\n"
		"Chip type:    %i\n"
		"VID:          0x%04x\n"
		"PID:          0x%04x\n"
		"Manufacturer: %s\n"
		"Product:      %s\n"
		"Serial:       %s\n"
		"CBUS:         %1x%1x %1x%1x 0%1x\n\n", eeprom.chip_type,
			eeprom.vendor_id, eeprom.product_id, eeprom.manufacturer,
			eeprom.product, eeprom.serial, eeprom.cbus_function[1],
			eeprom.cbus_function[0], eeprom.cbus_function[3],
			eeprom.cbus_function[2], eeprom.cbus_function[4]);

	if (cfg->eep_vendor_id)
		eeprom.vendor_id = cfg->eep_vendor_id;
	if (cfg->eep_product_id)
		eeprom.product_id = cfg->eep_product_id;
	if (cfg->eep_manuf)
		eeprom.manufacturer = cfg->eep_manuf;
	if (cfg->eep_prod)
		eeprom.product = cfg->eep_prod;
	if (cfg->eep_serial)
		eeprom.serial = cfg->eep_serial;
	if (cfg->eep_cbusio) {
		eeprom.cbus_function[0] = CBUS_TXLED;
		eeprom.cbus_function[1] = CBUS_RXLED;
		eeprom.cbus_function[2] = CBUS_TXDEN;
		eeprom.cbus_function[3] = CBUS_IOMODE;
		eeprom.cbus_function[4] = CBUS_SLEEP;
	}


	VERBOSE("\nUpdating with EEPROM config:\n"
		"Chip type:    %i\n"
		"VID:          0x%04x\n"
		"PID:          0x%04x\n"
		"Manufacturer: %s\n"
		"Product:      %s\n"
		"Serial:       %s\n"
		"CBUS:         %1x%1x %1x%1x 0%1x\n\n", eeprom.chip_type,
			eeprom.vendor_id, eeprom.product_id, eeprom.manufacturer,
			eeprom.product, eeprom.serial, eeprom.cbus_function[0],
			eeprom.cbus_function[1], eeprom.cbus_function[2],
			eeprom.cbus_function[3], eeprom.cbus_function[4]);

	eeprom.chip_type = ftdic.type;
	rc = ftdi_eeprom_build(&eeprom, buf);
	if (rc < 0) {
		fprintf(stderr, "Could not build EEPROM: %i\n", rc);
		exit(EXIT_FAILURE);
	}

	printf("Writing updated EEPROM image...");
	fflush(stdout);
	rc = ftdi_write_eeprom(&ftdic, buf);
	if (rc < 0) {
		fprintf(stderr, "\nCould not write EEPROM: %i\n", rc);
		exit(EXIT_FAILURE);
	}
	printf("done\n");
	sleep(10);

out:

    if (cfg->verbose)
	    fprintf(stderr, "Resetting USB device\n");

    /* USB reset is necessary in order to get the serial device back */
    ftdi_usb_reset(&ftdic);
    usb_reset(ftdic.usb_dev);
    ftdi_usb_close(&ftdic);
    ftdi_deinit(&ftdic);
}

void usage(poptContext poptc, int exitcode, char *error, char *addl)
{
	poptPrintUsage(poptc, stderr, 0);
	if (error)
		fprintf(stderr, "%s: %s\n", error, addl);
	exit(exitcode);
}

void parse_options(int argc, const char **argv, struct config_t *cfg)
{
	char rc, *tmp;
	poptContext poptc;

	struct poptOption options[] = {
		{"reset", 'r', POPT_ARG_VAL, &cfg->mode, MODE_RESET, "Reset INGA node (default)",
			NULL},
		{"flash", 'f', POPT_ARG_VAL, &cfg->mode, MODE_UPDATE_EEPROM, "Update FTDI EEPROM of INGA",
			NULL},
		{"device", 'd', POPT_ARG_STRING, &cfg->device_path, 0, "Path to the serial device",
			"pathname"},
		{"serial", 's', POPT_ARG_STRING, &cfg->device_serial, 0, "Serial of the FTDI",
			"usb_serial"},
		{"id", 'i', POPT_ARG_STRING, &cfg->device_id, 0, "Limit choice to USB device id",
			"device_id"},
		{"verbose", 'v', POPT_ARG_NONE, &cfg->verbose, 0, "Be verbose",
			NULL},
		POPT_AUTOHELP
		{ NULL, 0, 0, NULL, 0}
	};

	cfg->mode == MODE_RESET;

	poptc = poptGetContext(NULL, argc, argv, options, 0);

	while ((rc = poptGetNextOpt(poptc)) >= 0);

	if (poptPeekArg(poptc) != NULL) {
		poptPrintUsage(poptc, stderr, 0);
		exit(EXIT_FAILURE);
	}

	if (rc < -1) {
		/* Option parsing error */
		fprintf(stderr, "%s: %s\n", poptBadOption(poptc, POPT_BADOPTION_NOALIAS), poptStrerror(rc));
		exit(EXIT_FAILURE);
	}

	if (!cfg->device_path && !cfg->device_serial)
		usage(poptc, 1, "At least one of --device or --serial has to be set", "");

	poptFreeContext(poptc);

	if (!cfg->device_path)
		return;

	/* Resolve symlinks/relative paths */
	tmp = realpath(cfg->device_path, NULL);
	if (!tmp) {
		perror("Could not open device");
		exit(EXIT_FAILURE);
	}

	VERBOSE("Resolving path %s to %s\n", cfg->device_path, tmp);

	free(cfg->device_path);
	cfg->device_path = tmp;
}

struct config_t *init_config(void)
{
	struct config_t *cfg = malloc(sizeof(struct config_t));
	if (!cfg)
		return NULL;

	memset(cfg, 0, sizeof(struct config_t));

	/* Set some sensible defaults
	 * Enable CBUS3 IO, keep vendor/product ID as is,
	 * set manufacturer to IBR and product string to INGA,
	 * keep serial */
	cfg->eep_cbusio = 1;
	cfg->eep_manuf = "IBR";
	cfg->eep_prod = "INGA";
}

int main(int argc, const char **argv)
{
	struct config_t *cfg;

	cfg = init_config();
	if (!cfg) {
		fprintf(stderr, "Could not initialize config\n");
		exit(EXIT_FAILURE);
	}

	parse_options(argc, argv, cfg);

	if (cfg->verbose)
		fprintf(stderr, "Config: path: %s, serial: %s, id: %s\n", cfg->device_path, cfg->device_serial, cfg->device_id);

	if (cfg->mode == MODE_RESET)
		inga_reset(cfg);
	else if (cfg->mode == MODE_UPDATE_EEPROM)
		inga_eeprom(cfg);

	exit(EXIT_SUCCESS);
}
