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

#ifndef INGA_USB_H
#define INGA_USB_H

#include <ftdi.h>

struct inga_usb_config_t {
	char *device_path;
	char *device_serial;
	char *device_id;

	int busnum;
	int devnum;
};

struct inga_usb_device_t;

struct inga_usb_device_t *inga_usb_find_device(struct inga_usb_config_t *cfg, int verbose);

void inga_usb_free_device(struct inga_usb_device_t *usb);

int inga_usb_ftdi_init(struct ftdi_context *ftdi);
void inga_usb_ftdi_deinit(struct ftdi_context *ftdi);

int inga_usb_ftdi_open(struct ftdi_context *ftdi, struct inga_usb_device_t *usb);
int inga_usb_ftdi_close(struct ftdi_context *ftdi);

int inga_usb_ftdi_reset(struct ftdi_context *ftdi);

int inga_usb_ftdi_read_eeprom(struct ftdi_context *ftdi, unsigned char *eeprom);
int inga_usb_ftdi_write_eeprom(struct ftdi_context *ftdi, unsigned char *eeprom);

int inga_usb_ftdi_eeprom_decode(struct ftdi_eeprom *eeprom, unsigned char *output, int size);
int inga_usb_ftdi_eeprom_build(struct ftdi_eeprom *eeprom, unsigned char *output);

int inga_usb_ftdi_set_bitmode(struct ftdi_context *ftdi, unsigned char bitmask, unsigned char mode);

char *inga_usb_ftdi_get_error_string(struct ftdi_context *ftdi);

#endif /* INGA_USB_H */
