/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * $Id: discovery_aware_rdc.c,v 1.4 2010/11/23 18:11:00 nifi Exp $
 */

/**
 * \file
 *         A RDC implementation that is aware of uDTN-Discovery. 
 *         Based on 'nullrdc'
 * \author
 *         André Frambach <frambach@ibr.cs.tu-bs.de>
 */

#include "net/mac/discovery_aware_rdc.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "sys/process.h"
#include "sys/etimer.h"
#include <stdio.h>
#include <string.h>


#include "net/uDTN/discovery.h"
#include "net/uDTN/discovery_scheduler_events.h"

#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


#ifdef DISCOVERY_AWARE_RDC_CONF_ADDRESS_FILTER
#define DISCOVERY_AWARE_RDC_ADDRESS_FILTER DISCOVERY_AWARE_RDC_CONF_ADDRESS_FILTER
#else
#define DISCOVERY_AWARE_RDC_ADDRESS_FILTER 1
#endif /* DISCOVERY_AWARE_RDC_CONF_ADDRESS_FILTER */
#define DISCOVERY_AWARE_RDC_802154_AUTOACK 1

#define DISCOVERY_AWARE_RDC_802154_AUTOACK 1

#ifndef DISCOVERY_AWARE_RDC_802154_AUTOACK
#ifdef DISCOVERY_AWARE_RDC_CONF_802154_AUTOACK
#define DISCOVERY_AWARE_RDC_802154_AUTOACK DISCOVERY_AWARE_RDC_CONF_802154_AUTOACK
#else
#define DISCOVERY_AWARE_RDC_802154_AUTOACK 0
#endif /* DISCOVERY_AWARE_RDC_CONF_802154_AUTOACK */
#endif /* DISCOVERY_AWARE_RDC_802154_AUTOACK */

#ifndef DISCOVERY_AWARE_RDC_802154_AUTOACK_HW
#ifdef DISCOVERY_AWARE_RDC_CONF_802154_AUTOACK_HW
#define DISCOVERY_AWARE_RDC_802154_AUTOACK_HW DISCOVERY_AWARE_RDC_CONF_802154_AUTOACK_HW
#else
#define DISCOVERY_AWARE_RDC_802154_AUTOACK_HW 0
#endif /* DISCOVERY_AWARE_RDC_CONF_802154_AUTOACK_HW */
#endif /* DISCOVERY_AWARE_RDC_802154_AUTOACK_HW */

#if DISCOVERY_AWARE_RDC_802154_AUTOACK
#include "sys/rtimer.h"
#include "dev/watchdog.h"

#define ACK_WAIT_TIME                      RTIMER_SECOND / 2500
#define AFTER_ACK_DETECTED_WAIT_TIME       RTIMER_SECOND / 1500
#define ACK_LEN 3
#endif /* DISCOVERY_AWARE_RDC_802154_AUTOACK */

#if DISCOVERY_AWARE_RDC_802154_AUTOACK || DISCOVERY_AWARE_RDC_802154_AUTOACK_HW
struct seqno {
  rimeaddr_t sender;
  uint8_t seqno;
};

#ifdef NETSTACK_CONF_MAC_SEQNO_HISTORY
#define MAX_SEQNOS NETSTACK_CONF_MAC_SEQNO_HISTORY
#else /* NETSTACK_CONF_MAC_SEQNO_HISTORY */
#define MAX_SEQNOS 16
#endif /* NETSTACK_CONF_MAC_SEQNO_HISTORY */

static struct seqno received_seqnos[MAX_SEQNOS];
#endif /* DISCOVERY_AWARE_RDC_802154_AUTOACK || DISCOVERY_AWARE_RDC_802154_AUTOACK_HW */

#ifndef DISCOVERY_AWARE_RDC_CONF_RADIO_OFF_TIMEOUT
#define RADIO_OFF_SEND_TIMEOUT 0.1
#define RADIO_OFF_REC_TIMEOUT 0.1
#else
#define RADIO_OFF_SEND_TIMEOUT DISCOVERY_AWARE_RDC_CONF_RADIO_OFF_TIMEOUT
#define RADIO_OFF_REC_TIMEOUT DISCOVERY_AWARE_RDC_CONF_RADIO_OFF_TIMEOUT
#endif /* DISCOVERY_AWARE_RDC_CONF_RADIO_OFF_TIMEOUT */

PROCESS(discovery_aware_rdc_process, "DISCOVERY_AWARE_RDC process");

