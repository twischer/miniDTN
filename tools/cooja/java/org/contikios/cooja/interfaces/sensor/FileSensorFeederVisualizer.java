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

import java.awt.BorderLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.util.LinkedList;
import java.util.List;
import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.filechooser.FileNameExtensionFilter;
import org.apache.log4j.Logger;
import org.contikios.cooja.ClassDescription;
import org.contikios.cooja.Cooja;
import org.contikios.cooja.HasQuickHelp;
import org.contikios.cooja.interfaces.SensorInterface;
import org.contikios.cooja.interfaces.sensor.FileSensorFeeder.DataFileParseException;
import org.contikios.cooja.interfaces.sensor.FileSensorFeeder.FileFeederParameter;

/**
 *
 * @author Enrico Jorns
 */
@ClassDescription(value = "File Feeder")
public class FileSensorFeederVisualizer extends AbstractSensorFeederVisualizer implements HasQuickHelp {
  private static final Logger logger = Logger.getLogger(FileSensorFeederVisualizer.class);
  private final SensorInterface.SensorFeederConfigFrame configurator;
  private final JFileChooser dataFileChooser;
  private final DataFileInfoPanel dataFileInfoPanel;
  private FileSensorFeeder feeder;

  public FileSensorFeederVisualizer(SensorInterface.SensorFeederConfigFrame configurator, final Sensor sensor) {
    super(configurator);
    this.configurator = configurator;
    final JPanel content = new JPanel();
    content.setLayout(new GridBagLayout());
    GridBagConstraints c = new GridBagConstraints();
    //final JLabel pathLabelText = new JLabel("File:");
    
    final JLabel pathLabel = new JLabel("No file selected");
    pathLabel.setBorder(BorderFactory.createEmptyBorder(0, 0, 5, 0));
    c.gridx = 0;
    c.gridy = 0;
    c.gridwidth = 3;
    c.weightx = 1.0;
    c.ipadx = 5;
    content.add(pathLabel, c);
    
    dataFileInfoPanel = new DataFileInfoPanel();
    c.gridy += 1;
    c.gridwidth = 2;
    c.ipady = 0;
    c.anchor = GridBagConstraints.LINE_START;
    content.add(dataFileInfoPanel, c);
    
    final JButton openButton = new JButton("Open");
    c.gridx += 2;
    c.anchor = GridBagConstraints.CENTER;
    c.gridwidth = 1;
    c.weightx = 0.0;
    c.ipadx = 0;
    content.add(openButton, c);
    
    final JPanel dataColumnPanel = new JPanel();
    c.gridx = 0;
    c.gridy += 1;
    c.weightx = 0.7;
    c.fill = GridBagConstraints.HORIZONTAL;
    c.anchor = GridBagConstraints.LINE_START;
    content.add(dataColumnPanel, c);

    final JCheckBox loopBox = new JCheckBox("Loop data");
    c.gridx += 1;
    c.weightx = 0.3;
    c.fill = GridBagConstraints.NONE;
    c.anchor = GridBagConstraints.LINE_START;
    content.add(loopBox, c);
    final JButton playButton = new JButton("Set");
    playButton.setEnabled(false);
    c.gridx += 1;
    c.weightx = 0.0;
    c.fill = GridBagConstraints.HORIZONTAL;
    content.add(playButton, c);

    c.gridx = 0;
    c.gridwidth = 3;
    final List<FileChannelPanel> fcpList = new LinkedList<>();
    for (Sensor.Channel ch : sensor.getChannels()) {
      FileChannelPanel tcp = new FileChannelPanel(ch);
      tcp.setAlignmentX(RIGHT_ALIGNMENT);
      tcp.setup();
      c.gridy += 1;
      content.add(tcp, c);
      fcpList.add(tcp);
    }
    
    dataFileChooser = new JFileChooser();
    dataFileChooser.setFileFilter(new FileNameExtensionFilter("Sensor data file (*.csv)", "csv", "dat"));
    /* open handler */
    openButton.addActionListener(new ActionListener() {
      @Override
      public void actionPerformed(ActionEvent e) {
        int retval = dataFileChooser.showOpenDialog(content);
        if (retval != JFileChooser.APPROVE_OPTION) {
          return;
        }
        File dataFile = dataFileChooser.getSelectedFile();
        pathLabel.setText(dataFile.getPath());
        FileSensorFeederVisualizer.this.configurator.pack();
        
        feeder = new FileSensorFeeder(sensor);
        feeder.setDataFile(dataFile);
        try {
          feeder.preParse();
          dataFileInfoPanel.setFileInfo(
                  feeder.getDataColumnCount(),
                  feeder.getDataLines(),
                  feeder.getStartTime(),
                  feeder.getEndTime());
          for (int idx = 0; idx < fcpList.size(); idx++) {
            fcpList.get(idx).setColumns(feeder.getDataColumnCount());
            fcpList.get(idx).setSelectedColumn(idx);
          }
          dataColumnPanel.setLayout(new BorderLayout());
          FileSensorFeederVisualizer.this.configurator.pack();
          loopBox.setEnabled(true);
          playButton.setEnabled(true);
        }
        catch (DataFileParseException ex) {
          playButton.setEnabled(false);
          logger.error(ex.getMessage());
          JOptionPane.showMessageDialog(content, ex.getMessage(), "Parse Error", JOptionPane.ERROR_MESSAGE);
        }
       }
    });
    /* set handler */
    playButton.addActionListener(new ActionListener() {
      @Override
      public void actionPerformed(ActionEvent e) {
        for (int idx = 0; idx < fcpList.size(); idx++) {
          /* skip if not enabled for this channel */
          if (!fcpList.get(idx).isPanelEnabled()) {
            continue;
          }
          /* set value for the rest */
          feeder.setupForChannel(idx, fcpList.get(idx).toParameter());
        }
        feeder.setLoopEnabled(loopBox.isSelected());
        feeder.commit();
        setFeederChanged();
        
        // prevent double initialization of same feeder
        loopBox.setEnabled(false);
        playButton.setEnabled(false);
        FileSensorFeederVisualizer.this.configurator.setVisible(false);
      }
    });
    content.setVisible(true);
    add(content);
    setVisible(true);
  }

