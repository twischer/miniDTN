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
 * Feeds the sensor channels with gauss distributed data.
 * <p>
 * <b>Parameters:</b>
 * <ul>
 * <li>Mean</li>
 * <li>Standard deviation</li>
 * </ul>
 *
 * @author Enrico Jorns
 */
public class GaussianSensorFeeder extends AbstractSensorFeeder {

  private static final Logger logger = Logger.getLogger(GaussianSensorFeeder.class);

  final Sensor sensor;
  final Random random;
  long interval;
  private TimeEvent gaussianFeederEvent;

  public GaussianSensorFeeder(Sensor sensor) {
    super(sensor);
    this.sensor = sensor;
    this.random = new Random();
  }

  /**
   * Set the seed the random generator uses for emitting values.
   *
   * @param seed
   */
  public void setSeed(long seed) {
    random.setSeed(seed);
  }

  /**
   * Set the interval in which a new value should be set
   *
   * @param interval Interval in ms
   */
  public void setInterval(long interval) {
    this.interval = interval;
  }

  @Override
  public void commit() {
    final Simulation sim = sensor.getMote().getSimulation();
    /* Trigger initally */
    if (gaussianFeederEvent != null) {
      // already running
      return;
    }

    /* Create periodic event */
    gaussianFeederEvent = new MoteTimeEvent(sensor.getMote(), 0) {
      @Override
      public void execute(long t) {
        for (int idx = 0; idx < sensor.numChannels(); idx++) {
          if (!isEnabled(idx)) {
            continue;
          }
          /* updateChannel sensor with the new value */
          GaussianFeederParameter param = (GaussianFeederParameter) getParameter(idx);
          double value = random.nextGaussian() * param.deviation + param.mean;
          sensor.setValue(idx, value); // XXX This is the custom code!
        }
        if (interval <= 0) {
          logger.warn("Abort scheduling due to zero or negative interval " + interval);
          return;
        }
        sim.scheduleEvent(this, t + interval * 1000);
      }
    };
    /* initially start scheduling */
    sim.invokeSimulationThread(new Runnable() {
      @Override
      public void run() {
        /* Fixed reference! Thus the current set event may be scheduled multiple times! */
        sim.scheduleEvent(gaussianFeederEvent, sim.getSimulationTime());
      }
    });
  }

  @Override
  public void deactivate() {
    logger.info("Deactivate " + getClass().getSimpleName());
    /* XXX right way? */
    if (gaussianFeederEvent != null) {
      gaussianFeederEvent.remove();
      gaussianFeederEvent = null;
    }
  }

  @Override
  public String toString() {
    return "Gaussian Feeder @" + (1000 / interval) + "Hz";
  }

  @Override
  public String getName() {
    return "Gaussian";
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
      GaussianFeederParameter param = new GaussianFeederParameter(); // XXX custom code
      param.setConfigXML(((Element) ec).getChild("parameter"));
      
      setupForChannel(channelNr, param);
    }
    
    // set interval
    String intervalTxt = root.getChildText("interval"); // XXX partly custom code
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

  /**
   * Holds parameters to setup a sensor feeder channel.
   */
  public static class GaussianFeederParameter implements FeederParameter {

    public double mean;
    public double deviation;

    public GaussianFeederParameter(double mean, double deviation) {
      this.mean = mean;
      this.deviation = deviation;
    }

    public GaussianFeederParameter() {
      this(0.0, 0.0);
    }
    
    @Override
    public Element getConfigXML() {
//      List<Element> elements = new LinkedList<>();
      Element element = new Element("parameter");
      element.setAttribute("mean", Double.toString(mean));
      element.setAttribute("deviation", Double.toString(deviation));
//      elements.add(element);

      return element;
    }

    @Override
    public boolean setConfigXML(Element configXML) {
//      Element [] elements = new Element[configXML.size()];
//      configXML.toArray(elements);
//      if (elements.length != 1) {
//        logger.warn("Only a single parameter is allowed here");
//      }
      if (!configXML.getName().equals("parameter")) {
        logger.error("found " + configXML.getName()+ " but 'parameter' expected here!");
        return false;
      }
      
      this.mean = Double.parseDouble(configXML.getAttributeValue("mean"));
      this.deviation = Double.parseDouble(configXML.getAttributeValue("deviation"));
      return true;
    }

  }

}
