/*
 * Copyright (c) 2012, Swedish Institute of Computer Science.
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

package se.sics.cooja.emulatedmote;

import java.util.Collection;

import org.apache.log4j.Logger;
import org.jdom.Element;

import se.sics.cooja.Mote;
import se.sics.cooja.MoteTimeEvent;
import se.sics.cooja.RadioPacket;
import se.sics.cooja.Simulation;
import se.sics.cooja.interfaces.CustomDataRadio;
import se.sics.cooja.interfaces.Position;
import se.sics.cooja.interfaces.Radio;

/**
 * 802.15.4 radio class for COOJA.
 *
 * @author Joakim Eriksson, David Kopf, Fredrik Osterlind
 */

public abstract class Radio802154 extends Radio implements CustomDataRadio {
  private static Logger logger = Logger.getLogger(Radio802154.class);
  private final static boolean DEBUG = false;

  /**
   * Cross-level:
   * Inter-byte delay for delivering cross-level packet bytes.
   */
  public static final long DELAY_BETWEEN_BYTES =
    (long) (1000.0*Simulation.MILLISECOND/(250000.0/8.0)); /* us. Corresponds to 250kbit/s */

  protected RadioEvent lastEvent = RadioEvent.UNKNOWN;

  private boolean isTransmitting = false;
  protected boolean isReceiving = false;
  protected boolean isInterfered = false;

  private byte lastOutgoingByte;
  private byte lastIncomingByte;
  private RadioPacket lastOutgoingPacket = null;
  private RadioPacket lastIncomingPacket = null;

  private Mote mote;

  public Radio802154(Mote mote) {
    this.mote = mote;
  }

  private int txLen = 0;
  private int txExpLen = 0; /* transmitted bytes before overflow */
  private byte[] txBuffer = new byte[127/*data*/ + 15/*preamble*/];
  protected void handleTransmit(byte val) {
    if (!isTransmitting()) {
      lastEvent = RadioEvent.TRANSMISSION_STARTED;
      isTransmitting = true;
      if (DEBUG) logger.debug(mote.getID() + ": ----- 802.15.4 TRANSMISSION STARTED -----");
      setChanged();
      notifyObservers();
    }

    if (txLen >= txBuffer.length) {
      /* Bad size packet, too large */
      logger.warn("Error: bad size: " + txLen + ", dropping outgoing byte: " + val);
      return;
    }

    /* Send byte to radio medium */
    lastOutgoingByte = val;
    lastEvent = RadioEvent.CUSTOM_DATA_TRANSMITTED;
    if (DEBUG) logger.debug(mote.getID() + ":----- 802.15.4 CUSTOM DATA TRANSMITTED -----");
    setChanged();
    notifyObservers();

    txBuffer[txLen++] = val;

    /* TODO We assume that the 6th byte transmitted is the length.
     *  4xPREAMBLE, 1xSFD, 1xLEN, 127xDATA.
     *  Instead, we should search for the SFD. */
    if (txLen == 6) {
      txExpLen = val + 6;
    }

    if (txLen == txExpLen) {
      /* Send packet to radio medium */
      lastOutgoingPacket = Radio802154PacketConverter.fromCC2420ToCooja(txBuffer);
      lastEvent = RadioEvent.PACKET_TRANSMITTED;
      if (DEBUG) logger.debug(mote.getID() + ":----- 802.15.4 PACKET TRANSMITTED -----");
      setChanged();
      notifyObservers();

      isTransmitting = false;
      lastEvent = RadioEvent.TRANSMISSION_FINISHED;
      setChanged();
      notifyObservers();
      txLen = 0;
    }
  }

  public RadioPacket getLastPacketTransmitted() {
    return lastOutgoingPacket;
  }

  public RadioPacket getLastPacketReceived() {
    return lastIncomingPacket;
  }

