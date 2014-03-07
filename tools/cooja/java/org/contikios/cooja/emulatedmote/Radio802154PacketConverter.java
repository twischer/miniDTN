/*
 * Copyright (c) 2008, Swedish Institute of Computer Science.
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
package org.contikios.cooja.emulatedmote;

import org.apache.log4j.Logger;

import org.contikios.cooja.ConvertedRadioPacket;
import org.contikios.cooja.RadioPacket;

/**
 * Converts Cooja radio packets to full 802.15.4 radio packets.
 * 
 * Handles radio driver specifics such as length header and CRC footer.
 *
 * @author Fredrik Osterlind, Joakim Eriksson
 */
public class Radio802154PacketConverter {

  private static final Logger logger = Logger.getLogger(Radio802154PacketConverter.class);

  /**
   * 4 octets preamble
   */
  public static final boolean WITH_PREAMBLE = true;
  /**
   * 1 octet Start of frame Delimiter (SFD)
   */
  public static final boolean WITH_SFD = true;
  /**
   * 2 octets Frame Check Sequence (FCS)
   */
  public static final boolean WITH_FCS = true;

  /**
   * Converts from Cooja packet data (PHY payload) to PHY PDU.
   *
   * @param packet Cooja packet data
   * @return converted PHY layer data
   */
  public static byte[] fromPacketTo802154(RadioPacket packet) {
    byte cc2420Data[] = new byte[6 + 127];
    int pos = 0;
    byte packetData[] = packet.getPacketData();
    byte len; /* total packet minus preamble(4), synch(1) and length(1) */

    short accumulatedCRC = 0;

    /* --- Add PHY header */

    /* 4 bytes preamble */
    if (WITH_PREAMBLE) {
      cc2420Data[pos++] = 0;
      cc2420Data[pos++] = 0;
      cc2420Data[pos++] = 0;
      cc2420Data[pos++] = 0;
    }

    /* 1 byte synch */
    if (WITH_SFD) {
      cc2420Data[pos++] = 0x7A;
    }

    /* 1 byte length */
    len = (byte) packetData.length;
    if (WITH_FCS) {
      len += 2;
    }
    cc2420Data[pos++] = len;

    /* --- Add Payload */
    for (byte b : packetData) {
      accumulatedCRC = CRCCoder.crc16Add(b, accumulatedCRC);
    }
    System.arraycopy(packetData, 0, cc2420Data, pos, packetData.length);
    pos += packetData.length;

    /* 2 bytes Frame Check Sequence (FCS) */
    if (WITH_FCS) {
      cc2420Data[pos++] = (byte) (accumulatedCRC & 0xff);
      cc2420Data[pos++] = (byte) ((accumulatedCRC >> 8) & 0xff);
    }

    byte cc2420DataStripped[] = new byte[pos];
    System.arraycopy(cc2420Data, 0, cc2420DataStripped, 0, pos);

    /*logger.info("Data length: " + cc2420DataStripped.length);*/
    return cc2420DataStripped;
  }

  /**
   * Converts from PHY PDU to Cooja packet data (PHY payload).
   *
   * @param data PHY layer data
   * @return converted Cooja packet data
   */
  public static ConvertedRadioPacket from802154toPacket(byte[] data) {
    int pos = 0;
    int len; /* Payload */

    int originalLen;

    /* Use some CC2420/MAC specific field such as X-MAC response */

    /* (IGNORED) 4 bytes preamble */
    if (WITH_PREAMBLE) {
      pos += 4;
    }

    /* (IGNORED) 1 byte synch */
    if (WITH_SFD) {
      pos += 1;
    }

    /* 1 byte length */
    len = data[pos];
    originalLen = len;
    pos += 1;

    /* (IGNORED) 2 bytes checksum */
    if (WITH_FCS) {
      len -= 2;
    }

    /*logger.info("Payload pos: " + pos);
     logger.info("Payload length: " + len);*/
    byte originalData[] = new byte[originalLen];
    System.arraycopy(data, 6, originalData, 0, originalLen);
    if (len < 0) {
      //  logger.warn("No cross-level conversion available: negative packet length");
      return new ConvertedRadioPacket(new byte[0], originalData);
    }
    byte convertedData[] = new byte[len];
    System.arraycopy(data, pos, convertedData, 0, len);
    return new ConvertedRadioPacket(convertedData, originalData);
  }

}
