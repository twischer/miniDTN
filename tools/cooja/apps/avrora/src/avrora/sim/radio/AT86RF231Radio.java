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

import java.util.*;

/**
 * The <code>AT86RF231Radio</code> implements a simulation of the Atmel
 * AT86RF231 radio, and can also be used for the AT86RF230.
 * Verbose printers for this class include "radio.rf231"
 *
 * @author David A. Kopf
 */
public class AT86RF231Radio implements Radio {
    private final static boolean DEBUG = false;   //state changes, interrupts
    private final static boolean DEBUGV = false; //spi transfers, pin changes
    
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
    // The rf230 does not use all registers but initializes them anyway.
    public static final int TRX_STATUS   = 0x01;
    public static final int TRX_STATE    = 0x02;
    public static final int TRX_CTRL_0   = 0x03;
    public static final int TRX_CTRL_1   = 0x04;//rf231
    public static final int PHY_TX_PWR   = 0x05;
    public static final int PHY_RSSI     = 0x06;
    public static final int PHY_ED_LEVEL = 0x07;
    public static final int PHY_CC_CCA   = 0x08;
    public static final int CCA_THRESH   = 0x09;
    public static final int RX_CTRL      = 0x0A;//rf231
    public static final int SFD_VALUE    = 0x0B;//rf231
    public static final int TRX_CTRL_2   = 0x0C;//rf231
    public static final int ANT_DIV      = 0x0D;//rf231
    public static final int IRQ_MASK     = 0x0E;
    public static final int IRQ_STATUS   = 0x0F;
    public static final int VREG_CTRL    = 0x10;
    public static final int BATMON       = 0x11;
    public static final int XOSC_CTRL    = 0x12;
    public static final int RX_SYN       = 0x15;//rf231
    public static final int XAH_CTRL_1   = 0x17;//rf231
    public static final int PLL_CF       = 0x1A;
    public static final int PLL_DCU      = 0x1B;
    public static final int PART_NUM     = 0x1C;
    public static final int VERSION_NUM  = 0x1D;
    public static final int MAN_ID_0     = 0x1E;
    public static final int MAN_ID_1     = 0x1F;
    public static final int SHORT_ADDR_0 = 0x20;
    public static final int SHORT_ADDR_1 = 0x21;
    public static final int PAN_ID_0     = 0x22;
    public static final int PAN_ID_1     = 0x23;
    public static final int IEEE_ADDR_0  = 0x24;
    public static final int IEEE_ADDR_1  = 0x25;
    public static final int IEEE_ADDR_2  = 0x26;
    public static final int IEEE_ADDR_3  = 0x27;
    public static final int IEEE_ADDR_4  = 0x28;
    public static final int IEEE_ADDR_5  = 0x29;
    public static final int IEEE_ADDR_6  = 0x2A;
    public static final int IEEE_ADDR_7  = 0x2B;
    public static final int XAH_CTRL_0   = 0x2C;
    public static final int CSMA_SEED_0  = 0x2D;
    public static final int CSMA_SEED_1  = 0x2E;
    public static final int CSMA_BE      = 0x2F;//rf231
    //-- Other constants --------------------------------------------------
    private static final int NUM_REGISTERS = 0x3F;
    private static final int FIFO_SIZE     = 128;
    private static final int XOSC_START_TIME = 1000;// oscillator start time

    //-- Simulation objects -----------------------------------------------
    protected final AtmelInterpreter interpreter;
    protected final Microcontroller mcu;
    protected final Simulator sim;

    //-- Radio state ------------------------------------------------------
    protected final int xfreq;
    protected final byte[] registers = new byte[NUM_REGISTERS];
    protected final ByteFIFO trxFIFO = new ByteFIFO(FIFO_SIZE);
    protected double BERtotal = 0.0D;
    protected int BERcount = 0;
    protected boolean txactive,rxactive;
    protected boolean startingOscillator = false;

    protected Medium medium;
    protected Transmitter transmitter;
    protected Receiver receiver;

    //-- Pins ------------------------------------------------------------
    public final RF231Pin SCLK_pin   = new RF231Pin("SCLK");
    public final RF231Pin MISO_pin   = new RF231Pin("MISO");
    public final RF231Pin MOSI_pin   = new RF231Pin("MOSI");
    public final RF231Pin CS_pin     = new RF231Pin("SELN");
    public final RF231Pin SLPTR_pin  = new RF231Pin("SLP_TR");
    public final RF231Pin RSTN_pin   = new RF231Pin("RSTN");
    public final RF231Output IRQ_pin = new RF231Output("IRQ", new BooleanRegister());

    public final SPIInterface spiInterface = new SPIInterface();

    public int RF231_interrupt = -1;  //platform sets this to the correct interrupt number

    protected final SimPrinter printer;

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
    protected byte[] macPANId = new byte[2];
    protected byte[] ShortAddr;
    protected byte[] macShortAddr = new byte[2];
    protected static final byte[] SHORT_BROADCAST_ADDR = {-1, -1};
    protected byte[] LongAdr;
    protected byte[] IEEEAdr = new byte[8];
    protected static final byte[] LONG_BROADCAST_ADDR = {-1, -1, -1, -1, -1, -1, -1, -1};

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

    //RF231 energy
    protected static final String[] allModeNames = AT86RF231Energy.allModeNames();
    protected static final int[][] ttm = FiniteStateMachine.buildSparseTTM(allModeNames.length, 0);
    protected final FiniteStateMachine stateMachine;

