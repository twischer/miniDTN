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
import se.sics.cooja.avrmote.MicaZMote;
import se.sics.cooja.avrmote.MicaZMoteType;
import avrora.sim.radio.CC2420Radio;
import avrora.sim.radio.Radio;

/**
 * Cooja support for Avrora's CC2420Radio.
 *
 * @see MicaZMoteType
 * @author Joakim Eriksson, Fredrik Osterlind
 */
@ClassDescription("CC2420")
public class MicaZRadio extends Avrora802154Radio {
  private static Logger logger = Logger.getLogger(MicaZRadio.class);

  /* Avrora's FSM for CC2420Radio:
   * 0: Power Off:
   * 1: Power Down:
   * 2: Idle:
   * 3: Receive (Rx):
   * 4: Transmit (Tx):        0:
   * ...
   * 259: Transmit (Tx):        255:
   * 260: null
   * 261: null
   */
  private final static int STATE_POWEROFF = 0;
  private final static int STATE_POWERDOWN = 1;
  private final static int STATE_IDLE = 2;

  private CC2420Radio cc2420;

  public MicaZRadio(Mote mote) {
    super(mote,
        ((Radio) ((MicaZMote)mote).getMicaZ().getDevice("radio")),
        ((CC2420Radio) ((MicaZMote)mote).getMicaZ().getDevice("radio")).getFiniteStateMachine());
    cc2420 = (CC2420Radio) ((MicaZMote)mote).getMicaZ().getDevice("radio");
  }

  protected boolean isRadioOn(int state) {
    if (state == STATE_POWEROFF) {
      return false;
    }
    if (state == STATE_POWERDOWN) {
      return false;
    }
    if (state == STATE_IDLE) {
      return false;
    }

    /* XXX What if state is above 260 ("null")? On or off? */
    return true;
  }

  public double getFrequency() {
    return cc2420.getFrequency();
  }

  public double getCurrentOutputPower() {
    return cc2420.getPower();
  }

  public int getCurrentOutputPowerIndicator() {
    return cc2420.readRegister(CC2420Radio.TXCTRL) & 0x1f;
  }

  public int getOutputPowerIndicatorMax() {
    return 0x1f;
  }
}