  public void setReceivedPacket(RadioPacket packet) {
    /* Note:
     * Only nodes at other abstraction levels deliver full packets.
     * MSPSim motes with 802.15.4 radios would instead directly deliver bytes. */

    lastIncomingPacket = packet;

    /* Delivering packet bytes with delays */
    byte[] packetData = Radio802154PacketConverter.fromCoojaToCC2420(packet);
    long deliveryTime = getMote().getSimulation().getSimulationTime();
    for (byte b: packetData) {
      if (isInterfered()) {
        b = (byte) 0xFF;
      }

      final byte byteToDeliver = b;
      getMote().getSimulation().scheduleEvent(new MoteTimeEvent(mote, 0) {
        public void execute(long t) {
          handleReceive(byteToDeliver);
        }
      }, deliveryTime);
      deliveryTime += DELAY_BETWEEN_BYTES;
    }
  }

  /* Custom data radio support */
  public byte getLastCustomDataTransmitted() {
    return lastOutgoingByte;
  }

  public byte getLastCustomDataReceived() {
    return lastIncomingByte;
  }

  public final void receiveCustomData(byte data) {
    lastIncomingByte = data;
    handleReceive(data);
  }

  protected void radioOn() {
    lastEvent = RadioEvent.HW_ON;
    setChanged();
    notifyObservers();
  }

  protected void radioOff() {
    /* Radio was turned off during transmission.
     * May for example happen if watchdog triggers */
    if (isTransmitting()) {
      logger.warn("Turning off radio while transmitting, ending packet prematurely");

      /* Simulate end of packet */
      lastOutgoingPacket = new RadioPacket() {
        public byte[] getPacketData() {
          return new byte[0];
        }
      };

      lastEvent = RadioEvent.PACKET_TRANSMITTED;
      if (DEBUG) logger.debug(mote.getID() + ":----- 802.15.4 PACKET TRANSMITTED (Radio turned off) -----");
      setChanged();
      notifyObservers();

      /* Register that transmission ended in radio medium */
      if (DEBUG) logger.debug(mote.getID() + ":----- 802.15.4 TRANSMISSION FINISHED -----");
      isTransmitting = false;
      lastEvent = RadioEvent.TRANSMISSION_FINISHED;
      setChanged();
      notifyObservers();
    }

    lastEvent = RadioEvent.HW_OFF;
    setChanged();
    notifyObservers();
  }

  protected abstract void handleStartOfReception();
  protected abstract void handleEndOfReception();
  protected abstract void handleReceive(byte b);

  public boolean isTransmitting() {
    return isTransmitting;
  }

  public boolean isReceiving() {
    return isReceiving;
  }

  public boolean isInterfered() {
    return isInterfered;
  }

  public void signalReceptionStart() {
    isReceiving = true;

    handleStartOfReception();

    if (DEBUG) logger.debug(mote.getID() + ":----- 802.15.4 RECEPTION STARTED -----");
    lastEvent = RadioEvent.RECEPTION_STARTED;
    setChanged();
    notifyObservers();

  }

  public RadioEvent getLastEvent() {
    return lastEvent;
  }

  public final void signalReceptionEnd() {
    /* Deliver packet data */
    isReceiving = false;
    isInterfered = false;

    handleEndOfReception();

    if (DEBUG) logger.debug(mote.getID() + ":----- 802.15.4 RECEPTION FINISHED -----");
    lastEvent = RadioEvent.RECEPTION_FINISHED;
    setChanged();
    notifyObservers();
  }

  public void interfereAnyReception() {
    isInterfered = true;
    isReceiving = false;
    lastIncomingPacket = null;

    if (DEBUG) logger.debug(mote.getID() + ":-----  RECEPTION INTERFERED -----");
    lastEvent = RadioEvent.RECEPTION_INTERFERED;
    setChanged();
    notifyObservers();
  }

  public Mote getMote() {
    return mote;
  }

  public Position getPosition() {
    return mote.getInterfaces().getPosition();
  }

  public Collection<Element> getConfigXML() {
    return null;
  }

  public void setConfigXML(Collection<Element> configXML, boolean visAvailable) {
  }
}
