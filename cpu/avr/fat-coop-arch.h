/*
 * Copyright (c) 2012, Institute of Operating Systems and Computer Networks (TU Brunswick).
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
 * \addtogroup Drivers
 * @{
 *
 * \defgroup fat_coop_driver FAT Driver - Cooperative Additions - Platform specific
 *
 * <p></p>
 * @{
 *
 */

/**
 * \file
 *		FAT driver Coop Additions definitions - Platform specific
 * \author
 *      Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#ifndef FAT_COOP_ARCH_H
#define FAT_COOP_ARCH_H

void coop_finished_op();
void coop_mt_init( void *data );
void coop_mt_stop();
void coop_switch_sp();
void perform_next_step( QueueEntry *entry );
int calc_free_stack();

#endif /* FAT_COOP_ARCH_H */
