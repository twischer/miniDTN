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

package org.contikios.cooja;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import org.apache.log4j.Logger;
import org.contikios.cooja.MemMonitor.MemoryEventType;
import org.contikios.cooja.MemMonitor.MonitorType;
import org.contikios.cooja.NewAddressMemory.AddressMonitor;
import org.contikios.cooja.MemoryInterface.Symbol;

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
  private List<MemorySection> sections = new ArrayList<>();

  private final HashMap<String, Symbol> variables;

  /* used to map Cooja's address space to native (Contiki's) addresses */
  private final int offset;

  private final MemoryLayout layout;

  /**
   * @param layout
   * @param variables
   * @param offset Offset for internally used addresses
   */
  public SectionMoteMemory(MemoryLayout layout, HashMap<String, Symbol> variables, int offset) {
    super(layout);
    this.layout = layout;
    this.variables = variables;
    this.offset = offset;
  }

  @Override
  public Symbol[] getVariables() {
    return variables.values().toArray(new Symbol[0]);
  }

  @Override
  public String[] getVariableNames() {
    return variables.keySet().toArray(new String[0]);
  }

  @Override
  public boolean variableExists(String varName) {
    return variables.containsKey(varName);
  }

  @Override
  public Symbol getVariable(String varName) throws UnknownVariableException {
    /* Cooja address space */
    if (!variables.containsKey(varName)) {
      throw new UnknownVariableException(varName);
    }

    Symbol sym = variables.get(varName);
    
    return new Symbol(sym.type, sym.name, sym.section, sym.addr + offset, sym.size);
  }

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
  public byte[] getMemorySegment(long address, int size) {
    /* Cooja address space */
    address -= offset;

    for (MemorySection section : sections) {
      if (section.includesAddr(address)
              && section.includesAddr(address + size - 1)) {
        return section.getMemorySegment(address, size);
      }
    }

    logger.error(String.format(
            "Getting memory segment [0x%x,0x%x] failed: No section available",
            address, address + size - 1));
    return null;
  }

  @Override
  public void setMemorySegment(long address, byte[] data) {
    /* Cooja address space */
    address -= offset;

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
    logger.error(String.format(
            "Writing memory segment [0x%x,0x%x] failed: No section available",
            address, address + data.length - 1));
  }

  /**
   * Creates memory section for relative (jvm) address.
   *
   * Address will be converted from absolute to relative.
   *
   * @return true if creating succeeded, false otherwise
   */
  @Override
  public boolean addMemorySection(MemorySection section) {
    /* Cooja address space */
    for (MemorySection sec : sections) {
      /* check for overlap with existing region */
      if ((section.getStartAddr() <= sec.getStartAddr() + sec.getSize() - 1)
              && (section.getStartAddr() + section.getSize() - 1 >= sec.getStartAddr())) {
        logger.error(String.format(
                "Creating memory section %s [0x%x,0x%x] failed: Overlap with existing section [%x,%x]",
                section.getName(),
                section.getStartAddr(), section.getStartAddr() + section.getSize() - 1,
                sec.getStartAddr(), sec.getStartAddr() + sec.getSize() - 1));
        return false;
      }
    }

    sections.add(section);
    if (DEBUG) {
      logger.debug(String.format(
              "Created new memory section %s [0x%x,0x%x]",
              section.getName(),
              section.getStartAddr(), section.getStartAddr() + section.getSize() - 1));
    }
    return true;
  }

  // --- SectionMemory implementations
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
   * Get start address of given section in native address space.
   *
   * @param sectionNr Section position
   * @return Start address of section
   */
  public long getSectionNativeAddress(int sectionNr) {
    if (sectionNr >= sections.size()) {
      return -1;
    }
    return sections.get(sectionNr).getStartAddr();
  }

  /**
   * Get size of section at given position.
   *
   * @param sectionNr Section position
   * @return Size of section
   */
  public int getSizeOfSection(int sectionNr) {
    if (sectionNr >= sections.size()) {
      return -1;
    }

    return sections.get(sectionNr).getSize();
  }

  /**
   * Get data of section at given position.
   *
   * @param sectionNr Section position
   * @return Data at section
   */
  public byte[] getDataOfSection(int sectionNr) {
    if (sectionNr >= sections.size()) {
      return null;
    }

    return sections.get(sectionNr).getData();
  }

  @Override
  public SectionMoteMemory clone() {
    ArrayList<MemorySection> sectionsClone = new ArrayList<>();
    for (MemorySection section : sections) {
      sectionsClone.add(section.clone());
    }

    SectionMoteMemory clone = new SectionMoteMemory(layout, variables, offset);
    clone.sections = sectionsClone;

    return clone;
  }

  private final List<PolledMemorySegments> polledMemories = new ArrayList<>();

  public void pollForMemoryChanges() {
    for (PolledMemorySegments mem : polledMemories.toArray(new PolledMemorySegments[0])) {
      mem.notifyIfChanged();
    }
  }

  private class PolledMemorySegments {

    public final AddressMonitor mm;
    public final int address;
    public final int size;
    private byte[] oldMem;

    public PolledMemorySegments(AddressMonitor mm, int address, int size) {
      this.mm = mm;
      this.address = address;
      this.size = size;

      oldMem = getMemorySegment(address, size);
    }

    private void notifyIfChanged() {
      byte[] newMem = getMemorySegment(address, size);
      if (Arrays.equals(oldMem, newMem)) {
        return;
      }

      mm.memoryChanged(SectionMoteMemory.this, MemoryEventType.WRITE, address);
      oldMem = newMem;
    }
  }

  @Override
  public boolean addMemoryMonitor(MonitorType type, long address, int size, AddressMonitor mm) {
    if (type == MonitorType.R) {
      logger.warn("R type not supported");
      return false;
    }
    else if (type == MonitorType.RW) {
      logger.warn("R/W type not supported, fallback to W");
    }
    PolledMemorySegments t = new PolledMemorySegments(mm, (int) address, size);
    polledMemories.add(t);
    return true;
  }

  @Override
  public boolean removeMemoryMonitor(long address, int size, AddressMonitor mm) {
    for (PolledMemorySegments mcm : polledMemories) {
      if (mcm.mm != mm || mcm.address != address || mcm.size != size) {
        continue;
      }
      polledMemories.remove(mcm);
      return true;
    }
    return false;
  }
}
