package org.contikios.cooja;

import java.nio.ByteBuffer;

/**
 * Basic routines for memory access with multi-arch support.
 *
 * Handles endianess, integer size and address size.
 *
 * @author Enrico Joerns
 */
public class MemoryBuffer {

  private final MemoryLayout memLayout;
  private final ByteBuffer bbuf;

  private MemoryBuffer(MemoryLayout memLayout, ByteBuffer buffer) {
    this.memLayout = memLayout;
    this.bbuf = buffer;
  }

  /**
   * Returns MemoryBuffer for given MemoryLayout.
   *
   * @param layout
   * @param array
   * @return
   */
  public static MemoryBuffer getAddressMemory(MemoryLayout layout, byte[] array) {
    ByteBuffer b = ByteBuffer.wrap(array);
    b.order(layout.order); // preset endianess
    return new MemoryBuffer(layout, b);
  }

  /**
   *
   * @return
   */
  public byte[] getBytes() {
    if (bbuf.hasArray()) {
      return bbuf.array();
    }
    else {
      return null;
    }
  }

  /**
   *
   * @return
   */
  public byte getByte() {
    return bbuf.get();
  }

  /**
   *
   * @return
   */
  public short getShort() {
    return bbuf.getShort();
  }

  /**
   *
   * @return
   */
  public int getInt() {
    switch (memLayout.intSize) {
      case 2:
        return bbuf.getShort();
      case 4:
      default:
        return bbuf.getInt();
    }
  }

  /**
   *
   * @return
   */
  public long getLong() {
    return bbuf.getLong();
  }

  /**
   * Read stored address.
   *
   * @return pointer
   */
  public long getAddr() {
    switch (memLayout.addrSize) {
      case 2:
        return bbuf.getShort();
      case 4:
        return bbuf.getInt();
      case 8:
        return bbuf.getLong();
      default:
        return bbuf.getInt();
    }
  }

  /**
   *
   * @param value
   * @return This MemoryBuffer
   */
  public MemoryBuffer putByte(byte value) {
    bbuf.put(value);
    return this;
  }

  /**
   *
   * @param value
   * @return This MemoryBuffer
   */
  public MemoryBuffer putShort(short value) {
    bbuf.putShort(value);
    return this;
  }

  /**
   *
   * @param value
   * @return This MemoryBuffer
   */
  public MemoryBuffer putInt(int value) {
    switch (memLayout.intSize) {
      case 2:
        /**
         * @TODO: check for size?
         */
        bbuf.putShort((short) value);
        break;
      case 4:
      default:
        bbuf.putInt(value);
        break;
    }
    return this;
  }

  /**
   *
   * @param value
   * @return This MemoryBuffer
   */
  public MemoryBuffer putLong(long value) {
    bbuf.putLong(value);
    return this;
  }

  /**
   *
   * @param addr
   * @return This MemoryBuffer
   */
  public MemoryBuffer putAddr(long addr) {
    /**
     * @TODO: check for size?
     */
    switch (memLayout.addrSize) {
      case 2:
        bbuf.putShort((short) addr);
        break;
      case 4:
        bbuf.putInt((int) addr);
        break;
      case 8:
        bbuf.putLong(addr);
        break;
      default:
        bbuf.putInt((int) addr);
        break;
    }
    return this;
  }
}
