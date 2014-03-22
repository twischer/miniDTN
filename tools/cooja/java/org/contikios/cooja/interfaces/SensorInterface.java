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
package org.contikios.cooja.interfaces;

import org.contikios.cooja.interfaces.sensor.Sensor;
import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.PropertyVetoException;
import java.lang.reflect.InvocationTargetException;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Observable;
import java.util.Observer;
import java.util.Set;
import java.util.logging.Level;
import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JInternalFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSeparator;
import javax.swing.SwingUtilities;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import org.apache.log4j.Logger;
import org.contikios.cooja.ClassDescription;
import org.contikios.cooja.Cooja;
import org.contikios.cooja.HasQuickHelp;
import org.contikios.cooja.Mote;
import org.contikios.cooja.MoteInterface;
import org.contikios.cooja.SensorMote;
import org.contikios.cooja.interfaces.sensor.AbstractSensorFeeder;
import org.contikios.cooja.interfaces.sensor.AbstractSensorFeederVisualizer;
import org.contikios.cooja.interfaces.sensor.FileSensorFeederVisualizer;
import org.contikios.cooja.interfaces.sensor.GaussianSensorFeederVisualizer;
import org.contikios.cooja.interfaces.sensor.ManualSensorFeederVisualizer;
import org.contikios.cooja.interfaces.sensor.RandomSensorFeederVisualizer;
import org.contikios.cooja.interfaces.sensor.Sensor.Channel;
import org.jdom.Element;

/**
 * Visual interface for controlling all sensors available on the
 * selected device.
 *
 * @author Enrico Jorns
 */
@ClassDescription("Sensors")
public class SensorInterface extends MoteInterface implements HasQuickHelp {

  private static final Logger logger = Logger.getLogger(SensorInterface.class);

  private static final Class[] feederClazzList = {
    ManualSensorFeederVisualizer.class,
    RandomSensorFeederVisualizer.class,
    GaussianSensorFeederVisualizer.class,
    FileSensorFeederVisualizer.class
  };

  private final JPanel interfacePanel;
  private final List<FeederComboboxEntry> feederBoxList = new LinkedList<>();
  // Key: Sensor name, value: sensor panel
  private final Map<String, SensorPanel> sensorPanels = new HashMap<>();
  private final SensorMote sensormote;

  public SensorInterface(Mote mote) {
    /* Setup feeder list for Combobox */
    for (Class clazz : feederClazzList) {
      feederBoxList.add(new FeederComboboxEntry(Cooja.getDescriptionOf(clazz), clazz));
    }

    if (mote != null && SensorMote.class.isAssignableFrom(mote.getClass())) {
      sensormote = (SensorMote) mote;
    }
    else {
      throw new RuntimeException("Mote is not a SensorMote");
    }

    /* Create Panel */
    interfacePanel = new JPanel();
    interfacePanel.setLayout(new BoxLayout(interfacePanel, BoxLayout.Y_AXIS));
    Sensor[] sensors = sensormote.getSensors();
    /* Create a SensorPanel for each sensor found on this mote */
    for (Sensor sensor : sensors) {

      System.out.println("((Mote) sensormote).getSimulation().getCooja().getDesktopPane(): " + ((Mote) sensormote).getSimulation().getCooja().getDesktopPane());

      SensorPanel sensorPanel = new SensorPanel(((Mote) sensormote).getSimulation().getCooja(), sensor);
      sensorPanels.put(sensor.getName(), sensorPanel);
      interfacePanel.add(sensorPanel);

      /* Add separator if not last item */
      if (!sensors[sensors.length - 1].equals(sensor)) {
        interfacePanel.add(new JSeparator());
      }

    }

  }

  /**
   * Updates infos for all sensors.
   * Should be called e.g. if feeders were updated.
   */
  public void updateSensorInfo() {
    for (String spanel : sensorPanels.keySet()) {
      sensorPanels.get(spanel).setFeederDescription();
    }
  }

