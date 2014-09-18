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
 *
 */

package org.contikios.cooja.avrmote.interfaces;

import java.util.Observable;
import java.util.Observer;
import javax.swing.JLabel;
import javax.swing.JPanel;
import org.apache.log4j.Logger;
import org.contikios.cooja.Mote;
import org.contikios.cooja.interfaces.MoteID;

import org.contikios.cooja.mote.memory.MemoryInterface;
import org.contikios.cooja.mote.memory.MemoryInterface.SegmentMonitor;
import org.contikios.cooja.mote.memory.VarMemory;

/**
 * @author Fredrik Osterlind
 */
public class AvroraMoteID extends MoteID {
  private static final Logger logger = Logger.getLogger(AvroraMoteID.class);
  
  // Contiki stuff...
  private static final String VARNAME_MOTE_ID = "node_id";
  private static final String VARNAME_RSEED = "rseed";
  // TinyOS stuff...
  private static final String VARNAME_TOS_NODE_ID = "TOS_NODE_ID";
  private static final String VARNAME_ACT_MSG_ADDRC__ = "ActiveMessageAddressC__addr";
  private static final String VARNAME_ACT_MSG_ADDRC = "ActiveMessageAddressC$addr";

  private final Mote mote;
  private VarMemory moteMem = null;

  private int moteID = -1;

  private SegmentMonitor memoryMonitor;

  /**
   * Creates an interface to the mote ID at mote.
   *
   * @param m
   * @see Mote
   * @see org.contikios.cooja.MoteInterfaceHandler
   */
  public AvroraMoteID(Mote m) {
    this.mote = m;
    this.moteMem = new VarMemory(mote.getMemory());
  }

  @Override
  public int getMoteID() {
    return moteID;
  }

  @Override
  public void setMoteID(int newID) {
    if (moteID != newID) {
      setChanged();
    }
    moteID = newID;

    writeID();
    
    /** Monitor used to overwrite id memorywith fixed id if modified externally */
    if (memoryMonitor == null) {
      memoryMonitor = new MemoryInterface.SegmentMonitor() {

        @Override
        public void memoryChanged(MemoryInterface memory, SegmentMonitor.EventType type, long address) {
          writeID();
        }
      };
      if (moteMem.variableExists(VARNAME_MOTE_ID)) {
        moteMem.addVarMonitor(SegmentMonitor.EventType.WRITE, VARNAME_MOTE_ID, memoryMonitor);
      }
      if (moteMem.variableExists(VARNAME_TOS_NODE_ID)) {
        moteMem.addVarMonitor(SegmentMonitor.EventType.WRITE, VARNAME_TOS_NODE_ID, memoryMonitor);
      }
      if (moteMem.variableExists(VARNAME_ACT_MSG_ADDRC__)) {
        moteMem.addVarMonitor(SegmentMonitor.EventType.WRITE, VARNAME_ACT_MSG_ADDRC__, memoryMonitor);
      }
      if (moteMem.variableExists(VARNAME_ACT_MSG_ADDRC)) {
        moteMem.addVarMonitor(SegmentMonitor.EventType.WRITE, VARNAME_ACT_MSG_ADDRC, memoryMonitor);
      }
    }

    notifyObservers();
  }

  private void writeID() {
    int id = getMoteID();
    if (moteMem.variableExists(VARNAME_MOTE_ID)) {
      System.out.println("Set MoteID to " + id);
      moteMem.setIntValueOf(VARNAME_MOTE_ID, id);

      /* Set Contiki random seed variable if it exists */
      if (moteMem.variableExists(VARNAME_RSEED)) {
        moteMem.setIntValueOf(VARNAME_RSEED, (int) (mote.getSimulation().getRandomSeed() + id));
      }
    }
    if (moteMem.variableExists(VARNAME_TOS_NODE_ID)) {
      moteMem.setIntValueOf(VARNAME_TOS_NODE_ID, id);
    }
    if (moteMem.variableExists(VARNAME_ACT_MSG_ADDRC__)) {
      moteMem.setIntValueOf(VARNAME_ACT_MSG_ADDRC__, id);
    }
    if (moteMem.variableExists(VARNAME_ACT_MSG_ADDRC)) {
      moteMem.setIntValueOf(VARNAME_ACT_MSG_ADDRC, id);
    }
  }

  @Override
  public void removed() {
    super.removed();
    if (memoryMonitor != null) {
      if (moteMem.variableExists(VARNAME_MOTE_ID)) {
        moteMem.removeVarMonitor(VARNAME_MOTE_ID, memoryMonitor);
      }
      if (moteMem.variableExists(VARNAME_TOS_NODE_ID)) {
        moteMem.removeVarMonitor(VARNAME_TOS_NODE_ID, memoryMonitor);
      }
      if (moteMem.variableExists(VARNAME_ACT_MSG_ADDRC__)) {
        moteMem.removeVarMonitor(VARNAME_ACT_MSG_ADDRC__, memoryMonitor);
      }
      if (moteMem.variableExists(VARNAME_ACT_MSG_ADDRC)) {
        moteMem.removeVarMonitor(VARNAME_ACT_MSG_ADDRC, memoryMonitor);
      }
      memoryMonitor = null;
    }
  }

  @Override
  public JPanel getInterfaceVisualizer() {

    JPanel panel = new JPanel();
    final JLabel idLabel = new JLabel();

    idLabel.setText("Mote ID: " + getMoteID());

    panel.add(idLabel);

    Observer observer;
    this.addObserver(observer = new Observer() {
      @Override
      public void update(Observable obs, Object obj) {
        idLabel.setText("Mote ID: " + getMoteID());
      }
    });

    panel.putClientProperty("intf_obs", observer);

    return panel;
  }

  @Override
  public void releaseInterfaceVisualizer(JPanel panel) {
  }
}
