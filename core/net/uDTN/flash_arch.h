/*
 * Copyright (c) 2014, Institute of Operating Systems and Computer Networks, TU Braunschweig
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
 */

#ifndef FLASH_ARCH_H
#define FLASH_ARCH_H

/**
 * This is a dummy file that is necessary because storage_flash.c includes it.
 * Platform includes are "before" network includes and hence if a platform
 * contains a flash_arch.h file, it is preferred over this dummy file.
 * Platforms that do not have a flash_arch.h file would not be able to compile
 * uDTN because the include is not found. Those platforms will now automatically
 * include this dummy file to allow compilation.
 */
#define FLASH_INIT()
#define FLASH_WRITE_PAGE(page, offset, buffer, length)
#define FLASH_READ_PAGE(page, offset, buffer, length)
#define FLASH_ERASE_PAGE(page)
#define FLASH_PAGE_SIZE									8UL
#define FLASH_PAGE_OFFSET								0UL
#define FLASH_PAGES 									512UL

#endif /* FLASH_ARCH_H */
