/**
 * Copyright (c) 2004-2005, Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the University of California, Los Angeles nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package avrora.sim.mcu;

import avrora.sim.*;
import avrora.sim.radio.Radio;
import avrora.sim.radio.Medium;
import avrora.sim.radio.ATmega128RFA1Radio;
import avrora.sim.state.*;
import cck.text.StringUtil;
import cck.util.Arithmetic;
import cck.util.Util;

/**
 * Internal Radio interface. Used on the <code>ATMega128RFA1</code> platform for radio communication.
 *
 * @author Daniel Lee, Simon Han, David Kopf
 */
public class RADIO extends AtmelInternalDevice implements RADIODevice, InterruptTable.Notification {

    private static TRX_STATE_Reg TRX_STATE_reg;
//    final TRX_STATUS_Reg TRX_STATUS_reg;
    private static TRXPR_Reg TRXPR_reg;

    private static ATmega128RFA1Radio connectedRadio;
    private static RADIODevice connectedDevice;

    public void connect(ATmega128RFA1Radio r, RADIODevice d) {
        connectedRadio = r;
 //       connectedTransmitter = connectedRadio.getTransmitter();
        connectedDevice = d;
    }
    
    public RADIO(AtmelMicrocontroller m) {
        super("radio", m);
        TRX_STATE_reg = new TRX_STATE_Reg();
  //      TRX_STATUS_reg = new TRX_STATUS_Reg();
        TRXPR_reg = new TRXPR_Reg();    
        installIOReg("TRX_STATE", TRX_STATE_reg);
 //       installIOReg("TRX_STATUS", TRX_STATUS_reg);
        installIOReg("TRXPR", TRXPR_reg);
    
    }

    /**
     * Post interrupts
     */
 /*
    private void postRADIOInterrupt(int n) {
                        devicePrinter.println("posting interrupt " + n);
        interpreter.setPosted(interruptNum1, true);
    //    SPSR_reg.setSPIF();
    }

    private void unpostRADIOInterrupt() {
                            devicePrinter.println("unposting interrupt ");
        interpreter.setPosted(interruptNum1, false);
   //     SPSR_reg.clearSPIF();
    }
*/
    /**
     * TRXPR register. Emulates the TRXRST and SLPTR pins of an external RF231 radio.
     */
  //  class TRXPR_Reg implements ActiveRegister {

      //-- Radio states ---------------------------------------------------------
    public static final byte STATE_BUSY_RX      = 0x01;
    public static final byte STATE_BUSY_TX      = 0x02;
    public static final byte STATE_RX_ON        = 0x06; 
    public static final byte STATE_TRX_OFF      = 0x08;
    public static final byte STATE_PLL_ON       = 0x09;
    public static final byte STATE_SLEEP        = 0x0F;
    public static final byte STATE_BUSY_RX_AACK = 0x11;
    public static final byte STATE_BUSY_TX_ARET = 0x12;
    public static final byte STATE_RX_AACK_ON   = 0x16;
    public static final byte STATE_TX_ARET_ON   = 0x19;

    protected class TRXPR_Reg extends RWRegister {
        protected byte lastSLP = 0,lastRST = 0;
        public void write(byte val) {
 //         if (devicePrinter != null) devicePrinter.println("RADIO: wrote " + StringUtil.toMultirepString(val, 8) + " to TRXPR");
           super.write(val);  
     //      devicePrinter.println("RADIO: SLP pin change, state, status "+ TRX_STATE_reg.read());           
            if ((val&0x02) != lastSLP) {     //SLP pin toggle
                if (lastSLP == 0 ) {         //Raise pin
                //    devicePrinter.println("RADIO: SLP pin raised");
                    switch (TRX_STATE_reg.read()) {
                       //if TRX_OFF, -> SLEEP
                        case STATE_TRX_OFF:
                           connectedRadio.pinChangeSLP(val);
                        break;
                        //if PLL_ON or TX_ARET_ON -> BUSY_TX
                        case STATE_PLL_ON:
                        case STATE_TX_ARET_ON:
                           connectedRadio.pinChangeSLP(val);
                        break;
                     }
                } else {                   //Lower pin
                //    devicePrinter.println("RADIO: SLP pin lowered");
                    switch (TRX_STATE_reg.read()) {
                        //if SLEEP, ->TRX_OFF
                        case STATE_SLEEP:
                            connectedRadio.pinChangeSLP(val);
                            break;
                    }
 /* After initiating a state change
by a rising edge at Bit SLPTR in radio transceiver states TRX_OFF, RX_ON or
RX_AACK_ON, the radio transceiver remains in the new state as long as the pin is
logical high and returns to the preceding state with the falling edge.
*/
                }
                lastSLP = (byte) (val&0x02);
            }
 /* RST high resets all registers to default values and is cleared automatically.
  * The microcontroller has to set SLPTR to the default value
  */
            if ((val&0x01) != 0) {
                connectedRadio.reset();
                 //->TRX_OFF
                 //delay?
                 super.write((byte)(val&0x02)); //which should be zero
            }
        }
    }
    /**
     * TRX_STATE control register.
     */
    protected class TRX_STATE_Reg extends RWRegister {
        protected byte lastCMD = 0;
        public void write(byte val) {
            //Write the register only if the command has changed.
            //The newCommand mnethod will change the readonly TRAC_STATUS bits in RAM,
            //which will cause a recursive call to this routine that must be ignored.
            val = (byte) (val & 0x1F);
            
            if (val != lastCMD) {
                lastCMD = val;
//              if (devicePrinter != null) devicePrinter.println("RADIO: wrote " + val + " to TRX_STATE");
                super.write(val);
                connectedRadio.newCommand(val);
            }
        }
    }


    public void force(int inum) {
    devicePrinter.println("RADIO: force " + inum);
 //       SPSR_reg.setSPIF();
    }
    public void invoke(int inum) {
        devicePrinter.println("RADIO: invoke " + inum);
 //       unpostSPIInterrupt();
    }

}
