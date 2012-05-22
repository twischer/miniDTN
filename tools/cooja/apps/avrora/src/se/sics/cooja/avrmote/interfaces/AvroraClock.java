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

package se.sics.cooja.avrmote.interfaces;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Observable;
import java.util.Observer;

import java.util.Collection;
import java.util.Vector;

import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JToggleButton;
import javax.swing.JLabel;
import javax.swing.JPanel;

import org.apache.log4j.Logger;
import org.jdom.Element;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.Mote;
import se.sics.cooja.Simulation;
import se.sics.cooja.avrmote.AvroraMote;
import se.sics.cooja.interfaces.Clock;

import avrora.sim.FiniteStateMachine;
import avrora.sim.AtmelInterpreter;
import avrora.sim.mcu.DefaultMCU;
import avrora.sim.Simulator;
import avrora.sim.State;

/**
 * @author Fredrik Osterlind, Joakim Eriksson, David Kopf
 */
@ClassDescription("Cycle clock")
public class AvroraClock extends Clock {
  private static Logger logger = Logger.getLogger(AvroraClock.class);

  private Simulation mySimulation;
  private AtmelInterpreter myInterpreter;
  private FiniteStateMachine myFSM;

  private long timeDrift; /* Microseconds */
  private long startTime,lastTime,lastCycles;
  
  public AvroraClock(Mote mote) {
    mySimulation = mote.getSimulation();
    myInterpreter = (AtmelInterpreter)((AvroraMote)mote).CPU.getSimulator().getInterpreter();
    myFSM = ((DefaultMCU)((AvroraMote)mote).CPU.getSimulator().getMicrocontroller()).getFSM();
    startTime = mySimulation.getSimulationTime();
    lastTime = startTime;
    lastCycles = 0;
    timeDrift = 0;
  }

  public void setTime(long newTime) {
    logger.fatal("Can't change emulated CPU time");
  }

  public long getTime() {
    return mySimulation.getSimulationTime() + timeDrift;
  }

  public void setDrift(long drift) {
    timeDrift = drift;
  }

  public long getDrift() {
    return timeDrift;
  }

  JLabel timeLabel, cyclesLabel, stateLabel, watchLabel;
  Simulator.Probe liveProbe = null;
  
  private void updatePanel() {
    timeLabel.setText("Run Time  : " + ((double)(mySimulation.getSimulationTime()-startTime)/1000000) + "   Drift: " + (double)timeDrift/1000000);
    watchLabel.setText("Stopwatch : " + ((double)(mySimulation.getSimulationTime()-lastTime)/1000000));
    long cycleCount = myInterpreter.getState().getCycles();
    cyclesLabel.setText("Cycles        : " + (cycleCount - lastCycles));   
    stateLabel.setText("PC: 0x" +  Integer.toHexString(myInterpreter.getState().getPC()) + "      SP: 0x" +  Integer.toHexString(myInterpreter.getSP())
     + "     State: " + myFSM.getStateName(myFSM.getCurrentState()));
  }

  public JPanel getInterfaceVisualizer() {
    JPanel panel = new JPanel(new BorderLayout());
    Box boxw = Box.createVerticalBox();
    Box boxe = Box.createVerticalBox();

    timeLabel = new JLabel("");
    final JLabel dummyLabel = new JLabel(" ");
    watchLabel = new JLabel("");
    cyclesLabel = new JLabel("");
    stateLabel = new JLabel("");


    final JButton updateButton = new JButton("Update");
    final JButton resetButton = new JButton("Reset");
    final JToggleButton liveButton = new JToggleButton("Live",false);

    boxw.add(timeLabel);
    boxw.add(watchLabel);
    boxw.add(cyclesLabel);
    boxw.add(dummyLabel);
    boxw.add(stateLabel);
    boxe.add(updateButton);
    boxe.add(resetButton);
    boxe.add(liveButton);
    
    updatePanel();
    // increase initial width to prevent later scrollbar
    stateLabel.setText(stateLabel.getText() + "              ");
 
    updateButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            updatePanel();
        }
    });
    resetButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            lastTime = mySimulation.getSimulationTime();
            lastCycles = myInterpreter.getState().getCycles();
            updatePanel();
        }
    });
    liveButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            if (liveProbe == null) {
                liveProbe = new Simulator.Probe() {
                    public void fireBefore(State state, int pc) {
                    }
                    public void fireAfter(State state, int pc) {
                        updatePanel();
                    }
                };
                myInterpreter.insertProbe(liveProbe);
            } else {
                myInterpreter.removeProbe(liveProbe);
                liveProbe = null;
            }
        }
    });

    Observer observer;
	this.addObserver(observer = new Observer() {
		public void update(Observable obs, Object obj) {
		}
	});
 //  observer.update(null, null);

	panel.putClientProperty("intf_obs", observer);
    panel.add(BorderLayout.WEST, boxw);
    panel.add(BorderLayout.EAST, boxe);
	return panel;
	}

	public void releaseInterfaceVisualizer(JPanel panel) {
		Observer observer = (Observer) panel.getClientProperty("intf_obs");
		if (observer == null) {
			logger.fatal("Error when releasing panel, observer is null");
			return;
		}
        if (liveProbe != null) myInterpreter.removeProbe(liveProbe);
		this.deleteObserver(observer);
	}

  public Collection<Element> getConfigXML() {
    return null;
  }

  public void setConfigXML(Collection<Element> configXML, boolean visAvailable) {
  }
}
