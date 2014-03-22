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
import org.contikios.cooja.interfaces.sensor.ManualSensorFeeder.ManualFeederParameter;

/**
 * Sensor feeeder visualizer for manual value input
 *
 * @author Enrico Jorns
 */
@ClassDescription(value = "Manual Feeder")
public class ManualSensorFeederVisualizer extends AbstractSensorFeederVisualizer implements HasQuickHelp {

  private static final Logger logger = Logger.getLogger(ManualSensorFeederVisualizer.class);
  final SensorInterface.SensorFeederConfigFrame configurator;

  public ManualSensorFeederVisualizer(SensorInterface.SensorFeederConfigFrame configurator, final Sensor sensor) {
    super(configurator);
    this.configurator = configurator;
    JPanel content = new JPanel();
    content.setLayout(new BoxLayout(content, BoxLayout.Y_AXIS));
    /* Add channel inputs */
    final List<ManualChannelPanel> tcpList = new LinkedList<>();
    for (Sensor.Channel ch : sensor.getChannels()) {
      ManualChannelPanel tcp = new ManualChannelPanel(ch);
      tcp.setAlignmentX(RIGHT_ALIGNMENT);
      tcp.setup();
      content.add(tcp);
      tcpList.add(tcp);
    }
    JButton submitButton = new JButton("Set");
    submitButton.setAlignmentX(RIGHT_ALIGNMENT);
    content.add(submitButton);
    submitButton.addActionListener(new ActionListener() {
      @Override
      public void actionPerformed(ActionEvent e) {
        try {
          ManualSensorFeeder feeder = new ManualSensorFeeder(sensor);
          for (int idx = 0; idx < tcpList.size(); idx++) {
            /* skip if not enabled for this channel */
            if (!tcpList.get(idx).isPanelEnabled()) {
              continue;
            }
            /* set value for the rest */
            feeder.setupForChannel(idx, tcpList.get(idx).toParameter());
          }
          feeder.commit();
          setFeederChanged(); // XXX new!
        }
        catch (ParseException ex) {
          logger.error("Failed parsing ManualSensorFeeder input: " + ex.getMessage());
        }
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
    help += "<p>Allows to set fixed values";
    return help;
  }

  /**
   * Channel panel for setting random sensor updateChanneler parameters.
   */
  private static class ManualChannelPanel extends AbstractChannelPanel {

    private final JTextField channelInput;
    private final NumberFormat nf;

    public ManualChannelPanel(Sensor.Channel ch) {
      super(ManualSensorFeeder.class, ch);
      getPanel().setLayout(new BorderLayout(10, 0));
      getPanel().add(BorderLayout.WEST, new JLabel("Value"));
      nf = DecimalFormat.getNumberInstance();
      channelInput = new JFormattedTextField(nf);
      channelInput.setText(String.valueOf(ch.default_));
      channelInput.setColumns(10);
      getPanel().add(BorderLayout.CENTER, channelInput);

    }

    @Override
    public AbstractSensorFeeder.FeederParameter toParameter() throws ParseException {
      return new ManualFeederParameter(nf.parse(channelInput.getText()).doubleValue());
    }
  }

}
