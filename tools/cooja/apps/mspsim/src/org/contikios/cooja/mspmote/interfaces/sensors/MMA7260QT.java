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
 *
 * @author Enrico Joerns
 */
public class MMA7260QT implements SensorAdapter {

  public static final int X = 0;
  public static final int Y = 1;
  public static final int Z = 2;

  private final se.sics.mspsim.chip.MMA7260QT mma7260;

  private static final Channel[] channels = new Channel[]{
    new Channel("x"),
    new Channel("y"),
    new Channel("z")
  };

  /**
   * Create new MMA7260QT sensor.
   *
   * @param mma7260 Reference to mspsim's MMA7260QT
   */
  public MMA7260QT(/*Mote mote, */se.sics.mspsim.chip.MMA7260QT mma7260) {
//    super(mote);
    this.mma7260 = mma7260;
  }

  @Override
  public String getName() {
    return MMA7260QT.class.getSimpleName();
  }

  /**
   * Read ADC channel
   *
   * @param channel One of X, Y, Z
   * @return
   */
  @Override
  public double getValue(int channel) {
    switch (channel) {
      case X:
        return mma7260.getADCX();
      case Y:
        return mma7260.getADCY();
      case Z:
        return mma7260.getADCZ();
      default:
        return -1.0;
    }
  }

  @Override
  public void setValue(int channel, double value) {
    switch (channel) {
      case X:
        mma7260.setX(value);
        break;
      case Y:
        mma7260.setY(value);
        break;
      case Z:
        mma7260.setZ(value);
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
