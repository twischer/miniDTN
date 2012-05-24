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

package se.sics.cooja.avrmote.plugins;

import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Observable;
import java.util.Observer;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.GUI;
import se.sics.cooja.Mote;
import se.sics.cooja.MotePlugin;
import se.sics.cooja.PluginType;
import se.sics.cooja.Simulation;
import se.sics.cooja.SupportedArguments;
import se.sics.cooja.VisPlugin;
import se.sics.cooja.avrmote.AvroraMote;
import avrora.sim.AtmelInterpreter;

@ClassDescription("Avr Cycle Watcher")
@PluginType(PluginType.MOTE_PLUGIN)
@SupportedArguments(motes = {AvroraMote.class})
public class AvrCycleWatcher extends VisPlugin implements MotePlugin {
  private Mote avrMote;
  private AtmelInterpreter myInterpreter;
  private Simulation simulation;
  private Observer simObserver = null;
  private JTextField cycleTextField = new JTextField("");
  private JTextField resetTextField = new JTextField("");
  private long cycleReset = 0;

  public AvrCycleWatcher(Mote mote, Simulation simulationToVisualize, GUI gui) {
    super("Avr Cycle Watcher", gui);
    this.avrMote =  mote;
    simulation = simulationToVisualize;
    myInterpreter = (AtmelInterpreter)((AvroraMote)mote).CPU.getSimulator().getInterpreter();
    cycleTextField.setEditable(false);
    resetTextField.setEditable(false);

    getContentPane().setLayout(new BorderLayout());

    JButton updateButton = new JButton("Update");
    updateButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        updateLabels();
      }
    });

    JButton resetButton = new JButton("Reset");
    resetButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        cycleReset = myInterpreter.getState().getCycles();
        updateLabels();
      }
    });

    JPanel controlPanel = new JPanel(new GridLayout(2,3,5,5));
    controlPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
    controlPanel.add(new JLabel("Cycle counter:"));
    controlPanel.add(cycleTextField);
    controlPanel.add(updateButton);
    controlPanel.add(new JLabel("Since reset:"));
    controlPanel.add(resetTextField);
    controlPanel.add(resetButton);

    add(BorderLayout.CENTER, controlPanel);

    setSize(370, 100);

    simulation.addObserver(simObserver = new Observer() {
      public void update(Observable obs, Object obj) {
        updateLabels();
      }
    });

    updateLabels();

    // Tries to select this plugin
    try {
      setSelected(true);
    } catch (java.beans.PropertyVetoException e) {
      // Could not select
    }
  }

  private void updateLabels() {
    long cycleCount = myInterpreter.getState().getCycles();
    cycleTextField.setText("" + cycleCount);
    resetTextField.setText("" + (cycleCount - cycleReset));
  }

  public void closePlugin() {
    simulation.deleteObserver(simObserver);
  }

  public Mote getMote() {
    return avrMote;
  }

}
