/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 *
 */

package org.contikios.cooja.interfaces;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Observable;
import java.util.Observer;

import javax.swing.JLabel;
import javax.swing.JPanel;

import org.apache.log4j.Logger;
import org.jdom.Element;

import org.contikios.cooja.ClassDescription;
import org.contikios.cooja.MoteInterface;

/**
 * A MoteID represents a mote ID number. An implementation should notify all
 * observers if the mote ID is set or changed.
 *
 * @author Fredrik Osterlind
 */
@ClassDescription("ID")
public abstract class MoteID extends MoteInterface {
  private static Logger logger = Logger.getLogger(MoteID.class);

  /**
   * @return Current mote ID number
   */
  public abstract int getMoteID();

  /**
   * Sets mote ID to given number.
   * @param id New mote ID number
   */
  public abstract void setMoteID(int id);

  public Collection<Element> getConfigXML() {
    ArrayList<Element> config = new ArrayList<Element>();
    Element element = new Element("id");
    element.setText(Integer.toString(getMoteID()));
    config.add(element);
    return config;
  }

  public void setConfigXML(Collection<Element> configXML, boolean visAvailable) {
    for (Element element : configXML) {
      if (element.getName().equals("id")) {
        setMoteID(Integer.parseInt(element.getText()));
      }
    }
  }

  public JPanel getInterfaceVisualizer() {
    final JLabel idLabel = new JLabel("Mote ID: " + getMoteID());

    Observer observer;
    this.addObserver(observer = new Observer() {
      public void update(Observable obs, Object obj) {
        idLabel.setText("Mote ID: " + getMoteID());
      }
    });

    JPanel panel = new JPanel();
    panel.add(idLabel);
    panel.putClientProperty("intf_obs", observer);
    return panel;
  }

  public void releaseInterfaceVisualizer(JPanel panel) {
    Observer observer = (Observer) panel.getClientProperty("intf_obs");
    if (observer == null) {
      logger.fatal("Error when releasing panel, observer is null");
      return;
    }
    this.deleteObserver(observer);
  }
}
