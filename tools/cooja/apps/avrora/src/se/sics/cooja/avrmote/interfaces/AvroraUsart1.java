package se.sics.cooja.avrmote.interfaces;

import java.util.ArrayDeque;

import org.apache.log4j.Logger;

import se.sics.cooja.Mote;
import se.sics.cooja.MoteTimeEvent;
import se.sics.cooja.Simulation;
import se.sics.cooja.avrmote.RavenMote;
import se.sics.cooja.dialogs.SerialUI;
import avrora.sim.mcu.AtmelMicrocontroller;
import avrora.sim.platform.Raven;

public class AvroraUsart1 extends SerialUI {
  private static Logger logger = Logger.getLogger(RavenSerial.class);

  private RavenMote mote;
  private avrora.sim.mcu.USART usart;
  private MoteTimeEvent receiveNextByte;

  private ArrayDeque<Byte> rxData = new ArrayDeque<Byte>();

  public AvroraUsart1(Mote ravenMote) {
    mote = (RavenMote) ravenMote;
    receiveNextByte = new MoteTimeEvent(mote, 0) {
      public void execute(long t) {
        if (usart.receiving) {
          /* XXX TODO Postpone how long? */
          mote.getSimulation().scheduleEvent(this, t+Simulation.MILLISECOND);;
          return;
        }
        usart.startReceive();
      }
    };

    Raven raven = mote.getRaven();
    /* this should go into some other piece of code for serial data */
    AtmelMicrocontroller mcu = (AtmelMicrocontroller) raven.getMicrocontroller();
    usart = (avrora.sim.mcu.USART) mcu.getDevice(getUsart());
    if (usart != null) {
      usart.connect(new avrora.sim.mcu.USART.USARTDevice() {
        public avrora.sim.mcu.USART.Frame transmitFrame() {
          if (rxData.isEmpty()) {
            logger.warn("no data for uart");
            return new avrora.sim.mcu.USART.Frame((byte)'?', false, 8);
          }

          Byte data = rxData.pollFirst();
          if (!receiveNextByte.isScheduled() && rxData.size() > 0) {
            mote.getSimulation().scheduleEvent(receiveNextByte, mote.getSimulation().getSimulationTime());
          }

          return new avrora.sim.mcu.USART.Frame(data, false, 8);
        }
        public void receiveFrame(avrora.sim.mcu.USART.Frame frame) {
          dataReceived(frame.value);
        }
      });
    } else {
      System.out.println("*** Warning Raven could not find usart1 interface...");
    }
  }

  public String getUsart() {
    return "usart1";
  }

  public Mote getMote() {
    return mote;
  }

  public void writeArray(byte[] s) {
    for (byte b: s) {
      writeByte(b);
    }
  }

  public void writeByte(byte b) {
    if (usart == null) {
      return;
    }

    rxData.addLast(b);
    if (!receiveNextByte.isScheduled()) {
      mote.getSimulation().scheduleEvent(receiveNextByte, mote.getSimulation().getSimulationTime());
    }
  }

  public void writeString(String s) {
    if (usart == null) {
      return;
    }
    writeArray(s.getBytes());
    writeByte((byte)'\n');
  }
}
