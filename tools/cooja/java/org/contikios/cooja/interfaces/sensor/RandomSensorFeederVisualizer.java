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
import javax.swing.InputVerifier;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JFormattedTextField;
import javax.swing.JLabel;
import javax.swing.JPanel;
import org.apache.log4j.Logger;
import org.contikios.cooja.ClassDescription;
import org.contikios.cooja.Cooja;
import org.contikios.cooja.HasQuickHelp;
import org.contikios.cooja.interfaces.SensorInterface;
import org.contikios.cooja.interfaces.sensor.RandomSensorFeeder.RandomFeederParameter;

/**
 * Sensor feeeder visualizer for random values.
 *
 * @author Enrico Jorns
 */
@ClassDescription(value = "Random Feeder")
public class RandomSensorFeederVisualizer extends AbstractSensorFeederVisualizer implements HasQuickHelp {

  private static final Logger logger = Logger.getLogger(RandomSensorFeederVisualizer.class);
  final SensorInterface.SensorFeederConfigFrame configurator;

  public RandomSensorFeederVisualizer(SensorInterface.SensorFeederConfigFrame configurator, final Sensor sensor) {
    super(configurator);
    this.configurator = configurator;
    JPanel content = new JPanel();
    content.setLayout(new BoxLayout(content, BoxLayout.Y_AXIS));
    /* Add channel inputs */
    final List<AbstractChannelPanel> cpList = new LinkedList<>();
    for (Sensor.Channel ch : sensor.getChannels()) {
      AbstractChannelPanel cp = new RandomChannelPanel(ch);
      cp.setAlignmentX(RIGHT_ALIGNMENT);
      cp.setup();
      cpList.add(cp);
      content.add(cp);
    }
    final TimingPanel timingPanel = new TimingPanel();
    timingPanel.setAlignmentX(RIGHT_ALIGNMENT);
    content.add(timingPanel);
    JButton submitButton = new JButton("Set");
    submitButton.setAlignmentX(RIGHT_ALIGNMENT);
    content.add(submitButton);
    submitButton.addActionListener(new ActionListener() {
      @Override
      public void actionPerformed(ActionEvent e) {
        /* setup feeder */
        try {
          RandomSensorFeeder feeder = new RandomSensorFeeder(sensor);
          for (int idx = 0; idx < sensor.numChannels(); idx++) {
            RandomChannelPanel rcp = (RandomChannelPanel) cpList.get(idx);
            if (rcp == null || !rcp.isPanelEnabled()) {
              continue;
            }
            feeder.setupForChannel(idx, ((RandomChannelPanel) rcp).toParameter());
          }
          feeder.setInterval(timingPanel.getInterval());
          feeder.commit();
          setFeederChanged(); // XXX new!
        }
        catch (ParseException ex) {
          logger.error("Failed parsing RandomSensorFeeder input: " + ex.getMessage());
          return;
        }
        RandomSensorFeederVisualizer.this.configurator.setVisible(false);
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
    help += "<p>Feeds the sensor with uniform distributed random values";
    help += "<p>You can configure the value range by setting <em>lower</em> and <em>upper</em> bounds.";
    return help;
  }

  /**
   * Channel panel for setting random sensor updateChanneler parameters.
   */
  private static class RandomChannelPanel extends AbstractChannelPanel {

    private final JFormattedTextField lboundInput;
    private final JFormattedTextField uboundInput;
    private final NumberFormat nf;

    public RandomChannelPanel(Sensor.Channel ch) {
      super(RandomSensorFeeder.class, ch);
      getPanel().setLayout(new BoxLayout(getPanel(), BoxLayout.LINE_AXIS));
      nf = DecimalFormat.getNumberInstance();
      nf.setGroupingUsed(false);

      getPanel().add(new JLabel("Bounds ["));
      lboundInput = new JFormattedTextField(nf);
      lboundInput.setText("0");
      lboundInput.setColumns(8);
      getPanel().add(lboundInput);

      getPanel().add(new JLabel(" - "));
      uboundInput = new JFormattedTextField(nf);
      uboundInput.setText("0");
      uboundInput.setColumns(8);
      getPanel().add(uboundInput);
      getPanel().add(new JLabel("]"));

      /* verify that lbound <= ubound */
      InputVerifier leqVerifier = new InputVerifier() {

        @Override
        public boolean verify(JComponent input) {
          String ltext = lboundInput.getText();
          String utext = uboundInput.getText();
          /* we do not want to deactivate the user from editing the second field */
          if (ltext.equals("") || utext.equals("")) {
            return true;
          }
          double lval = Double.parseDouble(ltext);
          double uval = Double.parseDouble(utext);

          return lval <= uval;
        }
      };
      lboundInput.setInputVerifier(leqVerifier);
      uboundInput.setInputVerifier(leqVerifier);

      /* Disable panel if channel is already allocated to another feeder type */
//      if (ch.getFeeder() != null && !ch.getFeeder().getClass().isAssignableFrom(RandomSensorFeeder.class)) {
//        setPanelEnabled(false);
//      }
    }

    // XXX to interface
    @Override
    public AbstractSensorFeeder.FeederParameter toParameter() throws ParseException {
      return new RandomFeederParameter(
              nf.parse(lboundInput.getText()).doubleValue(),
              nf.parse(uboundInput.getText()).doubleValue()
      );
    }
  }

}
