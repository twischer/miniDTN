package se.sics.cooja.avrmote.interfaces;

import avrora.sim.mcu.AtmelMicrocontroller;
import avrora.sim.platform.RFA1;
import se.sics.cooja.Mote;
import se.sics.cooja.avrmote.RFA1Mote;
import se.sics.cooja.dialogs.SerialUI;

public class RFA1Serial extends SerialUI {

  RFA1Mote mote;
  
  public RFA1Serial(Mote rfa1Mote) {
    mote = (RFA1Mote) rfa1Mote;
    RFA1 rfa1 = mote.getRFA1();
    /* this should go into some other piece of code for serial data */
    AtmelMicrocontroller mcu = (AtmelMicrocontroller) rfa1.getMicrocontroller();
    avrora.sim.mcu.USART usart = (avrora.sim.mcu.USART) mcu.getDevice("usart0");
    if (usart != null) {
      usart.connect(  new avrora.sim.mcu.USART.USARTDevice() {
        public avrora.sim.mcu.USART.Frame transmitFrame() {
          return null;
          // return new avrora.sim.mcu.USART.Frame((byte)'a', false, 8);
        }
        public void receiveFrame(avrora.sim.mcu.USART.Frame frame) {
          dataReceived(frame.value);
        }
      });
    } else {
     System.out.println("*** Warning ATMega128rfa1 could not find usart1 interface..."); 
    }
  }
  
  public Mote getMote() {
    return mote;
  }
  
  /* not yet implemented ...*/
  public void writeArray(byte[] s) {
  }

  public void writeByte(byte b) {
  }

  public void writeString(String s) {
  }
}
