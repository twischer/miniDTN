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
public class BufferedMemorySection extends MemorySection {

  private static final Logger logger = Logger.getLogger(BufferedMemorySection.class);

  private byte[] secData = null;

  /**
   * Create a new memory section.
   *
   * @param name Name of the section
   * @param startAddr Start address of section
   * @param size Size of section [bytes]
   * @param readonly If set true, write operations to memory are rejected.
   */
  public BufferedMemorySection(String name, long startAddr, int size, boolean readonly) {
    super(name, startAddr, size, readonly);
    secData = new byte[size];
  }

  /**
   * Create a new memory section.
   *
   * @param name Name of the section
   * @param startAddr Start address of section
   * @param size Size of section [bytes]
   */
  public BufferedMemorySection(String name, long startAddr, int size) {
    this(name, startAddr, size, false);
  }

  /**
   * Returns the entire byte array which defines this section.
   *
   * @return Byte array
   */
  @Override
  public byte[] getData() {
    return secData;
  }

  /**
   * Returns memory segment from this section.
   *
   * @param addr Start address of memory segment
   * @param size Size of memory segment [bytes]
   * @return Memory segment
   */
  @Override
  public byte[] getMemorySegment(long addr, int size) {
    if (!inSection(addr, size)) {
      logger.warn(String.format(
              "Failed to read [0x%x,0x%x]: Outside segment [0x%x,0x%x]",
              addr, addr + size - 1,
              secStartAddr, secStartAddr + secData.length - 1));
      return null;
    }
    byte[] ret = new byte[size];
    System.arraycopy(secData, (int) (addr - secStartAddr), ret, 0, size);
    return ret;
  }

  /**
   * Sets a memory segment in this section.
   *
   * @param addr Start of memory segment
   * @param data Data of memory segment
   */
  @Override
  public void setMemorySegment(long addr, byte[] data) {
    if (!inSection(addr, data.length)) {
      logger.warn(String.format(
              "Failed to write [0x%x,0x%x]: Outside segment [0x%x,0x%x]",
              addr, addr + data.length - 1,
              secStartAddr, secStartAddr + data.length - 1));
      return;
    }
    if (readonly) {
      logger.warn(String.format(
              "Rejected write to read-only memory [0x%x,0x%x]",
              addr, addr + data.length - 1));
      return;
    }
    System.arraycopy(data, 0, this.secData, (int) (addr - secStartAddr), data.length);
  }

  /**
   * Clone this section.
   * 
   * Adds '.cone' to the sections name
   * 
   * @return Cloned instance of this sectoin
   */
  @Override
  public MemorySection clone() {
    MemorySection clone = new BufferedMemorySection(secName + ".clone", secStartAddr, secData.length);
    clone.setMemorySegment(secStartAddr, secData);
    return clone;
  }

  @Override
  public String toString() {
    return String.format(
            "%smemory section '%s' at [0x%x,0x%x]",
            readonly ? "readonly " : "",
            secName,
            secStartAddr, secStartAddr + secData.length - 1);
  }
}
