/**
 * Copyright (c) 2007, Regents of the University of California
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
 *
 */
package avrora.sim.radio;

import avrora.sim.Simulator;
import avrora.sim.clock.Synchronizer;
import avrora.sim.mcu.*;
import avrora.sim.output.SimPrinter;
import avrora.sim.state.*;
import avrora.sim.AtmelInterpreter;
import cck.text.StringUtil;
import cck.util.Arithmetic;
import cck.util.Util;
import avrora.sim.FiniteStateMachine;
import avrora.sim.energy.Energy;
import cck.text.*;
//import se.sics.cooja.interfaces.Radio;
//import se.sics.mspsim.util.CCITT_CRC;

import java.util.*;

/**
 * The <code>ATmega128RFA1Radio</code> implements a simulation of the internal
 * radio of the ATmeag128RFA1 chip.
 * Verbose printers for this class include "radio.rfa1"
 *
 * @author Ben L. Titzer
 * @author Rodolfo de Paz
 * @author David A. Kopf
 */
public class ATmega128RFA1Radio implements Radio {
//public class ATmega128RFA1Radio extends Radio {
    boolean DEBUG = false;
    public  byte rf231State = 0;
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
    public static final byte STATE_TRANSITION   = 0x1F;
    //-- Radio commands--------------------------------------------------------
    public static final byte CMD_NOP            = 0x00;
    public static final byte CMD_TX_START       = 0x02;
    public static final byte CMD_FORCE_TRX_OFF  = 0x03; 
    public static final byte CMD_FORCE_PLL_ON   = 0x04;
    public static final byte CMD_RX_ON          = 0x06;
    public static final byte CMD_TRX_OFF        = 0x08;
    public static final byte CMD_TX_ON          = 0x09;
    public static final byte CMD_RX_AACK_ON     = 0x16;
    public static final byte CMD_TX_ARET_ON     = 0x19;

    //-- Register addresses ---------------------------------------------------
    public static final int TRXFBST      = 0x180;
    public static final int TRX_STATE    = 0x142;
    public static final int TRX_STATUS   = 0x141;
    public static final int IRQ_MASK     = 0x14E;
    public static final int IRQ_STATUS   = 0x14F;
    public static final int SHORT_ADDR_0 = 0x160;
    public static final int SHORT_ADDR_1 = 0x161;
    public static final int PAN_ID_0     = 0x162;
    public static final int PAN_ID_1     = 0x163;
    public static final int PHY_CC_CCA   = 0x148;
    public static final int PHY_TX_PWR   = 0x145;
    public static final int TST_RX_LENGTH= 0x17B;

   //         rl.addIOReg("PHY_CC_CCA", 0x128);
   //     rl.addIOReg("PHY_ED_LEVEL", 0x127);
       public static final int PHY_RSSI = 0x146;

   //     rl.addIOReg("PHY_TX_PWR", 0x125);
/*
        rl.addIOReg("TST_RX_LENGTH", 0x15B);

        rl.addIOReg("TST_CTRL_DIGI", 0x156);

        rl.addIOReg("CSMA_BE", 0x14F);
        rl.addIOReg("CSMA_SEED_1", 0x14E);
        rl.addIOReg("CSMA_SEED_0", 0x14D);
        rl.addIOReg("XAH_CTRL_0", 0x14C);
        rl.addIOReg("IEEE_ADDR_7", 0x14B);
        rl.addIOReg("IEEE_ADDR_6", 0x14A);
        rl.addIOReg("IEEE_ADDR_5", 0x149);
        rl.addIOReg("IEEE_ADDR_4", 0x148);
        rl.addIOReg("IEEE_ADDR_3", 0x147);
        rl.addIOReg("IEEE_ADDR_2", 0x146);
        rl.addIOReg("IEEE_ADDR_1", 0x145);
        rl.addIOReg("IEEE_ADDR_0", 0x144);
        rl.addIOReg("PAN_ID_1", 0x143);
        rl.addIOReg("PAN_ID_0", 0x142);
        rl.addIOReg("SHORT_ADDR_1", 0x141);
        rl.addIOReg("SHORT_ADDR_0", 0x140); 

        rl.addIOReg("MAN_ID_1", 0x13F);
        rl.addIOReg("MAN_ID_0", 0x13E);
        rl.addIOReg("VERSION_NUM", 0x13D);
        rl.addIOReg("PART_NUM", 0x13C);
        rl.addIOReg("PLL_DCU", 0x13B);
        rl.addIOReg("PLL_CF", 0x13A);

        rl.addIOReg("FTN_CTRL", 0x138);
        rl.addIOReg("XAH_CTRL_1", 0x137);

        rl.addIOReg("RX_SYN", 0x135);

        rl.addIOReg("XOSC_CTRL", 0x132);
        rl.addIOReg("BATMON", 0x131);
        rl.addIOReg("VREG_CTRL", 0x130);   

        rl.addIOReg("IRQ_STATUS", 0x12F);
        rl.addIOReg("IRQ_MASK", 0x12E);
        rl.addIOReg("ANT_DIV", 0x12D);
        rl.addIOReg("TRX_CTRL_2", 0x12C);
        rl.addIOReg("SFD_VALUE", 0x12B);
        rl.addIOReg("RX_CTRL", 0x12A);
        rl.addIOReg("CCA_THRES", 0x129);
        rl.addIOReg("PHY_CC_CCA", 0x128);
        rl.addIOReg("PHY_ED_LEVEL", 0x127);
        rl.addIOReg("PHY_RSSI", 0x126);
        rl.addIOReg("PHY_TX_PWR", 0x125);
        rl.addIOReg("TRX_CTRL_1", 0x124);
        rl.addIOReg("TRX_CTRL_0", 0x123);
        rl.addIOReg("TRX_STATE", 0x122, "TRAC_STATUS[2:0],TRX_CMD[4:0]");
        rl.addIOReg("TRX_STATUS", 0x121, "CCA_DONE, CCA_STATUS, TST_STATUS, TRX_STATUS[4:0]");

        rl.addIOReg("AES_KEY", 0x11F);
        rl.addIOReg("AES_STATE", 0x11E);
        rl.addIOReg("AES_STATUS", 0x11D);
        rl.addIOReg("AES_CTRL", 0x11C);

        rl.addIOReg("TRXPR", 0x119, "......,SLPTR,TRXRST");
*/
    
