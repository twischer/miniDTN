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
import org.contikios.cooja.Memory.MemoryMonitor.EventType;

/**
 * Represents a mote memory.
 *
 * @author Enrico Jorns
 */
public class MoteMemory extends VarMemory {

  private final MemoryInterface mintf;

  /**
   * Creates MoteMemory from given MemoryInterface.
   * 
   * @param layout
   * @param mintf 
   */
  public MoteMemory(MemoryLayout layout, MemoryInterface mintf) {
    super(layout, mintf.getVariables());
    this.mintf = mintf;
  }

  @Override
  public void clearMemory() {
    mintf.clearMemory();
  }

  @Override
  public byte[] getMemorySegment(long address, int size) {
    return mintf.getMemorySegment(address, size);
  }

  @Override
  public void setMemorySegment(long address, byte[] data) {
    mintf.setMemorySegment(address, data);
  }

  @Override
  public int getTotalSize() {
    return mintf.getTotalSize();
  }

  private final Map<MemoryMonitor, MemoryInterface.SegmentMonitor> monitors = new HashMap<>();
  
  @Override
  public boolean addMemoryMonitor(EventType flag, long address, int size, final MemoryMonitor mm) {
    MemoryInterface.SegmentMonitor monitor = new MemoryInterface.SegmentMonitor() {

      @Override
      public void memoryChanged(MemoryInterface memory, EventType type, long address) {
        mm.memoryChanged(MoteMemory.this, type, address);
      }
    };
    monitors.put(mm, monitor);
    return mintf.addSegmentMonitor(flag, address, size, monitor);
  }

  @Override
  public boolean removeMemoryMonitor(long address, int size, MemoryMonitor mm) {
    return mintf.removeSegmentMonitor(address, size, monitors.get(mm));
  }
}
