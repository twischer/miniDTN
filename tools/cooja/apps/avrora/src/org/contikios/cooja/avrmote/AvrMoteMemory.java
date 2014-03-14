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
import java.util.LinkedList;
import java.util.List;
import org.contikios.cooja.MemMonitor.MemoryEventType;
import org.contikios.cooja.MemMonitor.MonitorType;
import org.contikios.cooja.MemoryInterface;

/**
 * @author Joakim Eriksson, Fredrik Osterlind, David Kopf
 */
public class AvrMoteMemory implements MemoryInterface {

  private static final Logger logger = Logger.getLogger(AvrMoteMemory.class);

  private final SourceMapping memoryMap;
  private final AVRProperties avrProperties;
  private final AtmelInterpreter interpreter;
  private final ArrayList<AvrByteMonitor> memoryMonitors = new ArrayList<>();

  private boolean coojaIsAccessingMemory;

  public AvrMoteMemory(
          SourceMapping map,
          AVRProperties avrProperties,
          AtmelInterpreter interpreter) {
    this.memoryMap = map;
    this.avrProperties = avrProperties;
    this.interpreter = interpreter;
  }

  @Override
  public void clearMemory() {
    setMemorySegment(0L, new byte[avrProperties.sram_size]);
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
    for (int i = 0; i < size; i++) {
      data[i] = (byte) (interpreter.getDataByte((int) address + i) & 0xff);
    }
    coojaIsAccessingMemory = false;
    return data;
  }

  @Override
  public void setMemorySegment(long address, byte[] data) {
    logger.debug("setMemorySegment("
            + String.format("0x%04x", address)
            + ", data:" + data.length + ")");

    /* XXX See comment in getMemorySegment. */
    coojaIsAccessingMemory = true;
    for (int i = 0; i < data.length; i++) {
      interpreter.writeDataByte((int) address + i, data[i]);
    }
    coojaIsAccessingMemory = false;
  }

  @Override
  public int getTotalSize() {
    return avrProperties.sram_size;
  }

  @Override
  public Symbol[] getVariables() {

    List<Symbol> symbols = new LinkedList<>();
    for (Iterator<Location> iter = memoryMap.getIterator(); iter.hasNext();) {
      Location loc = iter.next();
      if (loc == null || (loc.section.equals(".text"))) {
        continue;
      }
      symbols.add(new Symbol(Symbol.Type.VARIABLE, loc.name, loc.section, loc.vma_addr & 0x7fffff, loc.size));
    }
    return symbols.toArray(new Symbol[0]);
  }

  class AvrByteMonitor extends Watch.Empty {

    /** start address to monitor */
    final long address;
    /** size to monitor */
    final int size;
    /** Segment monitor to notify */
    final SegmentMonitor mm;
    /** MonitorType we are listening to */
    final MonitorType flag;

    public AvrByteMonitor(long address, int size, SegmentMonitor mm, MonitorType flag) {
      this.address = address;
      this.size = size;
      this.mm = mm;
      this.flag = flag;
    }

    @Override
    public void fireAfterRead(State state, int data_addr, byte value) {
      if (flag == MonitorType.W || coojaIsAccessingMemory) {
        return;
      }
      mm.memoryChanged(AvrMoteMemory.this, MemoryEventType.READ, data_addr);
    }

    @Override
    public void fireAfterWrite(State state, int data_addr, byte value) {
      if (flag == MonitorType.R || coojaIsAccessingMemory) {
        return;
      }
      mm.memoryChanged(AvrMoteMemory.this, MemoryEventType.WRITE, data_addr);
    }
  }

  @Override
  public boolean addSegmentMonitor(MonitorType flag, long address, int size, SegmentMonitor mm) {
    AvrByteMonitor mon = new AvrByteMonitor(address, size, mm, flag);

    memoryMonitors.add(mon);
    /* logger.debug("Added AvrByteMonitor " + Integer.toString(mon.hashCode()) + " for addr " + mon.address + " size " + mon.size + " with watch" + mon.watch); */

    /* Add byte monitor (watch) for every byte in range */
    for (int idx = 0; idx < mon.size; idx++) {
      interpreter.getSimulator().insertWatch(mon, (int) mon.address + idx);
      /* logger.debug("Inserted watch " + Integer.toString(mon.watch.hashCode()) + " for " + (mon.address + idx)); */
    }
    return true;
  }

  @Override
  public boolean removeSegmentMonitor(long address, int size, SegmentMonitor mm) {
    for (AvrByteMonitor mcm : memoryMonitors) {
      if (mcm.mm != mm || mcm.address != address || mcm.size != size) {
        continue;
      }
      for (int idx = 0; idx < mcm.size; idx++) {
        interpreter.getSimulator().removeWatch(mcm, (int) mcm.address + idx);
        /* logger.debug("Removed watch " + Integer.toString(mcm.watch.hashCode()) + " for " + (mcm.address + idx)); */
      }
      memoryMonitors.remove(mcm);
      return true;
    }
    return false;
  }
}
