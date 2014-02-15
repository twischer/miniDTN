/*
 * Copyright (c) 2007, Swedish Institute of Computer Science. All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer. 2. Redistributions in
 * binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution. 3. Neither the name of the
 * Institute nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
package org.contikios.cooja.mspmote;

import java.util.ArrayList;

import org.apache.log4j.Logger;

import org.contikios.cooja.MoteMemory.UnknownVariableException;
import org.contikios.cooja.MemMonitor.MemoryEventType;
import org.contikios.cooja.MemMonitor.MonitorType;
import org.contikios.cooja.MemoryLayout;
import org.contikios.cooja.Mote;
import org.contikios.cooja.MoteMemory;
import se.sics.mspsim.core.MSP430;
import se.sics.mspsim.core.Memory.AccessMode;
import se.sics.mspsim.core.Memory.AccessType;
import se.sics.mspsim.util.MapEntry;

public class MspMoteMemory extends MoteMemory {

  private static final Logger logger = Logger.getLogger(MspMoteMemory.class);
  private final ArrayList<MapEntry> mapEntries;

  private final MSP430 cpu;

  public MspMoteMemory(MemoryLayout layout, Mote mote, MapEntry[] allEntries, MSP430 cpu) {
    super(layout);
    this.mapEntries = new ArrayList<>();

    for (MapEntry entry : allEntries) {
      if (entry.getType() == MapEntry.TYPE.variable) {
        mapEntries.add(entry);
      }
    }

    this.cpu = cpu;
  }

  @Override
  public String[] getVariableNames() {
    String[] names = new String[mapEntries.size()];
    for (int i = 0; i < mapEntries.size(); i++) {
      names[i] = mapEntries.get(i).getName();
    }
    return names;
  }

  private MapEntry getMapEntry(String varName) throws UnknownVariableException {
    for (MapEntry entry : mapEntries) {
      if (entry.getName().equals(varName)) {
        return entry;
      }
    }
    throw new UnknownVariableException(varName);
  }

  @Override
  public long getVariableAddress(String varName) throws UnknownVariableException {
    MapEntry entry = getMapEntry(varName);
    return entry.getAddress();
  }

  @Override
  public int getVariableSize(String varName) throws UnknownVariableException {
    MapEntry entry = getMapEntry(varName);
    return entry.getSize();
  }

//  public int getIntegerLength() {
//    return 2;
//  }
  @Override
  public void clearMemory() {
    logger.fatal("clearMemory() not implemented");
  }

  @Override
  public byte[] getMemorySegment(long address, int size) {
    int[] memInts = new int[size];

    System.arraycopy(cpu.memory, (int) address, memInts, 0, size);

    /* Convert to byte array */
    byte[] memBytes = new byte[size];
    for (int i = 0; i < size; i++) {
      memBytes[i] = (byte) memInts[i];
    }

    return memBytes;
  }

  @Override
  public void setMemorySegment(long address, byte[] data) {
    /* Convert to int array */
    int[] memInts = new int[data.length];
    for (int i = 0; i < data.length; i++) {
      memInts[i] = data[i];
    }

    System.arraycopy(memInts, 0, cpu.memory, (int) address, data.length);
  }

  @Override
  public int getTotalSize() {
    return cpu.memory.length;
  }

  @Override
  public boolean variableExists(String varName) {
    for (MapEntry entry : mapEntries) {
      if (entry.getName().equals(varName)) {
        return true;
      }
    }

    return false;
  }

  /* TODO Check correct variable size in below methods */
