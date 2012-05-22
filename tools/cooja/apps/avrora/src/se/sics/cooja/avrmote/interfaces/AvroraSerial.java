package se.sics.cooja.avrmote.interfaces;

import se.sics.cooja.Mote;
import se.sics.cooja.avrmote.AvroraMote;
import se.sics.cooja.dialogs.SerialUI;
import se.sics.cooja.interfaces.SerialPort;

import org.apache.log4j.Logger;

public class AvroraSerial extends SerialUI implements SerialPort {
  private static Logger logger = Logger.getLogger(AvroraSerial.class);
  private Mote myMote;
  private avrora.sim.mcu.USART usart;
  private byte charToSend;
  private boolean charSent;
  
  public AvroraSerial(Mote mote) {
    myMote = mote;
    logger.debug("avrora serial constructor");
    /* this should go into some other piece of code for serial data */
    avrora.sim.mcu.AtmelMicrocontroller mcu = (avrora.sim.mcu.AtmelMicrocontroller) ((AvroraMote)myMote).CPU.getSimulator().getMicrocontroller();
    usart = (avrora.sim.mcu.USART) mcu.getDevice("usart1");
    if (usart == null) usart = (avrora.sim.mcu.USART) mcu.getDevice("usart0");

    if (usart != null) {
      usart.connect(  new avrora.sim.mcu.USART.USARTDevice() {
        public avrora.sim.mcu.USART.Frame transmitFrame() {
          charSent = true;
                  logger.debug("transmit " + charToSend);
          return new avrora.sim.mcu.USART.Frame(charToSend, false, 8);
        }
        public void receiveFrame(avrora.sim.mcu.USART.Frame frame) {
        logger.debug("receive " + frame.value);
          dataReceived(frame.value);
        }
      });
    } else {
     System.out.println("*** Warning Avrora could not get usart1 or usart0 interface..."); 
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

  public void writeString(String s) {
    for (int i=0; i<s.length();i++) {
        // must give the simulation time to receive the character
        //TODO:an actual sending baud rate
        charSent = false;
        charToSend = (byte)s.charAt(i);
        usart.startReceive();
        try{Thread.sleep(10);}catch (Exception ex){System.err.println("uarterr: " + ex.getMessage());};
        while(!charSent) {
            try{Thread.sleep(10);}catch (Exception ex){System.err.println("uarterr: " + ex.getMessage());};
        }
    }
  }
}
