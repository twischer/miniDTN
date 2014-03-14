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

package org.contikios.cooja;

import java.util.HashMap;
import java.util.Map;
import org.contikios.cooja.MemMonitor.MonitorType;
import org.contikios.cooja.MemoryInterface.Symbol;

/**
 *
 * @author Enrico Joerns
 */
public abstract class VarMemory extends NewAddressMemory {

  /** XXX add Symbol list to constructor argument */
  public VarMemory(MemoryLayout layout) {
    super(layout);
  }

  /**
   * @return All variable names known and residing in this memory
   */
  public abstract Symbol[] getVariables();

  /**
   * 
   * @return 
   */
  public abstract String[] getVariableNames();
  
  /**
   * Checks if given variable exists in memory.
   *
   * @param varName Variable name
   * @return True if variable exists, false otherwise
   */
  public abstract boolean variableExists(String varName);

  /**
   * Returns address of variable with given name.
   *
   * @param varName Variable name
   * @return Variable address
   */
  public abstract Symbol getVariable(String varName) throws UnknownVariableException;

  /**
   * 
   * @param varName
   * @return
   * @throws UnknownVariableException 
   */
  public long getVariableAddress(String varName) throws UnknownVariableException {
    return getVariable(varName).addr;
  }

  /**
   * 
   * @param varName
   * @return
   * @throws UnknownVariableException 
   */
  public int getVariableSize(String varName) throws UnknownVariableException {
    return getVariable(varName).size;
  }

  /**
   *
   * @param varName
   * @return
   */
  public byte getByteValueOf(String varName)
          throws UnknownVariableException {
    byte val = getByteValueOf(getVariable(varName).addr);
    return val;
  }

  /**
   *
   * @param varName
   * @return
   */
  public short getShortValueOf(String varName)
          throws UnknownVariableException {
    short val = getShortValueOf(getVariable(varName).addr);
    return val;
  }

  /**
   *
   * @param varName
   * @return
   */
  public int getIntValueOf(String varName)
          throws UnknownVariableException {
    int val = getIntValueOf(getVariable(varName).addr);
    return val;
  }

  /**
   *
   * @param varName
   * @return
   */
  public long getLongValueOf(String varName)
          throws UnknownVariableException {
    long val = getLongValueOf(getVariable(varName).addr);
    return val;
  }

  /**
   *
   * @param varName
   * @return
   */
  public long getAddrValueOf(String varName)
          throws UnknownVariableException {
    long val = getAddrValueOf(getVariable(varName).addr);
    return val;
  }

  /**
   *
   * @param varName
   * @param length
   * @return
   */
  public byte[] getByteArray(String varName, int length)
          throws UnknownVariableException {
    return getMemorySegment(getVariable(varName).addr, length);
  }

  /**
   *
   * @param varName
   * @param value
   */
  public void setByteValueOf(String varName, byte value)
          throws UnknownVariableException {
    setByteValueOf(getVariable(varName).addr, value);
  }

  /**
   *
   * @param varName
   * @param value
   */
  public void setShortValueOf(String varName, short value)
          throws UnknownVariableException {
    setShortValueOf(getVariable(varName).addr, value);
  }

  /**
   *
   * @param varName
   * @param value
   */
  public void setIntValueOf(String varName, int value)
          throws UnknownVariableException {
    setIntValueOf(getVariable(varName).addr, value);
  }

  /**
   *
   * @param varName
   * @param value
   */
  public void setLongValueOf(String varName, long value)
          throws UnknownVariableException {
    setLongValueOf(getVariable(varName).addr, value);
  }

  /**
   *
   * @param varName
   * @param value
   */
  public void setAddrValueOf(String varName, long value)
          throws UnknownVariableException {
    setAddrValueOf(getVariable(varName).addr, value);
  }

  /**
   *
   * @param varName
   * @param data
   */
  public void setByteArray(String varName, byte[] data)
          throws UnknownVariableException {
    setMemorySegment(getVariable(varName).addr, data);
  }

  /**
   * Monitor to listen for memory updates.
   */
  public interface VarMonitor extends MemMonitor {

    public void varChanged(VarMemory memory, MemoryEventType type, String varName);
  }

  /**
   * Maps VarMonitor to their internal created AddressMonitor.
   * Require for removing appropriate monitor.
   */
  Map<VarMonitor, AddressMonitor> monitorMapping = new HashMap<>();

  /**
   * Adds a AddressMonitor for the specified address region.
   *
   * @param flag Select memory operation(s) to listen for (read, write,
   * read/write)
   * @param varName
   * @param vm
   * @return
   */
  public boolean addVarMonitor(MonitorType flag, final String varName, final VarMonitor vm) {
    AddressMonitor mm = new AddressMonitor() {
      @Override
      public void memoryChanged(NewAddressMemory memory, MemoryEventType type, long address) {
        vm.varChanged(VarMemory.this, type, varName);
      }
    };
    monitorMapping.put(vm, mm);
    return addMemoryMonitor(
            flag,
            getVariable(varName).addr,
            getVariable(varName).size,
            mm);
  }

  /**
   * Removes AddressMonitor assigned to the specified region.
   *
   * @param varName
   * @param mm AddressMonitor to remove
   */
  public void removeVarMonitor(String varName, VarMonitor mm) {
    removeMemoryMonitor(getVariable(varName).addr, getVariable(varName).size, monitorMapping.get(mm));
  }
}