//  public int getIntValueOf(String varName) throws UnknownVariableException {
//    MapEntry entry = getMapEntry(varName);
//
//    int varAddr = entry.getAddress();
//    byte[] varData = getMemorySegment(varAddr, 2);
//    return parseInt(varData);
//  }
//  public void setIntValueOf(String varName, int newVal) throws UnknownVariableException {
//    MapEntry entry = getMapEntry(varName);
//    int varAddr = entry.getAddress();
//
//    int newValToSet = Integer.reverseBytes(newVal);
//
//    // Create byte array
//    int pos = 0;
//
//    byte[] varData = new byte[2];
//    varData[pos++] = (byte) ((newValToSet & 0xFF000000) >> 24);
//    varData[pos++] = (byte) ((newValToSet & 0xFF0000) >> 16);
//
//    setMemorySegment(varAddr, varData);
//  }
//  public byte getByteValueOf(String varName) throws UnknownVariableException {
//    MapEntry entry = getMapEntry(varName);
//    int varAddr = entry.getAddress();
//
//    byte[] varData = getMemorySegment(varAddr, 1);
//
//    return varData[0];
//  }
//  public void setByteValueOf(String varName, byte newVal) throws UnknownVariableException {
//    MapEntry entry = getMapEntry(varName);
//    int varAddr = entry.getAddress();
//
//    byte[] varData = new byte[1];
//
//    varData[0] = newVal;
//
//    setMemorySegment(varAddr, varData);
//  }
//  public byte[] getByteArray(String varName, int length) throws UnknownVariableException {
//    MapEntry entry = getMapEntry(varName);
//    int varAddr = entry.getAddress();
//
//    return getMemorySegment(varAddr, length);
//  }
//  public void setByteArray(String varName, byte[] data) throws UnknownVariableException {
//    MapEntry entry = getMapEntry(varName);
//    int varAddr = entry.getAddress();
//
//    setMemorySegment(varAddr, data);
//  }
  private final ArrayList<MemoryCPUMonitor> cpuMonitorArray = new ArrayList<>();

//  @Override
//  public boolean addMemoryMonitor(MonitorType flag, long address, int size, AddressMonitor mm) {
//    throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
//  }
//
//  @Override
//  public void removeMemoryMonitor(long address, int size, AddressMonitor mm) {
//    throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
//  }
  class MemoryCPUMonitor extends se.sics.mspsim.core.MemoryMonitor.Adapter {

    public final AddressMonitor mm;
    public final int address;
    public final int size;

    public MemoryCPUMonitor(AddressMonitor mm, int address, int size) {
      this.mm = mm;
      this.address = address;
      this.size = size;
    }

    @Override
    public void notifyReadAfter(int address, AccessMode mode, AccessType type) {
      mm.memoryChanged(MspMoteMemory.this, MemoryEventType.READ, address);
    }

    @Override
    public void notifyWriteAfter(int dstAddress, int data, AccessMode mode) {
      mm.memoryChanged(MspMoteMemory.this, MemoryEventType.WRITE, dstAddress);
    }
  }

  @Override
  public boolean addMemoryMonitor(MonitorType type, long address, int size, AddressMonitor mm) {
    if (type != MonitorType.RW) {
      logger.warn("r/w type option ignored");
    }
    MemoryCPUMonitor t = new MemoryCPUMonitor(mm, (int) address, size);
    cpuMonitorArray.add(t);

    for (int a = (int) address; a < address + size; a++) {
      cpu.addWatchPoint(a, t);
    }

    return true;
  }

  @Override
  public boolean removeMemoryMonitor(long address, int size, AddressMonitor mm) {
    for (MemoryCPUMonitor mcm : cpuMonitorArray) {
      if (mcm.mm != mm || mcm.address != address || mcm.size != size) {
        continue;
      }
      for (int a = (int) address; a < (int) address + size; a++) {
        cpu.removeWatchPoint(a, mcm);
      }
      cpuMonitorArray.remove(mcm);
      return true;
    }
    return false;
  }

//  public int parseInt(byte[] memorySegment) {
//    if (memorySegment.length < 2) {
//      return -1;
//    }
//    
//    int retVal = 0;
//    int pos = 0;
//    retVal += ((memorySegment[pos++] & 0xFF)) << 8;
//    retVal += ((memorySegment[pos++] & 0xFF)) << 0;
//
//    return Integer.reverseBytes(retVal) >> 16;
//  }
}
