/*
 * Copyright (c) 2008-2012, Swedish Institute of Computer Science.
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

package se.sics.cooja.mspmote.interfaces;

import org.apache.log4j.Logger;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.Mote;
import se.sics.cooja.emulatedmote.Radio802154;
import se.sics.cooja.mspmote.MspMote;
import se.sics.cooja.mspmote.MspMoteTimeEvent;
import se.sics.mspsim.chip.CC2420;
import se.sics.mspsim.chip.ChannelListener;
import se.sics.mspsim.chip.RFListener;
import se.sics.mspsim.core.Chip;
import se.sics.mspsim.core.OperatingModeListener;

/**
 * MSPSim 802.15.4 radio to COOJA wrapper.
 *
 * @author Fredrik Osterlind
 */
@ClassDescription("Mspsim's 802.15.4")
public class Msp802154Radio extends Radio802154 {
  private static Logger logger = Logger.getLogger(Msp802154Radio.class);

  private final MspMote mote;
  private final se.sics.mspsim.chip.Radio802154 radio;

  private RFListener rfListener;
  private OperatingModeListener operatingModeListener;
  private ChannelListener channelListener;


  /**
   * Current received signal strength.
   * May differ from CC2420's internal value which is an average of the last 8 symbols.
   */
  private double currentSignalStrength = 0;

  /**
   * Last 8 received signal strengths
   */
  private double[] rssiLast = new double[8];
  private int rssiLastCounter = 0;

  public Msp802154Radio(Mote m) {
    super(m);

    this.mote = (MspMote)m;
    this.radio = mote.getCPU().getChip(se.sics.mspsim.chip.Radio802154.class);
    if (radio == null) {
      throw new IllegalStateException("Mote is not equipped with an IEEE 802.15.4 radio");
    }

    radio.addRFListener(rfListener = new RFListener() {
      public void receivedByte(byte data) {
        handleTransmit(data);
      }
    });
    radio.addOperatingModeListener(operatingModeListener = new OperatingModeListener() {
      public void modeChanged(Chip source, int mode) {
        if (radio.isReadyToReceive()) {
          radioOn();
        } else {
          // TODO We should check if radio was already turned off?
          radioOff();
        }
      }
    });
    radio.addChannelListener(channelListener = new ChannelListener() {
      public void channelChanged(int channel) {
        /* XXX Currently assumes zero channel switch time */
        lastEvent = RadioEvent.UNKNOWN;
        setChanged();
        notifyObservers();
      }
    });
  }

  public void removed() {
    super.removed();

    radio.removeOperatingModeListener(operatingModeListener);
    radio.removeChannelListener(channelListener);
    radio.removeRFListener(rfListener);
  }

  public void handleReceive(byte b) {
    final byte inputByte;
    if (isInterfered()) {
      inputByte = (byte)0xFF;
    } else {
      inputByte = b;
    }

    /* XXX We need a separate time event to synchronize Mspsim's internal
     * clocks here */
    new MspMoteTimeEvent(mote, 0) {
      public void execute(long t) {
        super.execute(t);
        if (!radio.isReadyToReceive()) {
          /*logger.warn(String.format("Radio receiver not ready, dropping byte: %02x", inputByte));*/
        }
        radio.receivedByte(inputByte);
        mote.requestImmediateWakeup();
      }
    }.execute(mote.getSimulation().getSimulationTime());
  }

  public void handleStartOfReception() {
  }
  public void handleEndOfReception() {
  }

  public int getChannel() {
    return radio.getActiveChannel();
  }

  public double getCurrentSignalStrength() {
    return currentSignalStrength;
  }

  public void setCurrentSignalStrength(final double signalStrength) {
    if (signalStrength == currentSignalStrength) {
      return; /* ignored */
    }
    currentSignalStrength = signalStrength;
    if (rssiLastCounter == 0) {
      getMote().getSimulation().scheduleEvent(new MspMoteTimeEvent(mote, 0) {
        public void execute(long t) {
          super.execute(t);

          /* Update average */
          System.arraycopy(rssiLast, 1, rssiLast, 0, 7);
          rssiLast[7] = currentSignalStrength;
          double avg = 0;
          for (double v: rssiLast) {
            avg += v;
          }
          avg /= rssiLast.length;

          radio.setRSSI((int) avg);

          rssiLastCounter--;
          if (rssiLastCounter > 0) {
            mote.getSimulation().scheduleEvent(this, t+DELAY_BETWEEN_BYTES/2);
          }
        }
      }, mote.getSimulation().getSimulationTime());
    }
    rssiLastCounter = 8;
  }

  public double getCurrentOutputPower() {
    return radio.getOutputPower();
  }

  public int getCurrentOutputPowerIndicator() {
    return radio.getOutputPowerIndicator();
  }

  public int getOutputPowerIndicatorMax() {
    return 31;
  }

  public boolean isRadioOn() {
    if (radio.isReadyToReceive()) {
      return true;
    }
    if (radio.getMode() == CC2420.MODE_POWER_OFF) {
      return false;
    }
    if (radio.getMode() == CC2420.MODE_TXRX_OFF) {
      return false;
    }
    return true;
  }
}
