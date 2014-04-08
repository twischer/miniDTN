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
import java.awt.Color;
import java.awt.Component;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.text.ParseException;
import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JCheckBox;
import javax.swing.JFormattedTextField;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.border.TitledBorder;
import org.contikios.cooja.interfaces.SensorInterface;

/**
 * Provides a visual interface to a sensor feeder.
 *
 * An implementing subclass must:
 * <ul>
 * <li>provide input to configure a Feeder,</li>
 * <li>create a new instance of the feeder, set it up and commit it,</li>
 * <li>call <code>setFeederChanged()</code> after feeder setup</li>
 * </ul>
 *
 * @author Enrico Jorns
 */
public abstract class AbstractSensorFeederVisualizer extends JPanel {

  private final SensorInterface.SensorFeederConfigFrame configurator;

  public AbstractSensorFeederVisualizer(SensorInterface.SensorFeederConfigFrame configurator) {
    this.configurator = configurator;
  }

  /**
   * Forwards to observers of the enclosing SensorFeederConfigFrame
   */
  void setFeederChanged() {
    configurator.notifyFeederChangedListeners();
  }

  /**
   * Abstract base class for channel panels.
   *
   * Defines outer view of the panel.
   */
  protected static abstract class AbstractChannelPanel extends JPanel {

    private final Sensor.Channel channel;
    private final Class<? extends AbstractSensorFeeder> feederType;

    private final JPanel childPanel;
    private final JCheckBox selectBox;

    public AbstractChannelPanel(Class<? extends AbstractSensorFeeder> type, Sensor.Channel ch) {
      this.feederType = type;
      this.channel = ch;
      String panelTitle = "Channel: " + String.valueOf(ch.name);
      if (ch.unit != null) {
        panelTitle += " [" + String.valueOf(ch.unit) + "]";
      }
      TitledBorder tborder = BorderFactory.createTitledBorder(
              BorderFactory.createLineBorder(Color.GRAY),
              panelTitle);
      tborder.setTitlePosition(TitledBorder.CENTER);
      setLayout(new BorderLayout(0, 0));
      selectBox = new JCheckBox();
      selectBox.setSelected(true);
      add(BorderLayout.WEST, selectBox);
      childPanel = new JPanel();
      childPanel.setBorder(tborder);
      add(BorderLayout.CENTER, childPanel);
      selectBox.addItemListener(new ItemListener() {

        @Override
        public void itemStateChanged(ItemEvent e) {
          if (e.getStateChange() == ItemEvent.SELECTED) {
            setPanelEnabled(true);
          }
          else {
            setPanelEnabled(false);
          }
        }
      });

    }

    /**
     * Disables panel if channel is already allocated to another feeder type
     */
    public final void setup() {
      if (this.channel.getFeeder() != null && !this.channel.getFeeder().getClass().isAssignableFrom(this.feederType)) {
        setPanelEnabled(false);
      }
    }

    /**
     * Get a panel subclasses can add components to, set layout, etc.
     *
     * @return
     */
    public JPanel getPanel() {
      return childPanel;
    }

    /**
     * Selects/deselects checkbox and enables/disables panel content
     *
     * @param enabled
     */
    public void setPanelEnabled(boolean enabled) {
      childPanel.setEnabled(enabled);
      for (Component c : childPanel.getComponents()) {
        c.setEnabled(enabled);
        selectBox.setSelected(enabled);
      }
    }

    /**
     * True if this panel is enable
     *
     * @return
     */
    public boolean isPanelEnabled() {
      return childPanel.isEnabled();
    }

    /**
     * Returns parameter instance parsed from current panel settings.
     *
     * @return Parameter for the respective SesorFeeder
     * @throws ParseException Thrown if user entered invalid data
     * that could not be parsed correctly
     */
    public abstract AbstractSensorFeeder.FeederParameter toParameter() throws ParseException;

  }

  /**
   * Input panel for timing control.
   */
  protected static class TimingPanel extends JPanel {

    private static final String DEFAULT_INTERVAL_TEXT = "100"; /*[ms]*/

    private final JFormattedTextField intervalField;
    private final NumberFormat nf;

    public TimingPanel() {
      setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
      add(new JLabel("Update interval: "));
      nf = DecimalFormat.getIntegerInstance();
      intervalField = new JFormattedTextField(nf);
      intervalField.setHorizontalAlignment(JFormattedTextField.RIGHT);
      intervalField.setText(DEFAULT_INTERVAL_TEXT);
      intervalField.setColumns(8);
      intervalField.setMaximumSize(intervalField.getPreferredSize());
      add(intervalField);
      add(new JLabel("[ms]"));
    }

    public long getInterval() throws ParseException {
      return nf.parse(intervalField.getText()).longValue();
    }
  }

}
