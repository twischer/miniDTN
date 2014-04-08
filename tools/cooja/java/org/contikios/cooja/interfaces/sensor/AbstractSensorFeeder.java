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
import org.contikios.cooja.XMLExportable;
import org.jdom.Element;

/**
 * Abstract base class for sensor feeders.
 *
 */
public abstract class AbstractSensorFeeder implements XMLExportable {

  private static final Logger logger = Logger.getLogger(AbstractSensorFeeder.class);

  // sensor this feeder is registered for
  private final Sensor sensor;
  // feeder parameters for the different sensors channels
  private final FeederParameter[] params;
  // if this feeder is enabled for a sensor channel
  private final boolean[] enabled;

  public AbstractSensorFeeder(Sensor sensor) {
    this.sensor = sensor;
    this.params = new FeederParameter[this.sensor.numChannels()];
    this.enabled = new boolean[this.sensor.numChannels()];
  }

  /**
   * Commit feeder data.
   *
   * Timed feeders may start, manual feeders may schedule execution instantly
   *
   * @see getParameter
   */
  public abstract void commit();

  /**
   * Deactivates the feeder.
   * <p>
   * This functions stops all running timers, cleans up references etc.
   * <p>
   * Should be called by Sensor only.
   */
  abstract void deactivate();

  /**
   * True if this feeder is enabled for sensor channel.
   * <p>
   * Note: To be called by implementations.
   *
   * @param channel
   * @return
   */
  public boolean isEnabled(int channel) {
    return enabled[channel];
  }

  /**
   * Returns feeder parameter for channel or null if feeder is not
   * registered for this channel.
   * <p>
   * Note: To be called by implementations.
   *
   * @param channel
   * @return
   */
  public FeederParameter getParameter(int channel) {
    if (!enabled[channel]) {
      return null;
    }
    return params[channel];
  }

  /**
   * Sets up channel to use this feeder with given parameter.
   * <p>
   * A previously registered feeder will be release from the channel.
   *
   * @param channel Channel to setup
   * @param param Parameter to use for this registered feeder
   */
  public final void setupForChannel(int channel, FeederParameter param) {
    if (channel >= sensor.numChannels()) {
      logger.error("Sensor channel number " + channel + " out of range");
      return;
    }
    if (enabled[channel]) {
      logger.warn("Overwriting settings for already configured channel " + channel);
    }
    logger.info("Set up feeder '" + getName() + "' for channel " + channel);
    /* First release old feeder from this channel */
    if (sensor.getChannel(channel).getFeeder() != null) {
      sensor.getChannel(channel).getFeeder().releaseFromChannel(channel);
    }
    /* We register ourselve at the sensor */
    sensor.getChannel(channel).setFeeder(this); // XXX we may do this in commit...
    params[channel] = param;
    enabled[channel] = true;
  }

  /**
   * Releases this feeder from given channel.
   * <p>
   * If there is no channel left this feeder is registered for,
   * the feeder will be deactivated.
   * 
   * @param channel Channel to remove this feeder from
   */
  public final void releaseFromChannel(int channel) {
    logger.info("Release feeder '" + getName() + "' from channel " + channel);
    sensor.getChannel(channel).setFeeder(null);
    enabled[channel] = false;
    /* if any enabled, return */
    for (boolean en : enabled) {
      if (en) {
        return;
      }
    }
    /* if no more enabled, stop */
    deactivate();
  }

  /**
   * Releses this feder from all channels and deactivates it.
   */
  public void releaseAll() {
    for (Sensor.Channel ch : sensor.getChannels()) {
      if (ch.getFeeder().equals(this)) {
        ch.setFeeder(null);
      }
    }
    deactivate();
  }

  @Override
  public String toString() {
    return "Abstract Feeder";
  }

  /**
   * Returns simple and short name for the Feeder implementation.
   *
   * @return Name
   */
  public abstract String getName();

  /**
   * Creates <feeder>...</feeder> tag
   * @return 
   */
  @Override
  public Element getConfigXML() {
    Element feederElement = new Element("feeder");
    feederElement.addContent(getClass().getName());

    // save parameter type
    Class<? extends AbstractSensorFeeder.FeederParameter> paramClass = null;
    for (int idx = 0; idx < sensor.getChannels().length; idx++) {
      if (getParameter(idx) != null) {
        paramClass = getParameter(idx).getClass();
        break;
      }
    }
    if (paramClass == null) {
      logger.error("Could not get class for parameter type");
//          continue;
    }

    // save (only) channels associated to this feeder
    Element channelElement;
    for (int idx = 0; idx < sensor.getChannels().length; idx++) {
      if (sensor.getChannel(idx).getFeeder() != null && sensor.getChannel(idx).getFeeder().equals(this)) {
        channelElement = new Element("channel");
        channelElement.setAttribute("name", sensor.getChannel(idx).name);
        channelElement.addContent(getParameter(idx).getConfigXML());

        feederElement.addContent(channelElement);
      }
    }

    return feederElement;
  }
  
  /**
   *
   */
  public interface FeederParameter extends XMLExportable {

  }

}
