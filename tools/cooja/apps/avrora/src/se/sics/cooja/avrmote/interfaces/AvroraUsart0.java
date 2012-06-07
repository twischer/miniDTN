package se.sics.cooja.avrmote.interfaces;

import se.sics.cooja.Mote;

public class AvroraUsart0 extends AvroraUsart1 {
  public AvroraUsart0(Mote mote) {
    super(mote);
  }

  public String getUsart() {
    return "usart0";
  }
}
