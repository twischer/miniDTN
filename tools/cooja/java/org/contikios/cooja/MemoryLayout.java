package org.contikios.cooja;

import java.nio.ByteOrder;

/**
 *
 * @author Enrico Joerns
 */
public class MemoryLayout {
  
  public ByteOrder order;
  public int addrSize;
  public int intSize;
  
  public MemoryLayout(ByteOrder order, int addrSize, int intSize) {
    this.order = order;
    this.addrSize = addrSize;
    this.intSize = intSize;
  }
  
  public static MemoryLayout getNative() {
    return new MemoryLayout(ByteOrder.nativeOrder(), 4, 4); /** @TODO: 32 bit fixed ? */
  }
  
}
