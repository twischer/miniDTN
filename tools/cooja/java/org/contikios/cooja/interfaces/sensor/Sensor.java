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

import java.util.Set;
import org.apache.log4j.Logger;
import org.contikios.cooja.Mote;

/**
 *
 * @author Enrico Jorns
 */
public class Sensor {

  private final Mote mote;
  private final SensorAdapter sensor;
  private static final Logger logger = Logger.getLogger(Sensor.class);

  public Sensor(Mote mote, SensorAdapter sensor) {
    this.mote = mote;
    this.sensor = sensor;
  }

  /**
   * Returns the mote this sensor belongs to.
   *
   * @return Mote that owns this sensor.
   */
  public Mote getMote() {
    return mote;
  }

  /**
   * Returns name of this sensor.
   *
   * @return Name
   */
  public String getName() {
    return sensor.getName();
  }

  /**
   * Set value for a sensor channel
   *
   * @param channel Channel to set value for.
   * @param value Value to set
   */
  public final void setValue(int channel, double value) {
    System.out.println("Validating with model: " + getChannel(channel).getDataModel().getClass().getSimpleName());
    /* validate and forward to SensorAdapter */
    double retval = getChannel(channel).getDataModel().validate(value);
    sensor.setValue(channel, retval);
  }

  /**
   * Returns all channels of this sensor.
   *
   * @return All Channels of this sensor
   */
  public Channel[] getChannels() {
    return sensor.getChannels();
  }

  /**
   * Returns channel.
   *
   * @param channel index of channel to return
   * @return Channel at index
   */
  public Channel getChannel(int channel) {
    return sensor.getChannels()[channel];
  }
  
  /**
   * Returns number of channels for this sensor.
   *
   * @return Number of chanels
   */
  public int numChannels() {
    return sensor.getChannels().length;
  }

  public static class Channel<T> {

    public String name;
    public String unit;
    public DataModel model;
    public double default_ = 0.0;
    private AbstractSensorFeeder feeder = null;

    public Channel(String name, String unit, DataModel model) {
      this.name = name;
      this.unit = unit;
      this.model = model;
    }

    public Channel(String name) {
      this(name, null, new EmptyDataModel());
    }

    public DataModel getDataModel() {
      return model;
    }

    /* to be set by the feeder */
    public void setFeeder(AbstractSensorFeeder feeder) {
      this.feeder = feeder;
    }

    public AbstractSensorFeeder getFeeder() {
      return this.feeder;
    }
  }

  /**
   * Data model represents the input data a sensor accepts.
   *
   * They are closely related to the sensor specifications found in the
   * datasheet or other physical limitations.
   */
  public static abstract class DataModel {

    /**
     * If input value violates data model, nothing will happen.
     */
    public static final int STRATEGY_NONE = 0x01;
    /**
     * If input value violates data model, logger will print a warning.
     */
    public static final int STRATEGY_LOG_WARN = 0x02;
    /**
     * If input value violates data model, an expcetion is thrown.
     */
    public static final int STRATEGY_EXCEPTION = 0x04;
    /**
     * If input value violates data model, take the closest value.
     */
    public static final int STRATEGY_MAKE_VALID = 0x08;

    private int strategy;

    /**
     * Create new DataModel instance.
     *
     * @param strategy Strategy to use. Possible values are
     * STRATEGY_NONE, STRATEGY_LOG_WARN, STRATEGY_EXCEPTION,
     * STRATEGY_MAKE_VALID.
     * Values can be combined like (STRATEGY_LOG_WARN | STRATEGY_MAKE_VALID).
     * Note that STRATEGY_NONE supresses the other.
     */
    public DataModel(int strategy) {
      this.strategy = strategy;
    }

    /**
     * Create new DataModel instance.
     *
     * Strategy defaults to STRATEGY_LOG_WARN
     */
    public DataModel() {
      this(STRATEGY_LOG_WARN);
    }

    /**
     * Set strategy to use.
     *
     * @param strategy Strategy to use. Possible values are
     * STRATEGY_NONE, STRATEGY_LOG_WARN, STRATEGY_EXCEPTION,
     * STRATEGY_MAKE_VALID.
     * Values can be combined like (STRATEGY_LOG_WARN | STRATEGY_MAKE_VALID).
     * Note that STRATEGY_NONE supresses the other.
     */
    void setStrategy(int strategy) {
      this.strategy = strategy;
    }

    /**
     * Method to validate if input fits data model.
     *
     * @param data input data
     * @return true if input is isValid according to data model,
     * false otherwise
     */
    abstract boolean isValid(double data);

    /**
     * Returns message that indicates data is invalid for this model.
     *
     * @param data data that is known to be invalid
     * @return Message describing the reason for invalidity
     */
    abstract String invalidMsg(double data);

    /**
     * Returns closest valid data.
     *
     * @param data (invalid) input data
     * @return valid input data
     */
    abstract double makeValid(double data);

