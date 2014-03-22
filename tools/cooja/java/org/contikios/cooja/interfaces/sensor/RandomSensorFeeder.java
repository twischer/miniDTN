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

import java.util.Random;
import org.apache.log4j.Logger;
import org.contikios.cooja.MoteTimeEvent;
import org.contikios.cooja.Simulation;
import org.contikios.cooja.TimeEvent;
import org.jdom.Element;

/**
 *
 * @author Enrico Jorns
 */
public class RandomSensorFeeder extends AbstractSensorFeeder {

  private static final Logger logger = Logger.getLogger(RandomSensorFeeder.class);

  final Sensor sensor;
  final Random random;
  /* Interval between to feed invokations [ms] */
  long interval;
  private TimeEvent randomFeederEvent;

  public RandomSensorFeeder(Sensor sensor) {
    super(sensor);
    this.sensor = sensor;
    this.random = new Random();
  }

  public void setSeed(long seed) {
    random.setSeed(seed);
  }

  public void setInterval(long interval) {
    this.interval = interval;
  }

  @Override
  public void commit() {
    final Simulation sim = sensor.getMote().getSimulation();
    /* Trigger initally */
    if (randomFeederEvent != null) {
      logger.warn("Ignoring activation of Feeder already activated");
      return;
    }
    sim.invokeSimulationThread(new Runnable() {
      @Override
      public void run() {
        /* Fixed reference! Thus the current set event may be scheduled multiple times! */
        sim.scheduleEvent(randomFeederEvent, sim.getSimulationTime());
      }
    });
    /* Create periodic event */
    randomFeederEvent = new MoteTimeEvent(sensor.getMote(), 0) {
      @Override
      public void execute(long t) {
        for (int idx = 0; idx < sensor.numChannels(); idx++) {
          if (!isEnabled(idx)) {
            continue;
          }
          /* updateChannel sensor with the new value */
          RandomFeederParameter param = (RandomFeederParameter) getParameter(idx);
          double value = (param.ubound - param.lbound) * random.nextDouble() + param.lbound; // XXX This is the custom code!
          sensor.setValue(idx, value);
        }
        if (interval <= 0) {
          logger.warn("Abort scheduling due to zero or negative interval " + interval);
          return;
        }
        sim.scheduleEvent(this, t + interval * 1000);
      }
    };
  }

  @Override
  public void deactivate() {
    logger.info("Deactivate " + getClass().getSimpleName());
    /* XXX right way? */
    if (randomFeederEvent != null) {
      randomFeederEvent.remove();
      randomFeederEvent = null;
    }
  }

  @Override
  public String toString() {
    return "Random Feeder @" + (1000 / interval) + "Hz";
  }

  @Override
  public String getName() {
    return "Random";
  }

  @Override
  public Element getConfigXML() {
    Element e = super.getConfigXML();
    // add "interval"
    e.addContent(new Element("interval").setText(String.valueOf(interval)));

    return e;
  }

  @Override
  public boolean setConfigXML(Element root) {

    if (root == null) {
      logger.error("Root element 'feeder' not found");
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
      RandomFeederParameter param = new RandomFeederParameter();
      param.setConfigXML(((Element) ec).getChild("parameter"));
      
      setupForChannel(channelNr, param);
    }
    
    // set interval
    String intervalTxt = root.getChildText("interval");
    if (intervalTxt == null) {
      logger.warn("Interval not specified, will be set to default");
      setInterval(100);
    } else {
      setInterval(Integer.parseInt(intervalTxt));
    }
    
    // finally commit newly created feeder
    commit();

    return true;
  }

  public static class RandomFeederParameter implements FeederParameter {

    public double lbound;
    public double ubound;

    public RandomFeederParameter(double lbound, double ubound) {
      this.lbound = lbound;
      this.ubound = ubound;
    }

    public RandomFeederParameter() {
      this(0.0, 0.0);
    }

    @Override
    public Element getConfigXML() {
      Element element = new Element("parameter");
      element.setAttribute("lbound", Double.toString(lbound));
      element.setAttribute("ubound", Double.toString(ubound));

      return element;
    }

    @Override
    public boolean setConfigXML(Element configXML) {
      if (!configXML.getName().equals("parameter")) {
        logger.error("found " + configXML.getName() + " but 'parameter' expected here!");
        return false;
      }

      this.lbound = Double.parseDouble(configXML.getAttributeValue("lbound"));
      this.ubound = Double.parseDouble(configXML.getAttributeValue("ubound"));
      return true;
    }
  }

}