    public static final int MAIN = 0x10;
    public static final int MDMCTRL0 = 0x11;
    public static final int MDMCTRL1 = 0x12;
    public static final int RSSI = 0x13;
    public static final int SYNCWORD = 0x14;
    public static final int TXCTRL = 0x15;
    public static final int RXCTRL0 = 0x16;
    public static final int RXCTRL1 = 0x17;
    public static final int FSCTRL = 0x18;
    public static final int SECCTRL0 = 0x19;
    public static final int SECCTRL1 = 0x1a;
    public static final int BATTMON = 0x1b;
    public static final int IOCFG0 = 0x1c;
    public static final int IOCFG1 = 0x1d;
    public static final int MANFIDL = 0x1e;
    public static final int MANFIDH = 0x1f;
    public static final int FSMTC = 0x20;
    public static final int MANAND = 0x21;
    public static final int MANOR = 0x22;
    public static final int AGCCTRL0 = 0x23;
    public static final int AGCTST0 = 0x24;
    public static final int AGCTST1 = 0x25;
    public static final int AGCTST2 = 0x26;
    public static final int FSTST0 = 0x27;
    public static final int FSTST1 = 0x28;
    public static final int FSTST2 = 0x29;
    public static final int FSTST3 = 0x2a;
    public static final int RXBPFTST = 0x2b;
    public static final int FSMSTATE = 0x2c;
    public static final int ADCTST = 0x2d;
    public static final int DACTST = 0x2e;
    public static final int TOPTST = 0x2f;
    public static final int TXFIFO = 0x3e;
    public static final int RXFIFO = 0x3f;

    //-- Command strobes ---------------------------------------------------
    public static final int SNOP = 0x00;
    public static final int SXOSCON = 0x01;
    public static final int STXCAL = 0x02;
    public static final int SRXON = 0x03;
    public static final int STXON = 0x04;
    public static final int STXONCCA = 0x05;
    public static final int SRFOFF = 0x06;
    public static final int SXOSCOFF = 0x07;
    public static final int SFLUSHRX = 0x08;
    public static final int SFLUSHTX = 0x09;
    public static final int SACK = 0x0a;
    public static final int SACKPEND = 0x0b;
    public static final int SRXDEC = 0x0c;
    public static final int STXENC = 0x0d;
    public static final int SAES = 0x0e;

    //-- Other constants --------------------------------------------------
    private static final int NUM_REGISTERS = 0x40;
    private static final int FIFO_SIZE = 128;
    private static final int RAMSECURITYBANK_SIZE = 113;

    private static final int XOSC_START_TIME = 1000;// oscillator start time

    //-- Simulation objects -----------------------------------------------
    protected final AtmelInterpreter interpreter;
    protected final Microcontroller mcu;
    protected final Simulator sim;

    //-- Radio state ------------------------------------------------------
    protected final int xfreq;
 //   protected final char[] registers = new char[NUM_REGISTERS];
 //   protected final byte[] RAMSecurityRegisters = new byte[RAMSECURITYBANK_SIZE];
 //   protected final ByteFIFO txFIFO = new ByteFIFO(FIFO_SIZE);
//    protected final ByteFIFO rxFIFO = new ByteFIFO(FIFO_SIZE);
    protected double BERtotal = 0.0D;
    protected int BERcount = 0;
    protected boolean txactive,rxactive;

    protected Medium medium;
    protected Transmitter transmitter;
    protected Receiver receiver;

    protected final SimPrinter printer;

    // the CC2420 allows reversing the polarity of these outputs.
 //   protected boolean FIFO_active;// selects active high (true) or active low.
 //   protected boolean FIFOP_active;
    protected boolean CCA_active;
    protected boolean SFD_active;

    //Acks variables
    public static final int SENDACK_NONE = 0;
    public static final int SENDACK_NORMAL = 1;
    public static final int SENDACK_PEND = 2;
    protected int SendAck;
    protected boolean AutoAckPend;
    protected boolean lastCRCok;
    protected byte DSN;

    //Address recognition variables
    protected byte[] PANId;
    protected byte[] macPANId;
    protected byte[] ShortAddr;
    protected byte[] macShortAddr;
    protected static final byte[] SHORT_BROADCAST_ADDR = {-1, -1};
    protected byte[] LongAdr;
    protected byte[] IEEEAdr;
    protected static final byte[] LONG_BROADCAST_ADDR = {-1, -1, -1, -1, -1, -1, -1, -1};

    //LUT from cubic spline interpolation with all transmission power values
    protected static final double [] POWER_dBm = {-37.917,-32.984,-28.697,-25,
    -21.837,-19.153,-16.893,-15,-13.42,-12.097,-10.975,-10,-9.1238,-8.3343,
    -7.6277,-7,-6.4442,-5.9408,-5.467,-5,-4.5212,-4.0275,-3.5201,-3,-2.4711,
    -1.9492,-1.4526,-1,-0.6099,-0.3008,-0.0914,0};

