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
 *
 */

package se.sics.cooja.avrmote.plugins;

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.util.List;

import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPopupMenu;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.apache.log4j.Logger;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.GUI;
import se.sics.cooja.Mote;
import se.sics.cooja.MotePlugin;
import se.sics.cooja.PluginType;
import se.sics.cooja.Simulation;
import se.sics.cooja.SupportedArguments;
import se.sics.cooja.VisPlugin;
import se.sics.cooja.avrmote.AvroraMote;
import se.sics.cooja.dialogs.UpdateAggregator;

@ClassDescription("Avr CLI")
@PluginType(PluginType.MOTE_PLUGIN)
@SupportedArguments(motes = {AvroraMote.class})
public class AvrCLI extends VisPlugin implements MotePlugin {
  private static Logger logger = Logger.getLogger(AvrCLI.class);
  private static final long serialVersionUID = 2833218439838209672L;

  private Mote myMote;
  private JTextArea logArea;
  private JTextField commandField;
  private String[] history = new String[50];
  private int historyPos = 0;
  private int historyCount = 0;
  private String command;
  private avrora.sim.mcu.USART usart;
  private byte charToSend;
  private boolean charSent=true;
  private String stringToSend="";

  public AvrCLI(Mote mote, Simulation simulationToVisualize, GUI gui) {
    super("Avr CLI (" + mote.getID() + ')', gui);
    this.myMote = mote;

 avrora.sim.mcu.AtmelMicrocontroller mcu = (avrora.sim.mcu.AtmelMicrocontroller) ((AvroraMote)mote).CPU.getSimulator().getMicrocontroller();
    usart = (avrora.sim.mcu.USART) mcu.getDevice("usart0");
    if (usart == null) usart = (avrora.sim.mcu.USART) mcu.getDevice("usart1");
    if (usart != null) {
      usart.connect(  new avrora.sim.mcu.USART.USARTDevice() {
        public avrora.sim.mcu.USART.Frame transmitFrame() {
         charSent = true;
          return new avrora.sim.mcu.USART.Frame(charToSend, false, 8);
        }
        public void receiveFrame(avrora.sim.mcu.USART.Frame frame) {
          if (frame.value != 10) {
            stringToSend+=Character.toString((char)frame.value);
          } else {
            addCLIData(stringToSend);
            stringToSend="";
          }
        }
      });
    } else {
     System.out.println("*** Warning Avrora could not find usart1 or usart0 interface...");
    }

    final Container panel = getContentPane();

    logArea = new JTextArea(4, 20);
    logArea.setTabSize(8);
    logArea.setEditable(false);
    panel.add(new JScrollPane(logArea), BorderLayout.CENTER);
/*
    LineListener lineListener = new LineListener() {
      public void lineRead(String line) {
        addCLIData(line);
      }
    };

    PrintStream po = new PrintStream(new LineOutputStream(lineListener));
    final CommandContext commandContext = new CommandContext(mspMote.getCLICommandHandler(), null, "", new String[0], 1, null);
    commandContext.out = po;
    commandContext.err = po;
*/
    JPopupMenu popupMenu = new JPopupMenu();
    JMenuItem clearItem = new JMenuItem("Clear");
    clearItem.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        logArea.setText("");
      }
    });
    popupMenu.add(clearItem);
    logArea.setComponentPopupMenu(popupMenu);

    ActionListener action = new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        command = trim(commandField.getText());
        if (command != null) {
          try {
            int previous = historyCount - 1;
            if (previous < 0) {
              previous += history.length;
            }
            if (!command.equals(history[previous])) {
              history[historyCount] = command;
              historyCount = (historyCount + 1) % history.length;
            }
            historyPos = historyCount;
            addCLIData("> " + command);
            for (int i=0; i<command.length();i++) {
                // must give the simulation time to receive the character
                //TODO:an actual sending baud rate

                charToSend = (byte)command.charAt(i);
                while (usart.receiving) {
              //  while (!charSent) {
                logger.debug("oops"); //hangs if this happens
                    Thread.sleep(100);
                    break;
                }
                charSent = false;
                usart.startReceive();
                Thread.sleep(1);
            }
            charToSend = (byte)'\n';
            usart.startReceive();
         //   mspMote.executeCLICommand(command, commandContext);
            commandField.setText("");
          } catch (Exception ex) {
            System.err.println("could not send '" + command + "':");
            ex.printStackTrace();
            JOptionPane.showMessageDialog(panel,
                "could not send '" + command + "':\n"
                + ex, "ERROR",
                JOptionPane.ERROR_MESSAGE);
          }
        } else {
          commandField.getToolkit().beep();
        }
      }

    };
    commandField = new JTextField();
    commandField.addActionListener(action);
    commandField.addKeyListener(new KeyAdapter() {

      @Override
      public void keyPressed(KeyEvent e) {
        switch (e.getKeyCode()) {
        case KeyEvent.VK_UP: {
          int nextPos = (historyPos + history.length - 1) % history.length;
          if (nextPos == historyCount || history[nextPos] == null) {
            commandField.getToolkit().beep();
          } else {
            String cmd = trim(commandField.getText());
            if (cmd != null) {
              history[historyPos] = cmd;
            }
            historyPos = nextPos;
            commandField.setText(history[historyPos]);
          }
          break;
        }
        case KeyEvent.VK_DOWN: {
          int nextPos = (historyPos + 1) % history.length;
          if (nextPos == historyCount) {
            historyPos = nextPos;
            commandField.setText("");
          } else if (historyPos == historyCount || history[nextPos] == null) {
            commandField.getToolkit().beep();
          } else {
            String cmd = trim(commandField.getText());
            if (cmd != null) {
              history[historyPos] = cmd;
            }
            historyPos = nextPos;
            commandField.setText(history[historyPos]);
          }
          break;
        }
        }
      }

    });

    cliResponseAggregator.start();

    panel.add(commandField, BorderLayout.SOUTH);
    setSize(500,500);
  }

  public void closePlugin() {
    cliResponseAggregator.stop();
  }

  public void addCLIData(final String text) {
    cliResponseAggregator.add(text);
  }

  private static final int UPDATE_INTERVAL = 250;
  private UpdateAggregator<String> cliResponseAggregator = new UpdateAggregator<String>(UPDATE_INTERVAL) {
    protected void handle(List<String> ls) {
      String current = logArea.getText();
      int len = current.length();
      if (len > 4096) {
        current = current.substring(len - 4096);
      }

      /* Add */
      StringBuilder sb = new StringBuilder(current);
      for (String l: ls) {
        sb.append(l);
        sb.append('\n');
      }
      logArea.setText(sb.toString());
      logArea.setCaretPosition(sb.length());
    }
  };

  private String trim(String text) {
    return (text != null) && ((text = text.trim()).length() > 0) ? text : null;
  }

  public Mote getMote() {
    return myMote;
  }

}