    //Clear TxFIFO flag boolean value
    protected boolean ClearFlag;

    /**
     * The constructor for the AT86RF231Radio class creates a new instance of an
     * AT86RF231 radio connected to a microcontroller via SPI.
     * to the specified microcontroller with the given clock frequency.
     *
     * @param mcu   the microcontroller unit
     * @param xfreq the clock frequency of this microcontroller
     */
    public AT86RF231Radio(Microcontroller mcu, int xfreq) {
        // set up references to MCU and simulator
        this.mcu = mcu;
        this.sim = mcu.getSimulator();
        this.interpreter = (AtmelInterpreter)sim.getInterpreter();

        this.xfreq = xfreq;

        // create a private medium for this radio
        // the simulation may replace this later with a new one.
        setMedium(createMedium(null, null));

        //setup energy recording
        stateMachine = new FiniteStateMachine(sim.getClock(), AT86RF231Energy.startMode, allModeNames, ttm);
        new Energy("Radio", AT86RF231Energy.modeAmpere, stateMachine, sim.getEnergyControl());

        // set all registers to reset values, and clean FIFO
        reset();
        trxFIFO.clear();

        // get debugging channel.
        printer = sim.getPrinter("radio.rf231");
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
        if (DEBUG && printer!= null) printer.println("RF231: RESET");
        for (int cntr = 0; cntr < NUM_REGISTERS; cntr++) {
           resetRegister(cntr);
        }

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
     * The <code>readRegister()</code> method reads the value from the specified register
     * and takes any action(s) that are necessary for the specific register.
     *
     * @param addr the address of the register
     * @return an integer value representing the result of reading the register
     */
    byte readRegister(int addr) {
        byte val = registers[addr];
        if (DEBUGV && printer!=null) printer.println("RF231 " + regName(addr) + " => " + StringUtil.to0xHex(val, 2));
        switch (addr) {
            case IRQ_STATUS:
                //reading IEQ_STATUS clears all interrupts
                registers[addr] = 0;
                break;
            }
        return val;
    }

    /**
     * The <code>writeRegister()</code> method writes the specified value to the specified
     * register, taking any action(s) necessary and activating any command strobes as
     * required.
     *
     * @param addr the address of the register
     * @param val  the value to write to the specified register
     */
    void writeRegister(int addr, byte val) {
        if (DEBUGV && printer!=null) printer.println("RF231 " + regName(addr) + " <= " + StringUtil.to0xHex(val, 2));
        registers[addr] = val;

        switch (addr) {
            case TRX_STATE:
                newCommand((byte) (val & 0x1F));
                break;
        }
    }

    /**
     * The <code>newCommand()</code> method alters the radio state according to
     * the rules for possible state transitions.
     *
     * @param byte the command
     */
    public void newCommand(byte val) {
        switch (val) {
            case CMD_NOP:
                if (DEBUG && printer!=null) printer.println("RF231: NOP");
                break;
            case CMD_TX_START:
                if (DEBUG && printer!=null) printer.println("RF231: TX_START");
                // set TRAC_STATUS bits to INVALID?
                // registers[TRX_STATE] = (byte) (CMD_TX_START | 0xE0);
                rf231State = STATE_BUSY_TX;
                if (rxactive) receiver.shutdown();
                if (!txactive) transmitter.startup();
                break;
            case CMD_FORCE_TRX_OFF:
                if (DEBUG && printer!=null) printer.println("RF231: FORCE_TRX_OFF");
                rf231State = STATE_TRX_OFF;
                if (txactive) transmitter.shutdown();
                if (rxactive) receiver.shutdown();
                break;
            case CMD_FORCE_PLL_ON:
                if (DEBUG && printer!=null) printer.println("RF231: FORCE_PLL_ON");
                rf231State = STATE_PLL_ON;     
                break;
            case CMD_RX_ON:
                if (DEBUG && printer!=null) printer.println("RF231: RX_ON");
                rf231State = STATE_RX_ON;
                if (txactive) transmitter.shutdown();
                if (!rxactive) receiver.startup();
                break;
            case CMD_TRX_OFF:
                if (DEBUG && printer!=null) printer.println("RF231: TRX_OFF");
                rf231State = STATE_TRX_OFF;
                if (txactive) transmitter.shutdown();
                if (rxactive) receiver.shutdown();
                break;
            case CMD_TX_ON:
                if (DEBUG && printer!=null) printer.println("RF231: PLL_ON");
                rf231State = STATE_PLL_ON;
                break;
            case CMD_RX_AACK_ON:
                if (DEBUG && printer!=null) printer.println("RF231: RX_AACK_ON");
           //     if (rf231State == etc.
                rf231State = STATE_RX_AACK_ON;
                // set TRAC_STATUS bits to INVALID
                registers[TRX_STATE] = (byte) (CMD_TX_START | 0xE0);
                if (txactive) transmitter.shutdown();
                if (!rxactive) receiver.startup();
                break;
            case CMD_TX_ARET_ON:
                if (DEBUG && printer!=null) printer.println("RF231: TX_ARET_ON");
                rf231State = STATE_BUSY_TX_ARET;
                // set TRAC_STATUS bits to INVALID
                registers[TRX_STATE] = (byte) (CMD_TX_START | 0xE0);
                if (rxactive) receiver.shutdown();
                if (!txactive) transmitter.startup();
                break;
            default:
                if (DEBUG && printer!=null) printer.println("RF231: Invalid TRX_CMD, treat as NOP" + val);
                break;
        }

        registers[TRX_STATUS] = (byte) (rf231State);
    }
    /**
     * The <code>pinChangeRST()</code> method indicates a change in the RST pin,
     *
     * @param val the new pin status ( 0 = low)
     */
  /*
    public void pinChangeRST(byte val) {
        if (val != 0) {
   //         sleep();
        } else {
        }
    }
*/
    /**
     * The <code>pinChangeSLP()</code> method indicates a change in the multifunction SLPTR pin,
     *
     * @param val the new pin status ( 0 = low)
     * @return the new radio state
     */
    public void pinChangeSLP(byte val) {
        if (DEBUG && printer!=null) printer.println("RF231: SLP pin change, state "+ rf231State);     
        if (val != 0) {  //pin was raised
            switch (rf231State) {
                //off -> sleep
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
                //sleep -> trx_off
                case STATE_SLEEP:
                    stateMachine.transition(1);//change to idle state
                    rf231State = STATE_TRX_OFF;
                    break;
                default:
                    //dont know what to do here
                    break;
            }
        }
        registers[TRX_STATUS] = (byte) (rf231State);
    }
    /**
     * The <code>resetRegister()</code> method resets the specified register's value
     * to its default.
     *
     * @param addr the address of the register to reset
     */
    void resetRegister(int addr) {
        byte val = 0x00;
        switch (addr) {
            case TRX_CTRL_0:
                val = (byte) 0x19;
                break;
            case TRX_CTRL_1:
                val = (byte) 0x20;//rf230 sets to zero
                break;
            case PHY_TX_PWR:
                val = (byte) 0xC0;//rf230 sets to zero
                break;
            case PHY_ED_LEVEL:
                val = (byte) 0xFF;//rf230 sets to zero
                break;
            case PHY_CC_CCA:
                val = (byte) 0x2B;
                break;
            case CCA_THRESH:
                val = (byte) 0xC7;
                break;
            case RX_CTRL:
                val = (byte) 0xB7;//rf230 sets to 0xBC
                break;
            case SFD_VALUE:
                val = (byte) 0xA7;
                break;
            case TRX_CTRL_2:
                val = (byte) 0x00;//rf230 sets to 0x04
                break;
            case ANT_DIV:
                val = (byte) 0x03;//rf230 sets to zero
                break;
            case IRQ_MASK:
                val = (byte) 0x00;//NB: rf230 sets to 0xFF
                break;
            case VREG_CTRL:
                //val = (byte) 0x00;//reads as 4 when access is possible
                break;              
            case BATMON:
                val = (byte) 0x02;//reads as 0x22 when access is possible
                break;
            case XOSC_CTRL:
                val = (byte) 0xF0;
                break;
            case 0x18:
                val = (byte) 0x58;
                break;
            case 0x19:
                val = (byte) 0x55;
                break;
            case PLL_CF:
                val = (byte) 0x57;//rf230 sets to 0x5F
                break;
            case PLL_DCU:
                val = (byte) 0x20;
                break;
            case PART_NUM:
                val = (byte) 0x03;//rf230 sets of 0x02
                break;
            case VERSION_NUM:
                val = (byte) 0x02;
                break;
            case MAN_ID_0:
                val = (byte) 0x1F;
                break;
            case SHORT_ADDR_0:
            case SHORT_ADDR_1:
            case PAN_ID_0:
            case PAN_ID_1:
                val = (byte) 0xFF;//rf230 sets to zeros
                break;
            case XAH_CTRL_0:
                val = (byte) 0x38;
                break;
            case CSMA_SEED_0:
                val = (byte) 0xEA;
                break;
            case CSMA_SEED_1:
                val = (byte) 0x42;//rf230 sets to 0xC2
                break;
            case CSMA_BE:
                val = (byte) 0x53;//rf230 sets to zero
                break;
            case 0x39:
               val = (byte) 0x40;
               break;
        }
        registers[addr] = val;
    }

    protected static final int CMD_R_REG = 0;
    protected static final int CMD_W_REG = 1;
    protected static final int CMD_R_BUF = 2;
    protected static final int CMD_W_BUF = 3;
    protected static final int CMD_R_RAM = 4;
    protected static final int CMD_W_RAM = 5;

    //-- state for managing configuration information
    protected int configCommand;
    protected int configByteCnt;
    protected int configRegAddr;
    protected byte configByteHigh;
    protected int configRAMAddr;
    protected int configRAMBank;

    protected byte receiveConfigByte(byte val) {
        configByteCnt++;
        if (configByteCnt == 1) {
          // the first byte is the command byte
          //  byte status = getStatus();
            if (Arithmetic.getBit(val, 7)) {
                // register bit is set, bit2 determines write or read
                configCommand = Arithmetic.getBit(val,6) ? CMD_W_REG : CMD_R_REG;
                configRegAddr = val & 0x3f;
            }
            else if (Arithmetic.getBit(val,5)) {
                // frame buffer access
                configCommand = Arithmetic.getBit(val,6) ? CMD_W_BUF : CMD_R_BUF;
            } else {
                // SRAM access
                configCommand = Arithmetic.getBit(val,6) ? CMD_W_RAM : CMD_R_RAM;
            }
            return 0;
        } else if (configByteCnt == 2) {
       //    if (!oscStable.getValue() && configCommand != CMD_R_REG && configCommand != CMD_W_REG) {
                // with crystal oscillator disabled only register access is possible
          //      return 0;
         //   }
            // the second byte is the value to read or write, or the starting SRAM address
            switch (configCommand) {
                case CMD_R_REG:
                    return readRegister(configRegAddr);
                case CMD_W_REG:
                    writeRegister(configRegAddr, val);
                    return 0;
                case CMD_R_BUF:
                    return readFIFO(trxFIFO);
                case CMD_W_BUF:
                    return writeFIFO(trxFIFO, val, true);
                case CMD_R_RAM:
                case CMD_W_RAM:
                    configRAMAddr = val & 0x7F;
                    return 0;
            }
        } else {
         //   if (!oscStable.getValue() && configCommand != CMD_R_REG && configCommand != CMD_W_REG) {
                // with crystal oscillator disabled only register access is possible
           //     return 0;
          //  }
            // subsequent bytes are valid for fifo and RAM accesses
            switch (configCommand) {
                case CMD_R_BUF:
                    return readFIFO(trxFIFO);
                case CMD_W_BUF:
                    return writeFIFO(trxFIFO, val, true);
                case CMD_R_RAM:
                    return trxFIFO.peek(configRAMAddr++);
                case CMD_W_RAM:
                     return trxFIFO.poke(configRAMAddr++, val);
            }
        }
        return 0;
    }

    protected byte readFIFO(ByteFIFO fifo) {
        byte val = fifo.remove();
        if (DEBUGV && printer!=null) printer.println("RF231 Read FIFO -> " + StringUtil.toMultirepString(val, 8));
        /*
        if (fifo == rxFIFO) {
            if (fifo.empty()) {
                // reset the FIFO pin when the read FIFO is empty.
                FIFO_pin.level.setValue(!FIFO_active);
            }
            if (fifo.size() < getFIFOThreshold()) {
                // reset FIFOP pin when the number of bytes in the FIFO is below threshold
                FIFOP_pin.level.setValue(!FIFOP_active);
            }
        }
        */
        return val;
    }

    protected byte writeFIFO(ByteFIFO fifo, byte val, boolean st) {
        if (DEBUGV && printer!=null) printer.println("rf231 Write FIFO <= " + StringUtil.toMultirepString(val, 8));
        fifo.add(val);
        return 0;
    }

    public Simulator getSimulator() {
        return sim;
    }

    public double getPower() {
        //return power in dBm
        double power=0;
        switch (registers[PHY_TX_PWR]&0x0f) {
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
        if (DEBUGV && printer!=null) printer.println("RF231: getPower returns "+ power + " dBm");
        return power;
    }

    public int getChannel() {
        if (DEBUGV && printer!=null) printer.println("RF231: getChannel returns "+ (registers[PHY_CC_CCA] & 0x1F));
        return registers[PHY_CC_CCA] & 0x1F;
    }

    public double getFrequency() {
        double frequency = 2405 + 5*(getChannel() - 11 );
        if (DEBUGV && printer!=null) printer.println("RF231: getFrequency returns "+frequency+" MHz (channel " + getChannel() + ")");
        return frequency;
    }

    public class ClearChannelAssessor implements BooleanView {
        public void setValue(boolean val) {
             if (DEBUGV && printer!=null) printer.println("RF231: set clearchannel");
             // ignore writes.
        }

        public boolean getValue() {
          if (DEBUGV && printer!=null) printer.println("RF231: CleanChannelAssesor.getValue");
          return true;
        }
    }

    public class SPIInterface implements SPIDevice {

        public SPI.Frame exchange(SPI.Frame frame) {
            if (DEBUGV && printer!=null) printer.println("RF231 new SPI frame exchange " + StringUtil.toMultirepString(frame.data, 8));
            if (!CS_pin.level && RSTN_pin.level) {
                // configuration requires CS pin to be held low and RSTN pin to be held high
                return SPI.newFrame(receiveConfigByte(frame.data));
            } else {
                return SPI.newFrame((byte) 0);
            }
        }

        public void connect(SPIDevice d) {
            // do nothing.
        }
    }


    private void pinChange_CS(boolean level) {
        // a change in the CS level always restarts a config command.
        configByteCnt = 0;
    }

    private void pinChange_SLPTR(boolean level) {
        if (level) {  //pin was raised
            if (DEBUG && printer!=null) printer.println("RF231 SLPTR pin raised");
            switch (rf231State) {
                //off -> sleep
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
            if (DEBUG && printer!=null) printer.println("RF231 SLPTR pin lowered");
            switch (rf231State) {
                //sleep -> trx_off
                case STATE_SLEEP:
                    stateMachine.transition(1);//change to idle state
                    rf231State = STATE_TRX_OFF;
                    break;
                default:
                    //dont know what to do here
                    break;
            }
        }
        registers[TRX_STATUS] = (byte) (rf231State);
    }

    private void pinChange_RSTN(boolean level) {
        if (!level) {
            // high->low indicates reset
            reset();
            stateMachine.transition(1);//change to power down state
            if (DEBUG && printer!=null) printer.println("RF231 reset by RSTN pin");
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
                   // val = registers[SFD_VALUE];
                    val = (byte) 0x7A;//sky radio compatibility
                  //  val = (byte) 0xA7;
                    state = TX_LENGTH;
                    break;
                case TX_LENGTH:
                    if (SendAck != SENDACK_NONE) {//ack frame
                        wasAck = true;
                        length = 5;
                    } else {//data frame
                        wasAck = false;
                        // length is the first byte in the FIFO buffer
                        length = trxFIFO.remove();
                        if (DEBUGV && printer!=null) printer.println("RF231: Tx frame length " + length);
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
                        val = trxFIFO.remove();
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
                    if ((registers[TRX_CTRL_1] & 0x20) !=0) {  //Test TX_AUTO_CRC_ON bit
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
            if (DEBUGV && printer!=null) printer.println("RF231 " + StringUtil.to0xHex(val, 2) + " --------> ");
            // common handling of end of transmission
            if (state == TX_END) {
                rf231State = STATE_PLL_ON;
                registers[TRX_STATUS] = (byte) (rf231State);
                //Set the TRAC status bits in the TRAC_STATUS register
                //0 success 1 pending 2 waitforack 3 accessfail 5 noack 7 invalid
                //TODO:wait for autoack success
                registers[TRX_STATE] = (byte) (registers[TRX_STATE] & 0x1F);
                
                // set the interrupt bit in the status register if interrupts or polling is enabled
                if ((registers[IRQ_MASK] & 0x08) !=0) { //interrupt is enabled
                    registers[IRQ_STATUS] |= 0x08;
                    if (RF231_interrupt > 0) interpreter.setPosted(RF231_interrupt, true);
                    if (DEBUG && printer!=null) printer.println("RF231: TRX_END interrupt");
                } else if ((registers[TRX_CTRL_1] & 0x02) == 1) { //polling enabled
                    registers[IRQ_STATUS] |= 0x08;
                }

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
                // the PLL lock time is implemented as the leadCycles in Medium
                trxFIFO.clear();
                stateMachine.transition(6 + 15 - (registers[PHY_TX_PWR]&0x0f));//change to Tx(power) state
                state = TX_IN_PREAMBLE;
                counter = 0;
                beginTransmit(getPower(),getFrequency());
                if (DEBUG && printer!=null) printer.println("RF231: TX Startup");
            }
        }

        void shutdown() {
            // note that the stateMachine.transition() is not called here any more!
            if (txactive) {
                txactive = false;
                endTransmit();
                if (DEBUG && printer!=null) printer.println("RF231: TX shutdown");
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
            int rssi_val = v ? 20 : 0;
            if (DEBUG && printer!=null) printer.println("RF231: setrssivalid "+ rssi_val);
            registers[PHY_RSSI] = (byte) (rssi_val | (registers[PHY_RSSI] & 0xE0));
        }
        
        private boolean getRssiValid (){
            if (DEBUGV && printer!=null) printer.println("RF231: getRssivalid");
            //RF231 RSSI is always valid in non-extended mode
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
            if (DEBUG && printer!=null) printer.println("RF231: returncorr " + corr);
            return corr;
        }

        public void setRSSI (double Prec){
            //RSSI register in units of 3dBm, 0 <-90dBm, 28 >=-10 dBm
            int rssi_val = (((int) Math.rint(Prec) + 90) / 3) +1;
            if (rssi_val < 0) rssi_val = 0;
            if (rssi_val > 28) rssi_val = 28;
            if (DEBUG && printer!=null) printer.println("RF231: setrssi " + rssi_val);
            registers[PHY_RSSI] = (byte) (rssi_val | (registers[PHY_RSSI] & 0xE0));
        }

        public double getRSSI (){
            int rssi_val = registers[PHY_RSSI] & 0x1F;
            return -90 + 3*(rssi_val-1);
        }

        public void setBER (double BER){
            if (DEBUG && printer!=null) printer.println("RF231: setBER");
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
            if (DEBUG && printer!=null) printer.println("RF231: getPER " + PER);
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
                    if (DEBUG && printer!=null) printer.println("RF231: sendack");
                    shutdown();
                    transmitter.startup();
                } else {
                    if (lock) {
                        // the medium is still locked, so there could be more packets!
                        if (DEBUG && printer!=null) printer.println("RF231: still locked");
                        // fire the probes manually
                        if (probeList != null) probeList.fireAfterReceiveEnd(Receiver.this);
                    }
                }
                return b;
            }

              
            if (!lock) {
                if (DEBUG && printer!=null) printer.println("RF231 notlock, state= "+state);
                // the transmission lock has been lost
                switch (state) {
                    case RECV_SFD_MATCHED_2:
                    case RECV_IN_PACKET:
                    case RECV_CRC_1:
                    case RECV_CRC_2:
                        // set the interrupt bit in the status register if interrupts or polling is enabled
                        if ((registers[IRQ_MASK] & 0x02) !=0) { //interrupt is enabled
                            registers[IRQ_STATUS] |= 0x02;
                            if (RF231_interrupt > 0) interpreter.setPosted(RF231_interrupt, true);
                            if (DEBUG && printer!=null) printer.println("RF231: PLL_UNLOCK interrupt");
                        } else if ((registers[TRX_CTRL_1] & 0x02) == 1) { //polling enabled
                            registers[IRQ_STATUS] |= 0x02;
                        }

                        //packet lost in middle -> drop frame
                        // fall through                
                    case RECV_SFD_MATCHED_1: // packet has just started
                        if (DEBUG && printer!=null) printer.println("RF231 packet started");
                        state = RECV_SFD_SCAN;
                        // set the interrupt bit in the status register if interrupts or polling is enabled
                        if ((registers[IRQ_MASK] & 0x04) !=0) { //interrupt is enabled
                            registers[IRQ_STATUS] |= 0x04;
                            if (RF231_interrupt > 0) interpreter.setPosted(RF231_interrupt, true);
                            if (DEBUG && printer!=null) printer.println("RF231: RX_START interrupt");
                        } else if ((registers[TRX_CTRL_1] & 0x02) == 1) { //polling enabled
                            registers[IRQ_STATUS] |= 0x04;
                        }
                        break;
                }
                return b;
            }
            
            if (DEBUGV && printer!=null) printer.println("RF231 <======== " + StringUtil.to0xHex(b, 2));
            switch (state) {
                case RECV_SFD_MATCHED_1:
                   // if (b == (byte) 0xA7) {
                    if (b == (byte) 0x7A) {  //sky compatibility
                    // check against the second byte of the SYNCWORD register.
                        state = RECV_SFD_MATCHED_2;
                        // set the interrupt bit in the status register if interrupts or polling is enabled
                        if ((registers[IRQ_MASK] & 0x04) !=0) { //interrupt is enabled
                            registers[IRQ_STATUS] |= 0x04;
                            if (RF231_interrupt > 0) interpreter.setPosted(RF231_interrupt, true);
                            if (DEBUG && printer!=null) printer.println("RF231: RX_START interrupt");
                        } else if ((registers[TRX_CTRL_1] & 0x02) == 1) { //polling enabled
                            registers[IRQ_STATUS] |= 0x04;
                        }
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
                    
                    // Start transferring bytes to FIFO
                    trxFIFO.add((byte) length);
                    counter = 0;
                    state = RECV_IN_PACKET;
                    crc = 0;
                    break;

                case RECV_IN_PACKET:
                    // we are in the body of the packet.
                    counter++;
                    trxFIFO.add(b);

                    //Address Recognition and sequence number
                    if (counter <= 13) {
                        boolean satisfied = matchAddress(b, counter);
                        // address match enabled only in RA_AACK mode TODO: should be BUSY_RX_AACK
                        if (rf231State == STATE_RX_AACK_ON) {
                            // is AACK_I_AM_COORD set?
                            if ((registers[CSMA_SEED_1] & 0x04) == 0) {
                                if (!satisfied) {
                                    //reject frame
                                    if (DEBUG && (printer!=null)) printer.println("Dropped, no address match");   
                                    // wait for end of packet
                                    state = RECV_WAIT;
                                    break;
                                } else if (counter == 13) { //TODO: use the correct number based on short/long address
                                    // set the interrupt bit in the status register if interrupts or polling is enabled
                                    if ((registers[IRQ_MASK] & 0x20) !=0) { //interrupt is enabled
                                        registers[IRQ_STATUS] |= 0x20;
                                        if (RF231_interrupt > 0) interpreter.setPosted(RF231_interrupt, true);
                                        if (DEBUG && printer!=null) printer.println("RF231: MASK_AMI interrupt");
                                    } else if ((registers[TRX_CTRL_1] & 0x02) == 1) { //polling enabled
                                        registers[IRQ_STATUS] |= 0x20;
                                    }
                                }
                            }
                        }
                    }
                    
                    // no overflow occurred and address ok
                 //   if (autoCRC.getValue()) {
                    if (true) { //todo: check crc request bits if any
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
                    trxFIFO.add(b);
                    crcLow = b;
                    state = RECV_CRC_2;
                    break;
                
                case RECV_CRC_2:
                    state = RECV_END_STATE; 
                    trxFIFO.add(b);                
                    char crcResult = (char) Arithmetic.word(crcLow, b);

                    //LQI is written in this position
                    b = (byte) ((byte)getCorrelation() & 0x7f);
                 //   if (crcResult == crc) {
                                        if (true) {
                        b |= 0x80;
                        lastCRCok = true;
                        if (DEBUG && printer!=null) printer.println("RF231: CRC passed");
                    }
                    else {
                        // Frame is not rejected if CRC is invalid
                        // reset ACK flags set by the SACK/SACKPEND commands since ACK is only sent when CRC is valid
                        lastCRCok = false;
                        SendAck = SENDACK_NONE;
                        if (DEBUG && printer!=null) printer.println("RF231: CRC failed");
                    }
            
                    trxFIFO.add(b);

                    // set the interrupt bit in the status register if interrupts or polling is enabled
                    if ((registers[IRQ_MASK] & 0x08) !=0) { //interrupt is enabled
                        registers[IRQ_STATUS] |= 0x08;
                        if (RF231_interrupt > 0) interpreter.setPosted(RF231_interrupt, true);
                        if (DEBUG && printer!=null) printer.println("RF231: TRX_END interrupt");
                    } else if ((registers[TRX_CTRL_1] & 0x02) == 1) { //polling enabled
                         registers[IRQ_STATUS] |= 0x08;
                    }

                    if (lastCRCok && (rf231State == STATE_RX_AACK_ON) && (trxFIFO.peek(1) & 0x20) == 0x20) {//TODO:should be BUSY_RX_AACK
                        //send ack if we are not receiving ack frame
                        // but not if AACK_DIS_ACK is set
                        //TODO: handle AACK_FVN_MODE and AACK_SET_PC
                        if ((registers[CSMA_SEED_1] & 0x10) == 0) {
                            if ((trxFIFO.peek(1) & 0x07) != 2) {
                                // the type of the ACK only depends on a previous received SACK or SACKPEND
                                SendAck = AutoAckPend ? SENDACK_PEND : SENDACK_NORMAL;
                            }
                        }
                    }              
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
            //if (counter > 1 && (trxFIFO.peek(1) & 0x04) == 4 && RESERVED_FRAME_MODE.getValue()) {
            if (counter > 1 && (trxFIFO.peek(1) & 0x04) == 4) {
                // no further address decoding is done for reserved frames
                return true;
            }
            switch (counter) {
                case 1://frame type subfield contents an illegal frame type?
                   // if ((trxFIFO.peek(1) & 0x04) == 4 && !(RESERVED_FRAME_MODE.getValue()))
                    if ((trxFIFO.peek(1) & 0x04) == 4)
                        return false;
                    break;
                case 3://Sequence number
                    if ((trxFIFO.peek(1) & 0x07) != 0 && (trxFIFO.peek(1) & 0x04) != 4) {
                        DSN = b;
                        lastCRCok = false;  // we have a new DSN now. Therefore, we cannot send an ACK for the last frame any more.
                    }
                    break;
                case 5:
                    PANId = trxFIFO.peekField(4, 6);
                    macPANId[0] = registers[PAN_ID_0];
                    macPANId[1] = registers[PAN_ID_1];
                    if (((trxFIFO.peek(2) >> 2) & 0x02) != 0) {//DestPANId present?
                        if (!Arrays.equals(PANId, macPANId) && !Arrays.equals(PANId, SHORT_BROADCAST_ADDR))
                            return false;
                    } else
                    if (((trxFIFO.peek(2) >> 2) & 0x03) == 0) {//DestPANId and dest addresses are not present
                        if (((trxFIFO.peek(2) >> 6) & 0x02) != 0) {//SrcPANId present
                            if ((trxFIFO.peek(1) & 0x07) == 0) {//beacon frame: SrcPANid shall match macPANId unless macPANId = 0xffff
                              //  if (!Arrays.equals(PANId, macPANId) && !Arrays.equals(macPANId, SHORT_BROADCAST_ADDR) && !BCN_ACCEPT.getValue())
                                if (!Arrays.equals(PANId, macPANId) && !Arrays.equals(macPANId, SHORT_BROADCAST_ADDR))
                                    return false;
                            } else
                            if (((trxFIFO.peek(1) & 0x07) == 1) || ((trxFIFO.peek(1) & 0x07) == 3)) {//data or mac command
                                if (rf231State == STATE_RX_AACK_ON) { //TODO: should be BUSY_AACK
                                    // is AACK_I_AM_COORD set?
                                    if ((registers[CSMA_SEED_1] & 0x04) == 0) return false;
                                }
                                if (!(Arrays.equals(PANId,macPANId))) return false;
                            }
                        }
                    }
                    break;
                case 7://If 32-bit Destination Address exits check if  match
                    if (((trxFIFO.peek(2) >> 2) & 0x03) == 2) {
                        ShortAddr = trxFIFO.peekField(6, 8);
                        macShortAddr[0] = registers[SHORT_ADDR_0];
                        macShortAddr[1] = registers[SHORT_ADDR_1];
                       // printer.println("shortadr " + ShortAddr[0]+ShortAddr[1]);
                      //  printer.println("macshortaddr " + macShortAddr[0]+" "+macShortAddr[1]);
                        if (!Arrays.equals(ShortAddr, macShortAddr) && !Arrays.equals(ShortAddr, SHORT_BROADCAST_ADDR))
                            return false;
                    }
                    break;
             // case 12://If 64-bit Destination Address exits check if match
                //dak bumped this up a byte, works with sky. The SFD change caused this?
                case 13://If 64-bit Destination Address exits check if match
                    if (((trxFIFO.peek(2) >> 2) & 0x03) == 3) {                   
                     // LongAdr = rxFIFO.peekField(8, 16);
                        LongAdr = trxFIFO.peekField(6, 14);//dak
                        IEEEAdr[0] = registers[IEEE_ADDR_0];
                        IEEEAdr[1] = registers[IEEE_ADDR_1];
                        IEEEAdr[2] = registers[IEEE_ADDR_2];
                        IEEEAdr[3] = registers[IEEE_ADDR_3];
                        IEEEAdr[4] = registers[IEEE_ADDR_4];
                        IEEEAdr[5] = registers[IEEE_ADDR_5];
                        IEEEAdr[6] = registers[IEEE_ADDR_6];
                        IEEEAdr[7] = registers[IEEE_ADDR_7];
                        if (!Arrays.equals(LongAdr, IEEEAdr) && !Arrays.equals(LongAdr, LONG_BROADCAST_ADDR)) {
                            if (printer != null) {
                              printer.println(" longadr " + LongAdr[0]+LongAdr[1]+LongAdr[2]+LongAdr[3]+LongAdr[4]+LongAdr[5]+LongAdr[6]+LongAdr[7]);
                              printer.println(" IEEEAdr " + IEEEAdr[0]+IEEEAdr[1]+IEEEAdr[2]+IEEEAdr[3]+IEEEAdr[4]+IEEEAdr[5]+IEEEAdr[6]+IEEEAdr[7]);
                            }
                            return false;
                        }
                    }
                    break;
            }
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
                trxFIFO.clear();
                stateMachine.transition(3);//change to receive state TODO:low sensitivity state
                state = RECV_SFD_SCAN;
                clearBER();
                beginReceive(getFrequency());
                clock.insertEvent(rssiValidEvent, 4*cyclesPerByte);  // 8 symbols = 4 bytes
                if (DEBUGV && printer!=null) printer.println("RF231: RX startup");
            }
        }

        void shutdown() {
            // note that stateMachine.transition() is not called here any more
            if (rxactive) {
                rxactive = false;
                endReceive();
                setRssiValid(false);
                if (DEBUGV && printer!=null) printer.println("RF231: RX shutdown");
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

    /**
     * The <code>RF231Pin</code>() class models pins that are inputs and outputs to the RF231 chip.
     */
    public class RF231Pin implements Microcontroller.Pin.Input, Microcontroller.Pin.Output {
        protected final String name;
        protected boolean level;

        public RF231Pin(String n) {
            name = n;
        }

        public void write(boolean level) {
            if (this.level != level) {
                // level changed
                this.level = level;
                if (this == CS_pin) pinChange_CS(level);
                else if (this == RSTN_pin) pinChange_RSTN(level);
                else if (this == SLPTR_pin) pinChange_SLPTR(level);
                if (DEBUGV && printer!=null) printer.println("RF231 Write pin " + name + " -> " + level);
            }
        }

        public boolean read() {
            if (DEBUGV && printer!=null) printer.println("RF231 Read pin " + name + " -> " + level);
            return level;
        }
    }

    public class RF231Output implements Microcontroller.Pin.Input {

        protected BooleanView level;
        protected final String name;

        public RF231Output(String n, BooleanView lvl) {
            name = n;
            level = lvl;
        }

        public boolean read() {
            boolean val = level.getValue();
            if (DEBUG && printer!=null) printer.println("RF231 Read (output) pin " + name + " -> " + val);
            return val;
        }
    }

    public static String regName(int reg) {
        switch (reg) {
            case TRX_STATUS:
                return "TRX_STATUS  ";
            case TRX_STATE:
                return "TRX_STATE   ";
            case TRX_CTRL_0:
                return "TRX_CTRL_0  ";
            case TRX_CTRL_1:
                return "TRX_CTRL_1  ";
            case PHY_TX_PWR:
                return "PHY_TX_PWR  ";
            case PHY_RSSI:
                return "PHY_RSSI    ";
            case PHY_ED_LEVEL:
                return "PHY_ED_LEVEL";
            case PHY_CC_CCA:
                return "PHY_CC_CCA  ";
            case CCA_THRESH:
                return "CCA_THRESH  ";
            case RX_CTRL:
                return "RX_CTRL     ";
            case SFD_VALUE:
                return "SFD_VALUE   ";
            case TRX_CTRL_2:
                return "TRX_CTRL_2  ";
            case ANT_DIV:
                return "ANT_DIV     ";
            case IRQ_MASK:
                return "IRQ_MASK    ";
            case IRQ_STATUS:
                return "IRQ_STATUS  ";
            case VREG_CTRL:
                return "VREG_CTRL   ";
            case BATMON:
                return "BATMON      ";
            case XOSC_CTRL:
                return "XOSC_CTRL   ";
            case RX_SYN:
                return "RX_SYN      ";
            case XAH_CTRL_1:
                return "XAH_CTRL_1  ";
            case PLL_CF:
                return "PLL_CF      ";
            case PLL_DCU:
                return "PLL_DCU     ";
            case PART_NUM:
                return "PART_NUM    ";
            case VERSION_NUM:
                return "VERSION_NUM ";
            case MAN_ID_0:
                return "MAN_ID_0    ";
            case MAN_ID_1:
                return "MAN_ID_1    ";
            case SHORT_ADDR_0:
                return "SHORT_ADDR_0";
            case SHORT_ADDR_1:
                return "SHORT_ADDR_1";
            case PAN_ID_0:
                return "PAN_ID_0    ";
            case PAN_ID_1:
                return "PAN_ID_1    ";
            case IEEE_ADDR_0:
                return "IEEE_ADDR_0 ";
            case IEEE_ADDR_1:
                return "IEEE_ADDR_1 ";
            case IEEE_ADDR_2:
                return "IEEE_ADDR_2 ";
            case IEEE_ADDR_3:
                return "IEEE_ADDR_3 ";
            case IEEE_ADDR_4:
                return "IEEE_ADDR_4 ";
            case IEEE_ADDR_5:
                return "IEEE_ADDR_5 ";
            case IEEE_ADDR_6:
                return "IEEE_ADDR_6 ";
            case IEEE_ADDR_7:
                return "IEEE_ADDR_7 ";
            case XAH_CTRL_0:
                return "XAH_CTRL_0  ";
            case CSMA_SEED_0:
                return "CSMA_SEED_0 ";
            case CSMA_SEED_1:
                return "CSMA_SEED_1 ";
            case CSMA_BE:
                return "CSMA_BE     ";
            default:
                return StringUtil.to0xHex(reg, 2) + "    ";
        }
    }
/*
    public static String strobeName(int strobe) {
        switch (strobe) {
            case SNOP:
                return "SNOP    ";
            case SXOSCON:
                return "SXOSCON ";
            case STXCAL:
                return "STXCAL  ";
            case SRXON:
                return "SRXON   ";
            case STXON:
                return "STXON   ";
            case STXONCCA:
                return "STXONCCA";
            case SRFOFF:
                return "SRFOFF  ";
            case SXOSCOFF:
                return "SXOSCOFF";
            case SFLUSHRX:
                return "SFLUSHRX";
            case SFLUSHTX:
                return "SFLUSHTX";
            case SACK:
                return "SACK    ";
            case SACKPEND:
                return "SACKPEND";
            case SRXDEC:
                return "SRXDEC  ";
            case STXENC:
                return "STXENC  ";
            case SAES:
                return "SAES    ";
            default:
                return StringUtil.to0xHex(strobe, 2) + "    ";
        }
    }
*/
}