  @Override
  public JPanel getInterfaceVisualizer() {
    return interfacePanel;
  }

  @Override
  public void releaseInterfaceVisualizer(JPanel panel) {
    /* close all sensor panels and remove references */
    for (String sname : sensorPanels.keySet()) {
      sensorPanels.get(sname).getConfigFrame().setVisible(false);
    }
  }

  @Override
  public String getQuickHelp() {
    String help = "";
    help += "<p><b>" + Cooja.getDescriptionOf(this) + "</b>";
    help += "<p>Lists available sensors and allows to feed them with data.";

    return help;
  }
   
  @Override
  public Collection<Element> getConfigXML() {
    List<Element> elements = new LinkedList<>();
    Element sensorElement;

    for (Sensor sensor : sensormote.getSensors()) {
      
      sensorElement = new Element("sensor");
      sensorElement.addContent(sensor.getName());
      
      // get set of all _used_ feeders
      Set<AbstractSensorFeeder> feederSet = new HashSet<>();
      for (Channel ch : sensor.getChannels()) {
        if (ch.getFeeder() == null) {
          continue;
        }
        feederSet.add(ch.getFeeder());
      }

      // save feeders
      for (AbstractSensorFeeder feeder : feederSet) {
        sensorElement.addContent(feeder.getConfigXML());
      }

      // store config frame config if visible
      // XXX Does not work because when activating MoteInterfaceViewer the interface
      //     is reselected and closes previously opened windows.
//      if (sensorPanels.get(sensor.getName()).getConfigFrame().isVisible()) {
//        sensorElement.addContent(new Element("config_frame").addContent(Cooja.getXMLFromInternalFrame(sensorPanels.get(sensor.getName()).getConfigFrame())));
//      }

      elements.add(sensorElement);
    }
    
    return elements;
  }

  @Override
  public void setConfigXML(Collection<Element> configXML, boolean visAvailable) {
    
    Map<String, AbstractSensorFeeder> feederMap = new HashMap<>();
    
    // Iterate over each sensor
    for (Element element: configXML) {
      if (!element.getName().equals("sensor")) {
        continue;
      }
      
      // get Sensor for this name
      String sensorName = element.getTextTrim();
      Sensor sensor = null;
      for (Sensor s : sensormote.getSensors()) {
        if (s.getName().equals(sensorName)) {
          sensor = s;
          break;
        }
      }
      if (sensor == null) {
        logger.error("Sensor " + sensor + " was not found and will not be configured");
        continue;
      }

      // Iterate over each feeder
      for (Object ef : element.getChildren("feeder")) {
        Element feederElement = (Element) ef;
        String feederClassName = feederElement.getTextTrim();
        if (feederClassName == null) {
          logger.error("Feeder type not set in XML");
          continue;
        }

        // get feeder type
        Class<? extends AbstractSensorFeeder> feederClass
                = ((Mote) sensormote).getSimulation().getCooja()
                .tryLoadClass(this, AbstractSensorFeeder.class, feederClassName);
        if (feederClass == null) {
          logger.fatal("Could not load Feeder class: " + feederClassName);
          return;
        }
        // create new feeder instance
        AbstractSensorFeeder feeder;
        try {
          feeder = feederClass.getConstructor(new Class[]{Sensor.class})
                  .newInstance(sensor);
        }
        catch (NoSuchMethodException | SecurityException | InstantiationException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex) {
          logger.error("Failed creating feeder " + feederClassName, ex);
          continue;
        }

        // setup feeder
        feeder.setConfigXML(feederElement);
        
      }

      // restore config frame if it was visible
//      if (element.getChild("config_frame") != null) {
//        Cooja.setInternalFrameByXML(sensorPanels.get(sensor.getName()).getConfigFrame(), element.getChild("config_frame").getChildren());
//        sensorPanels.get(sensor.getName()).getConfigFrame().setVisible(true);
//      }
    }
    
    // Trigger update of feeder labels
    updateSensorInfo();

  }

  /**
   * 
   */
  private class FeederComboboxEntry {

