package se.sics.cooja.avrmote.interfaces;

import se.sics.cooja.Mote;
import se.sics.cooja.avrmote.AvroraMote;
import se.sics.cooja.dialogs.SerialUI;
import se.sics.cooja.interfaces.SerialPort;

import org.apache.log4j.Logger;

public class AvroraSerial0 extends SerialUI implements SerialPort {
  private static Logger logger = Logger.getLogger(AvroraSerial0.class);
  private Mote myMote;
  private avrora.sim.mcu.USART usart;
  private byte charToSend;
  private boolean charSent;
  
  public AvroraSerial0(Mote mote) {
    myMote = mote;
    /* this should go into some other piece of code for serial data */
    avrora.sim.mcu.AtmelMicrocontroller mcu = (avrora.sim.mcu.AtmelMicrocontroller) ((AvroraMote)myMote).CPU.getSimulator().getMicrocontroller();
    usart = (avrora.sim.mcu.USART) mcu.getDevice("usart0");

    if (usart != null) {
      usart.connect(  new avrora.sim.mcu.USART.USARTDevice() {
        public avrora.sim.mcu.USART.Frame transmitFrame() {
          charSent = true;
          return new avrora.sim.mcu.USART.Frame(charToSend, false, 8);
        }
        public void receiveFrame(avrora.sim.mcu.USART.Frame frame) {
          dataReceived(frame.value);
        }
      });
    } else {
     System.out.println("*** Warning Avrora could not get usart0 interface..."); 
    }
  }
  
  public Mote getMote() {
    return myMote;
  }
  
  /* not yet implemented ...*/
  public void writeArray(byte[] s) {
  logger.debug("writearray");
  }

  public void writeByte(byte b) {
  logger.debug("writebyte");
  }

  public void writeString(String s) { try {
    for (int i=0; i<s.length();i++) {
        charToSend = (byte)s.charAt(i);
        // must give the simulation time to receive the character
        //TODO:an actual sending baud rate
        while (usart.receiving) {
            logger.debug("oops"); //hangs if this happens
            Thread.sleep(100);
            break;
        }
        usart.startReceive();
        Thread.sleep(1);
    }
  }catch (Exception e){System.err.println("uarterr: " + e.getMessage());}}
}
