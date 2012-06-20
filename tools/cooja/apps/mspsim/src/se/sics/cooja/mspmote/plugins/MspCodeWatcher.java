/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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

package se.sics.cooja.mspmote.plugins;

import java.awt.Color;
import java.awt.Component;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import javax.swing.JTabbedPane;

import org.apache.log4j.Logger;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.GUI;
import se.sics.cooja.Mote;
import se.sics.cooja.PluginType;
import se.sics.cooja.Simulation;
import se.sics.cooja.SupportedArguments;
import se.sics.cooja.WatchpointMote;
import se.sics.cooja.mspmote.MspMote;
import se.sics.cooja.mspmote.MspMoteType;
import se.sics.cooja.plugins.Debugger;
import se.sics.mspsim.core.EmulationException;
import se.sics.mspsim.ui.DebugUI;
import se.sics.mspsim.util.DebugInfo;
import se.sics.mspsim.util.ELFDebug;

@ClassDescription("Souce-level debugger")
@PluginType(PluginType.MOTE_PLUGIN)
@SupportedArguments(motes = {MspMote.class})
public class MspCodeWatcher extends Debugger {
  private static Logger logger = Logger.getLogger(MspCodeWatcher.class);

  private MspMote mspMote;
  private DebugUI assCodeUI;
  private ELFDebug debug;
  private String[] debugSourceFiles;

  /**
   * Source-level debugger for Mspsim-based motes.
   *
   * @param mote MspMote
   * @param sim Simulation
   * @param gui Simulator
   */
  public MspCodeWatcher(Mote mote, Simulation sim, GUI gui) {
    super((WatchpointMote)mote, sim, gui, "Msp Debugger - " + mote);
    this.mspMote = (MspMote) mote;

    try {
      debug = ((MspMoteType)mspMote.getType()).getELF().getDebug();
    } catch (IOException e) {
      throw (IllegalStateException) new IllegalStateException("No debugging information available: " + mspMote).initCause(e);
    }

    debugSourceFiles = debug.getSourceFiles();
    if (debugSourceFiles == null) {
      throw new IllegalStateException("No source info found in firmware");
    }

    /* XXX Temporary workaround: removing source files duplicates awaiting Mspsim update */
    {
      ArrayList<String> newDebugSourceFiles = new ArrayList<String>();
      for (String sf: debugSourceFiles) {
        boolean found = false;
        for (String nsf: newDebugSourceFiles) {
          if (sf.equals(nsf)) {
            found = true;
            break;
          }
        }
        if (!found) {
          newDebugSourceFiles.add(sf);
        }
      }
      debugSourceFiles = newDebugSourceFiles.toArray(new String[0]);
    }
  }

  public void addTabs(JTabbedPane pane, WatchpointMote mote) {
    assCodeUI = new DebugUI(((MspMote)mote).getCPU(), true);
    for (Component c: assCodeUI.getComponents()) {
      c.setBackground(Color.WHITE);
    }
    pane.addTab("Instructions", null, assCodeUI, null);
  }

  public void showCurrentPC() {
    super.showCurrentPC();

    /* Instructions */
    assCodeUI.updateRegs();
    assCodeUI.repaint();
  }

  public String[] getSourceFiles() {
    return debugSourceFiles;
  }

  public void step(WatchpointMote watchpointMote) {
    try {
      mspMote.getCPU().stepInstructions(1);
    } catch (EmulationException ex) {
      logger.fatal("Error: ", ex);
    }
  }

  public int getCurrentPC() {
    return mspMote.getCPU().getPC();
  }
  public SourceLocation getSourceLocation(int pc) {
    DebugInfo debugInfo = debug.getDebugInfo(pc);
    if (debugInfo == null) {
      logger.warn("No source info at " + String.format("0x%04x", pc));
      return null;
    }
    return new SourceLocation(
        new File(debugInfo.getPath(), debugInfo.getFile()),
        debugInfo.getLine()
    );
  }

  public void startPlugin() {
    super.startPlugin();
  }

  public void closePlugin() {
    super.closePlugin();
  }

}
