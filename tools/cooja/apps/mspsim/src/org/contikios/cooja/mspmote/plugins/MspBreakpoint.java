/*
 * Copyright (c) 2012, Swedish Institute of Computer Science.
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

package org.contikios.cooja.mspmote.plugins;

import java.io.File;

import org.apache.log4j.Logger;

import org.contikios.cooja.Watchpoint;
import org.contikios.cooja.mspmote.MspMote;
import se.sics.mspsim.core.Memory;
import se.sics.mspsim.core.MemoryMonitor;

/**
 * Mspsim watchpoint.
 *
 * @author Fredrik Osterlind
 */
public class MspBreakpoint extends Watchpoint<MspMote> {
  private static Logger logger = Logger.getLogger(MspBreakpoint.class);

  private MemoryMonitor memoryMonitor = null;

  public MspBreakpoint(MspMote mote) {
    super(mote);
    /* expects setConfigXML(..) */
  }

  public MspBreakpoint(MspMote mote, Integer address, File codeFile, Integer lineNr) {
    super(mote, address, codeFile, lineNr);
  }

  public void registerBreakpoint() {
    memoryMonitor = new MemoryMonitor.Adapter() {
      public void notifyReadBefore(int addr, Memory.AccessMode mode, Memory.AccessType type) {
        if (type != Memory.AccessType.EXECUTE) {
          return;
        }

        getMote().signalBreakpointTrigger(MspBreakpoint.this);
      }
    };
    getMote().getCPU().addWatchPoint(getExecutableAddress(), memoryMonitor);
  }

  public void unregisterBreakpoint() {
    getMote().getCPU().removeWatchPoint(getExecutableAddress(), memoryMonitor);
  }
}