    /**
     *
     * @param data
     * @return
     */
    double validate(double data) {
      // if valid, immediately return original data
      if (isValid(data)) {
        return data;
      }
      // if strategy is NONE, also simply return
      if ((strategy & STRATEGY_NONE) != 0) {
        return data;
      }
      if ((strategy & STRATEGY_LOG_WARN) != 0) {
        logger.warn(invalidMsg(data));
      }
      if ((strategy & STRATEGY_EXCEPTION) != 0) {
        throw new DataModelExcpetion(invalidMsg(data));
      }
      if ((strategy & STRATEGY_MAKE_VALID) != 0) {
        return makeValid(data);
      }
      return data;
    }

  }

  /**
   * All values are isValid.
   */
  public static class EmptyDataModel extends DataModel {

    @Override
    public boolean isValid(double data) {
      return true;
    }

    @Override
    String invalidMsg(double data) {
      return "";
    }

    @Override
    double makeValid(double data) {
      return data;
    }
  }

  /**
   * Values need to e within a defined range.
   */
  public static class RangeDataModel extends DataModel {

    private final double lbound;
    private final double ubound;

    /**
     * Creates new RangeDataModel instance.
     *
     * @param lbound Lower bound
     * @param ubound Upper bound
     * @param strategy Strategy to use. Possible values are:
     * <code>DataModel.STRATEGY_NONE</code>,
     * <code>DataModel.STRATEGY_LOG_WARN</code>,
     * <code>DataModel.STRATEGY_EXCEPTION</code>,
     * <code>DataModel.STRATEGY_MAKE_VALID</code>.
     * <p>
     * Values can be combined like <code>(STRATEGY_LOG_WARN | STRATEGY_MAKE_VALID).</code>
     * Note that STRATEGY_NONE supresses the other.
     */
    public RangeDataModel(double lbound, double ubound, int strategy) throws DataModelExcpetion {
      super(strategy);
      if (lbound > ubound) {
        throw new DataModelExcpetion("Lower bound (" + lbound + ") set to be large than upper bound (" + ubound + ")");
      }
      this.lbound = lbound;
      this.ubound = ubound;
    }

    /**
     * Creates new RangeDataModel instance.
     *
     * @param lbound Lower bound
     * @param ubound Upper bound
     */
    public RangeDataModel(double lbound, double ubound) {
      this(lbound, ubound, STRATEGY_LOG_WARN);
    }

    @Override
    public boolean isValid(double data) {
      return data >= lbound && data <= ubound;
    }

    @Override
    public String invalidMsg(double data) {
      return String.format("Value %f not in range [%f : %f]", data, lbound, ubound);
    }

    /* if value exceeds bound, return bound */
    @Override
    double makeValid(double data) {
      if (data <= lbound) {
        return lbound;
      }
      else if (data >= ubound) {
        return ubound;
      }
      else {
        return data;
      }
    }

  }

  /**
   * Values need to be of a finite set.
   */
  public static class SetDataModel extends DataModel {

    private final Set<Double> set;

    public SetDataModel(Set<Double> set, int strategy) {
      super(strategy);
      this.set = set;
    }

    public SetDataModel(Set<Double> set) {
      this(set, STRATEGY_LOG_WARN);
    }

    @Override
    public boolean isValid(double data) {
      return set.contains(data);
    }

    @Override
    String invalidMsg(double data) {
      return "Value " + String.valueOf(data) + " not in set";
    }

    /* return entry of set that is closest to value */
    @Override
    double makeValid(double data) {
      Double closest = null;
      for (Double d : set) {
        if (closest == null) {
          closest = d;
        }
        else if (Math.abs(d - data) < Math.abs(closest - data)) {
          closest = d;
        }
      }
      return closest;
    }

  }

  /**
   * Values need to be integers (i.e. *.0).
   */
  public static class IntegerDataModel extends DataModel {

    public IntegerDataModel(int strategy) {
      super(strategy);
    }

    public IntegerDataModel() {
      this(STRATEGY_LOG_WARN);
    }

    @Override
    public boolean isValid(double data) {
      return (data == Math.floor(data)) && !Double.isInfinite(data);
    }

    @Override
    String invalidMsg(double data) {
      return String.valueOf(data) + "is not an integer";
    }

    /* round to next integer */
    @Override
    double makeValid(double data) {
      return Math.round(data);
    }

  }

  /**
   * Values are either 0.0 (false) or 1.0 (true).
   */
  public static class BooleanDataModel extends DataModel {

    @Override
    public boolean isValid(double data) {
      return data == 0.0 || data == 1.0;
    }

    @Override
    String invalidMsg(double data) {
      return String.valueOf(data) + " is neither 0.0 nor 1.0";
    }

    /* every value != 0.0 is assumed to be 1.0 */
    @Override
    double makeValid(double data) {
      return 1.0;
    }

  }

  /**
   * Exception that is thrown by data model validation.
   */
  public static class DataModelExcpetion extends RuntimeException {

    public DataModelExcpetion(String msg) {
      super(msg);
    }

  }

  /* XXX Configuration interface? */
}
