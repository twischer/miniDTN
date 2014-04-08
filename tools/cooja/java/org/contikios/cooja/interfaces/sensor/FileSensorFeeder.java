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

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.text.ParseException;
import java.util.logging.Level;
import org.apache.log4j.Logger;
import org.contikios.cooja.MoteTimeEvent;
import org.contikios.cooja.Simulation;
import org.contikios.cooja.TimeEvent;
import org.jdom.Element;

/**
 * A data file is a simple csv where the first column is a stamp
 * and all other columns are sensor input data.
 * Lines starting with '#' are interpreted as comments.
 * <p>
 * After having set up file name etc. run <code>preParse()</code> to
 * get basic informations about data file and to check it for errors.
 * <p>
 * <code>activate()</code> will start scheduling events based on timestamps and
 * data.
 *
 * @author Enrico Joerns
 */
public class FileSensorFeeder extends AbstractSensorFeeder {

  private static final String DELIMITER_TOKEN = ",";
  private static final String COMMENT_TOKEN = "#";

  private static final Logger logger = Logger.getLogger(FileSensorFeeder.class);

  final Sensor sensor;
  private File datafile;
  BufferedReader fileReader;
  private TimeEvent fileFeederEvent;
  /* Enables/Disables looped data file parsing, i.e. reload file when reachin EOF */
  boolean loop = false;

  // pre-parse information
  /* Number of data lines in file */
  int dataLines;
  int dataColumns;
  long startTime;
  long endTime;
  // runtime information
  long lastStamp, nextScheduleTime;
  double[] lineValues;

  public FileSensorFeeder(final Sensor sensor) {
    super(sensor);
    this.sensor = sensor;
  }

  public void setDataFile(File file) {
    datafile = file;
  }

  /**
   *
   * @param loop
   */
  public void setLoopEnabled(boolean loop) {
    System.out.println("setLoopeEnabled(): " + loop);
    this.loop = loop;
  }

  /**
   * Returns number of data lines found in file.
   * <p>
   * Note: Requires preParse to be executed!
   *
   * @return number of lines
   */
  public int getDataLines() {
    return dataLines;
  }

  /**
   * Returns number of data columns found in file.
   * <p>
   * Note: Requires preParse to be executed!
   *
   * @return number of columns
   */
  public int getDataColumnCount() {
    return dataColumns;
  }
  
  /**
   * Returns first time in file.
   *
   * @return
   */
  public long getStartTime() {
    return startTime;
  }

  /**
   * Returns last time in file.
   *
   * @return
   */
  public long getEndTime() {
    return endTime;
  }
  
  @Override
  public Element getConfigXML() {

    Element root = super.getConfigXML();
    
    /* Store data file path */
    Element element = new Element("filename");
    /* use cooja's way to get portable path name */
    File file = sensor.getMote().getSimulation().getCooja().createPortablePath(datafile);
    element.setText(file.getPath().replaceAll("\\\\", "/"));
    element.setAttribute("EXPORT", "copy");
    root.addContent(1, element);
    /* Store loop setting */
    root.addContent(2, new Element("loop").setText(String.valueOf(loop)));

    return root;
  }

  @Override
  public boolean setConfigXML(Element root) {
    
    /* Read and set data file */
    String fileValue = root.getChildText("filename");
    if (fileValue == null) {
      logger.error("Failed parsing required element 'filename'. Aborting");
      return false;
    }
    File dataFile = new File(root.getChildText("filename"));
    if (!dataFile.exists()) {
      dataFile = sensor.getMote().getSimulation().getCooja().restorePortablePath(dataFile);
    }
    setDataFile(dataFile);
    
    /* Read and set loop parameter */
    String loopValue = root.getChildText("loop");
    if (loopValue == null) {
      logger.warn("Failed parsing element 'loop'. Assume false.");
      setLoopEnabled(false);
    }
    else {
      setLoopEnabled(Boolean.parseBoolean(loopValue));
    }

    // XXX replace with test code?
    /* iterate over each channel */
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
      FileFeederParameter param = new FileFeederParameter(); // XXX custom code
      param.setConfigXML(((Element) ec).getChild("parameter"));

      setupForChannel(channelNr, param);
    }
    
    // finally commit newly created feeder
    commit();

