/*
 * Copyright (c) 2014, TU Braunschweig.
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
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
 */

package org.contikios.cooja.mote.memory;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.apache.log4j.Logger;
import org.contikios.cooja.mote.memory.Memory.MemoryMonitor.EventType;
import org.contikios.cooja.mote.memory.MemoryInterface.MoteMemoryException;
import org.contikios.cooja.mote.memory.Memory.MemoryMonitor;
import org.contikios.cooja.mote.memory.MemoryInterface.Symbol;

/**
 * Represents a mote memory consisting of non-overlapping memory sections with
 * symbol addresses.
 * <p>
 * When an non-existing memory segment is written, a new section is
 * automatically
 * created for this segment.
 * <p>
 *
 * @author Fredrik Osterlind
 * @author Enrico Jorns
 */
public class SectionMoteMemory extends VarMemory implements SectionMemory {

  private static final Logger logger = Logger.getLogger(SectionMoteMemory.class);
  private static final boolean DEBUG = logger.isDebugEnabled();

  /* Both normal and readonly sections */
  private final List<MemorySection> sections = new ArrayList<>();

  private final MemoryLayout layout;

  /**
   * Creates a new section-based memory.
   * 
   * @param layout Memory layout of mote
   */
  public SectionMoteMemory(MemoryLayout layout) {
    super(layout);
    this.layout = layout;
  }
  
  // -- Memory implementations

  @Override
  public void clearMemory() {
    sections.clear();
  }

  @Override
  public int getTotalSize() {
    int totalSize = 0;
    for (MemorySection section : sections) {
      totalSize += section.getSize();
    }
    return totalSize;
  }

  @Override
  public byte[] getMemorySegment(long address, int size) throws MoteMemoryException {

    for (MemorySection section : sections) {
      if (section.includesAddr(address)
              && section.includesAddr(address + size - 1)) {
        return section.getMemorySegment(address, size);
      }
    }

    throw new MoteMemoryException(
            "Getting memory segment [0x%x,0x%x] failed: No section available",
            address, address + size - 1);
  }

  @Override
  public void setMemorySegment(long address, byte[] data) throws MoteMemoryException {

    for (MemorySection section : sections) {
      if (section.inSection(address, data.length)) {
        section.setMemorySegment(address, data);
        if (DEBUG) {
          logger.debug(String.format(
                  "Wrote memory segment [0x%x,0x%x]",
                  address, address + data.length - 1));
        }
        return;
      }
    }
    throw new MoteMemoryException(
            "Writing memory segment [0x%x,0x%x] failed: No section available",
            address, address + data.length - 1);
  }
  
  // --- SectionMemory implementations

  /**
   * Adds a section to this memory.
   * 
   * A new section will be checked for address overlap with existing sections.
   *
   * @return true if adding succeeded, false otherwise
   */
  @Override
  public boolean addMemorySection(MemorySection section) {
    
    if (section == null) {
      return false;
    }
    
    /* Cooja address space */
    for (MemorySection sec : sections) {
      /* check for overlap with existing region */
      if ((section.getStartAddr() <= sec.getStartAddr() + sec.getSize() - 1)
              && (section.getStartAddr() + section.getSize() - 1 >= sec.getStartAddr())) {
        logger.error(String.format(
                "Adding %s failed: Overlap with existing %s",
                section.toString(),
                sec.toString()));
        return false;
      }
    }

    sections.add(section);
    if (section.getVariables() != null) {
      for (Symbol s : section.getVariables()) {
        addVariable(s);
      }
    }
    
    if (DEBUG) {
      logger.debug("Added " + section.toString());
    }
    return true;
  }

  /**
   * Returns the total number of sections in this memory.
   *
   * @return Number of sections
   */
  @Override
  public int getNumberOfSections() {
    return sections.size();
  }

  @Override
  public MemorySection getSection(String name) {
    for (MemorySection memsec : sections) {
      if (memsec.getName().equals(name)) {
        return memsec;
      }
    }

    logger.warn("Section '" + name + "' not found");
    return null;
  }

  @Override
  public List<MemorySection> getSections() {
    return sections;
  }

  /**
   * Clones this memory including all sections and variables.
   */
  @Override
  public SectionMoteMemory clone() {
    ArrayList<MemorySection> sectionsClone = new ArrayList<>();
    for (MemorySection section : sections) {
      sectionsClone.add(section.clone());
    }

    SectionMoteMemory clone = new SectionMoteMemory(layout);
    for (MemorySection sec : sectionsClone) {
      clone.addMemorySection(sec);
    }

    return clone;
  }

  private final List<PolledMemorySegment> polledMemories = new ArrayList<>();

  public void pollForMemoryChanges() {
    for (PolledMemorySegment mem : polledMemories.toArray(new PolledMemorySegment[0])) {
      mem.notifyIfChanged();
    }
  }

  /**
   * Memory segment monitor that needs to be polled manually
   * to check for memory modifications.
   * 
   * @note This cannot be used to detect read operations on the segment
   */
  private class PolledMemorySegment {

    private final MemoryMonitor mm;
    private final long address;
    private final int size;
    private byte[] oldMem;

    public PolledMemorySegment(MemoryMonitor mm, long address, int size) {
      this.mm = mm;
      this.address = address;
      this.size = size;

      oldMem = getMemorySegment(address, size);
    }

    /**
     * Inkoves memoryChanged() if segment changed since last call.
     */
    private void notifyIfChanged() {
      byte[] newMem = getMemorySegment(address, size);
      if (Arrays.equals(oldMem, newMem)) {
        return;
      }

      mm.memoryChanged(SectionMoteMemory.this, EventType.WRITE, address);
      oldMem = newMem;
    }                                            
  }

  @Override
  public boolean addMemoryMonitor(EventType type, long address, int size, MemoryMonitor mm) {
    if (type == EventType.READ) {
      logger.warn("R type not supported");
      return false;
    }
    else if (type == EventType.READWRITE) {
      logger.warn("R/W type not supported, fallback to W");
    }
    PolledMemorySegment t = new PolledMemorySegment(mm, address, size);
    polledMemories.add(t);
    return true;
  }

  @Override
  public boolean removeMemoryMonitor(long address, int size, MemoryMonitor mm) {
    for (PolledMemorySegment mcm : polledMemories) {
      if (mcm.mm == mm && mcm.address == address && mcm.size == size) {
        polledMemories.remove(mcm);
        return true;
      }
    }
    return false;
  }
}