    //LUT for max and min correlation values depending on PER
    protected static final int [] Corr_MAX = {110,109,109,109,107,107,107,107,107,
    107,107,107,103,102,102,102,101,101,101,101,99,94,92,94,101,97,98,97,97,97,97,97,
    94,94,94,94,94,94,94,94,94,94,94,94,92,89,89,89,89,89,88,88,88,88,88,86,86,86,
    86,86,86,86,86,86,85,85,85,85,85,85,83,83,83,83,83,83,83,83,79,78,78,78,78,78,
    76,76,76,74,74,74,74,74,74,74,74,74,74,66,65,65,65};
    protected static final int [] Corr_MIN = {95,95,94,91,90,90,89,89,89,88,88,88,82,
    82,82,82,76,76,76,76,76,76,74,74,74,74,74,74,72,72,72,72,72,72,72,72,69,69,69,69,
    69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,67,67,67,67,67,67,65,65,65,65,65,
    65,65,64,64,63,63,63,63,63,63,63,63,63,61,61,61,60,60,60,58,58,56,56,56,55,55,55,
    50,50,50,50,50,50,50};
    protected double Correlation;

    //CC2420Radio energy
    protected static final String[] allModeNames = CC2420Energy.allModeNames();
    protected static final int[][] ttm = FiniteStateMachine.buildSparseTTM(allModeNames.length, 0);
    protected final FiniteStateMachine stateMachine;

    //Clear TxFIFO flag boolean value
    protected boolean ClearFlag;

    /**
     * The constructor for the ATmega128RFA1Radio class creates a new instance internal
     * to the specified microcontroller with the given clock frequency.
     *
     * @param mcu   the microcontroller unit in which this radio resides
     * @param xfreq the clock frequency of this microcontroller
     */
    public ATmega128RFA1Radio(Microcontroller mcu, int xfreq) {
        // set up references to MCU and simulator
        this.mcu = mcu;
        this.sim = mcu.getSimulator();
        this.interpreter = (AtmelInterpreter)sim.getInterpreter();

    //    this.mcuRegs = mcu.getRegisterLayout();
        this.xfreq = xfreq;

        // create a private medium for this radio
        // the simulation may replace this later with a new one.
        setMedium(createMedium(null, null));

        //setup energy recording
        stateMachine = new FiniteStateMachine(sim.getClock(), CC2420Energy.startMode, allModeNames, ttm);
        new Energy("Radio", CC2420Energy.modeAmpere, stateMachine, sim.getEnergyControl());

        // reset all registers
        reset();

        // get debugging channel.
        printer = DEBUG? sim.getPrinter("radio.rfa1") : null;
    }

    /**
     * The <code>getFiniteStateMachine()</code> method gets a reference to the finite state
     * machine that represents this radio's state. For example, there are states corresponding
     * to "on", "off", "transmitting", and "receiving". The state names and numbers will vary
     * by radio implementation. The <code>FiniteStateMachine</code> instance allows the user
     * to instrument the state transitions in order to gather information during simulation.
     * @return a reference to the finite state machine for this radio
     */
    public FiniteStateMachine getFiniteStateMachine() {
        return stateMachine;
    }

    public void reset() {
        if (printer != null) printer.println("RF231: RESET");

  //      for (int cntr = 0; cntr < NUM_REGISTERS; cntr++) {
  //          resetRegister(cntr);
  //      }

        SendAck = SENDACK_NONE;  // reset these internal variables
        AutoAckPend = false;
        lastCRCok = false;
        ClearFlag = false;
        rf231State = STATE_TRX_OFF;

        txactive = rxactive = true;
        transmitter.shutdown();
        receiver.shutdown();
    }

