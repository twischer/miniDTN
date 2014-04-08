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
package org.contikios.cooja.mspmote.interfaces.sensors;

import org.contikios.cooja.interfaces.sensor.Sensor.Channel;
import org.contikios.cooja.interfaces.sensor.SensorAdapter;

/**
 * Interface to the MSPSim SHT11 implementation.
 *
 * @author Enrico Joerns
 */
public class SHT11 implements SensorAdapter {

  /* data channels */
  private static final int HUMIDITY = 0;
  private static final int TEMPERATURE = 1;

  private final se.sics.mspsim.chip.SHT11 sht11;

  Channel[] channels = new Channel[]{
    new Channel("Humidity"),
    new Channel("Temperature")
  };
  
  /**
   * Create new SHT11 sensor.
   * @param sht11 Reference to mspsim's SHT11
   */
  public SHT11(/*Mote mote, */se.sics.mspsim.chip.SHT11 sht11) {
//    super(mote);
    this.sht11 = sht11;
  }

  @Override
  public String getName() {
    return SHT11.class.getSimpleName();
  }

  /**
   * Get data from the SHT11
   *
   * @param channel One of HUMIDITY, TEMPERATURE
   * @return Value read from sensor
   */
  @Override
  public double getValue(int channel) {
    switch (channel) {
      case HUMIDITY:
        return sht11.getHumidity();
      case TEMPERATURE:
        return sht11.getTemperature();
      default:
        return -1;
    }
  }

  /**
   * Set data of the SHT11
   * 
   * @param channel One of HUMIDITY, TEMPERATURE
   * @param value Value to set
   */
  @Override
  public void setValue(int channel, double value) {
    switch (channel) {
      case HUMIDITY:
        sht11.setHumidity((int) value);
        break;
      case TEMPERATURE:
        sht11.setTemperature((int) value);
        break;
      default:
        break;
    }
  }

//  @Override
//  public int numChannels() {
//    return channels.length;
//  }

  @Override
  public Channel[] getChannels() {
    return channels;
  }
}
