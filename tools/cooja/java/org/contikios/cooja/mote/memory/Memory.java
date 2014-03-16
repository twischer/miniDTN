/*
 * Copyright (c) 2014, TU Braunschweig.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
package org.contikios.cooja.mote.memory;

import org.contikios.cooja.mote.memory.MemoryInterface.MoteMemoryException;
import org.contikios.cooja.mote.memory.MemoryLayout.DataType;

/**
 * Represents memory that can be accessed with address and size informations.
 *
 * @author Enrico Jorns
 */
public abstract class Memory {

  private final MemoryLayout memLayout;

  /**
   * Creates new memory for given MemoryLayout.
   *
   * @param layout MemoryLayout
   */
  public Memory(MemoryLayout layout) {
    memLayout = layout;
  }

  /**
   * Returns MemoryLayout associated with this memory
   *
   * @return
   */
  public MemoryLayout getMemoryLayout() {
    return memLayout;
  }

  /**
   * Clears the entire memory.
   */
  public abstract void clearMemory();

  /**
   * Returns a memory segment.
   *
   * @param address Start address of memory segment
   * @param size Size of memory segment
   * @return Memory segment or null if segment not available
   */
  public abstract byte[] getMemorySegment(long address, int size) throws MoteMemoryException;

  /**
   * Sets a memory segment.
   *
   * @param address Start address of memory segment
   * @param data Data
   */
  public abstract void setMemorySegment(long address, byte[] data) throws MoteMemoryException;

  /**
   * Returns the sum of all byte array sizes in this memory.
   * This is not neccessarily the the same as the total memory range,
   * since the entire memory range may not be handled by this memory.
   *
   * @return Total size
   */
  public abstract int getTotalSize();

  // -- Get fixed size types
  /**
   * Read 8 bit integer from address.
   *
   * @param addr Address to read from
   * @return 8 bit value read from address
   */
  public byte getInt8ValueOf(long addr) {
    return getMemorySegment(addr, DataType.INT8.getSize())[0];
  }

  /**
   * Read 16 bit integer from address.
   *
   * @param addr Address to read from
   * @return 16 bit value read from address
   */
  public short getInt16ValueOf(long addr) {
    return MemoryBuffer.wrap(
            memLayout,
            getMemorySegment(addr, DataType.INT16.getSize())).getInt16();
  }

  /**
   * Read 32 bit integer from address.
   *
   * @param addr Address to read from
   * @return 32 bit value read from address
   */
  public int getInt32ValueOf(long addr) {
    return MemoryBuffer.wrap(
            memLayout,
            getMemorySegment(addr, DataType.INT32.getSize())).getInt32();
  }

  /**
   * Read 64 bit integer from address.
   *
   * @param addr Address to read from
   * @return 64 bit value read from address
   */
  public long getInt64ValueOf(long addr) {
    return MemoryBuffer.wrap(
            memLayout,
            getMemorySegment(addr, DataType.INT64.getSize())).getInt64();
  }

  // -- Get compiler-dependent types
  /**
   * Read byte from address.
   *
   * @param addr Address to read from
   * @return byte read from address
   */
  public byte getByteValueOf(long addr) {
    return getMemorySegment(addr, DataType.BYTE.getSize())[0];
  }

  /**
   * Read short from address.
   *
   * @param addr Address to read from
   * @return short read from address
   */
  public short getShortValueOf(long addr) {
    return MemoryBuffer.wrap(memLayout, getMemorySegment(addr, 2)).getShort();
  }

  /**
   * Read integer from address.
   * <p>
   * Note: Size of integer depends on platform type.
   *
   * @param addr Address to read from
   * @return integer read from address
   */
  public int getIntValueOf(long addr) {
    return MemoryBuffer.wrap(memLayout, getMemorySegment(addr, memLayout.intSize)).getInt();
  }

  /**
   * Read long from address.
   *
   * @param addr Address to read from
   * @return long read from address
   */
  public long getLongValueOf(long addr) {
    return MemoryBuffer.wrap(memLayout, getMemorySegment(addr, 4)).getLong();
  }

  /**
   * Read pointer from address.
   * <p>
   * Note: Size of pointer depends on platform type.
   *
   * @param addr Address to read from
   * @return pointer read from address
   */
  public long getAddrValueOf(long addr) {
    return MemoryBuffer.wrap(memLayout, getMemorySegment(addr, memLayout.addrSize)).getAddr();
  }

