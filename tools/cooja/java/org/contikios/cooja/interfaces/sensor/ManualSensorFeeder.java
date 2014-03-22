/*
 * Copyright (c) 2014, TU Braunschweig
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
package org.contikios.cooja.interfaces.sensor;

import org.apache.log4j.Logger;
import org.contikios.cooja.Simulation;
import org.contikios.cooja.TimeEvent;
import org.jdom.Element;

/**
 *
 * @author Enrico Jorns
 */
public class ManualSensorFeeder extends AbstractSensorFeeder {

  private static final Logger logger = Logger.getLogger(ManualFeederParameter.class);

  final Sensor sensor;

  public ManualSensorFeeder(Sensor sensor) {
    super(sensor);
    this.sensor = sensor;
  }

  @Override
  public void commit() {
    final Simulation sim = sensor.getMote().getSimulation();
    sim.invokeSimulationThread(new Runnable() {
      @Override
      public void run() {
        sim.scheduleEvent(new TimeEvent(0) {
          @Override
          public void execute(long t) {
            /* set all enabled channels */
            for (int idx = 0; idx < sensor.numChannels(); idx++) {
              if (!isEnabled(idx)) {
                continue;
              }
              double value = ((ManualFeederParameter) getParameter(idx)).value; // XXX This is the custom code!
              sensor.setValue(idx, value);
            }
          }
        }, sim.getSimulationTime());
      }
    });
  }

  @Override
  public void deactivate() {
    logger.info("Deactivate " + getClass().getSimpleName());
  }

  @Override
  public String getName() {
    return "Manual";
  }

  @Override
  public String toString() {
    return "Manual Feeder";
  }

  @Override
  public boolean setConfigXML(Element root) {

    if (root == null) {
      logger.error("Roote element 'feeder' not found");
      return false;
    }

    // iterate over each channel
    for (Object ec : root.getChildren("channel")) {
      String chName = ((Element) ec).getAttributeValue("name");
      // get channel number from channel name
      int channelNr = -1;
      for (int idx = 0; idx < sensor.getChannels().length; idx++) {
        if (sensor.getChannel(idx).name.equals(chName)) {
          channelNr = idx;
        }
      }
      if (channelNr == -1) {
        logger.error("Could not find and set up channel " + chName);
        continue;
      }

      //  setup parameter by xml
      ManualFeederParameter param = new ManualFeederParameter();
      param.setConfigXML(((Element) ec).getChild("parameter"));
      
      setupForChannel(channelNr, param);
    }

    // finally commit newly created feeder
    commit();

    return true;
  }

  public static class ManualFeederParameter implements FeederParameter {

    public double value;

    public ManualFeederParameter(double value) {
      this.value = value;
    }
    
    public ManualFeederParameter() {
      this(0.0);
    }

    @Override
    public Element getConfigXML() {
      Element element = new Element("parameter");
      element.setAttribute("value", Double.toString(value));

      return element;
    }

    @Override
    public boolean setConfigXML(Element configXML) {
      if (!configXML.getName().equals("parameter")) {
        logger.error("found " + configXML.getName() + " but 'parameter' expected here!");
        return false;
      }

      this.value = Double.parseDouble(configXML.getAttributeValue("value"));
      return true;
    }

  }

}