  @Override
  public String getQuickHelp() {
    String help = "";
    help += "<b>" + Cooja.getDescriptionOf(this) + "</b>";
    help += "<p>Load sensor data from csv file.";
    help += "<p>The <em>loop</em> option allows endless feeding from file. "
            + "When reaching end of file it will be reloaded and read again.";
    help += "<p>When opening a data file it will be pre-parsed to check for"
            + "common errors.";
    help += "<p><em>File Format:</em>";
    help += "<p>First row must provide chonological timestamps in ms.";
    help += "<p>Lines starting with <tt>#</tt> are treated as comments.";
    return help;
  }

  /**
   * Panel for showing basic information about data file.
   */
  private class DataFileInfoPanel extends JPanel {

    private final JLabel dataColumnsLabel;
    private final JLabel dataLinesLabel;
    private final JLabel startTimeLabel;
    private final JLabel endTimeLabel;

    public DataFileInfoPanel() {
      setBorder(BorderFactory.createEmptyBorder(5, 0, 5, 5));
      setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
      JPanel ypanel = new JPanel(new BorderLayout());
      ypanel.add(BorderLayout.WEST, new JLabel("Data columns:"));
      dataColumnsLabel = new JLabel("---");
      ypanel.add(BorderLayout.EAST, dataColumnsLabel);
      add(ypanel);

      ypanel = new JPanel(new BorderLayout());
      ypanel.add(BorderLayout.WEST, new JLabel("Data lines:"));
      dataLinesLabel = new JLabel("---");
      ypanel.add(BorderLayout.EAST, dataLinesLabel);
      add(ypanel);

      ypanel = new JPanel(new BorderLayout());
      ypanel.add(BorderLayout.WEST, new JLabel("Start time:"));
      startTimeLabel = new JLabel("---");
      ypanel.add(BorderLayout.EAST, startTimeLabel);
      add(ypanel);

      ypanel = new JPanel(new BorderLayout());
      ypanel.add(BorderLayout.WEST, new JLabel("End time:"));
      endTimeLabel = new JLabel("---");
      ypanel.add(BorderLayout.EAST, endTimeLabel);
      add(ypanel);
    }
    
    public void setFileInfo(int columns, int lines, long startTime, long endTime) {
      dataColumnsLabel.setText(String.valueOf(columns));
      dataLinesLabel.setText(String.valueOf(lines));
      startTimeLabel.setText(String.valueOf(startTime));
      endTimeLabel.setText(String.valueOf(endTime));
    }
  }

  /**
   * 
   */
  private class FileChannelPanel extends AbstractChannelPanel {

    private final JComboBox channelInput;

    public FileChannelPanel(Sensor.Channel ch) {
      super(ManualSensorFeeder.class, ch);
      getPanel().setLayout(new BorderLayout(10, 0));
      getPanel().add(BorderLayout.CENTER, new JLabel("Data column"));
      channelInput = new JComboBox<>();
      channelInput.setEnabled(false); // enable initial as we do not have any data
      getPanel().add(BorderLayout.EAST, channelInput);
    }

    /**
     * Setup number of columns to choose from.
     *
     * @param idx Number of columns to choose from
     */
    public void setColumns(int columns) {
      channelInput.removeAllItems();
      for (int idx = 0; idx < columns; idx++) {
        channelInput.addItem(idx);
        channelInput.setEnabled(true);
      }
    }

    /**
     * Sets the selected column number.
     *
     * @param column Column number to set selected.
     */
    public void setSelectedColumn(int column) {
      channelInput.setSelectedItem(column);
    }

    @Override
    public AbstractSensorFeeder.FeederParameter toParameter() {
      return new FileFeederParameter((int) channelInput.getSelectedItem());
    }
  }
 
}
