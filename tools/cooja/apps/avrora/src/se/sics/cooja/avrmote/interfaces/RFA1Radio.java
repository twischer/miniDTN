/*
 * Copyright (c) 2009, Swedish Institute of Computer Science.
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

import avrora.sim.FiniteStateMachine;
import avrora.sim.FiniteStateMachine.Probe;
import avrora.sim.platform.RFA1;
import avrora.sim.radio.ATmega128RFA1Radio;
import avrora.sim.radio.Medium;

import se.sics.cooja.*;
import se.sics.cooja.avrmote.RFA1Mote;
import se.sics.cooja.emulatedmote.Radio802154;

/**
 * ATMega128RFA1 radio to COOJA wrapper.
 *
 * @author Joakim Eriksson
 * @author David Kopf
 */
@ClassDescription("RFA1")
public class RFA1Radio extends Radio802154 {
  private static Logger logger = Logger.getLogger(RFA1Radio.class);

  private RFA1 rfa1;
  private ATmega128RFA1Radio rf231;

//  private int mode;
  Medium.Transmitter trans;
  ATmega128RFA1Radio.Receiver recv;
  FiniteStateMachine fsm;
  
  public RFA1Radio(Mote mote) {
    super(mote);
    rfa1 = ((RFA1Mote)mote).getRFA1();
    rf231 = (ATmega128RFA1Radio) rfa1.getDevice("radio");
   
    trans = rf231.getTransmitter();
    fsm = rf231.getFiniteStateMachine();
    recv = (ATmega128RFA1Radio.Receiver) rf231.getReceiver();
    trans.insertProbe(new Medium.Probe.Empty() {
        public void fireBeforeTransmit(Medium.Transmitter t, byte val) {
            handleTransmit(val);
        }
    });
    fsm.insertProbe(new Probe() {
        public void fireBeforeTransition(int arg0, int arg1) {
        }
        public void fireAfterTransition(int arg0, int arg1) {
            System.out.println("RFA1 FSM: " + arg0 + " " + arg1);
            RadioEvent re = null;
            if (arg1 >= 3) {
                re = RadioEvent.HW_ON;
            } else {
                re = RadioEvent.HW_OFF;
                 /*
                if (arg0 > 3 && arg1 == 2) {
                   // likely that radio dips into 2 before going back to 3
                    re = RadioEvent.HW_OFF;
                }
                */
            }
            if (re != null) {
                lastEvent = re;
                lastEventTime = RFA1Radio.this.mote.getSimulation().getSimulationTime();
                setChanged();
                notifyObservers();
            }
        }
    });
    
    
  }

  public int getChannel() {
//System.out.println("RFA1 getchannel " + rf231.getChannel());
    return rf231.getChannel();
  }

  public int getFrequency() {
                System.out.println("RFA1 getfreq " +  rf231.getFrequency());
      return (int) rf231.getFrequency();
  }

  public boolean isReceiverOn() {
      FiniteStateMachine fsm = rf231.getFiniteStateMachine();
      //Receiver is on in state 3, transmitter for > 3
  //            System.out.println("RFA1 isreceiveron " + fsm.getCurrentState());
   //   return fsm.getCurrentState() == 3;
            return true;
  }

  public void signalReceptionStart() {
                System.out.println("RFA1 receptionstart");
//    rf231.setCCA(true);
//    hasFailedReception = mode == rf231.MODE_TXRX_OFF;
      super.signalReceptionStart();
  }

  public double getCurrentOutputPower() {
                      System.out.println("RFA1 getoutputpower " + rf231.getPower());
    return rf231.getPower();
  }

  public int getCurrentOutputPowerIndicator() {
 //                       System.out.println("RFA1 getOutputPowerIndicator" );
    return 31; //rf231.getOutputPowerIndicator();
  }

  public int getOutputPowerIndicatorMax() {
  //                    System.out.println("RFA1 getOutputPowerIndicatorMax" );
    return 31;
  }

  public double getCurrentSignalStrength() {
 //                   System.out.println("RFA1 getsignalstrength " + recv.getRSSI());
    return recv.getRSSI();
  }

  public void setCurrentSignalStrength(double signalStrength) {
  //                System.out.println("RFA1 setsignalstrength " + signalStrength);
    recv.setRSSI(signalStrength);
  }

  protected void handleEndOfReception() {
                        System.out.println("RFA1 handleEndOfReception" );
      /* tell the receiver that the packet is ended */
      recv.nextByte(false, (byte)0);
  }

  protected void handleReceive(byte b) {
                       System.out.println("RFA1 handleReceive" );
      recv.nextByte(true, (byte)b);
  }
}