    private final String key;
    private final Class<? extends AbstractSensorFeederVisualizer> clazz;

    public FeederComboboxEntry(String key, Class<? extends AbstractSensorFeederVisualizer> clazz) {
      this.key = key;
      this.clazz = clazz;
    }

    @Override
    public String toString() {
      return key;
    }
  }

  /**
   * Shows Sensor, feeder info and provides "Configure" button
   *
   * Each sensor pannel has a SensorFeederConfigFrame instance.
   */
  private class SensorPanel extends JPanel {

    private final Sensor sensor;
    private final JLabel feederDescLabel;
    private final SensorFeederConfigFrame sfeedConfFrame;

    SensorPanel(Cooja gui, Sensor sensor) {
      this.sensor = sensor;
      setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
      setBorder(BorderFactory.createEmptyBorder(3, 3, 3, 3));
      JPanel sensorSubPanel = new JPanel();
      sensorSubPanel.setLayout(new BorderLayout());
      sensorSubPanel.add(BorderLayout.CENTER, new JLabel(sensor.getName()));
      JButton configureButton = new JButton("Configure");
      sensorSubPanel.add(BorderLayout.EAST, configureButton);
      add(sensorSubPanel);

      JPanel feederPanel = new JPanel(new BorderLayout());
      feederPanel.add(BorderLayout.WEST, new JLabel("Feeder:"));
      feederDescLabel = new JLabel();
      setFeederDescription();
      feederPanel.add(feederDescLabel);
      add(BorderLayout.CENTER, feederPanel);

      /* Add config frame for this */
      sfeedConfFrame = new SensorFeederConfigFrame(gui, sensor);
      gui.getDesktopPane().add(sfeedConfFrame);
      sfeedConfFrame.pack();
      /* Update feeder description if notified about feeder change */
      sfeedConfFrame.addFeederChangedListener(new Observer() {

        @Override
        public void update(Observable o, Object arg) {
          setFeederDescription();
        }
      });
      
      configureButton.addActionListener(new ActionListener() {

        @Override
        public void actionPerformed(ActionEvent e) {
          /* set visible if invisible */
          if (!sfeedConfFrame.isVisible()) {
            sfeedConfFrame.setLocation(sfeedConfFrame.getDesktopPane().getMousePosition());
            sfeedConfFrame.setVisible(true);
          }
          /* select window */
          try {
            for (JInternalFrame existingPlugin : sfeedConfFrame.getDesktopPane().getAllFrames()) {
              existingPlugin.setSelected(false);
            }
            sfeedConfFrame.setSelected(true);
          }
          catch (PropertyVetoException ex) {
            logger.info(ex.getMessage());
          }
        }
      });

    }

    /**
     * Returns reference to associated config frame.
     *
     * @return
     */
    public SensorFeederConfigFrame getConfigFrame() {
      return sfeedConfFrame;
    }

    /**
     * Sets feeder description dependent from selected feeders.
     */
    private void setFeederDescription() {
      StringBuilder feederDesc = new StringBuilder();
      for (Channel c : sensor.getChannels()) {
        feederDesc.append(" ").append(c.getFeeder() == null ? "N/A" : c.getFeeder().getName());
      }
      feederDescLabel.setText(feederDesc.toString());
      // pack parent JInternalFrame as content may have changed
      Container ancestor = SwingUtilities.getAncestorOfClass(JInternalFrame.class, interfacePanel);
      if (ancestor != null) {
        ((JInternalFrame) ancestor).pack();
      }
    }

  }

  /**
   * Frame for configuring the sensor.
   *
   * Holds a selection box and a pane for different Visualizers.
   */
  public class SensorFeederConfigFrame extends JInternalFrame implements HasQuickHelp {

    private final Sensor sensor;
    private final JComboBox feederSelector;
    /* This panel holds the currently selected AbstractSensorFeederVisualizer */
    private final JPanel visualizerRootPane;
    private final Observer visualizerChangedFeederObserver;
    private final FeederUpdateObservable updateObservable = new FeederUpdateObservable();
    private final Cooja gui;
    private AbstractSensorFeederVisualizer currentVisualizer = null;