  // -- Set fixed size types
  /**
   * Write 8 bit integer to address.
   *
   * @param addr Address to write to
   * @param value 8 bit value to write
   */
  public void setInt8ValueOf(long addr, byte value) {
    setMemorySegment(addr, new byte[]{value});
  }

  /**
   * Write 16 bit integer to address.
   *
   * @param addr Address to write to
   * @param value 16 bit value to write
   */
  public void setInt16ValueOf(long addr, short value) {
    setMemorySegment(addr, MemoryBuffer.wrap(
            memLayout,
            new byte[DataType.INT16.getSize()]).putShort(value).getBytes());
  }

  /**
   * Write 32 bit integer to address.
   *
   * @param addr Address to write to
   * @param value 32 bit value to write
   */
  public void setInt32ValueOf(long addr, int value) {
    setMemorySegment(addr, MemoryBuffer.wrap(
            memLayout,
            new byte[DataType.INT32.getSize()]).putInt(value).getBytes());
  }

  /**
   * Write 64 bit integer to address.
   *
   * @param addr Address to write to
   * @param value 64 bit value to write
   */
  public void setInt64ValueOf(long addr, long value) {
    setMemorySegment(addr, MemoryBuffer.wrap(
            memLayout,
            new byte[DataType.INT64.getSize()]).putLong(value).getBytes());
  }

  // -- Set compiler-dependent types
  /**
   * Write byte to address.
   *
   * @param addr Address to write to
   * @param value byte to write
   */
  public void setByteValueOf(long addr, byte value) {
    setMemorySegment(addr, new byte[]{value});
  }

  /**
   * Write short to address.
   *
   * @param addr Address to write to
   * @param value short to write
   */
  public void setShortValueOf(long addr, short value) {
    setMemorySegment(addr, MemoryBuffer.wrap(memLayout, new byte[2]).putShort(value).getBytes());
  }

  /**
   * Write integer to address.
   * <p>
   * Note: Size of integer depends on platform type.
   *
   * @param addr Address to write to
   * @param value integer to write
   */
  public void setIntValueOf(long addr, int value) {
    setMemorySegment(addr, MemoryBuffer.wrap(memLayout, new byte[memLayout.intSize]).putInt(value).getBytes());
  }

  /**
   * Write long to address.
   *
   * @param addr Address to write to
   * @param value long to write
   */
  public void setLongValueOf(long addr, long value) {
    setMemorySegment(addr, MemoryBuffer.wrap(memLayout, new byte[4]).putLong(value).getBytes());
  }

  /**
   * Write pointer to address.
   * <p>
   * Note: Size of pointer depends on platform type.
   *
   * @param addr Address to write to
   * @param value pointer to write
   */
  public void setAddrValueOf(long addr, long value) {
    setMemorySegment(addr, MemoryBuffer.wrap(memLayout, new byte[memLayout.addrSize]).putAddr(value).getBytes());
  }

  /**
   * Monitor to listen for memory updates.
   */
  public interface MemoryMonitor {
    /**
     * Type of memory access occured / to listen for.
     */
    public static enum EventType {

      READ,
      WRITE,
      READWRITE
    }

    /**
     * Invoked if a monitored memory segment is accessed.
     * 
     * @param memory Memory that was accessed
     * @param type Type of access (read/write)
     * @param address Address of access
     */
    void memoryChanged(Memory memory, EventType type, long address);
  }

  /**
   * Adds a MemoryMonitor for the specified address region.
   *
   * @param flag Select memory operation(s) to listen for (read, write,
   * read/write)
   * @param address Start address of monitored data region
   * @param size Size of monitored data region
   * @param mm MemoryMonitor to add
   * @return true if monitor could be added, false if not
   */
  public abstract boolean addMemoryMonitor(MemoryMonitor.EventType flag, long address, int size, MemoryMonitor mm);

  /**
   * Removes MemoryMonitor assigned to the specified region.
   *
   * @param address Start address of Monitor data region
   * @param size Size of Monitor data region
   * @param mm MemoryMonitor to remove
   * @return true if monitor was removed, false if not
   */
  public abstract boolean removeMemoryMonitor(long address, int size, MemoryMonitor mm);

}
