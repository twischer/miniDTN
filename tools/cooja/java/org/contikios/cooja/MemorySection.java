/*
 * Copyright (c) 2014, TU Braunschweig
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
 */
package org.contikios.cooja;

import org.apache.log4j.Logger;

/**
 * A named memory section.
 * 
 * It contains a byte array and a start address.
 * 
 * Read-only memory is supported by setting the 'readonly' flag in the constructor.
 *
 * @author Fredrik Osterlind
 * @author Enrico Jörns
 */
public class MemorySection {

  private static final Logger logger = Logger.getLogger(MemorySection.class);

  private final long startAddr;
  private final boolean readonly;
  private byte[] data = null;

  /**
   * Create a new memory section.
   *
   * @param startAddr Start address of section
   * @param data Data of section
   * @param readonly If set true, write operations to memory are rejected.
   */
  public MemorySection(long startAddr, byte[] data, boolean readonly) {
    this.startAddr = startAddr;
    this.data = data;
    this.readonly = readonly;
  }

  /**
   * Create a new memory section.
   *
   * @param startAddr Start address of section
   * @param data Data of section
   */
  public MemorySection(long startAddr, byte[] data) {
    this(startAddr, data, false);
  }

  /**
   * Returns start address of this memory section.
   *
   * @return Start address
   */
  public long getStartAddr() {
    return startAddr;
  }

  /**
   * Returns size of this memory section.
   *
   * @return Size
   */
  public int getSize() {
    return data.length;
  }

  /**
   * Returns the entire byte array which defines this section.
   *
   * @return Byte array
   */
  public byte[] getData() {
    return data;
  }

  /**
   * True if given address is part of this memory section.
   *
   * @param addr
   * Address
   * @return True if given address is part of this memory section, false
   * otherwise
   */
  public boolean includesAddr(long addr) {
    if (data == null) {
      return false;
    }
    return addr >= startAddr && addr < (startAddr + data.length);
  }

  /**
   * True if the whole address range specified by address and size
   * lies inside this memory section.
   *
   * @param addr Start address of segment to check
   * @param size Size of segment to check
   *
   * @return True if given address range lies inside address range of this
   * section
   */
  public boolean inSection(long addr, int size) {
    return ((addr >= startAddr) && (addr + size <= startAddr + data.length - 1));
  }

  /**
   * Returns memory segment from this section.
   *
   * @param addr Start address of memory segment
   * @param size Size of memory segment
   * @return Memory segment
   */
  public byte[] getMemorySegment(long addr, int size) {
    if (!inSection(addr, size)) {
      logger.warn(String.format(
              "Failed to read [0x%x,0x%x]: Outside segment [0x%x,0x%x]",
              addr, addr + size - 1,
              startAddr, startAddr + data.length - 1));
      return null;
    }
    byte[] ret = new byte[size];
    System.arraycopy(data, (int) (addr - startAddr), ret, 0, size);
    return ret;
  }

  /**
   * Sets a memory segment.
   *
   * @param addr
   * Start of memory segment
   * @param data
   * Data of memory segment
   */
  public void setMemorySegment(long addr, byte[] data) {
    if (!inSection(addr, data.length)) {
      logger.warn(String.format(
              "Failed to write [0x%x,0x%x]: Outside segment [0x%x,0x%x]",
              addr, addr + data.length - 1,
              startAddr, startAddr + data.length - 1));
      return;
    }
    if (readonly) {
      logger.warn(String.format(
              "Rejected write to read-only memory [0x%x,0x%x]",
              addr, addr + data.length - 1));
      return;
    }
    System.arraycopy(data, 0, this.data, (int) (addr - startAddr), data.length);
  }

  @Override
  public MemorySection clone() {
    byte[] dataClone = new byte[data.length];
    System.arraycopy(data, 0, dataClone, 0, data.length);
    MemorySection clone = new MemorySection(startAddr, dataClone);
    return clone;
  }

  @Override
  public String toString() {
    return String.format(
            "%smemory section at [0x%x,0x%x]",
            readonly ? "readonly " : "",
            startAddr, startAddr + data.length - 1);
  }
}
