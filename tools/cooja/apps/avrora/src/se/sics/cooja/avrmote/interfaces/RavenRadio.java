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

package se.sics.cooja.avrmote.interfaces;

import org.apache.log4j.Logger;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.Mote;
import se.sics.cooja.avrmote.RavenMote;
import se.sics.cooja.emulatedmote.Radio802154;
import avrora.sim.FiniteStateMachine;
import avrora.sim.FiniteStateMachine.Probe;
import avrora.sim.platform.Raven;
import avrora.sim.radio.AT86RF231Radio;
import avrora.sim.radio.Medium;

/**
 * Atmel AT86RF230 radio to COOJA wrapper, using the AT86RF231 emulator.
 *
 * @author David Kopf
 */
@ClassDescription("AT86RF230 Radio")
public class RavenRadio extends Radio802154 {
  private static Logger logger = Logger.getLogger(RavenRadio.class);
  private final static boolean DEBUG = false;
  private final static boolean DEBUGV = false;

  private Raven raven;
  Medium.Transmitter trans;
  AT86RF231Radio.Receiver recv;
  FiniteStateMachine fsm;
  private AT86RF231Radio rf230;

  public RavenRadio(Mote mote) {
    super(mote);
    raven = ((RavenMote)mote).getRaven();
    rf230 = (AT86RF231Radio) raven.getDevice("radio");

    trans = rf230.getTransmitter();
    fsm = rf230.getFiniteStateMachine();
    recv = (AT86RF231Radio.Receiver) rf230.getReceiver();
    trans.insertProbe(new Medium.Probe.Empty() {
        public void fireBeforeTransmit(Medium.Transmitter t, byte val) {
            handleTransmit(val);
        }
    });
    fsm.insertProbe(new Probe() {
        public void fireBeforeTransition(int arg0, int arg1) {
        }
        public void fireAfterTransition(int arg0, int arg1) {
            if (DEBUG) System.out.println("Raven FSM: " + arg0 + " " + arg1);
            RadioEvent re = null;
            if (arg1 >= 1) {
                re = RadioEvent.HW_ON;
            } else {
                re = RadioEvent.HW_OFF;
            }
            if (re != null) {
                lastEvent = re;
                lastEventTime = RavenRadio.this.mote.getSimulation().getSimulationTime();
                setChanged();
                notifyObservers();
            }
        }
    });
  }

  public int getChannel() {
    if (DEBUG) System.out.println("Raven getChannel " + rf230.getChannel());
    return rf230.getChannel();
  }

  public int getFrequency() {
    if (DEBUG) System.out.println("Raven getFrequency " +  rf230.getFrequency());
    return (int) rf230.getFrequency();
  }

  public boolean isRadioOn() {
    //This apparently means is radio on
    //Receiver is on in state 3-5, and a transmitter for > 5
    return (rf230.getFiniteStateMachine().getCurrentState() > 2);
  }

  public void signalReceptionStart() {
      if (DEBUG) System.out.println("Raven signalReceptionStart");
//    hasFailedReception = mode == rf230.MODE_TXRX_OFF;
      super.signalReceptionStart();
  }

  public double getCurrentOutputPower() {
    if (DEBUG) System.out.println("Raven getCurrentOutputPower " + rf230.getPower());
    return rf230.getPower();
  }

  public int getCurrentOutputPowerIndicator() {
    if (DEBUG) System.out.println("Raven getOutputPowerIndicator" );
    return 31; //rf230.getOutputPowerIndicator();
  }

  public int getOutputPowerIndicatorMax() {
    if (DEBUG) System.out.println("Raven getOutputPowerIndicatorMax" );
    return 31;
  }

  public double getCurrentSignalStrength() {
    if (DEBUG) System.out.println("Raven getCurrentSignalStrength " + recv.getRSSI());
    return recv.getRSSI();
  }

  public void setCurrentSignalStrength(double signalStrength) {
    if (DEBUG) System.out.println("Raven setCurrentSignalStrength " + signalStrength);
    recv.setRSSI(signalStrength);
  }

  protected void handleEndOfReception() {
    if (DEBUG) System.out.println("Raven handleEndOfReception" );
      /* tell the receiver that the packet is ended */
      recv.nextByte(false, (byte)0);
  }

  protected void handleReceive(byte b) {
    if (DEBUGV) System.out.println("Raven handleReceive" );
    recv.nextByte(true, b);
  }
}