    /**
     * The <code>newCommand()</code> method alters the radio state according to
     * the rules for possible state transitions.
     *
     * @param byte the command
     */
    public void newCommand(byte val) {
        //TODO:check for invalid state transitions
        switch (val) {
            case CMD_NOP:
                if (printer != null) printer.println("RF231: NOP");
                break;
            case CMD_TX_START:
                if (printer != null) printer.println("RF231: TX_START");
                interpreter.writeDataByte(TRX_STATE, (byte) (CMD_TX_START | 0xE0));
                rf231State = STATE_BUSY_TX;
                if (rxactive) receiver.shutdown();
                if (!txactive) transmitter.startup();
                break;
            case CMD_FORCE_TRX_OFF:
                if (printer != null) printer.println("RF231: FORCE_TRX_OFF");
                interpreter.writeDataByte(TRX_STATE, (byte) (CMD_FORCE_TRX_OFF | 0x00));
                rf231State = STATE_TRX_OFF;
                if (txactive) transmitter.shutdown();
                if (rxactive) receiver.shutdown();
                break;
            case CMD_FORCE_PLL_ON:
                if (printer != null) printer.println("RF231: FORCE_PLL_ON");
                interpreter.writeDataByte(TRX_STATE, (byte) (CMD_FORCE_PLL_ON | 0x00));
                rf231State = STATE_PLL_ON;     
                break;
            case CMD_RX_ON:
                if (printer != null) printer.println("RF231: RX_ON");
                rf231State = STATE_RX_ON;
                if (txactive) transmitter.shutdown();
                if (!rxactive) receiver.startup();
                break;
            case CMD_TRX_OFF:
                if (printer != null) printer.println("RF231: TRX_OFF");
                rf231State = STATE_TRX_OFF;
                if (txactive) transmitter.shutdown();
                if (rxactive) receiver.shutdown();
                break;
            case CMD_TX_ON:
                if (printer != null) printer.println("RF231: PLL_ON");
                rf231State = STATE_PLL_ON;
                break;
            case CMD_RX_AACK_ON:
                if (printer != null) printer.println("RF231: RX_AACK_ON");
                rf231State = STATE_RX_AACK_ON;
                interpreter.writeDataByte(TRX_STATE, (byte) (CMD_RX_AACK_ON | 0xE0));
                if (txactive) transmitter.shutdown();
                if (!rxactive) receiver.startup();
                break;
            case CMD_TX_ARET_ON:
                if (printer != null) printer.println("RF231: TX_ARET_ON");
                rf231State = STATE_BUSY_TX_ARET;
                //set status to invalid. This will give a recursive call to the state register handler,
                //which it will detect and ignore.
                interpreter.writeDataByte(TRX_STATE, (byte) (CMD_TX_ARET_ON | 0xE0));
                if (rxactive) receiver.shutdown();
                if (!txactive) transmitter.startup();
                break;
            default:
                if (printer != null) printer.println("RF231: Invalid TRX_CMD, treat as NOP" + val);
                break;
        }
        interpreter.writeDataByte(TRX_STATUS, (byte) (rf231State));
    }
    /**
     * The <code>pinChangeSLP()</code> method indicates a change in the multifunction SLPTR pin,
     * or for the ATmega128rfa1, the SLPTR bit in the TRXPR register.
     *
     * @param val the new pin status ( 0 = low)
     * @return the new radio state
     */
    public void pinChangeSLP(byte val) {
        if (printer != null) printer.println("rfa1: SLP pin change, state "+ rf231State);     
        if (val != 0) {  //pin was raised
            switch (rf231State) {
                //Sleep if off, initiate transmission if tx on
                case STATE_TRX_OFF:
                    stateMachine.transition(0);//change to off state
                    rf231State = STATE_SLEEP;
                    break;
                case STATE_PLL_ON:
                    rf231State = STATE_BUSY_TX;
                    break;
                case STATE_TX_ARET_ON:
                    rf231State = STATE_BUSY_TX_ARET;
                    break;
                default:
                    //dont know what to do here
                    break;
            }
        } else {    //pin was lowered
            switch (rf231State) {
                //Go to idle if sleeping
                case STATE_SLEEP:
                    stateMachine.transition(1);//change to idle state
                    rf231State = STATE_TRX_OFF;
                    break;
                default:
                    //dont know what to do here
                    break;
            }
        }
        interpreter.writeDataByte(TRX_STATUS, (byte) (rf231State));
    }
    /**
     * The <code>resetRegister()</code> method resets the specified register's value
     * to its default.
     *
     * @param addr the address of the register to reset
     */
    void resetRegister(int addr) {
    /*
        char val = 0x0000;
        switch (addr) {
            case MAIN:
                val = 0xf800;
                break;
            case MDMCTRL0:
                val = 0x0ae2;
                break;
            case RSSI:
                val = 0xe080;
                break;
            case SYNCWORD:
                val = 0xa70f;
                break;
            case TXCTRL:
                val = 0xa0ff;
                break;
            case RXCTRL0:
                val = 0x12e5;
                break;
            case RXCTRL1:
                val = 0x0a56;
                break;
            case FSCTRL:
                val = 0x4165;
                break;
            case IOCFG0:
                val = 0x0040;
                break;
        }
        registers[addr] = val;
        */
    }

    /**
     * The <code>computeStatus()</code> method computes the status byte of the radio.
     */
    void computeStatus() {
                 if (printer != null) printer.println("RF231: computestatus");
        // do nothing.
    }

    public Simulator getSimulator() {
        return sim;
    }

    public double getPower() {
        //return power in dBm
        double power=0;
        switch (interpreter.getDataByte(PHY_TX_PWR)&0x0f) {
            case  0:power=  3.0;break;
            case  1:power=  2.8;break;
            case  2:power=  2.3;break;
            case  3:power=  1.8;break;
            case  4:power=  1.3;break;
            case  5:power=  0.7;break;
//          case  6:power=  0.0;break;
            case  7:power= -1.0;break;
            case  8:power= -2.0;break;
            case  9:power= -3.0;break;
            case 10:power= -4.0;break;
            case 11:power= -5.0;break;
            case 12:power= -7.0;break;
            case 13:power= -9.0;break;
            case 14:power=-12.0;break;
            case 15:power=-17.0;break;
        }
//      if (printer != null) printer.println("RF231: getPower returns "+ power + " dBm");
        return power;
    }

    public int getChannel() {
        return interpreter.getDataByte(PHY_CC_CCA) & 0x1F;
    }

    public double getFrequency() {
 //       int channel=interpreter.getDataByte(PHY_CC_CCA) & 0x1F;
        double frequency = 2405 + 5*(getChannel() - 11 );
//      if (printer != null) printer.println("RF231: getFrequency returns "+frequency+" MHz (channel " + channel + ")");
        return frequency;
    }

    public class ClearChannelAssessor implements BooleanView {
        public void setValue(boolean val) {
             if (printer != null) printer.println("RF231: set clearchannel");
             // ignore writes.
        }

        public boolean getValue() {
          printer.println("RF231: getValue");
          return true;
          /*
            if (!receiver.getRssiValid())
                // CC2420 sets the pin inactive when RSSI is invalid!
                return false;
            else
                return receiver.isChannelClear(readRegister(RSSI),readRegister(MDMCTRL0));
            */
        }
    }

