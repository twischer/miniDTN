/*
 * Copyright (c) 2013, TU Braunschweig
 * All rights reserved.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *  File system functions for the raven/inga-webserver.
 * \author
 *  Enrico Joerns <e.joerns@tu-bs.de>
 */

/*
 * Provides file system functions for the raven/inga-webserver.
 * 
 * Currently implemented for coffee on internal flash, external flash and sd card
 */

#ifndef HTTPD_FS_ARCH_H
#define	HTTPD_FS_ARCH_H


#if COFFEE_FILES

/* Coffee file system can be static or dynamic. If static, new files
   can not be created and rewrites of an existing file can not be
   made beyond the initial allocation.
 */
#include "cfs-coffee-arch.h"

#define httpd_fs_cpy      avr_httpd_fs_cpy
#define httpd_fs_getchar  avr_httpd_fs_getchar
#define httpd_fs_strcmp   avr_httpd_fs_strcmp
#define httpd_fs_strchr   avr_httpd_fs_strchr

#ifdef COFFEE_AVR_FLASH
#define avr_httpd_fs_cpy(dest,addr,size) avr_flash_read((CFS_CONF_OFFSET_TYPE) addr, (uint8_t *)dest, (CFS_CONF_OFFSET_TYPE) size)
#define http_fs_read avr_flash_read
#endif

#ifdef COFFEE_AVR_EXTERNAL
#define avr_httpd_fs_cpy(dest,addr,size) external_flash_read((CFS_CONF_OFFSET_TYPE) addr, (uint8_t *)dest, (CFS_CONF_OFFSET_TYPE) size)
#define http_fs_read external_flash_read
#endif

#ifdef COFFEE_AVR_SDCARD
#define avr_httpd_fs_cpy(dest,addr,size) sd_read((CFS_CONF_OFFSET_TYPE) addr, (uint8_t *)dest, (CFS_CONF_OFFSET_TYPE) size)
#define http_fs_read sd_read
#endif

#endif


#endif	/* HTTPD_FS_ARCH_H */

