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
 *
 */

package se.sics.cooja.avrmote;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;

import org.apache.log4j.Logger;
import org.jdom.Element;

import se.sics.cooja.Mote;
import se.sics.cooja.MoteInterface;
import se.sics.cooja.MoteInterfaceHandler;
import se.sics.cooja.MoteMemory;
import se.sics.cooja.MoteType;
import se.sics.cooja.Simulation;
import se.sics.cooja.motes.AbstractEmulatedMote;
import avrora.arch.avr.AVRProperties;
import avrora.core.LoadableProgram;
import avrora.sim.AtmelInterpreter;
import avrora.sim.Simulator;
import avrora.sim.State;
import avrora.sim.mcu.AtmelMicrocontroller;
import avrora.sim.mcu.EEPROM;
import avrora.sim.platform.MicaZ;
import avrora.sim.platform.Platform;
import avrora.sim.platform.PlatformFactory;

/**
 * @author Joakim Eriksson, Fredrik Osterlind, David Kopf
 */
public class AvroraMote extends AbstractEmulatedMote implements Mote {
  public static Logger logger = Logger.getLogger(AvroraMote.class);

  private MoteInterfaceHandler myMoteInterfaceHandler;
  public AtmelMicrocontroller CPU = null;
  public PlatformFactory FACTORY = null;
  public Platform PLATFORM = null;
  public String TARGET = "none";
  private LoadableProgram program = null;
  public AtmelInterpreter INTERPRETER = null;
  public AvrMoteMemory MEMORY = null;
  private AVRProperties avrProperties = null;
  public MoteType MOTETYPE = null;

  public EEPROM EEPROM = null;
  
  /* Stack monitoring variables */
  private boolean stopNextInstruction = false;

  public AvroraMote() {
    MOTETYPE = null;
    CPU = null;
    /* TODO myMemory = null; */
    myMoteInterfaceHandler = null;
  }

  public AvroraMote(Simulation simulation, MoteType type) {
  }

  public void getFactory() throws Exception {
  }
  
  protected void getAMote(Simulation simulation, MoteType type) {
    setSimulation(simulation);
    MOTETYPE = type;

    /* Schedule us immediately */
    requestImmediateWakeup();
  }
  

  protected boolean initEmulator(File fileELF) {
    try {
    program = new LoadableProgram(fileELF);
    program.load();
    // get the factory type from the super
    getFactory();
    PLATFORM = FACTORY.newPlatform(1, program.getProgram());
    CPU = (AtmelMicrocontroller) PLATFORM.getMicrocontroller();
    EEPROM = (EEPROM) CPU.getDevice("eeprom");
    
    avrProperties = (AVRProperties) CPU.getProperties();
    Simulator sim = CPU.getSimulator();
    INTERPRETER = (AtmelInterpreter) sim.getInterpreter();
    MEMORY = new AvrMoteMemory(program.getProgram().getSourceMapping(), avrProperties, INTERPRETER);
    } catch (Exception e) {
      logger.fatal("Error creating avrora "+ TARGET +" mote: ", e);
      return false;
    }
    return true;
  }

  /**
   * Abort current tick immediately.
   * May for example be called by a breakpoint handler.
   */
  public void stopNextInstruction() {
    stopNextInstruction = true;
  }

  private MoteInterfaceHandler createMoteInterfaceHandler() {
    return new MoteInterfaceHandler(this, getType().getMoteInterfaceClasses());
  }

  protected void initMote() {
    if (MOTETYPE != null) {
      initEmulator(MOTETYPE.getContikiFirmwareFile());
      myMoteInterfaceHandler = createMoteInterfaceHandler();
    } else logger.debug("MOTETYPE null");
  }

  public void setEEPROM(int address, int i) {
      byte[] eedata = EEPROM.getContent();
      eedata[address] = (byte) i;
  }
  
  public void setState(State newState) {
    logger.warn("Avrora motes can't change state");
  }

  public int getID() {

    if (getInterfaces() == null) {
        logger.debug("AvroraMote getInterfaces null");
        return 0;
    }
    return getInterfaces().getMoteID().getMoteID();
  }

  /* called when moteID is updated */
  public void idUpdated(int newID) {
      
  }

  public MoteType getType() {
    return MOTETYPE;
  }

  public void setType(MoteType type) {
    MOTETYPE = type;
  }

  public MoteInterfaceHandler getInterfaces() {
    return myMoteInterfaceHandler;
  }

  public void setInterfaces(MoteInterfaceHandler moteInterfaceHandler) {
    myMoteInterfaceHandler = moteInterfaceHandler;
  }

  private long cyclesExecuted = 0;
  private long cyclesUntil = 0;
  public void execute(long t) {
    /* Wait until mote boots */
    if (myMoteInterfaceHandler.getClock().getTime() < 0) {
      scheduleNextWakeup(t - myMoteInterfaceHandler.getClock().getTime());
      return;
    }

    if (stopNextInstruction) {
      stopNextInstruction = false;
      throw new RuntimeException("Avrora requested simulation stop");
    } 

    /* TODO Poll mote interfaces? */

    /* Execute one millisecond */
    cyclesUntil += this.getCPUFrequency()/1000;
    while (cyclesExecuted < cyclesUntil) {
      int nsteps = INTERPRETER.step();
      if (nsteps > 0) {
        cyclesExecuted += nsteps;
      } else {
        logger.debug("halted?");
        try{Thread.sleep(200);}catch (Exception e){System.err.println("avoraMote: " + e.getMessage());}
      }
    }

    /* TODO Poll mote interfaces? */

    /* Schedule wakeup every millisecond */
    /* TODO Optimize next wakeup time */
    scheduleNextWakeup(t + Simulation.MILLISECOND);
  }

  @SuppressWarnings("unchecked")
  public boolean setConfigXML(Simulation simulation, Collection<Element> configXML, boolean visAvailable) {
    setSimulation(simulation);
    initEmulator(MOTETYPE.getContikiFirmwareFile());
    myMoteInterfaceHandler = createMoteInterfaceHandler();

    for (Element element: configXML) {
      String name = element.getName();

      if (name.equals("motetype_identifier")) {
        /* Ignored: handled by simulation */
      } else if (name.equals("interface_config")) {
        Class<? extends MoteInterface> moteInterfaceClass = simulation.getGUI().tryLoadClass(
              this, MoteInterface.class, element.getText().trim());

        if (moteInterfaceClass == null) {
          logger.fatal("Could not load mote interface class: " + element.getText().trim());
          return false;
        }

        MoteInterface moteInterface = getInterfaces().getInterfaceOfType(moteInterfaceClass);
        moteInterface.setConfigXML(element.getChildren(), visAvailable);
      }
    }

    /* Schedule us immediately */
    requestImmediateWakeup();
    return true;
  }

  public Collection<Element> getConfigXML() {
    ArrayList<Element> config = new ArrayList<Element>();
    Element element;

    /* Mote interfaces */
    for (MoteInterface moteInterface: getInterfaces().getInterfaces()) {
      element = new Element("interface_config");
      element.setText(moteInterface.getClass().getName());

      Collection<Element> interfaceXML = moteInterface.getConfigXML();
      if (interfaceXML != null) {
        element.addContent(interfaceXML);
        config.add(element);
      }
    }

    return config;
  }

  public MoteMemory getMemory() {
    return MEMORY;
  }

  public void setMemory(MoteMemory memory) {
    MEMORY = (AvrMoteMemory) memory;
  }

  public String toString() {
    return "Avr " + getID();
  }
}
