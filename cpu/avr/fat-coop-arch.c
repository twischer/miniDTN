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
 *		FAT driver Coop Additions implementation - Platform specific
 * \author
 *      Wolf-Bastian P�ttner <poettner@ibr.cs.tu-bs.de>
 */

#include "cfs-fat.h"
#include "fat_coop.h"
#include "fat-coop-arch.h"

static uint8_t stack[FAT_COOP_STACK_SIZE];
static uint8_t *sp = 0;
static uint8_t *sp_save = 0;

/**
 * This Function jumps back to perform_next_step()
 */
void coop_finished_op() {
	/* Change SP back to original */
	coop_switch_sp();
}

/**
 * This function is mostly copied from the arm/mtarch.c file.
 */
void coop_mt_init( void *data ) {
  memset(stack, 0, FAT_COOP_STACK_SIZE);

  /* coop_init function that is to be invoked if the thread dies */
  stack[FAT_COOP_STACK_SIZE -  1] = (unsigned char)((unsigned short)coop_finished_op) & 0xff;
  stack[FAT_COOP_STACK_SIZE -  2] = (unsigned char)((unsigned short)coop_finished_op >> 8) & 0xff;

  /* The thread handler. Invoked when RET is called in coop_switch_sp */
  stack[FAT_COOP_STACK_SIZE -  3] = (unsigned char)((unsigned short)operation) & 0xff;
  stack[FAT_COOP_STACK_SIZE -  4] = (unsigned char)((unsigned short)operation >> 8) & 0xff;

  /* Register r0-r23 in t->stack[MTARCH_STACKSIZE -  5] to
   *  t->stack[MTARCH_STACKSIZE -  28].
   *
   * Per calling convention, the argument to the thread handler function
   *  (i.e. the 'data' pointer) is passed via r24-r25.
   * See http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_reg_usage) */
  stack[FAT_COOP_STACK_SIZE - 29] = (unsigned char)((unsigned short)data) & 0xff;
  stack[FAT_COOP_STACK_SIZE - 30] = (unsigned char)((unsigned short)data >> 8) & 0xff;

  /* Initialize stack pointer: Space for 2 2-byte-addresses and 32 registers,
   * post-decrement POP / pre-increment PUSH scheme */
  sp = &stack[FAT_COOP_STACK_SIZE - 1 - 4 - 32];
}

/**
 * Set the interal stack to NULL to end process
 */
void coop_mt_stop() {
	sp = NULL;
}

/**
 * Function switches the stack pointers for Atmel based devices (32 GPR's).
 */
void coop_switch_sp() {
/* Disable interrupts while we perform the context switch */
  cli ();

  /* Push 32 general purpuse registers */
  __asm__("push r0");
  __asm__("push r1");
  __asm__("push r2");
  __asm__("push r3");
  __asm__("push r4");
  __asm__("push r5");
  __asm__("push r6");
  __asm__("push r7");
  __asm__("push r8");
  __asm__("push r9");
  __asm__("push r10");
  __asm__("push r11");
  __asm__("push r12");
  __asm__("push r13");
  __asm__("push r14");
  __asm__("push r15");
  __asm__("push r16");
  __asm__("push r17");
  __asm__("push r18");
  __asm__("push r19");
  __asm__("push r20");
  __asm__("push r21");
  __asm__("push r22");
  __asm__("push r23");
  __asm__("push r24");
  __asm__("push r25");
  __asm__("push r26");
  __asm__("push r27");
  __asm__("push r28");
  __asm__("push r29");
  __asm__("push r30");
  __asm__("push r31");

  /* Switch stack pointer */
  if( sp_save == NULL ) {
	sp_save = (uint8_t *) SP;
	SP = (uint16_t) sp;
  } else {
	sp = (uint8_t *) SP;
	SP = (uint16_t) sp_save;
	sp_save = NULL;
  }

  /* Pop 32 general purpose registers */
  __asm__("pop r31");
  __asm__("pop r30");
  __asm__("pop r29");
  __asm__("pop r28");
  __asm__("pop r27");
  __asm__("pop r26");
  __asm__("pop r25");
  __asm__("pop r24");
  __asm__("pop r23");
  __asm__("pop r22");
  __asm__("pop r21");
  __asm__("pop r20");
  __asm__("pop r19");
  __asm__("pop r18");
  __asm__("pop r17");
  __asm__("pop r16");
  __asm__("pop r15");
  __asm__("pop r14");
  __asm__("pop r13");
  __asm__("pop r12");
  __asm__("pop r11");
  __asm__("pop r10");
  __asm__("pop r9");
  __asm__("pop r8");
  __asm__("pop r7");
  __asm__("pop r6");
  __asm__("pop r5");
  __asm__("pop r4");
  __asm__("pop r3");
  __asm__("pop r2");
  __asm__("pop r1");
  __asm__("pop r0");

  /* Reenable interrupts */
  sei ();
}

/**
 * Changes Stack pointer to internal Thread. Returns after one step has been completed.
 *
 * If this function was called with one specific QueueEntry, then this function must be
 * called again and again with the same QueueEntry, until the QueueEntry is finished
 * executing.
 * \param *entry The QueueEntry, which should be processed.
 */
void perform_next_step( QueueEntry *entry ) {
	if( sp == NULL ) {
		coop_mt_init( (void *) entry );
	}

	coop_switch_sp();
}

int calc_free_stack() {
	int i;

	for( i = 0; i < FAT_COOP_STACK_SIZE; i++ ) {
		if(stack[i] != 0) {
			break;
		}
	}

	return i;
}
