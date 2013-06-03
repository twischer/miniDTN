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
#ifndef __HTTPD_FS_H__
#define __HTTPD_FS_H__

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

#include "cfs.h"

struct httpd_fs_file {
  char *start;// pointer to first byte of file
  char *pos;  // pointer to current position in file
  int len;    // length of file
};

extern struct httpd_fs_file* files;

/* file must be allocated by caller and will be filled in
   by the function. */

#define HTTPD_SEEK_SET CFS_SEEK_SET
#define HTTPD_SEEK_CUR CFS_SEEK_CUR
#define HTTPD_SEEK_END CFS_SEEK_END

#define HTTPD_FS_READ  CFS_READ

/* Wrapper function for (pseudo)file system calls. */
#if HTTPD_CFS
#define httpd_fs_open cfs_open
#define httpd_fs_close cfs_close
#define httpd_cf_read cfs_read
#define httpd_fs_seek cfs_seek
#define httpd_fs_filesize(fd) // TODO....
#endif
#else
uint16_t httpd_fs_open(const char *name, struct httpd_fs_file *file);
void httpd_fs_close(int fd);
int httpd_fs_read(int fd, void* buf, unsigned int len);
cfs_offset_t httpd_fs_seek(int fd, cfs_offset_t offset, int whence);
cfs_offset_t httpd_fs_filesize(int fd);
#endif

#if WEBSERVER_CONF_FILESTATS
extern uint16_t httpd_filecount[];
uint16_t httpd_fs_count(char *name);
void* httpd_fs_get_root(void);
#endif

void httpd_fs_init(void);

#endif /* __HTTPD_FS_H__ */
