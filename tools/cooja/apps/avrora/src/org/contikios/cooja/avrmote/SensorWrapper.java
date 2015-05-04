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

package org.contikios.cooja.avrmote;

import avrora.sim.platform.sensors.PushSensorSource;
import org.contikios.cooja.interfaces.sensor.Sensor.Channel;
import org.contikios.cooja.interfaces.sensor.Sensor.DataModel;
import org.contikios.cooja.interfaces.sensor.Sensor.RangeDataModel;
import org.contikios.cooja.interfaces.sensor.SensorAdapter;

/**
 * Wraps an Avrora sensor into a Cooja sensor.
 * 
 * Uses a range data model.
 *
 * @author Enrico Joerns
 */
public class SensorWrapper implements SensorAdapter {
  
  final avrora.sim.platform.sensors.Sensor sensor;
  final PushSensorSource source = new PushSensorSource();
  final Channel[] channels;
  final double[] values;
  
  public SensorWrapper(avrora.sim.platform.sensors.Sensor sensor) {
    this.sensor = sensor; 
    this.sensor.setSensorSource(source);
    values = new double[this.sensor.getChannels().length];
    /* create Cooja channels from Avrora channels */
    channels = new Channel[this.sensor.getChannels().length];
    for (int idx = 0; idx < channels.length; idx++) {
      avrora.sim.platform.sensors.Sensor.Channel ch = this.sensor.getChannels()[idx];
      channels[idx] = new Channel(ch.name, ch.unit, new RangeDataModel(ch.lbound, ch.ubound, DataModel.STRATEGY_LOG_WARN));
    }
  }

  @Override
  public String getName() {
    return sensor.getClass().getSimpleName();
  }
  
  @Override
  public double getValue(int channel) {
    throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
  }

  @Override
  public void setValue(int channel, double value) {
    values[channel] = value;
    source.setSensorData(values);
  }
  
  @Override
  public Channel[] getChannels() {
    return channels;
  }
  
}