    return true;
  }
  
  // XXX Test code, move to super class
  public void setChannelByXML(Class<? extends FeederParameter> paramClass, Element root) throws InstantiationException, IllegalAccessException {
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
      FeederParameter param;
      param = paramClass.newInstance();
      param.setConfigXML(((Element) ec).getChild("parameter"));

      setupForChannel(channelNr, param);
    }
  }

  public class DataFileParseException extends RuntimeException {

    public DataFileParseException(String msg) {
      super(msg);
    }

    public DataFileParseException(int line, String msg) {
      super("Line " + line + ": " + msg);
    }

  }

  /**
   * Pre-parses the file to get information about data columns etc.
   * <p>
   * Sets:
   * <ul>
   * <li>columnMask</li>
   * </ul>
   */
  public void preParse() throws DataFileParseException {
    BufferedReader fr;
    try {
      fr = new BufferedReader(new FileReader(datafile));
    }
    catch (FileNotFoundException ex) {
      throw new DataFileParseException(ex.getMessage());
    }

    int dataColumns_ = -1;
    long lastStamp_ = 0;
    String line;
    int line_ = 0;
    int comment_lines = 0;
    boolean firstDataLine = true;
    try {
      while ((line = fr.readLine()) != null) {
        line_++;
        /* ignore comment lines (starting with '#') */
        if (line.startsWith(COMMENT_TOKEN)) {
          comment_lines++;
          continue;
        }
        String[] tokens = line.split(DELIMITER_TOKEN);
        long currStamp_ = Long.parseLong(tokens[0]);

        /* Test if nr of columns is consistent */
        if (dataColumns_ > 0 && tokens.length - 1 != dataColumns_) {
          throw new DataFileParseException(line_, "Inconsistent number of columns");
        }
        dataColumns_ = tokens.length - 1;

        /* Test if nr of columns is minimum number of channels for this sensor */
        if (dataColumns_ < sensor.numChannels()) {
          throw new DataFileParseException(
                  "Not enough data columns for this sensor ("
                  + "found: " + dataColumns_
                  + ", required: " + sensor.numChannels()
                  + ")");
        }

        /* Test if time stamp order is chronological */
        if (currStamp_ < lastStamp_) {
          throw new DataFileParseException(line_, "Non-chronological timestamp data");
        }
        lastStamp_ = currStamp_;
        
        /* Save first / last time stamp */
        if (firstDataLine) {
          startTime = currStamp_;
          firstDataLine = false;
        } else {
          endTime = currStamp_;
        }

        /* Test for invalid data */
        for (int col = 1; col <= dataColumns_; col++) {
          try {
            Double.parseDouble(tokens[col]);
          }
          catch (NumberFormatException ex) {
            throw new DataFileParseException(line_, "Invalid data in column " + col + ": '" + tokens[col] + "'");
          }
        }

      }
    }
    catch (IOException ex) {
      throw new DataFileParseException(ex.getMessage());
    }
    finally {
      try {
        fr.close();
      }
      catch (IOException ex) {
        logger.error(ex);
      }
    }
    logger.info(String.format(
            "preParsed %d lines (%d data, %d comments)", line_, line_ - comment_lines, comment_lines));
    /* Set pre parse info */
    dataLines = line_ - comment_lines;
    /* setup column mapping. */
    dataColumns = dataColumns_;
  }

  @Override
  public void commit() {
    if (fileReader != null || fileFeederEvent != null) {
      throw new RuntimeException("Reactivating already activated feeder is not allowed!");
//      logger.warn("Reactivating feeder");
//      fileFeederEvent.remove();
//      lastStamp = 0;
    }

    /* open file for parsing */
    try {
      fileReader = new BufferedReader(new FileReader(datafile));
    }
    catch (FileNotFoundException ex) {
      logger.error(ex);
      return;
    }

    final Simulation sim = sensor.getMote().getSimulation();
    /* Create periodic event */
    fileFeederEvent = new MoteTimeEvent(sensor.getMote(), 0) {
      @Override
      public void execute(long t) {
        System.out.println("fileFeederEvent at " + t);
        /* feed with data if available */
        if (lineValues != null) {
          for (int idx = 0; idx < sensor.numChannels(); idx++) {
            if (!isEnabled(idx)) {
              continue;
            }
            int column = ((FileFeederParameter) getParameter(idx)).column; // XXX This is the custom code!
            sensor.setValue(idx, lineValues[column]);
          }
        }
        /* try to parse next line */
        try {
          boolean success = parseNext();
          if (!success) {
            logger.info("Stopped data file parsing");
            return;
          }
        }
        catch (ParseException ex) {
          java.util.logging.Logger.getLogger(FileSensorFeeder.class.getName()).log(Level.SEVERE, null, ex);
          return;
        }

        /* reschedule execution */
        if (nextScheduleTime < 0) {
          logger.warn("Abort scheduling due to zero or negative interval " + nextScheduleTime);
          return;
        }
        sim.scheduleEvent(this, t + nextScheduleTime * 1000); /* schedule is in us */

      }
    };

    /* schedule new event at currStamp */
    sim.invokeSimulationThread(new Runnable() {
      @Override
      public void run() {
        /* Fixed reference! Thus the current set event may be scheduled multiple times! */
        sim.scheduleEvent(fileFeederEvent, sim.getSimulationTime());
      }
    });
  }

  @Override
  public void deactivate() {
    if (fileReader != null) {
      try {
        fileReader.close();
      }
      catch (IOException ex) {
        java.util.logging.Logger.getLogger(FileSensorFeeder.class.getName()).log(Level.SEVERE, null, ex);
      }
      fileReader = null;
    }
    lineValues = null;
    fileFeederEvent.remove();
    fileFeederEvent = null;
  }

  /**
   * Parses next line for time stamp and data.
   *
   * Updates currStamp to be stamp of line parsed.
   * Sets lastStamp to stamp of last line
   *
   * @return true if parsing next line succeeded, false if note
   */
  private boolean parseNext() throws ParseException {
    /* read next line */
    String line;
    try {
      line = fileReader.readLine();
      /* Test for EOF */
      if (line == null) {
        fileReader.close();
        /* if loop enabled, reopen file */
        if (!loop) {
          logger.info("Reached end of file: closing.");
          return false;
        }
        logger.info("Reached end of file: reload");
        fileReader = new BufferedReader(new FileReader(datafile));
        line = fileReader.readLine();
        lastStamp = 0;
      }
    }
    catch (IOException ex) {
      logger.error(ex);
      return false;
    }

    /* skip comments */
    if (line.startsWith(COMMENT_TOKEN)) {
      System.out.println("Comment: " + line);
      return true;
    }
    
    String[] tokens = line.split(DELIMITER_TOKEN);

    if (lineValues == null) {
      lineValues = new double[tokens.length - 1];
    }
    /* check number of tokens */
    if (lineValues.length != tokens.length - 1) {
      logger.error("Inconsistent data token column count");
      return false;
    }
    /* parse line tokens */
    for (int idx = 0; idx < tokens.length; idx++) {
      if (idx == 0) {
        /* read stamp and calculate next event time */
        try {
          long currStamp = Long.parseLong(tokens[0]);
          nextScheduleTime = (currStamp - lastStamp);
          lastStamp = currStamp;
        }
        catch (NumberFormatException ex) {
          logger.error("Failed parsing data row. Caused by: " + ex.getMessage());
          throw new ParseException("Failed parsing time stamp", idx);
        }
      }
      else {
        /* read data */
        try {
          lineValues[idx - 1] = Double.parseDouble(tokens[idx]);
          System.out.print("[" + Double.parseDouble(tokens[idx]) + "] ");
        }
        catch (NumberFormatException ex) {
          logger.error("Failed parsing data row. Caused by: " + ex.getMessage());
          throw new ParseException("Failed parsing data", idx);
        }
      }
    }
    System.out.println();
    return true;
  }

  @Override
  public String toString() {
    return "File Feeder";
  }

  @Override
  public String getName() {
    return "File";
  }

  public static class FileFeederParameter implements FeederParameter {

    private int column;

    public FileFeederParameter() {
      column = 0;
    }

    public FileFeederParameter(int column) {
      this.column = column;
    }

    @Override
    public Element getConfigXML() {
      Element element = new Element("parameter");
      element.setAttribute("column", Integer.toString(column));

      return element;
    }

    @Override
    public boolean setConfigXML(Element configXML) {
      if (!configXML.getName().equals("parameter")) {
        logger.error("found " + configXML.getName() + " but 'parameter' expected here!");
        return false;
      }

      this.column = Integer.parseInt(configXML.getAttributeValue("column"));
      return true;
    }
  }

}
