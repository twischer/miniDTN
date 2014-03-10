/*
 * Copyright (c) 2009, Swedish Institute of Computer Science. All rights
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

package org.contikios.cooja.avrmote;

import java.util.ArrayList;
import java.util.Iterator;

import org.apache.log4j.Logger;

import avrora.arch.avr.AVRProperties;
import avrora.core.SourceMapping;
import avrora.core.SourceMapping.Location;
import avrora.sim.AtmelInterpreter;
import avrora.sim.Simulator.Watch;
import avrora.sim.State;
import org.contikios.cooja.UnknownVariableException;
import org.contikios.cooja.MemMonitor.MemoryEventType;
import org.contikios.cooja.MemMonitor.MonitorType;
import org.contikios.cooja.MemoryLayout;
import org.contikios.cooja.MoteMemory;

/**
 * @author Joakim Eriksson, Fredrik Osterlind, David Kopf
 */
public class AvrMoteMemory extends MoteMemory {
  private static final Logger logger = Logger.getLogger(AvrMoteMemory.class);

  private final SourceMapping memoryMap;
  private final AtmelInterpreter interpreter;
  private final ArrayList<AvrMemoryMonitor> memoryMonitors = new ArrayList<>();

  private boolean coojaIsAccessingMemory;

  public AvrMoteMemory(MemoryLayout layout, SourceMapping map, AVRProperties avrProperties, AtmelInterpreter interpreter) {
    super(layout);
    this.memoryMap = map;
    this.interpreter = interpreter;
  }

  @Override
  public void clearMemory() {
    logger.fatal("clearMemory() not implemented");
  }

  @Override
  public byte[] getMemorySegment(long address, int size) {
    /*logger.info("getMemorySegment(" + String.format("0x%04x", address) +
        ", " + size + ")");*/

    /* XXX Unsure whether this is the appropriate method to use, as it
     * triggers memoryRead monitor. Right now I'm using a flag to indicate
     * that Cooja (as opposed to Contiki) read the memory, to avoid infinite
     * recursion. */
    coojaIsAccessingMemory = true;
    byte[] data = new byte[(int) size];
    for (int i=0; i < size; i++) {
      data[i] = (byte) (interpreter.getDataByte((int) address + i) & 0xff);
    }
    coojaIsAccessingMemory = false;
    return data;
  }

  @Override
  public int getTotalSize() {
    logger.warn("getTotalSize() not implemented");
    return -1;
  }

  @Override
  public void setMemorySegment(long address, byte[] data) {
    coojaIsAccessingMemory = true;
    logger.info("setMemorySegment(" + String.format("0x%04x", address) +
        ", arr:" + data.length + ")");

    /* XXX See comment in getMemorySegment. */
    coojaIsAccessingMemory = true;
    for (int i=0; i < data.length; i++) {
      interpreter.writeDataByte((int) address + i, data[i]);
    }
    coojaIsAccessingMemory = false;
  }

//  @Override
//  public byte getByteValueOf(String varName) throws UnknownVariableException {
//    return getMemorySegment(getVariableAddress(varName), 1)[0];
//  }
//
//  @Override
//  public int getIntValueOf(String varName) throws UnknownVariableException {
//    byte[] arr = getMemorySegment(getVariableAddress(varName), getIntegerLength());
//    return parseInt(arr);
//  }

//  @Override
//  public int getIntegerLength() {
//    return 2; /* XXX */
//  }

  @Override
  public long getVariableAddress(String varName)
          throws UnknownVariableException {
    /* RAM addresses start at 0x800000 in Avrora */
    Location loc = memoryMap.getLocation(varName);
    if (loc == null || (loc.section.equals(".text"))) {
      throw new UnknownVariableException(varName);
    }
    int ret = loc.vma_addr & 0x7fffff;
    /*logger.info("Symbol '" + memoryMap.getLocation(varName).name +
     "': vma_addr: " + String.format("0x%04x", vma) +
     ", returned address: " + String.format("0x%04x", ret));*/
    return ret;
  }
  
  @Override
  public int getVariableSize(String varName)
  throws UnknownVariableException {
    /* RAM addresses start at 0x800000 in Avrora */
    Location loc = memoryMap.getLocation(varName);
    if (loc == null || (loc.section.equals(".text"))) {
      throw new UnknownVariableException(varName);
    }
    return loc.size;
  }