static uint8_t radio_may_be_turned_off = 0;
static struct etimer radio_off_timeout_timer;
static uint8_t radio_status;
static volatile uint8_t send_flag = 0;
static volatile uint8_t rec_flag = 0;

static uint16_t to_modifier = 1;

// Enable CLEAR NEIGHBOURS
#define DISCOVERY_AWARE_RDC_CONF_CLEAR_NEIGHBOURS


static int on(void);
static int off(int);

/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
  int ret;
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &rimeaddr_node_addr);
#if DISCOVERY_AWARE_RDC_802154_AUTOACK || DISCOVERY_AWARE_RDC_802154_AUTOACK_HW
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_ACK, 1);
#endif /* DISCOVERY_AWARE_RDC_802154_AUTOACK || DISCOVERY_AWARE_RDC_802154_AUTOACK_HW */

  if (!radio_status) {
    on();
  }


  if(NETSTACK_FRAMER.create() == 0) {
    /* Failed to allocate space for headers */
    ret = MAC_TX_ERR_FATAL;
  } else {

#if DISCOVERY_AWARE_RDC_802154_AUTOACK
    int is_broadcast;
    uint8_t dsn;
    dsn = ((uint8_t *)packetbuf_hdrptr())[2] & 0xff;

    NETSTACK_RADIO.prepare(packetbuf_hdrptr(), packetbuf_totlen());

    is_broadcast = rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                                &rimeaddr_null);

    if(NETSTACK_RADIO.receiving_packet() ||
       (!is_broadcast && NETSTACK_RADIO.pending_packet())) {

      /* Currently receiving a packet over air or the radio has
         already received a packet that needs to be read before
         sending with auto ack. */
      ret = MAC_TX_COLLISION;

    } else {
      switch(NETSTACK_RADIO.transmit(packetbuf_totlen())) {
      case RADIO_TX_OK:
        if(is_broadcast) {
          ret = MAC_TX_OK;
        } else {
          rtimer_clock_t wt;

          /* Check for ack */
          wt = RTIMER_NOW();
          watchdog_periodic();
          while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + ACK_WAIT_TIME));

          ret = MAC_TX_NOACK;
          if(NETSTACK_RADIO.receiving_packet() ||
             NETSTACK_RADIO.pending_packet() ||
             NETSTACK_RADIO.channel_clear() == 0) {
            int len;
            uint8_t ackbuf[ACK_LEN];

            wt = RTIMER_NOW();
            watchdog_periodic();
            while(RTIMER_CLOCK_LT(RTIMER_NOW(),
                                  wt + AFTER_ACK_DETECTED_WAIT_TIME));

            if(NETSTACK_RADIO.pending_packet()) {
              len = NETSTACK_RADIO.read(ackbuf, ACK_LEN);
              if(len == ACK_LEN && ackbuf[2] == dsn) {
                /* Ack received */
                ret = MAC_TX_OK;
              } else {
                /* Not an ack or ack not for us: collision */
                ret = MAC_TX_COLLISION;
              }
            }
          }
        }
        break;
      case RADIO_TX_COLLISION:
        ret = MAC_TX_COLLISION;
        break;
      default:
        ret = MAC_TX_ERR;
        break;
      }
    }

#else /* ! DISCOVERY_AWARE_RDC_802154_AUTOACK */

    switch(NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen())) {
    case RADIO_TX_OK:
      ret = MAC_TX_OK;
      break;
    case RADIO_TX_COLLISION:
      ret = MAC_TX_COLLISION;
      break;
    case RADIO_TX_NOACK:
      ret = MAC_TX_NOACK;
      break;
    default:
      ret = MAC_TX_ERR;
      break;
    }

#endif /* ! DISCOVERY_AWARE_RDC_802154_AUTOACK */
  }



  if (!rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &rimeaddr_null)
          &&  ret == MAC_TX_OK) {
    send_flag = 1;
#ifdef DISCOVERY_AWARE_RDC_CONF_DYNAMIC_TIMEOUT
    to_modifier<<=1;
#endif /* DISCOVERY_AWARE_RDC_CONF_DYNAMIC_TIMEOUT */
  }

  mac_call_sent_callback(sent, ptr, ret, 1);
}
/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
#if DISCOVERY_AWARE_RDC_802154_AUTOACK
  if(packetbuf_datalen() == ACK_LEN) {
    /* Ignore ack packets */
  } else
