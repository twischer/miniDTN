package se.sics.cooja.avrmote.interfaces;

import avrora.sim.mcu.AtmelMicrocontroller;
import avrora.sim.platform.Raven;
import se.sics.cooja.Mote;
import se.sics.cooja.avrmote.RavenMote;
import se.sics.cooja.dialogs.SerialUI;

public class RavenSerial extends SerialUI {

  RavenMote mote;
  
  public RavenSerial(Mote ravenMote) {
    mote = (RavenMote) ravenMote;
    Raven raven = mote.getRaven();
    /* this should go into some other piece of code for serial data */
    AtmelMicrocontroller mcu = (AtmelMicrocontroller) raven.getMicrocontroller();
    avrora.sim.mcu.USART usart = (avrora.sim.mcu.USART) mcu.getDevice("usart1");
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
     System.out.println("*** Warning Raven could not find usart1 interface..."); 
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
