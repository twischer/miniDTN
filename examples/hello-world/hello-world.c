/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 * $Id: hello-world.c,v 1.1 2006/10/02 21:46:46 adamdunkels Exp $
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"

#include "net/netstack.h"
#include "net/packetbuf.h"
#include "dev/leds.h"
#include "net/dtn/dtn-network.h"
#include "net/dtn/sdnv.h"
#include "net/dtn/bundle.h"
#include <string.h>


#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
  printf("Hello, world\n");
    leds_on(1);
        uint8_t *foo;
        foo=(uint8_t *) malloc(10);
        memset(foo,0xfe,10);
        struct bundle_t bundle;
        create_bundle(&bundle ,foo ,10);
        uint8_t i;
        uint64_t bla=1;
        set_attr(&bundle, DEST_NODE, &bla);
        set_attr(&bundle, DEST_SERV, &bla);
        set_attr(&bundle, SRC_NODE, &bla);
        set_attr(&bundle, SRC_SERV,&bla);
        set_attr(&bundle, FLAGS, &bla);
        set_attr(&bundle, REP_NODE, &bla);
        set_attr(&bundle, REP_SERV, &bla);
        set_attr(&bundle, CUST_NODE, &bla);
        set_attr(&bundle, CUST_SERV, &bla);
        set_attr(&bundle, TIME_STAMP_SEQ_NR, &bla);
        set_attr(&bundle, LIFE_TIME, &bla);
        set_attr(&bundle, P_FLAGS, &bla);
        set_attr(&bundle, TIME_STAMP, &bla);
	rimeaddr_t dest={{15,0}};
        printf("main size: %u\n ",bundle.size);
	dtn_network_send(bundle.block,bundle.size,dest);
    	uint8_t *tmp=bundle.block;
	for(i=0; i<bundle.size; i++){
		printf("%u ",*tmp);
		tmp++;
	}
	printf("\n");
		
    leds_off(1);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
