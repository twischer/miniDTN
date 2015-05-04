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

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.text.ParseException;
import java.util.LinkedList;
import java.util.List;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JFormattedTextField;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import org.apache.log4j.Logger;
import org.contikios.cooja.ClassDescription;
import org.contikios.cooja.Cooja;
import org.contikios.cooja.HasQuickHelp;
import org.contikios.cooja.interfaces.SensorInterface;

/**
 * Sensor feeder visualizer for random values.
 *
 * @author Enrico Jorns
 */
@ClassDescription(value = "Gaussian Feeder")
public class GaussianSensorFeederVisualizer extends AbstractSensorFeederVisualizer implements HasQuickHelp {

  private static final Logger logger = Logger.getLogger(GaussianSensorFeederVisualizer.class);
  final SensorInterface.SensorFeederConfigFrame configurator;

  public GaussianSensorFeederVisualizer(SensorInterface.SensorFeederConfigFrame configurator, final Sensor sensor) {
    super(configurator);
    this.configurator = configurator;
    JPanel content = new JPanel();
    content.setLayout(new BoxLayout(content, BoxLayout.Y_AXIS));
    /* Add channel inputs */
    final List<GaussianChannelPanel> gcpList = new LinkedList<>();
    for (Sensor.Channel ch : sensor.getChannels()) {
      GaussianChannelPanel gcp = new GaussianChannelPanel(ch);
      gcp.setAlignmentX(RIGHT_ALIGNMENT);
      gcp.setup();
      gcpList.add(gcp);
      content.add(gcp);
    }
    final TimingPanel timingPanel = new TimingPanel();
    timingPanel.setAlignmentX(RIGHT_ALIGNMENT);
    content.add(timingPanel);
    JButton submitButton = new JButton("Set");
    content.add(submitButton);
    submitButton.setAlignmentX(RIGHT_ALIGNMENT);
    submitButton.addActionListener(new ActionListener() {
      @Override
      public void actionPerformed(ActionEvent e) {
        /* Create a new feeder from the user settings */
        try {
          GaussianSensorFeeder feeder = new GaussianSensorFeeder(sensor);
          for (int idx = 0; idx < gcpList.size(); idx++) {
            GaussianChannelPanel gcp = gcpList.get(idx);
            if (gcp == null || !gcp.isPanelEnabled()) {
              continue;
            }
            feeder.setupForChannel(idx, gcp.toParameter());
          }
          feeder.setInterval(timingPanel.getInterval());
          feeder.commit();
          setFeederChanged(); // XXX new!
        }
        catch (ParseException ex) {
          logger.error("Failed parsing GaussianSensorFeeder input: " + ex.getMessage());
          return;
        }
        GaussianSensorFeederVisualizer.this.configurator.setVisible(false);
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
    help += "<p>Feeds the sensor with Gaussian distributed random values";
    help += "<p>You can configure the value range by setting <em>mean</em> and <em>standard deviation</em>.";
    return help;
  }

  /**
   * Channel panel for setting gaussian sensor updateChanneler parameters.
   */
  private static class GaussianChannelPanel extends AbstractChannelPanel {

    JTextField meanInput;
    JTextField devInput;
    NumberFormat nf;

    public GaussianChannelPanel(Sensor.Channel ch) {
      super(GaussianSensorFeeder.class, ch);
      getPanel().setLayout(new BoxLayout(getPanel(), BoxLayout.LINE_AXIS));
      nf = DecimalFormat.getNumberInstance();
      nf.setGroupingUsed(false);

      getPanel().add(new JLabel("Mean"));
      meanInput = new JFormattedTextField(nf);
      meanInput.setText(String.valueOf(ch.default_));
      meanInput.setColumns(8);
      getPanel().add(meanInput);

      getPanel().add(new JLabel("Std dev."));
      devInput = new JFormattedTextField(nf);
      devInput.setText("1.0");
      devInput.setColumns(8);
      getPanel().add(devInput);

    }

    @Override
    public AbstractSensorFeeder.FeederParameter toParameter() throws ParseException {
      return new GaussianSensorFeeder.GaussianFeederParameter(
              nf.parse(meanInput.getText()).doubleValue(),
              nf.parse(devInput.getText()).doubleValue());
    }
  }

}
