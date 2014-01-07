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
 * This file is part of the lwIP TCP/IP stack.
 */
#ifndef HTTPD_FS_H_
#define HTTPD_FS_H_

/**
 * \file File handling interface for webserver-nano
 * 
 * Provides a partly cfs compatible interface for file system access
 * to webserver-nano.
 * Calls are either handeled by the httpd fake file system or by one
 * of the available cfs implementations such as cfs-coffee or cfs-fat.
 * 
 * Makro WEBSERVER_CONF_FILESTATS enables file statistics.
 * 
 * \author Enrico Joerns <e.joerns@tu-bs.de>
 */

#include "contiki-net.h"
#include "httpd.h"

#if HTTPD_FS_FAT
#include "cfs-fat.h"
#elif HTTPD_FS_COFFEE
#include "cfs-coffee-arch.h"
#endif

struct httpd_fs_file_desc {
  struct httpd_fsdata_file *file;
  cfs_offset_t offset;
  uint8_t flags;
};

/* file must be allocated by caller and will be filled in
   by the function. */

#define HTTPD_SEEK_SET CFS_SEEK_SET
#define HTTPD_SEEK_CUR CFS_SEEK_CUR
#define HTTPD_SEEK_END CFS_SEEK_END

#define HTTPD_FS_READ  CFS_READ

#define HTTPD_FS_RAM    1
#define HTTPD_FS_FLASH  2

#ifdef HTTPD_FS_CONF_STOARGE
#define HTTPD_FS_STORAGE  HTTPD_FS_CONF_STOARGE
#else
#if defined(__AVR__)
#include <avr/pgmspace.h>
#define HTTPD_FS_STORAGE  HTTPD_FS_FLASH
#else
#define HTTPD_FS_STORAGE  HTTPD_FS_RAM
#endif
#endif

/* httpd file system is in ram */
#if HTTPD_FS_STORAGE==HTTPD_FS_RAM
#define HTTPD_STRING_ATTR
#define httpd_fs_cpy        memcpy_P
#define httpd_fs_strcmp     strcmp_P
/* httpd file system is in progmem */
#elif HTTPD_FS_STORAGE==HTTPD_FS_FLASH
#define HTTPD_STRING_ATTR   PROGMEM
#define httpd_fs_strcmp     strcmp
#define httpd_fs_cpy        memcpy
#endif /* HTTPD_FS_STORAGE */

/* Wrapper function for (pseudo)file system calls. */
#if HTTPD_CFS
#define httpd_fs_open   cfs_open
#define httpd_fs_close  cfs_close
#define httpd_fs_read   cfs_read
#define httpd_fs_seek   cfs_seek
#else
int httpd_fs_open(const char *name, int mode);
void httpd_fs_close(int fd);
int httpd_fs_read(int fd, void* buf, unsigned int len);
cfs_offset_t httpd_fs_seek(int fd, cfs_offset_t offset, int whence);
#endif /* HTTPD_CFS */

#if WEBSERVER_CONF_FILESTATS
extern uint16_t httpd_filecount[];
uint16_t httpd_fs_count(char *name);
void* httpd_fs_get_root(void);
#endif

void httpd_fs_init(void);

#endif /* HTTPD_FS_H_ */