    private static final int TX_IN_PREAMBLE = 0;
    private static final int TX_SFD = 1;
    private static final int TX_LENGTH = 3;
    private static final int TX_IN_PACKET = 4;
    private static final int TX_CRC_1 = 5;
    private static final int TX_CRC_2 = 6;
    private static final int TX_END = 7;
    private static final int TX_WAIT = 8;

    protected static final int[] reverse_bits = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff};


    public class Transmitter extends Medium.Transmitter {

        protected int state;
        protected int counter;
        protected int length;
        protected int TRXFBptr;
        protected short crc;
        protected int crct;
        protected short contikicrc;
        protected boolean wasAck;

        public Transmitter(Medium m) {
            super(m, sim.getClock());
        }

        public byte nextByte() {
            byte val = 0;
            switch (state) {
                case TX_IN_PREAMBLE:
                    //always 4 octects of zero
                    if (++counter >= 4) state = TX_SFD;
                    break;
                case TX_SFD:
                    // TODO:read SFD_VALUE
                  //  val = (byte) 0xA7;
                    val = (byte) 0x7A;  //sky compatibility
                    state = TX_LENGTH;
                    break;
                case TX_LENGTH:
                    if (SendAck != SENDACK_NONE) {//ack frame
                        wasAck = true;
                        length = 5;
                    } else {//data frame
                        wasAck = false;
                        // length is the first byte in the SRAM buffer
                        TRXFBptr = 0x180;
                        length =  interpreter.getDataByte(TRXFBptr++);
                    }
                    state = TX_IN_PACKET;
                    counter = 0;
                    crc = 0;
                    crct=0;
                    contikicrc=0;
                    val = (byte) length;
                    break;
                case TX_IN_PACKET:
                    if (SendAck == SENDACK_NONE) {//data frame
                        val = interpreter.getDataByte(TRXFBptr++);
                        counter++;
                    } else {//ack frame
                        switch (counter) {
                            case 0://FCF_low
                                if (SendAck == SENDACK_NORMAL) {
                                    val = 2;
                                    break;
                                } else if (SendAck == SENDACK_PEND) {
                                    val = 0x12;  //  type ACK + frame pending flag
                                    break;
                                }
                            case 1://FCF_hi
                                val = 0;
                                break;
                            case 2://Sequence number
                                val = DSN;
                                SendAck = SENDACK_NONE;
                                break;
                        }
                        counter++;
                    }
                    //Calculate CRC and switch state if necessary
                    //  if (autoCRC.getValue()) {
                    if (true) {
                        // accumulate CRC if enabled.
                        crc = crcAccumulate(crc, (byte) reverse_bits[((int) val) & 0xff]);
                        if (counter >= length - 2) {
                            // switch to CRC state if when 2 bytes remain.
                            state = TX_CRC_1;
                        }
                    } else if (counter >= length) {
                        // AUTOCRC not enabled, switch to packet end mode when done.
                        state = TX_END;
                    }
                    break;
                case TX_CRC_1:
                    state = TX_CRC_2;
                    val = Arithmetic.high(crc);
                    val = (byte) reverse_bits[((int) val) & 0xff];
                    break;
                case TX_CRC_2:
                    val = Arithmetic.low(crc);
                    val = (byte) reverse_bits[((int) val) & 0xff];
                    state = TX_END;
                    break;
            }
            if (printer != null) printer.println("RF231 " + StringUtil.to0xHex(val, 2) + " --------> ");

            // common handling of end of transmission
            if (state == TX_END) {
                rf231State = STATE_PLL_ON;
                interpreter.writeDataByte(TRX_STATUS, (byte) (rf231State));
                //Set the TRAC status bits in the TRAC_STATUS register
                //0 success 1 pending 2 waitforack 3 accessfail 5 noack 7 invalid
                interpreter.writeDataByte(TRX_STATE, (byte) (interpreter.getDataByte(TRX_STATE) & 0x1F));
                //interpreter.setPosted(mcu.getProperties().getInterrupt("TRX24 TX_END"), true);
                interpreter.setPosted(64, true);
                if (printer != null) printer.println("RF231: TX_END interrupt");
                shutdown();
         //       receiver.startup();// auto transition back to receive mode.
                
                // transmitter stays in this state until it is really switched off
       //         state = TX_WAIT;
            }
            return val;
        }

        void startup() {
            if (!txactive) {
                txactive = true;

    //            if (corePd) state = 1; //power down state
    //            else state = 2; // core, e.g. crystal on state
     //          if (!corePd && !biasPd) state = 3; // crystal and bias on state
      //          if (!corePd && !biasPd && !fsPd) state = 4; // crystal, bias and synth. on
       //         if (!corePd && !biasPd && !fsPd && !rxtx && !rxPd) state = 5; // receive state
       //         if (!corePd && !biasPd && !fsPd && rxtx && !txPd) state = PA_POW_reg.getPower() + 6;
                // the PLL lock time is implemented as the leadCycles in Medium!
                //TODO:Read power from RAM
                stateMachine.transition(0x1f+4);//change to Tx(0 dbm) state 7 = -25dBm 11=-15 15=-10 23=-5 27=-3 31=-1 35=0 
                state = TX_IN_PREAMBLE;
                counter = 0;
                beginTransmit(getPower(),getFrequency());
                if (printer != null) printer.println("RF231: TX Startup");
            }
        }

        void shutdown() {
            // note that the stateMachine.transition() is not called here any more!
            if (txactive) {
                txactive = false;
                endTransmit();
                if (printer != null) printer.println("RF231: TX shutdown");
            }
        }
    }


    int crcAccumulatet(int crc, byte val) {
	    crc = ((crc >> 8) & 0xff) | (crc << 8) & 0xffff;
	    crc ^= (val & 0xff);
	    crc ^= (crc & 0xff) >> 4;
	    crc ^= (crc << 12) & 0xffff;
	    crc ^= (crc & 0xff) << 5;
	    crc = crc & 0xffff;
        return crc;
    } 

    short crcAccumulate(short crc, byte val) {
        int i = 8;
        crc = (short) (crc ^ val << 8);
        do {
            if ((crc & 0x8000) != 0) crc = (short) (crc << 1 ^ 0x1021);
            else crc = (short) (crc << 1);
        } while (--i > 0);
        return crc;
    }


    private static final int RECV_SFD_SCAN = 0;
    private static final int RECV_SFD_MATCHED_1 = 1;
    private static final int RECV_SFD_MATCHED_2 = 2;
    private static final int RECV_IN_PACKET = 3;
    private static final int RECV_CRC_1 = 4;
    private static final int RECV_CRC_2 = 5;
    private static final int RECV_END_STATE = 6;
    private static final int RECV_OVERFLOW = 7;
    private static final int RECV_WAIT = 8;

    public class Receiver extends Medium.Receiver {
        protected int state;
        protected int RXFBptr;
        protected int counter;
        protected int length;
        protected short crc;
        protected byte crcLow;

        public Receiver(Medium m) {
            super(m, sim.getClock());
        }

        private void setRssiValid (boolean v){
            //0 <-90dBm, 28 >=-10 dBm
            //Don't change upper three bits
            //TODO:implement random values for bits 5 and 6

 //           int rssi_val = v ? 20 : 0;
//          if (printer!=null) printer.println("RF231: setrssivalid "+ rssi_val);
 //           interpreter.writeDataByte(PHY_RSSI, (byte) (rssi_val | (interpreter.getDataByte(PHY_RSSI) & 0xE0)));
        }
        
        private boolean getRssiValid (){
            if (printer != null) printer.println("RF231: getRssivalid");
            //RF230 RSSI is always valid in non-extended mode
            return true;
        }
        
        public double getCorrelation (){
            int PERindex = (int)(getPER()*100);
            Random random = new Random();
            //get the range, casting to long to avoid overflow problems
            long range = (long)Corr_MAX[PERindex] - (long)Corr_MIN[PERindex] + 1;
            // compute a fraction of the range, 0 <= frac < range
            long fraction = (long)(range * random.nextDouble());
            double corr = fraction + Corr_MIN[PERindex];
            if (printer!=null) printer.println("RF231: returncorr " + corr);
            return corr;
        }

        public void setRSSI (double Prec){
            //RSSI register in units of 3dBm, 0 <-90dBm, 28 >=-10 dBm
            int rssi_val = (((int) Math.rint(Prec) + 90) / 3) +1;
            if (rssi_val < 0) rssi_val = 0;
            if (rssi_val > 28) rssi_val = 28;
            if (printer!=null) printer.println("RF231: setrssi " + rssi_val);
            interpreter.writeDataByte(PHY_RSSI, (byte) (rssi_val | interpreter.getDataByte(PHY_RSSI) & 0xE0));
        }

        public double getRSSI (){
            int rssi_val = interpreter.getDataByte(PHY_RSSI) & 0x1F;
            return -90 + 3*(rssi_val-1);
        }

        public void setBER (double BER){
            if (printer != null) printer.println("RF231: setber");
            BERcount++;
            if (BERcount > 5) {
                BERtotal += BER;
            }
        }
        
        public double getPER (){
            double PER = 0.0D;
            if (BERcount > 5) {
                //compute average BER after SHR
                PER = BERtotal/(BERcount-5);
                //considering i.i.s errors i compute PER
                PER = 1D-Math.pow((1D-PER),(BERcount-5)*8);
            }
            clearBER();
            if (printer!=null) printer.println("RF231: getPER " + PER);
            return PER;
        }
        
        public void clearBER() {
            BERcount = 0;
            BERtotal = 0.0D;
        }

        public byte nextByte(boolean lock, byte b) {
            if (state == RECV_END_STATE) {
                state = RECV_SFD_SCAN; // to prevent loops when calling shutdown/endReceive
                // packet ended before
                if (SendAck != SENDACK_NONE && lastCRCok) {//Send Ack?
                    printer.println("RF231: sendack");
                    shutdown();
                    transmitter.startup();
                } else {
                    if (lock) {
                        // the medium is still locked, so there could be more packets!
                        if (printer != null) printer.println("RF231: still locked");
                        // fire the probes manually
                        if (probeList != null) probeList.fireAfterReceiveEnd(Receiver.this);
                    }
                }
                return b;
            }

              
            if (!lock) {
                if (printer != null) printer.println("RF231 notlocked, state= "+state);
                // the transmission lock has been lost
                switch (state) {
                    case RECV_SFD_MATCHED_2:
                    case RECV_IN_PACKET:
                    case RECV_CRC_1:
                    case RECV_CRC_2:
                         if (printer != null) printer.println("RF231: PLL_UNLOCK Interrupt");
                         //interpreter.setPosted(mcu.getProperties().getInterrupt("TRX24 PLL_UNLOCK"), true);
                         interpreter.setPosted(58, true);
                        //packet lost in middle -> drop frame
                        // fall through
                    
                    case RECV_SFD_MATCHED_1: // packet has just started
                        if (printer != null) printer.println("RF231 packet started");
                        state = RECV_SFD_SCAN;
                        interpreter.setPosted(60, true);
                        break;
                }
                return b;
            }
            
            if (printer != null) printer.println("RF231 <======== " + StringUtil.to0xHex(b, 2));
            switch (state) {
                case RECV_SFD_MATCHED_1:
      //              if (b == (byte) 0xA7) {
                    if (b == (byte) 0x7A) {  //for sky emulation compatibility
                    // check against the second byte of the SYNCWORD register.
                        state = RECV_SFD_MATCHED_2;
                        if (printer != null) printer.println("RF231: RX_START interrupt");
                        //interpreter.setPosted(mcu.getProperties().getInterrupt("TRX24 RX_START"), true);
                        interpreter.setPosted(60, true);
                        break;
                    }
                    // fallthrough if we failed to match the second byte
                    // and try to match the first byte again.
                case RECV_SFD_SCAN:
                    // check against the first byte of the SYNCWORD register.
                    if (b == (byte) 0x00) {
                        state = RECV_SFD_MATCHED_1;
                    } else {
                        state = RECV_SFD_SCAN;
                    }
                    break;

                case RECV_SFD_MATCHED_2:
                    // SFD matched. read the length from the next byte.
                    length = b & 0x7f;
                    if (length == 0) {  // ignore frames with 0 length
                        state = RECV_SFD_SCAN;
                        break;
                    }
                    // Store length in TXT_RX_LENGTH register
                    interpreter.writeDataByte(TST_RX_LENGTH, (byte) length);
                    // Start transferring bytes to RAM buffer
                    RXFBptr = 0x180;
                    counter = 0;
                    state = RECV_IN_PACKET;
                    crc = 0;
                    break;

                case RECV_IN_PACKET:
                    // we are in the body of the packet.
                    counter++;
                    interpreter.writeDataByte(RXFBptr++, b);
                    /*TODO
                    //Address Recognition
                    if (ADR_DECODE.getValue()) {
                        boolean satisfied = matchAddress(b, counter);
                        if (!satisfied) {
                            //reject frame
                            //interpreter.setPosted(mcu.getProperties().getInterrupt("TRX24 RX_END"), true);
                            // wait for end of packet
                            state = RECV_WAIT;
                            break;
                        }
                    } else {
        */
                        // sequence number - save it outside of address recognition since it is needed for SACK/SACKPEND commands as well
                        if (counter == 3 && (interpreter.getDataByte(0x180) & 0x07) != 0 && (interpreter.getDataByte(0x180) & 0x04) != 4) {
                            DSN = b;
                            lastCRCok = false;  // we have a new DSN now. Therefore, we cannot send an ACK for the last frame any more.
                        }
               //     }
                    
                    // no overflow occurred and address ok
                 //   if (autoCRC.getValue()) {
                    if (true) {
                        crc = crcAccumulate(crc, (byte) reverse_bits[((int) b) & 0xff]);
                        if (counter == length - 2) {
                            // transition to receiving the CRC.
                            state = RECV_CRC_1;
                        }
                    } else if (counter == length) {
                        // no AUTOCRC, but reached end of packet.
                        clearBER();  // will otherwise be done by getCorrelation() in state RECV_CRC_2
                        lastCRCok = false;
                        state = RECV_END_STATE;
                    }

                    break;
                case RECV_CRC_1:
                    interpreter.writeDataByte(RXFBptr++, b);
                    crcLow = b;
                    state = RECV_CRC_2;
                    break;
                
                case RECV_CRC_2:
                    state = RECV_END_STATE; 
                    interpreter.writeDataByte(RXFBptr++, b);                    
                    crcLow = (byte)reverse_bits[((int) crcLow) & 0xff]; 
                    b = (byte)reverse_bits[((int) b) & 0xff];
                    short crcResult = (short) Arithmetic.word(b, crcLow);
                    //LQI is written in this position
                    b = (byte) ((byte)getCorrelation() & 0x7f);
                    if (crcResult == crc) {
                        b |= 0x80;  //TODO: should this really increase the LQI
                        lastCRCok = true;
                //       if (printer != null) printer.println("RF231 CRC passed");
                    } else {
                        // TODO: reject frame if not promiscuous mode
                        // reset ACK flags set by the SACK/SACKPEND commands since ACK is only sent when CRC is valid
                        lastCRCok = false;
                        SendAck = SENDACK_NONE;
                        if (printer != null) printer.println("RF231 CRC failed");
                    }
            
                    interpreter.writeDataByte(RXFBptr++, b);
                    if (printer !=null) printer.println("RF231: RX_END interrupt");
                    //interpreter.setPosted(mcu.getProperties().getInterrupt("TRX24 RX_END"), true);
                    interpreter.setPosted(61, true);
/*
                    if (lastCRCok && autoACK.getValue() && (interpreter.getDataByte(0x181) & 0x20) == 0x20) {//autoACK
                        //send ack if we are not receiving ack frame
                        if ((interpreter.getDataByte(0x181) & 0x07) != 2) {
                            printer.println("RF231: send ack");
                            // the type of the ACK only depends on a previous received SACK or SACKPEND
                            SendAck = AutoAckPend ? SENDACK_PEND : SENDACK_NORMAL;
                        }
                    }
   */                 
                    break;
                case RECV_OVERFLOW:
                    // do nothing. we have encountered an overflow.
                    break;
                case RECV_WAIT:
                    // just wait for the end of the packet
                    if (++counter == length) {
                        clearBER();  // will otherwise be done by getCorrelation()
                        state = RECV_SFD_SCAN;
                        SendAck = SENDACK_NONE;  // just in case we received SACK(PEND) in the meantime
                    }
                    break;
            }
            return b;
        }
        
        private boolean matchAddress(byte b, int counter) {
        /*
            if (counter > 1 && (interpreter.getDataByte(0x181) & 0x04) == 4 && RESERVED_FRAME_MODE.getValue()) {
                // no further address decoding is done for reserved frames
                return true;
            }
            switch (counter) {
                case 1://frame type subfield contents an illegal frame type?
                    if ((interpreter.getDataByte(0x181) & 0x04) == 4 && !(RESERVED_FRAME_MODE.getValue()))
                        return false;
                    break;
                case 3://Sequence number
                    if ((interpreter.getDataByte(0x181) & 0x07) != 0 && (interpreter.getDataByte(0x181) & 0x04) != 4) DSN = b;
                    break;
                case 5:
                    PANId[0]=interpreter.getDataByte(0x184);
                    PANId[1]=interpreter.getDataByte(0x185);
                    macPANId[0]=interpreter.getDataByte(PAN_ID_0);
                    macPANId[1]=interpreter.getDataByte(PAN_ID_1);
                    if (((interpreter.getDataByte(0x182) >> 2) & 0x02) != 0) {//DestPANId present?
                        if (!Arrays.equals(PANId, macPANId) && !Arrays.equals(PANId, SHORT_BROADCAST_ADDR))
                            return false;
                    } else
                    if (((interpreter.getDataByte(0x182) >> 2) & 0x03) == 0) {//DestPANId and dest addresses are not present
                        if (((interpreter.getDataByte(0x182) >> 6) & 0x02) != 0) {//SrcPANId present
                            if ((interpreter.getDataByte(0x181) & 0x07) == 0) {//beacon frame: SrcPANid shall match macPANId unless macPANId = 0xffff
                //                if (!Arrays.equals(PANId, macPANId) && !Arrays.equals(macPANId, SHORT_BROADCAST_ADDR) && !BCN_ACCEPT.getValue())
                 //                   return false;
                            } else
                            if (((interpreter.getDataByte(0x181)& 0x07) == 1) || ((interpreter.getDataByte(0x181) & 0x07) == 3)) {//data or mac command
                        //        if (!PAN_COORDINATOR.getValue() || !Arrays.equals(PANId,macPANId)) return false;
                            }
                        }
                    }
                    break;
                case 7://If 32-bit Destination Address exits check if  match
                    ShortAddr[0] = interpreter.getDataByte(0x186);
                    ShortAddr[1] = interpreter.getDataByte(0x187);
                    macShortAddr[0] = interpreter.getDataByte(SHORT_ADDR_0);
                    macShortAddr[1] = interpreter.getDataByte(SHORT_ADDR_1);
                    if (((interpreter.getDataByte(0x186) >> 2) & 0x03) == 2) {
                        if (!Arrays.equals(ShortAddr, macShortAddr) && !Arrays.equals(ShortAddr, SHORT_BROADCAST_ADDR))
                            return false;
                    }
                    break;
                case 12://If 64-bit Destination Address exits check if match
                    if (((rxFIFO.peek(2) >> 2) & 0x03) == 3) {
                        LongAdr = rxFIFO.peekField(8, 16);
                        IEEEAdr = ByteFIFO.copyOfRange(RAMSecurityRegisters, 96, 104);
                        if (!Arrays.equals(LongAdr, IEEEAdr) && !Arrays.equals(LongAdr, LONG_BROADCAST_ADDR))
                            return false;
                    }
                    break;
            }
            */
            return true;
        }

        protected boolean inPacket() {
            return state == RECV_SFD_MATCHED_1
                || state == RECV_SFD_MATCHED_2
                || state == RECV_IN_PACKET
                || state == RECV_CRC_1
                || state == RECV_CRC_2;
        }

        /**
         * The <code>RssiValid</code> class implements a Simulator Event
         * that is fired when the RSSI becomes valid after 8 symbols
         */
        protected class RssiValid implements Simulator.Event {
        
            public void fire() {
                if (activated) {
                    setRssiValid(true);
                }
            }
        }
        protected RssiValid rssiValidEvent = new RssiValid();
        
        void startup() {
            if (!rxactive) {
                rxactive = true;
                stateMachine.transition(3);//change to receive state
                state = RECV_SFD_SCAN;
                clearBER();
                beginReceive(getFrequency());
                clock.insertEvent(rssiValidEvent, 4*cyclesPerByte);  // 8 symbols = 4 bytes
                if (printer!=null) printer.println("RF231: RX startup");
            }
        }

        void shutdown() {
            // note that stateMachine.transition() is not called here any more
            if (rxactive) {
                rxactive = false;
                endReceive();
                setRssiValid(false);
                if (printer != null) printer.println("RF231: RX shutdown");
            }
        }

        void resetOverflow() {
            state = RECV_SFD_SCAN;
        }
    }

    private long toCycles(long us) {
        return us * sim.getClock().getHZ() / 1000000;
    }

    public static Medium createMedium(Synchronizer synch, Medium.Arbitrator arbitrator) {
        return new Medium(synch, arbitrator, 250000, 48, 8, 8 * 128);
    }

    public Medium.Transmitter getTransmitter() {
        return transmitter;
    }

    public Medium.Receiver getReceiver() {
        return receiver;
    }

    public void setMedium(Medium m) {
        medium = m;
        transmitter = new Transmitter(m);
        receiver = new Receiver(m);
    }

    public Medium getMedium() {
        return medium;
    }

}

