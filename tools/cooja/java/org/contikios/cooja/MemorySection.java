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

/**
 * An abstract memory section.
 * 
 * Read-only memory is supported by setting the 'readonly' flag in the constructor.
 *
 * @author Fredrik Osterlind
 * @author Enrico Jörns
 */
public abstract class MemorySection {

  protected final String secName;
  protected final long secStartAddr;
  protected final int secSize;
  protected final boolean readonly;

  /**
   * Create a new memory section.
   *
   * @param name
   * @param startAddr Start address of section
   * @param size
   * @param readonly If set true, write operations to memory are rejected.
   */
  public MemorySection(String name, long startAddr, int size, boolean readonly) {
    this.secName = name;
    this.secStartAddr = startAddr;
    this.secSize = size;
    this.readonly = readonly;
  }

  /**
   * Create a new memory section.
   *
   * @param name
   * @param startAddr Start address of section
   * @param size
   */
  public MemorySection(String name, long startAddr, int size) {
    this(name, startAddr, size, false);
  }

  /**
   * Returns name of this memory section.
   *
   * @return secName
   */
  public String getName() {
    return secName;
  }

  /**
   * Returns start address of this memory section.
   *
   * @return Start address
   */
  public long getStartAddr() {
    return secStartAddr;
  }

  /**
   * Returns size of this memory section.
   *
   * @return Size
   */
  public int getSize() {
    return secSize;
  }

  /**
   * Returns the entire byte array which defines this section.
   *
   * @return Byte array
   */
  public abstract byte[] getData();

  /**
   * True if given address is part of this memory section.
   *
   * @param addr
   * Address
   * @return True if given address is part of this memory section, false
   * otherwise
   */
  public boolean includesAddr(long addr) {
    return addr >= secStartAddr && addr < (secStartAddr + secSize);
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
    return ((addr >= secStartAddr) && (addr + size <= secStartAddr + secSize));
  }

  /**
   * Returns memory segment from this section.
   *
   * @param addr Start address of memory segment
   * @param size Size of memory segment
   * @return Memory segment
   */
  public abstract byte[] getMemorySegment(long addr, int size);

  /**
   * Sets a memory segment.
   *
   * @param addr
   * Start of memory segment
   * @param data
   * Data of memory segment
   */
  public abstract void setMemorySegment(long addr, byte[] data);

  @Override
  public abstract MemorySection clone();

  @Override
  public String toString() {
    return String.format(
            "%smemory section at [0x%x,0x%x]",
            readonly ? "readonly " : "",
            secStartAddr, secStartAddr + secSize - 1);
  }
}
