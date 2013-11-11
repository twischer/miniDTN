/*
 * Copyright (c) 2012, Institute of Operating Systems and Computer Networks (TU Braunschweig).
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
 * Author: Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */


/**
 * \file
 *      Diskio driver definitions - Platform Specific
 * \author
 *      Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef DISKIO_ARCH_H
#define DISKIO_ARCH_H

#include "dev/sdcard.h"
#include "dev/at45db.h"

#define SD_READ_BLOCK(block_start_address, buffer) \
        sdcard_read_block( block_start_address, buffer )
#define SD_WRITE_BLOCK(block_start_address, buffer) \
        sdcard_write_block( block_start_address, buffer )
#define SD_INIT() \
        sdcard_init()
#define SD_GET_BLOCK_NUM() \
        sdcard_get_block_num()
#define SD_GET_BLOCK_SIZE() \
        sdcard_get_block_size()


#define FLASH_READ_BLOCK(block_start_address, offset, buffer, length) \
        at45db_read_page_bypassed( block_start_address, offset, buffer, length)
#define FLASH_WRITE_BLOCK(block_start_address, offset, buffer, length) \
        at45db_write_buffer( offset, buffer, length ); \
        at45db_buffer_to_page( block_start_address );
#define FLASH_INIT() \
        at45db_init()

#endif /* DISKIO_ARCH_H */