  @Override
  public String[] getVariableNames() {
    ArrayList<String> symbols = new ArrayList<>();
    for (Iterator i = memoryMap.getIterator(); i.hasNext();) {
      Location loc = (Location) i.next();
      /* Skip '.text' code section symbols */
      if (!loc.section.equals(".text")) {
        symbols.add(loc.name);
      }
    }
    return symbols.toArray(new String[0]);
  }

//  @Override
//  public void setByteArray(String varName, byte[] data)
//  throws UnknownVariableException {
//    setMemorySegment(getVariableAddress(varName), data);
//  }

//  @Override
//  public void setByteValueOf(String varName, byte newVal)
//  throws UnknownVariableException {
//    setMemorySegment(getVariableAddress(varName), new byte[] {newVal});
//  }

//  @Override
//  public void setIntValueOf(String varName, int newVal)
//  throws UnknownVariableException {
//    int varAddr = getVariableAddress(varName);
//    int newValToSet = Integer.reverseBytes(newVal);
//    int pos = 0;
//    byte[] varData = new byte[2];
//    varData[pos++] = (byte) ((newValToSet & 0xFF000000) >> 24);
//    varData[pos++] = (byte) ((newValToSet & 0xFF0000) >> 16);
//    setMemorySegment(varAddr, varData);
//  }

  @Override
  public boolean variableExists(String varName) {
    return memoryMap.getLocation(varName) != null;
  }

  class AvrMemoryMonitor {

    long address;
    int size;
    AddressMonitor mm;
    Watch watch;
  }
  

  @Override
  public boolean addMemoryMonitor(MonitorType flag, long address, int size, AddressMonitor mm) {
    final AvrMemoryMonitor mon = new AvrMemoryMonitor();
    mon.address = address;
    mon.size = size;
    mon.mm = mm;
    if (flag == MonitorType.R) {
      mon.watch = new Watch.Empty() {
        @Override
        public void fireAfterRead(State state, int data_addr, byte value) {
          if (coojaIsAccessingMemory) {
            return;
          }
          mon.mm.memoryChanged(AvrMoteMemory.this, MemoryEventType.READ, data_addr);
        }
      };
    }
    else if (flag == MonitorType.W) {
      mon.watch = new Watch.Empty() {
        @Override
        public void fireAfterWrite(State state, int data_addr, byte value) {
          if (coojaIsAccessingMemory) {
            return;
          }
          mon.mm.memoryChanged(AvrMoteMemory.this, MemoryEventType.WRITE, data_addr);
        }
      };
    }
    else {
      mon.watch = new Watch.Empty() {
        @Override
        public void fireAfterRead(State state, int data_addr, byte value) {
          if (coojaIsAccessingMemory) {
            return;
          }
          mon.mm.memoryChanged(AvrMoteMemory.this, MemoryEventType.READ, data_addr);
        }

        @Override
        public void fireAfterWrite(State state, int data_addr, byte value) {
          if (coojaIsAccessingMemory) {
            return;
          }
          mon.mm.memoryChanged(AvrMoteMemory.this, MemoryEventType.WRITE, data_addr);
        }
      };
    }
    memoryMonitors.add(mon);
    /* logger.debug("Added AvrMemoryMonitor " + Integer.toString(mon.hashCode()) + " for addr " + mon.address + " size " + mon.size + " with watch" + mon.watch); */

    /* Add a watch for every byte in range */
    for (int idx = 0; idx < mon.size; idx++) {
      interpreter.getSimulator().insertWatch(mon.watch, (int) mon.address + idx);
      /* logger.debug("Inserted watch " + Integer.toString(mon.watch.hashCode()) + " for " + (mon.address + idx)); */
    }
    return true;
  }
  
  @Override
  public boolean removeMemoryMonitor(long address, int size, AddressMonitor mm) {
    for (AvrMemoryMonitor mcm: memoryMonitors) {
      if (mcm.mm != mm || mcm.address != address || mcm.size != size) {
        continue;
      }
      for (int idx = 0; idx < mcm.size; idx++) {
        interpreter.getSimulator().removeWatch(mcm.watch, (int) mcm.address + idx);
        /* logger.debug("Removed watch " + Integer.toString(mcm.watch.hashCode()) + " for " + (mcm.address + idx)); */
      }
      memoryMonitors.remove(mcm);
      return true;
    }
    return false;
  }

//  @Override
//  public int parseInt(byte[] memorySegment) {
//    if (memorySegment.length < 2) {
//      return -1;
//    }
//
//    int retVal = 0;
//    int pos = 0;
//    retVal += ((memorySegment[pos++] & 0xFF)) << 8;
//    retVal += ((memorySegment[pos++] & 0xFF));
//
//    return Integer.reverseBytes(retVal) >> 16;
//  }
//
//  @Override
//  public byte[] getByteArray(String varName, int length)
//      throws UnknownVariableException {
//    return getMemorySegment(getVariableAddress(varName), length);
//  }
}