    public SensorFeederConfigFrame(final Cooja gui, Sensor sensor) {
      this.gui = gui;
      this.sensor = sensor;

      visualizerChangedFeederObserver = new Observer() {

        @Override
        public void update(Observable o, Object arg) {
        }
      };

      final JPanel panel = new JPanel();
      panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));

      /* Combobox with all feeders available */
      feederSelector = new JComboBox(feederBoxList.toArray(new FeederComboboxEntry[feederBoxList.size()]));
      panel.add(feederSelector);

      /* here we put our sensor configurator in */
      visualizerRootPane = new JPanel();
      panel.add(visualizerRootPane);
      updateConfigRootPane();

      feederSelector.addActionListener(new ActionListener() {

        @Override
        public void actionPerformed(ActionEvent e) {
          updateConfigRootPane();
        }
      });

      add(panel);

      /* Window settings */
      setTitle(sensor.getName());
      setClosable(true);
      setDefaultCloseOperation(HIDE_ON_CLOSE);

      addInternalFrameListener(new InternalFrameAdapter() {

        @Override
        public void internalFrameActivated(InternalFrameEvent e) {
          super.internalFrameActivated(e);
          gui.loadQuickHelp(SensorFeederConfigFrame.this);
        }

      });

      pack();
      System.out.println("Created myself: " + this.hashCode());
    }

    /**
     * Sets the visualizer root pane dependent on selection from selector.
     */
    private void updateConfigRootPane() {
      Class<? extends AbstractSensorFeederVisualizer> clazz = ((FeederComboboxEntry) feederSelector.getSelectedItem()).clazz;
      try {

        /* replace content of visualizerRootPane with new one */
        currentVisualizer = clazz.getConstructor(new Class[]{SensorFeederConfigFrame.class, Sensor.class})
                .newInstance(SensorFeederConfigFrame.this, SensorFeederConfigFrame.this.sensor);
        currentVisualizer.setVisible(true);

        visualizerRootPane.removeAll();
        visualizerRootPane.add(currentVisualizer);

        /* Also update the quick help */
        gui.loadQuickHelp(SensorFeederConfigFrame.this);

        SensorFeederConfigFrame.this.pack();
      }
      catch (NoSuchMethodException | SecurityException | InstantiationException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex) {
        java.util.logging.Logger.getLogger(SensorInterface.class.getName()).log(Level.SEVERE, null, ex);
      }
    }

    /**
     * Returns the currently selected visualizer.
     *
     * @return
     */
    public AbstractSensorFeederVisualizer getCurrentVisualizer() {
      return currentVisualizer;
    }

    /**
     * Adds observer to be notified if a feeder was changed.
     *
     * @param observer Observer that should be notified
     */
    public void addFeederChangedListener(Observer observer) {
      updateObservable.addObserver(observer);
    }

    /**
     *
     */
    public void notifyFeederChangedListeners() {
      updateObservable.notifyFeederChanged();
    }

    @Override
    public String getQuickHelp() {
      String help = "";
      help += "<b>Sensor Feeder Setup</b>";
      help += "<p>Configure sensor data input.";

      if (currentVisualizer == null) {
        help += "<p>No Feeder found";
      }
      else {
        if (currentVisualizer instanceof HasQuickHelp) {
          help += "<p>" + ((HasQuickHelp) currentVisualizer).getQuickHelp();
        }
        else {
          help += "<p><b>" + Cooja.getDescriptionOf(currentVisualizer) + "</b>";
        }
      }

      return help;
    }

    /**
     * Observable clients can register with to receive feeder update
     * notifications.
     */
    private class FeederUpdateObservable extends Observable {

      public void setFeederChanged() {
        setChanged();
      }

      public void notifyFeederChanged() {
        setChanged();
        notifyObservers();
      }
    }

  }

}