#endif /* DISCOVERY_AWARE_RDC_802154_AUTOACK */
  if(NETSTACK_FRAMER.parse() == 0) {
#if DISCOVERY_AWARE_RDC_ADDRESS_FILTER
  } else if(!rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                                         &rimeaddr_node_addr) &&
            !rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                          &rimeaddr_null)) {
#endif /* DISCOVERY_AWARE_RDC_ADDRESS_FILTER */
  } else {
#if DISCOVERY_AWARE_RDC_802154_AUTOACK || DISCOVERY_AWARE_RDC_802154_AUTOACK_HW
    /* Check for duplicate packet by comparing the sequence number
       of the incoming packet with the last few ones we saw. */
    int i;
    for(i = 0; i < MAX_SEQNOS; ++i) {
      if(packetbuf_attr(PACKETBUF_ATTR_PACKET_ID) == received_seqnos[i].seqno &&
         rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER),
                      &received_seqnos[i].sender)) {
        /* Drop the packet. */
         return;
      }
    }
    for(i = MAX_SEQNOS - 1; i > 0; --i) {
      memcpy(&received_seqnos[i], &received_seqnos[i - 1],
             sizeof(struct seqno));
    }
    received_seqnos[0].seqno = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
    rimeaddr_copy(&received_seqnos[0].sender,
                  packetbuf_addr(PACKETBUF_ADDR_SENDER));
#endif /* DISCOVERY_AWARE_RDC_802154_AUTOACK */

#ifdef DISCOVERY_AWARE_RDC_CONF_DYNAMIC_TIMEOUT
    if (!rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &rimeaddr_null)) {
      to_modifier <<= 1;
    } else {
      to_modifier++;
    }
#endif /* DISCOVERY_AWARE_RDC_CONF_DYNAMIC_TIMEOUT */


    if (!rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &rimeaddr_null)) {
      rec_flag = 1;
    }

    NETSTACK_MAC.input();
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  radio_status = 1;
  return NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  if(keep_radio_on) {
    return NETSTACK_RADIO.on();
  } else {
    radio_status = 0;
    PRINTF("RDC: Radio OFF\n");
    return NETSTACK_RADIO.off();
  }
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  on();
  PRINTF("RDC: init\n");
  process_start(&discovery_aware_rdc_process, NULL);
}

PROCESS_THREAD(discovery_aware_rdc_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT();

    if (dtn_disco_start_event == ev) {
      PRINTF("RDC: received START event\n");
      radio_may_be_turned_off = 0;
      if (!radio_status) {
        on();
      }
      continue;
    }

    if (dtn_disco_stop_event == ev) {
      PRINTF("RDC: received STOP event\n");
      radio_may_be_turned_off = 1;
      etimer_set(&radio_off_timeout_timer, RADIO_OFF_SEND_TIMEOUT * to_modifier * CLOCK_SECOND);
      continue;
    }

    if (etimer_expired(&radio_off_timeout_timer) && radio_may_be_turned_off) {
      PRINTF("RDC: Radio off timeout reached!\n");
      if (radio_status) {
        if (  NETSTACK_RADIO.pending_packet() ||
              NETSTACK_RADIO.receiving_packet() ||
              !NETSTACK_RADIO.channel_clear() ||
              rec_flag == 1 ||
              send_flag == 1) {
          PRINTF("RDC: NOT turning radio OFF because of pending packet or recent traffic.\n");
          send_flag = 0;
          rec_flag = 0;
          etimer_set(&radio_off_timeout_timer, RADIO_OFF_SEND_TIMEOUT * to_modifier * CLOCK_SECOND);
        } else {
          PRINTF("RDC: Turning radio OFF.\n");
          radio_status = 0;
          send_flag = 0;
          rec_flag = 0;
#ifdef DISCOVERY_AWARE_RDC_CONF_DYNAMIC_TIMEOUT
          to_modifier = 1;
#endif /* DISCOVERY_AWARE_RDC_CONF_DYNAMIC_TIMEOUT */

          NETSTACK_RADIO.off();

#ifdef DISCOVERY_AWARE_RDC_CONF_CLEAR_NEIGHBOURS
          // empty neightbour list to prevent routing from senseless activity
          PRINTF("Clearing neighbours after radio off\n.");
          DISCOVERY.clear();
#endif /* DISCOVERY_AWARE_RDC_CONF_CLEAR_NEIGHBOURS */
        }
      } else {
        PRINTF("RDC: NOT Turning radio off because it is already.\n");
      }
      continue;
    }

  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
const struct rdc_driver discovery_aware_rdc_driver = {
  "discovery_aware_rdc",
  init,
  send_packet,
  packet_input,
  on,
  off,
  channel_check_interval,
};
/*---------------------------------------------------------------------------*/

