package org.contikios.cooja;

import org.contikios.cooja.MemMonitor.MonitorType;

/**
 *
 * @author Enrico Joerns
 */
public abstract class NewAddressMemory {
  
  private final MemoryLayout memLayout;
  
  /**
   * 
   * @param layout 
   */
  public NewAddressMemory(MemoryLayout layout) {
    memLayout = layout;
  }
  
  /**
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
  public abstract byte[] getMemorySegment(long address, int size);

  /**
   * Sets a memory segment.
   *
   * @param address Start address of memory segment
   * @param data Data
   */
  public abstract void setMemorySegment(long address, byte[] data);

  /**
   * Returns the sum of all byte array sizes in this memory.
   * This is not neccessarily the the same as the total memory range,
   * since the entire memory range may not be handled by this memory.
   *
   * @return Total size
   */
  public abstract int getTotalSize();
  
  /**
   * 
   * @param addr
   * @return 
   */
  public byte getByteValueOf(long addr) {
    return getMemorySegment(addr, 1)[0];
  }

  /**
   * 
   * @param addr
   * @return 
   */
  public short getShortValueOf(long addr) {
    return MemoryBuffer.getAddressMemory(memLayout, getMemorySegment(addr, 2)).getShort();
  }
  
  /**
   * 
   * @param addr
   * @return 
   */
  public int getIntValueOf(long addr) {
    return MemoryBuffer.getAddressMemory(memLayout, getMemorySegment(addr, memLayout.intSize)).getInt();
  }
  
  /**
   * 
   * @param addr
   * @return 
   */
  public long getLongValueOf(long addr) {
    return MemoryBuffer.getAddressMemory(memLayout, getMemorySegment(addr, 4)).getLong();
  }
  
  /**
   * 
   * @param addr
   * @return 
   */
  public long getAddrValueOf(long addr) {
    return MemoryBuffer.getAddressMemory(memLayout, getMemorySegment(addr, memLayout.addrSize)).getAddr();
  }
  
  /**
   * 
   * @param addr
   * @param value 
   */
  public void setByteValueOf(long addr, byte value) {
    setMemorySegment(addr, new byte[] {value});
  }
  
  /**
   * 
   * @param addr
   * @param value 
   */
  public void setShortValueOf(long addr, short value) {
    setMemorySegment(addr, MemoryBuffer.getAddressMemory(memLayout, new byte[2]).putShort(value).getBytes());
  }

  /**
   * 
   * @param addr
   * @param value 
   */
  public void setIntValueOf(long addr, int value) {
    setMemorySegment(addr, MemoryBuffer.getAddressMemory(memLayout, new byte[memLayout.intSize]).putInt(value).getBytes());
  }

  /**
   * 
   * @param addr
   * @param value 
   */
  public void setLongValueOf(long addr, long value) {
    setMemorySegment(addr, MemoryBuffer.getAddressMemory(memLayout, new byte[4]).putLong(value).getBytes());
  }

  /**
   * 
   * @param addr
   * @param value 
   */
  public void setAddrValueOf(long addr, long value) {
    setMemorySegment(addr, MemoryBuffer.getAddressMemory(memLayout, new byte[memLayout.addrSize]).putAddr(value).getBytes());
  }
  
//  public enum MemoryEventType {
//
//    READ, WRITE
//  };

  /**
   * Monitor to listen for memory updates.
   */
  public interface AddressMonitor extends MemMonitor {

    public void memoryChanged(NewAddressMemory memory, MemoryEventType type, long address);
  }

  /**
   * Memory access type to listen for.
   */
//  public enum MonitorType {
//
//    R, W, RW
//  };

  /**
   * Adds a AddressMonitor for the specified address region.
   *
   * @param flag Select memory operation(s) to listen for (read, write,
   * read/write)
   * @param address Start address of monitored data region
   * @param size Size of monitored data region
   * @param mm AddressMonitor to add
   * @return true if monitor could be added, false if not
   */
  public abstract boolean addMemoryMonitor(MonitorType flag, long address, int size, AddressMonitor mm);

  /**
   * Removes AddressMonitor assigned to the specified region.
   *
   * @param address Start address of Monitor data region
   * @param size Size of Monitor data region
   * @param mm AddressMonitor to remove
   * @return true if monitor was removed, false if not
   */
  public abstract boolean removeMemoryMonitor(long address, int size, AddressMonitor mm);

}
