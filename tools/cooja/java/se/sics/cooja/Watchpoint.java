/*
 * Copyright (c) 2012, Swedish Institute of Computer Science.
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

package se.sics.cooja;

import java.awt.Color;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;

import org.apache.log4j.Logger;
import org.jdom.Element;

import se.sics.cooja.util.StringUtils;

/**
 * Watchpoint.
 *
 * @author Fredrik Osterlind
 */
public abstract class Watchpoint<T extends WatchpointMote> {
  private static Logger logger = Logger.getLogger(Watchpoint.class);

  private T watchpointMote;

  private int address = -1; /* Executable address */
  private File codeFile = null; /* Source code, may be null*/
  private int lineNr = -1; /* Source code line number, may be null */

  private Color color = Color.BLACK;

  private boolean stopsSimulation = true;
  private String msg = null;
  private String contikiCode = null;

  public Watchpoint(T mote) {
    this.watchpointMote = mote;
    /* expects setConfigXML(..) */
  }

  public Watchpoint(T mote, Integer address, File codeFile, Integer lineNr) {
    this.watchpointMote = mote;
    this.address = address;
    this.codeFile = codeFile;
    this.lineNr = lineNr;

    createMonitor();
  }

  public T getMote() {
    return watchpointMote;
  }

  public void createMonitor() {
    registerBreakpoint();

    /* Remember Contiki code, to verify it when reloaded */
    if (contikiCode == null) {
      final String code = StringUtils.loadFromFile(codeFile);
      if (code != null) {
        String[] lines = code.split("\n");
        if (getLineNumber()-1 < lines.length) {
          contikiCode = lines[lineNr-1].trim();
        }
      }
    }
  }

  public abstract void registerBreakpoint();
  public abstract void unregisterBreakpoint();

  public Color getColor() {
    return color;
  }
  public void setColor(Color color) {
    this.color = color;
  }

  public String getDescription() {
    String desc = "";
    if (codeFile != null) {
      desc += codeFile.getPath() + ":" + lineNr + " (0x" + Integer.toHexString(address) + ")";
    } else if (address >= 0) {
      desc += "0x" + Integer.toHexString(address);
    }
    if (msg != null) {
      desc += "\n\n" + msg;
    }
    return desc;
  }

  public File getCodeFile() {
    return codeFile;
  }
  public int getLineNumber() {
    return lineNr;
  }
  public int getExecutableAddress() {
    return address;
  }

  public void setStopsSimulation(boolean stops) {
    stopsSimulation = stops;
  }
  public boolean stopsSimulation() {
    return stopsSimulation;
  }

  public void setUserMessage(String msg) {
    this.msg = msg;
  }
  public String getUserMessage() {
    return msg;
  }

  public Collection<Element> getConfigXML() {
    ArrayList<Element> config = new ArrayList<Element>();
    Element element;

    element = new Element("stops");
    element.setText("" + stopsSimulation);
    config.add(element);

    element = new Element("codefile");
    File file = watchpointMote.getSimulation().getGUI().createPortablePath(codeFile);
    element.setText(file.getPath().replaceAll("\\\\", "/"));
    config.add(element);

    element = new Element("line");
    element.setText("" + lineNr);
    config.add(element);

    if (contikiCode != null) {
      element = new Element("contikicode");
      element.setText(contikiCode);
      config.add(element);
    }

    if (msg != null) {
      element = new Element("msg");
      element.setText(msg);
      config.add(element);
    }

    if (color != null) {
      element = new Element("color");
      element.setText("" + color.getRGB());
      config.add(element);
    }

    return config;
  }

  public boolean setConfigXML(Collection<Element> configXML, boolean visAvailable) {
    /* Already knows mote and breakpoints */

    for (Element element : configXML) {
      if (element.getName().equals("codefile")) {
        File file = new File(element.getText());
        file = watchpointMote.getSimulation().getGUI().restorePortablePath(file);

        try {
          codeFile = file.getCanonicalFile();
        } catch (IOException e) {
        }

        if (codeFile == null || !codeFile.exists()) {
          return false;
        }
      } else if (element.getName().equals("line")) {
        lineNr = Integer.parseInt(element.getText());
      } else if (element.getName().equals("contikicode")) {
        String lastContikiCode = element.getText().trim();

        /* Verify that Contiki code did not change */
        final String code = StringUtils.loadFromFile(codeFile);
        if (code != null) {
          String[] lines = code.split("\n");
          if (lineNr-1 < lines.length) {
            contikiCode = lines[lineNr-1].trim();
          }
        }

        if (!lastContikiCode.equals(contikiCode)) {
          logger.warn("Detected modified Contiki code at breakpoint: " + codeFile.getPath() + ":" + lineNr + ".");
          logger.warn("From: '" + lastContikiCode + "'");
          logger.warn("  To: '" + contikiCode + "'");
        }
      } else if (element.getName().equals("msg")) {
        msg = element.getText();
      } else if (element.getName().equals("color")) {
        color = new Color(Integer.parseInt(element.getText()));
      } else if (element.getName().equals("stops")) {
        stopsSimulation = Boolean.parseBoolean(element.getText());
      }
    }

    /* Update executable address */
    address = watchpointMote.getExecutableAddressOf(codeFile, lineNr);
    if (address < 0) {
      logger.fatal("Could not restore breakpoint, did source code change?");
      return false;
    }
    createMonitor();

    return true;
  }

  public String toString() {
    return getMote() + ": " + getDescription();
  }

}
