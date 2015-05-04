/*
 * Copyright (c) 2012, Swedish Institute of Computer Science. All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer. 2. Redistributions in
 * binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution. 3. Neither the name of the
 * Institute nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.contikios.cooja.motes;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;

import javax.swing.Icon;
import javax.swing.JComponent;
import javax.swing.JLabel;

import org.apache.log4j.Logger;
import org.jdom.Element;

import org.contikios.cooja.Cooja;
import org.contikios.cooja.MoteInterface;
import org.contikios.cooja.MoteType;
import org.contikios.cooja.ProjectConfig;
import org.contikios.cooja.Simulation;
import org.contikios.cooja.interfaces.IPAddress;

public abstract class AbstractMoteType implements MoteType {
  public static Logger logger = Logger.getLogger(AbstractMoteType.class);

  private String identifier = null;
  private String description = null;

  /* If source file is defined, the firmware is recompiled when loading simulations */
  private File fileSource = null;
  private String compileCommands = null;
  private File fileFirmware = null;

  private Class<? extends MoteInterface>[] moteInterfaceClasses = null;

  public String getIdentifier() {
    return identifier;
  }
  public void setIdentifier(String identifier) {
    this.identifier = identifier;
  }

  public String getDescription() {
    return description;
  }
  public void setDescription(String description) {
    this.description = description;
  }

  public String getCompileCommands() {
    return compileCommands;
  }
  public void setCompileCommands(String command) {
    this.compileCommands = command;
  }

  public File getContikiSourceFile() {
    return fileSource;
  }
  public void setContikiSourceFile(File file) {
    fileSource = file;
  }

  public File getContikiFirmwareFile() {
    return fileFirmware;
  }
  public void setContikiFirmwareFile(File file) {
    this.fileFirmware = file;
  }

  public Class<? extends MoteInterface>[] getMoteInterfaceClasses() {
    return moteInterfaceClasses;
  }
  public void setMoteInterfaceClasses(Class<? extends MoteInterface>[] classes) {
    moteInterfaceClasses = classes;
  }

  public ProjectConfig getConfig() {
    /*logger.warn("getConfig() not implemented");*/
    return null;
  }

  public abstract Icon getMoteTypeIcon();

  public JComponent getTypeVisualizer() {
    StringBuilder sb = new StringBuilder();
    // Identifier
    sb.append("<html><table><tr><td>Identifier</td><td>")
    .append(getIdentifier()).append("</td></tr>");

    // Description
    sb.append("<tr><td>Description</td><td>")
    .append(getDescription()).append("</td></tr>");

    /* Contiki source */
    sb.append("<tr><td>Contiki source</td><td>");
    if (getContikiSourceFile() != null) {
      sb.append(getContikiSourceFile().getAbsolutePath());
    } else {
      sb.append("[not specified]");
    }
    sb.append("</td></tr>");

    /* Contiki firmware */
    sb.append("<tr><td>Contiki firmware</td><td>")
    .append(getContikiFirmwareFile().getAbsolutePath()).append("</td></tr>");

    /* Compile commands */
    String compileCommands = getCompileCommands();
    if (compileCommands == null) {
        compileCommands = "";
    }
    sb.append("<tr><td valign=top>Compile commands</td><td>")
    .append(compileCommands.replace("<", "&lt;").replace(">", "&gt;").replace("\n", "<br>")).append("</td></tr>");

    JLabel label = new JLabel(sb.append("</table></html>").toString());
    label.setVerticalTextPosition(JLabel.TOP);
    /* Icon (if available) */
    if (!Cooja.isVisualizedInApplet()) {
      Icon moteTypeIcon = getMoteTypeIcon();
      if (moteTypeIcon != null) {
        label.setIcon(moteTypeIcon);
      }
    }
    return label;
  }

  public abstract Class<? extends MoteInterface>[] getAllMoteInterfaceClasses();
  public abstract File getExpectedFirmwareFile(File source);

  public Collection<Element> getConfigXML(Simulation simulation) {
    ArrayList<Element> config = new ArrayList<Element>();
    Element element;

    element = new Element("identifier");
    element.setText(getIdentifier());
    config.add(element);

    element = new Element("description");
    element.setText(getDescription());
    config.add(element);

    if (fileSource != null) {
      element = new Element("source");
      File file = simulation.getCooja().createPortablePath(fileSource);
      element.setText(file.getPath().replaceAll("\\\\", "/"));
      element.setAttribute("EXPORT", "discard");
      config.add(element);
      element = new Element("commands");
      element.setText(compileCommands);
      element.setAttribute("EXPORT", "discard");
      config.add(element);
    }

    element = new Element("firmware");
    File file = simulation.getCooja().createPortablePath(fileFirmware);
    element.setText(file.getPath().replaceAll("\\\\", "/"));
    element.setAttribute("EXPORT", "copy");
    config.add(element);

    for (Class<? extends MoteInterface> moteInterface : getMoteInterfaceClasses()) {
      element = new Element("moteinterface");
      element.setText(moteInterface.getName());
      config.add(element);
    }
    return config;
  }

  @SuppressWarnings("unchecked")
  public boolean setConfigXML(Simulation simulation,
      Collection<Element> configXML, boolean visAvailable)
      throws MoteTypeCreationException {

    ArrayList<Class<? extends MoteInterface>> intfClassList = new ArrayList<Class<? extends MoteInterface>>();
    for (Element element : configXML) {
      String name = element.getName();

      if (name.equals("identifier")) {
        identifier = element.getText();
      } else if (name.equals("description")) {
        description = element.getText();
      } else if (name.equals("source")) {
        fileSource = new File(element.getText());
        if (!fileSource.exists()) {
          fileSource = simulation.getCooja().restorePortablePath(fileSource);
        }
      } else if (name.equals("command")) {
        /* Backwards compatibility: command is now commands */
        logger.warn("Old simulation config detected: old version only supports a single compile command");
        compileCommands = element.getText();
      } else if (name.equals("commands")) {
        compileCommands = element.getText();
      } else if (name.equals("firmware")) {
        fileFirmware = new File(element.getText());
        if (!fileFirmware.exists()) {
          fileFirmware = simulation.getCooja().restorePortablePath(fileFirmware);
        }
      } else if (name.equals("elf")) {
        /* Backwards compatibility: elf is now firmware */
        logger.warn("Old simulation config detected: firmware specified as elf");
        fileFirmware = new File(element.getText());
      } else if (name.equals("moteinterface")) {
        String intfClass = element.getText().trim();

        /* Backwards compatibility: se.sics -> org.contikios */
        if (intfClass.startsWith("se.sics")) {
          intfClass = intfClass.replaceFirst("se\\.sics", "org.contikios");
        }

        /* Backwards compatibility: MspIPAddress -> IPAddress */
        if (intfClass.equals("org.contikios.cooja.mspmote.interfaces.MspIPAddress")) {
          logger.warn("Old simulation config detected: IP address interface was moved");
          intfClass = IPAddress.class.getName();
        } else if (intfClass.equals("org.contikios.cooja.mspmote.interfaces.ESBLog")) {
          logger.warn("Old simulation config detected: ESBLog was replaced by MspSerial");
          intfClass = "org.contikios.cooja.mspmote.interfaces.MspSerial";
        } else if (intfClass.equals("org.contikios.cooja.mspmote.interfaces.SkySerial")) {
          logger.warn("Old simulation config detected: SkySerial was replaced by MspSerial");
          intfClass = "org.contikios.cooja.mspmote.interfaces.MspSerial";
        } else if (intfClass.equals("org.contikios.cooja.mspmote.interfaces.SkyByteRadio")) {
          logger.warn("Old simulation config detected: SkyByteRadio was replaced by Msp802154Radio");
          intfClass = "org.contikios.cooja.mspmote.interfaces.Msp802154Radio";
        } else if (intfClass.equals("org.contikios.cooja.avrmote.interfaces.MicaClock")) {
          logger.warn("Old simulation config detected: MicaClock was replaced by AvroraClock");
          intfClass = "org.contikios.cooja.avrmote.interfaces.AvroraClock";
        } else if (intfClass.equals("org.contikios.cooja.avrmote.interfaces.MicaSerial")) {
          logger.warn("Old simulation config detected: MicaClock was replaced by AvroraUsart0");
          intfClass = "org.contikios.cooja.avrmote.interfaces.AvroraUsart0";
        } else if (intfClass.equals("org.contikios.cooja.avrmote.interfaces.MicaZLED")) {
          logger.warn("Old simulation config detected: MicaClock was replaced by AvroraLED");
          intfClass = "org.contikios.cooja.avrmote.interfaces.AvroraLED";
        }

        Class<? extends MoteInterface> moteInterfaceClass =
          simulation.getCooja().tryLoadClass(this, MoteInterface.class, intfClass);

        if (moteInterfaceClass == null) {
          logger.warn("Can't find mote interface class: " + intfClass);
        } else {
          intfClassList.add(moteInterfaceClass);
        }
      } else {
        logger.fatal("Unrecognized entry in loaded configuration: " + name);
        throw new MoteTypeCreationException(
            "Unrecognized entry in loaded configuration: " + name);
      }
    }

    Class<? extends MoteInterface>[] intfClasses = intfClassList.toArray(new Class[0]);

    if (intfClasses.length == 0) {
      /* Backwards compatibility: No interfaces specifed */
      logger.warn("Old simulation config detected: no mote interfaces specified, assuming all.");
      intfClasses = getAllMoteInterfaceClasses();
    }
    setMoteInterfaceClasses(intfClasses);

    if (fileFirmware == null) {
      if (fileSource == null) {
        throw new MoteTypeCreationException("Neither source or firmware specified");
      }

      /* Backwards compatibility: Generate expected firmware file name from source */
      logger.warn("Old simulation config detected: no firmware file specified, generating expected");
      fileFirmware = getExpectedFirmwareFile(fileSource);
    }

    return configureAndInit(Cooja.getTopParentContainer(), simulation, visAvailable);
  }
}
