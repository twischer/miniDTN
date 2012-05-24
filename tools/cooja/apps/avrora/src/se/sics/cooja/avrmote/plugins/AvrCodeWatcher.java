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

import java.awt.Color;
import java.awt.Component;
import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import java.util.Observable;
import java.util.Observer;
import java.util.Collection;
import java.util.Vector;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;

import java.io.File;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;

import javax.swing.SwingConstants;
import javax.swing.AbstractAction;
import javax.swing.JFileChooser;
import javax.swing.JComponent;
import javax.swing.filechooser.FileFilter;
import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JToggleButton;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JSlider;
import javax.swing.JSplitPane;
import javax.swing.JTabbedPane;
import javax.swing.JTable;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.JScrollBar;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.text.DefaultCaret;
import javax.swing.text.Highlighter;
import javax.swing.text.DefaultHighlighter;
import javax.swing.SwingUtilities;
import javax.swing.plaf.basic.BasicComboBoxRenderer;

import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.Dimension;

import org.apache.log4j.Logger;
import org.jdom.Element;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.GUI;
import se.sics.cooja.Mote;
import se.sics.cooja.MotePlugin;
import se.sics.cooja.PluginType;
import se.sics.cooja.Simulation;
import se.sics.cooja.VisPlugin;
import se.sics.cooja.avrmote.*;
import se.sics.cooja.interfaces.Clock;
import se.sics.cooja.plugins.MoteInterfaceViewer;
//import se.sics.cooja.Watchpoint;
//import se.sics.cooja.WatchpointMote;
//import se.sics.cooja.WatchpointMote.WatchpointListener;
import se.sics.cooja.dialogs.MessageList;

import avrora.sim.AtmelInterpreter;
import avrora.sim.mcu.DefaultMCU;
import avrora.sim.FiniteStateMachine;
import avrora.sim.Simulator;
import avrora.sim.State;

/**
 * @author David Kopf
 */
@ClassDescription("Avr Code Watcher test")
@PluginType(PluginType.MOTE_PLUGIN)
public class AvrCodeWatcher extends VisPlugin implements MotePlugin {
  private static Logger logger = Logger.getLogger(AvrCodeWatcher.class);
  private static final int SOURCECODE = 0;
  private static final int BREAKPOINTS = 2;
  private Simulation simulation;
  private AtmelInterpreter interpreter;
  private Mote myMote;
  private FiniteStateMachine myFSM;
  private File objdumpFile = null;
  private long timeDrift; /* Microseconds */
  private long startTime,lastTime,lastCycles,displayDelay;
  private boolean sourceActive=false,asmActive=false;
  private JPanel jPanel;
  private JSplitPane splitPane;
  private Dimension originalViewerDimension;
//    private WatchpointMote watchpointMote;
//  private WatchpointListener watchpointListener;
    private JComboBox fileComboBox;
      private JTabbedPane mainPane;
  private File[] sourceFiles;
    private ArrayList<Rule> rules;
      private Observer observer;
        private MessageList rulesDebuggingOutput = new MessageList();
  private boolean rulesWithDebuggingOutput = false;
  private int[] rulesMatched = null;
  private int[] rulesOK = null;
  private JTable table = null;
 // private ELFDebug debug;
  private String[] debugSourceFiles;

  public AvrCodeWatcher(Mote mote, Simulation simulationToVisualize, GUI gui) {
    super("Avr Code Watcher - " + mote, gui);
    myMote = mote;
 //       this.watchpointMote = (WatchpointMote) mote;
    simulation = mote.getSimulation();
 //   simulation = simulationToVisualize
  //  interpreter = null;
    interpreter = (AtmelInterpreter)((AvroraMote)myMote).CPU.getSimulator().getInterpreter();
    myFSM = ((DefaultMCU)((AvroraMote)myMote).CPU.getSimulator().getMicrocontroller()).getFSM();

    if (interpreter == null) {
        logger.debug("Mote interpreter is null");
    }
    if (myFSM == null) {
        logger.debug("microcontroller FSM is null");
    }
    startTime = simulation.getSimulationTime();
    lastTime = startTime;
    lastCycles = 0;
    timeDrift = 0;
    /*
    try {
      debug = ((MspMoteType)mspMote.getType()).getELF().getDebug();
      if (debug == null) {
        throw new RuntimeException("No debugging info found in firmware, aborting");
      }
      debugSourceFiles = debug.getSourceFiles();
      if (debugSourceFiles == null) {
        throw new RuntimeException("No debugging info found in firmware, aborting");
      }
    } catch (IOException e1) {
      throw new RuntimeException("No debugging info found in firmware, aborting");
    }
*/
    /* XXX Temporary workaround: source file removing duplicates awaiting Mspsim update */
  /*  {
      ArrayList<String> newDebugSourceFiles = new ArrayList<String>();
      for (String sf: debugSourceFiles) {
        boolean found = false;
        for (String nsf: newDebugSourceFiles) {
          if (sf.equals(nsf)) {
            found = true;
            break;
          }
        }
        if (!found) {
          newDebugSourceFiles.add(sf);
        }
      }
      debugSourceFiles = newDebugSourceFiles.toArray(new String[0]);
    }
*/

    rules = new ArrayList<Rule>();

  //  loadDefaultRules();

    getContentPane().setLayout(new BorderLayout());
  //  getInterfaceVisualizer();
    /* Create source file list */
    fileComboBox = new JComboBox();
    fileComboBox.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        sourceFileSelectionChanged();
      }
    });
    fileComboBox.setRenderer(new BasicComboBoxRenderer() {
      private static final long serialVersionUID = -2135703608960229528L;
      public Component getListCellRendererComponent(JList list, Object value,
          int index, boolean isSelected, boolean cellHasFocus) {
        if (isSelected) {
          setBackground(list.getSelectionBackground());
          setForeground(list.getSelectionForeground());
          if (index > 0) {
            list.setToolTipText(sourceFiles[index-1].getPath());
          }
        } else {
          setBackground(list.getBackground());
          setForeground(list.getForeground());
        }
        setFont(list.getFont());
        setText((value == null) ? "" : value.toString());
        return this;
      }
    });

    /* Browse code control (north) */
    Box sourceCodeControl = Box.createHorizontalBox();
    sourceCodeControl.add(new JButton(stepAction));
    sourceCodeControl.add(Box.createHorizontalStrut(10));
    sourceCodeControl.add(new JLabel("Current location: "));
    sourceCodeControl.add(new JButton(currentFileAction));
    sourceCodeControl.add(Box.createHorizontalGlue());
    sourceCodeControl.add(new JLabel("Source files: "));
    sourceCodeControl.add(fileComboBox);
    sourceCodeControl.add(Box.createHorizontalStrut(5));
    sourceCodeControl.add(new JButton(mapAction));
    sourceCodeControl.add(Box.createHorizontalStrut(10));

    /* Execution control panel (south of source code panel) */

    /* Layout */
    mainPane = new JTabbedPane();

  //  sourceCodeUI = new CodeUI(watchpointMote);
  //    JPanel sourceCodePanel = new JPanel(new BorderLayout());
    JPanel sourceCodePanel = getInterfaceVisualizer();
 //   sourceCodePanel.add(BorderLayout.CENTER, sourceCodeUI);
  //  sourceCodePanel.add(BorderLayout.SOUTH, sourceCodeControl);
    mainPane.addTab("Source code", null, sourceCodePanel, null); /* SOURCECODE */
/*
    assCodeUI = new DebugUI(this.mspMote.getCPU(), true);
    for (Component c: assCodeUI.getComponents()) {
      c.setBackground(Color.WHITE);
    }
    mainPane.addTab("Instructions", null, assCodeUI, null);
*/
   // breakpointsUI = new BreakpointsUI(mspMote, this);
//mainPane.addTab("Breakpoints", null, breakpointsUI, "Right-click source code to add"); /* BREAKPOINTS */

    add(BorderLayout.CENTER, mainPane);

    /* Listen for breakpoint changes */
    /*
    watchpointMote.addWatchpointListener(watchpointListener = new WatchpointListener() {
      public void watchpointTriggered(final Watchpoint watchpoint) {
        SwingUtilities.invokeLater(new Runnable() {
          public void run() {
            logger.info("Watchpoint triggered: " + watchpoint);
            if (simulation.isRunning()) {
              return;
            }
            breakpointsUI.selectBreakpoint(watchpoint);
            sourceCodeUI.updateBreakpoints();
            showCurrentPC();
          }
        });
      }
      public void watchpointsChanged() {
        sourceCodeUI.updateBreakpoints();
      }
    });
    */

    setSize(750, 800);
 //   showCurrentPC();
  }

  public void startPlugin() {
    super.startPlugin();
    updateFileComboBox();
  }
    private void updateFileComboBox() {
  // logger.debug("updatefilecombobox");
    sourceFiles = getSourceFiles(myMote, rules);
    fileComboBox.removeAllItems();
    fileComboBox.addItem("[view sourcefile]");
    
  //  for (File f: sourceFiles) {
  //    fileComboBox.addItem(f.getName());
  //  }
    
    fileComboBox.setSelectedIndex(0);
  //  getInterfaceVisualizer();
  }
  public void closePlugin() {
 //   watchpointMote.removeWatchpointListener(watchpointListener);
  //  watchpointListener = null;

    simulation.deleteObserver(observer);
    observer = null;

    /* TODO XXX Unregister breakpoints? */
  }
    private class Rule {
    String from = "";
    String to = "";
    int prefixMatches = 0;
    int locatesFile = 0;
    public Rule(String from, String to) {
      this.from = from;
      this.to = to;
    }
    File apply(File f) {
      if (f == null) {
        return null;
      }
      if (from == null) {
        return null;
      }
      if (!f.getPath().startsWith(from)) {
        if (rulesWithDebuggingOutput) {
          rulesDebuggingOutput.addMessage(this + " does not match: " + f.getPath(), MessageList.ERROR);
        }
        return null;
      }
      prefixMatches++;
      if (rulesWithDebuggingOutput) {
        rulesDebuggingOutput.addMessage(this + " testing on: " + f.getPath());
      }
      if (to == null) {
        if (rulesWithDebuggingOutput) {
          rulesDebuggingOutput.addMessage(this + " enter substition: " + f.getPath(), MessageList.ERROR);
        }
        return null;
      }
      File file = new File(to + f.getPath().substring(from.length()));
      if (!file.exists()) {
        if (rulesWithDebuggingOutput) {
          rulesDebuggingOutput.addMessage(this + " not found: " + file.getPath(), MessageList.ERROR);
        }
        return null;
      }
      if (rulesWithDebuggingOutput) {
        rulesDebuggingOutput.addMessage(this + " OK: " + f.getPath());
      }
      locatesFile++;
      return file;
    }
    public String toString() {
      return "[" + from + "|" + to + "]";
    }
  }
  private File[] getSourceFiles(Mote mote, ArrayList<Rule> rules) {
    File contikiSource = mote.getType().getContikiSourceFile();
    if (contikiSource != null) {
      try {
        contikiSource = contikiSource.getCanonicalFile();
      } catch (IOException e1) {
      }
    }

    /* Verify that files exist */
  //  logger.debug("verifyfilesexist");
    /*
    ArrayList<File> existing = new ArrayList<File>();
    for (String sourceFile: debugSourceFiles) {
        logger.debug("generatingexisting");
      File f = applySubstitutionRules(sourceFile, rules);
      if (f != null && f.exists()) {
        existing.add(f);
      }
    }

    File[] sorted = existing.toArray(new File[0]);
    Arrays.sort(sorted, new Comparator<File>() {
      public int compare(File o1, File o2) {
        return o1.getName().compareToIgnoreCase(o2.getName());
      }
    });
    return sorted;
    */
    return new File[1];
    
  }

  public void displaySourceFile(final File file, final int line, final boolean markCurrent) {
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        mainPane.setSelectedIndex(SOURCECODE); /* code */
    //    sourceCodeUI.displayNewCode(file, line, markCurrent);
      }});
  }

  private void sourceFileSelectionChanged() {
    int index = fileComboBox.getSelectedIndex();
    if (index <= 0) {
      return;
    }

    File selectedFile = sourceFiles[index-1];
    displaySourceFile(selectedFile, -1, false);
  }

  public void setTime(long newTime) {
    logger.fatal("Can't change emulated CPU time");
  }

  public long getTime() {
    return simulation.getSimulationTime() + timeDrift;
  }

  public void setDrift(long drift) {
    timeDrift = drift;
  }

  public long getDrift() {
    return timeDrift;
  }
  
  private JLabel timeLabel, cyclesLabel, stateLabel, watchLabel, sourceLabel, assemLabel;
  private JTextArea srcText=null, asmText=null;
  private JScrollPane stackPane, tracePane, srcPane, asmPane;
  private JToggleButton runButton;

  private JTextField searchText;
  private JButton prevButton,nextButton;
  private String searchString;
  private int currIndex, prevIndex, nextIndex;
  private Highlighter srcHilite = null, asmHilite = null, searchHilite = null;
  private Highlighter.HighlightPainter srcPainter = null, asmPainter = null, searchPainter = null;

  private int[] srcPC, asmPC, srcStartPC, asmStartPC, asmBreakPC, srcBreakPC;
  private int srcNumBreaks = 0, asmNumBreaks = 0;
  private int lastAsmLineStart=-1, lastSrcLineStart=-1,  srcRoutineStart = 0, srcLastLine = 0, lastLineOfSource = 0;
  private int srcMarkedLine = -1, asmMarkedLine = -1, stepOverPC = 0;
  private String srcLastTag, asmLastTag;

  private Simulator.Probe liveProbe = null;
  private boolean probeInserted = false;
  private boolean stepOverPress = false, stepIntoPress = false, halted = false, disableBreaks = false, liveUpdate = false;
  private boolean outofroutine = false;
  private boolean interruptOccurred = false;
  
  // insert or remove avrora pc probe as needed for breakpoints or stepping
  void setProbeState() {
    if (liveUpdate || halted || (!disableBreaks && (asmActive && (asmNumBreaks > 0)) || (!asmActive && (srcNumBreaks > 0))) ) {
    // create the probe the first time
    if (liveProbe == null) liveProbe = new Simulator.Probe() {
            public void fireBefore(State state, int pc) {
            //fireBefore may be before the instruction is executed?
            }

            public void fireAfter(State state, int pc) {

                // check for breakpoints
                if (!disableBreaks) {
                    if (asmActive) {
                        if (asmNumBreaks > 0) {
                            for (int i=0;i<asmNumBreaks;i++) {
                                if (asmBreakPC[i] == pc) {
                                    halted = true;
                                    srcRoutineStart = 0;
                                    stepOverPC = 0;
                                    srcLastLine = 0;
                                }
                            }
                        }
                    } else if (srcNumBreaks > 0) {
                        for (int i=0;i<srcNumBreaks;i++) {
                            if (srcBreakPC[i] == pc) {
                                halted = true;
                                srcRoutineStart = 0;
                                stepOverPC = 0;
                                srcLastLine = 0;
                            }
                        }
                    }
                }
                if (!halted) {
                    // always update if live
                    if (liveUpdate) {
                        if (updatePanel(pc)) {
                            // add delay if pc marker changed in either pane
                            if (displayDelay > 0) {
                                try{Thread.sleep(displayDelay);}catch (Exception e){System.err.println("displayDelay: " + e.getMessage());}
                            }
                        }
                    }
                    return;
                }
                
                // check for asm step over
                if (stepOverPC != 0) {
                    if (stepOverPC != pc) return;
               //     logger.debug("stepoverPC breakpoint "+ Integer.toHexString(stepOverPC) + " pc is "+Integer.toHexString(pc)  );
                    stepOverPC = 0;
                }
                // check for src step over
                if (asmActive) {
                } else if (srcRoutineStart != 0) {
                    int i = pc/2;
                    while (asmPC[--i] == 0) {};
                    boolean atlinestart = (srcPC[i] != srcPC[pc/2]);
                    // if stepping over wait till pc returns to a different line in this routine
                    if (srcStartPC[pc/2] != srcRoutineStart) {
                        outofroutine = true;
                       // logger.debug("Skip " + Integer.toHexString(pc) + " in routine " +srcStartPC[pc/2]+ " at line " + srcPC[pc/2]);
                        return;
                    } else if (srcPC[pc/2] != srcLastLine) {
                        if (srcPC[pc/2] < 2) {
                            if (!interruptOccurred) {
                                interruptOccurred = true;
                                logger.debug("interrupt while stepping over!");
                            }
                            //srcRoutineStart = 0; //stop at next source line in interrupt
                            return;
                        } else {
                            srcLastLine = 0;
                            srcRoutineStart = 0;    
                      //      logger.debug("Stop at " + Integer.toHexString(pc) + " in routine " +srcStartPC[pc/2]+ " at line " + srcPC[pc/2]);
                        }
                    } else if (outofroutine) {
                        //we returned to the same line in this routine
                        //stop only if start of line                       
                        if (atlinestart) {
                 //           logger.debug("Stop at same line " + Integer.toHexString(pc) + " in routine " +srcStartPC[pc/2]+ " at line " + srcPC[pc/2]);
                        } else {  
                  //          logger.debug("not start " + Integer.toHexString(pc) + " " +srcPC[pc/2 - 1] + " "+  srcPC[pc/2]);
                            return;
                        }
                        //wait till line number changes
                        srcRoutineStart = 0;                        
                    } else {
                    //logger.debug("asm skip " + Integer.toHexString(pc) + " in routine " +srcStartPC[pc/2]+ " at line " + srcPC[pc/2]); 
                    return;
                    }
                // check for src step into
                } else if (srcLastLine !=0) {
             //   logger.debug("stepping over line "+srcLastLine);
                    //if stepping into wait for line number change
                    if (srcPC[pc/2] != srcLastLine) {
                    //    logger.debug("src line change " + Integer.toHexString(pc) + " in routine " +srcStartPC[pc/2]+ " at line " + srcPC[pc/2]); 
                        //unless it is off the end of the source
                        if (srcStartPC[pc/2] < lastLineOfSource) {
                            srcLastLine = 0;
                        }
                    }
                    return;
                }

                updatePanel(pc);
           //      logger.debug("the pc is  0x" + Integer.toHexString(pc));
                interruptOccurred = false;
                stepOverPress = stepIntoPress = false;
                if (halted) runButton.setSelected(false);
                while (halted) {
                    try{Thread.sleep(20);}catch (Exception e){System.err.println("Err: " + e.getMessage());}
                    if (stepIntoPress) {
                        //  stop at next PC
                        if (asmActive) {
                            stepOverPC = 0;
                            break;
                        } else {
                            // stop when src line number changes
                            srcRoutineStart = 0;
                            srcLastLine = srcPC[pc/2];
                            break;
                            
                        }
                    }
                    if (stepOverPress) try {
                        if (asmActive) {
                            int linenumber = asmPC[pc/2];
                            int linestart = asmText.getLineStartOffset(linenumber);
                            String string = asmText.getText().substring(linestart+24,linestart+28);
                         //   logger.debug("asm stepover 0x" + Integer.toHexString(pc) + " at " + linenumber + " " + string);

                            if (string.equals("call")) {
                                stepOverPC = pc + 4;
                                return;
                            } else if (string.equals("rcal")) {
                                stepOverPC = pc + 2;
                                return;
                            }
                            break;
                        } else {                                              
                            // convert to step if last line of routine or explicit return
                            int i = pc/2;
                            srcRoutineStart = 0;
                            srcLastLine = srcPC[i];
                            if (srcLastLine >= lastLineOfSource) {
                              //  logger.debug("line past end of source");
                                break;
                            }
                            int linestart = srcText.getLineStartOffset(srcLastLine);
                            int lineend = srcText.getLineStartOffset(srcLastLine+1);
                            String str = srcText.getText().substring(linestart,lineend);
                            if (srcStartPC[i] != srcStartPC[i+1]) {
                              //  logger.debug("last line of routine " + srcStartPC[i] + " " + srcStartPC[i+1]);
                                outofroutine = false;
                            } else if (str.indexOf("return",0) >0 ) {
                               // logger.debug(" explicit return found line "+ srcStartPC[i]+" " + str);
                            } else if ((str.indexOf("}",0) > 0) && (str.indexOf("}",0) < 8 ) ) {
                              //  logger.debug("  } found line "+ srcStartPC[i]+" " + str);
                            } else {
                                // run until pc reenters this routine
                                srcRoutineStart = srcStartPC[pc/2];
                                outofroutine = false;
                             //   logger.debug("srcroutine start 0x" + Integer.toHexString(pc) + " in " + srcRoutineStart + " at line " + srcLastLine + " " + str);
                            }
                            
                            break; 
                       }
                    }catch (Exception e){System.err.println("stepOverPress error: " + e.getMessage());}
                }
            }
        };
        if (!probeInserted) {
       //    logger.debug("addprobe");
            interpreter.insertProbe(liveProbe);
            probeInserted = true;
        }
    } else {
        if (probeInserted) {
      //  logger.debug("removeprobe");
            interpreter.removeProbe(liveProbe);
            probeInserted = false;
        }
    }
  }

// scroll startIndex character to visible in source pane
private void srcScrollVis(int startIndex) {try{
    JScrollBar scrollbar = srcPane.getVerticalScrollBar();
    int rowheight = srcText.getScrollableUnitIncrement(srcPane.getVisibleRect(),SwingConstants.VERTICAL,1);
    int topline = scrollbar.getValue()/rowheight;
    int linenumber = srcText.getLineOfOffset(startIndex);
    if ((linenumber-8) < topline) {
        topline = linenumber-8;
        if (topline < 0) topline = 0;
        scrollbar.setValue(topline*rowheight);
    } else if ((linenumber+2) > (topline+32)) {
        topline = linenumber-8; 
        scrollbar.setValue(topline*rowheight);
    }
}catch(Exception e) {System.err.println("srcScrollVis: " + e.getMessage());}}

// scroll startIndex character to visible in assembly pane
private void asmScrollVis(int startIndex) {try{
    JScrollBar scrollbar = asmPane.getVerticalScrollBar();
    int rowheight = asmText.getScrollableUnitIncrement(srcPane.getVisibleRect(),SwingConstants.VERTICAL,1);
    int topline = scrollbar.getValue()/rowheight;
    int linenumber = asmText.getLineOfOffset(startIndex);
    if ((linenumber-8) < topline) {
        topline = linenumber-8;
        if (topline < 0) topline = 0;
        scrollbar.setValue(topline*rowheight);
    } else if ((linenumber+2) > (topline+32)) {
        topline = linenumber-8; 
        scrollbar.setValue(topline*rowheight);
    }
}catch(Exception e) {System.err.println("asmScrollVis: " + e.getMessage());}}

// toggle breakpoint indicator at lineNumber in src. Returns true if BP was set
private boolean srcBPIndicatorToggle(int lineNumber) {try{
    int linestart = srcText.getLineStartOffset(lineNumber);
    String firstchar = srcText.getText().substring(linestart,linestart+1);
    if (firstchar.equals("@")) {
        srcText.replaceRange(" ",linestart,linestart+1);
        return false;
    } else if (firstchar.equals("#")) {
         srcText.replaceRange("=",linestart,linestart+1);
         srcLastTag = "=";
        return false;
    } else if (firstchar.equals("=") && !asmActive) {
        srcText.replaceRange("#",linestart,linestart+1);
        srcLastTag = "@";
        return true;
    } else {
        srcText.replaceRange("@",linestart,linestart+1);
        srcLastTag = "@";
        return true;
    }
}catch(Exception e) {System.err.println("srcBPIndicatorToggle: " + e.getMessage());return false;}}

// toggle breakpoint indicator at lineNumber in asm. Returns true if BP was set
private boolean asmBPIndicatorToggle(int lineNumber) {try{
    int linestart = asmText.getLineStartOffset(lineNumber);
    String firstchar = asmText.getText().substring(linestart,linestart+1);
    if (firstchar.equals("@")) {
        asmText.replaceRange(" ",linestart,linestart+1);
        return false;
    } else if (firstchar.equals("#")) {
         asmText.replaceRange("=",linestart,linestart+1);
         asmLastTag = "=";
        return false;
    } else if (firstchar.equals("=") && asmActive) {
        asmText.replaceRange("#",linestart,linestart+1);
        asmLastTag = "@";
        return true;
    } else {
        asmText.replaceRange("@",linestart,linestart+1);
        asmLastTag = "@";
        return true;
    }
}catch(Exception e) {System.err.println("srcBPIndicatorToggle: " + e.getMessage());return false;}}

  // update the panel to reflect current or passed PC
  // returns true if the src or asm line markers changed on the display
  private boolean updatePanel(int pc) {
    boolean updated = false;
    if (pc == 0 ) pc = interpreter.getState().getPC();
    timeLabel.setText("Run Time  : " + ((double)(simulation.getSimulationTime()-startTime)/1000000) + "   Drift: " + (double)timeDrift/1000000);
    watchLabel.setText("Stopwatch : " + ((double)(simulation.getSimulationTime()-lastTime)/1000000));
    long cycleCount = interpreter.getState().getCycles();
    cyclesLabel.setText("Cycles        : " + (cycleCount - lastCycles));   
    stateLabel.setText("PC: 0x" +  Integer.toHexString(pc) + "      SP: 0x" +  Integer.toHexString(interpreter.getSP())
    + "     State: " + myFSM.getStateName(myFSM.getCurrentState()));
    // not started yet
    if (pc == 0) return(false);

    if (sourceActive) try {
        String strLine;
        int si, i, index;
        pc = pc/2;

         // display pc marker in source when line number changes
        if (srcPane.getSize().width > 42) {
            int linenumber = srcPC[pc];
            // Do nothing if line number past end of source (library asm routines)
            if (linenumber < lastLineOfSource) {
            if (linenumber != srcMarkedLine) {
                updated = true;
                srcMarkedLine = linenumber;           
                if (lastSrcLineStart >= 0) srcText.replaceRange(srcLastTag,lastSrcLineStart,lastSrcLineStart+1);
                int linestart = srcText.getLineStartOffset(linenumber); 
                srcLastTag = srcText.getText().substring(linestart,linestart+1);
                srcText.replaceRange(srcLastTag.equals("@") ? "#" : "=",linestart,linestart+1);
                lastSrcLineStart=linestart;
                srcScrollVis(linestart);
            }
            }
        }
        // display pc marker in asm when line number changes
        if (asmPane.getSize().width > 42) {
            int linenumber = asmPC[pc];
            if (linenumber != asmMarkedLine) {
                if (asmActive) updated = true;
                asmMarkedLine = linenumber;   
                if (lastAsmLineStart >= 0)  asmText.replaceRange(asmLastTag,lastAsmLineStart,lastAsmLineStart+1);
                int linestart = asmText.getLineStartOffset(linenumber);
                asmLastTag = asmText.getText().substring(linestart,linestart+1);
                asmText.replaceRange(asmLastTag.equals("@") ? "#" : "=",linestart,linestart+1);
                lastAsmLineStart = linestart;
                asmScrollVis(linestart);
            }
        }
        
    } catch (Exception e){System.err.println("updatePanel: " + e.getMessage());};
    return updated;
  } 

//Issue shell avr-objdump and read stdout to generate source and assembly panes and pc indexing tables
private Process objdumpProcess;

 private void indexObjdumpFile() {
    String[] command, environment;
    
    if (myMote.getType() instanceof MicaZMoteType) {
        objdumpFile = ((MicaZMoteType)myMote.getType()).getContikiFirmwareFile();
    } else if (myMote.getType() instanceof RavenMoteType) {
        objdumpFile = ((RavenMoteType)myMote.getType()).getContikiFirmwareFile();
    } else if (myMote.getType() instanceof RFA1MoteType) {
        objdumpFile = ((RFA1MoteType)myMote.getType()).getContikiFirmwareFile();
    } else {
        logger.debug(myMote.getType() + " not known");
        return;
    }
    if (objdumpFile == null) {
        logger.debug("can not load ContikiFirmwareFile");
        return;
    }
    if (!objdumpFile.exists()) {
//      JFileChooser fileChooser = new JFileChooser("C:\\cygwin\\home\\dak\\contiki\\examples\\udp-ipv6\\udp-client.avr-raven.source");
        JFileChooser fileChooser = new JFileChooser("");
        JPanel chooseDialog = new JPanel();
        int returnVal = fileChooser.showOpenDialog(chooseDialog);
        if (returnVal != JFileChooser.APPROVE_OPTION) {
            objdumpFile = null;
            return;
        }
        objdumpFile = fileChooser.getSelectedFile();
    }

    File directory = objdumpFile.getParentFile();

    try {
        command = new String[] {"avr-objdump","-S",objdumpFile.getName()};
        environment = new String[0];
        objdumpProcess = Runtime.getRuntime().exec(command, environment, directory);
    } catch (IOException e) {
        logger.warn("Error creating avr-objdump process");
    }          
    if (objdumpProcess == null) return;
    final BufferedReader processNormal = new BufferedReader(
        new InputStreamReader(objdumpProcess.getInputStream()));
    final BufferedReader processError = new BufferedReader(
        new InputStreamReader(objdumpProcess.getErrorStream()));

    Thread readInput = new Thread(new Runnable() {
         public void run() {
            String strLine, strPC;
            int strLength, pc = 0;
            boolean isasm;
            int srcline = 0, asmline = 0, srcStartLine = 0, asmStartLine = 0, lastSrcPc = 0;
            srcPC = new int[32768]; asmPC = new int[32768];
            srcStartPC = new int[32768]; //asmStartPC = new int[32768];
            srcBreakPC = new int[100];
            asmBreakPC = new int[100];
          try {
            while ((strLine = processNormal.readLine()) != null) {
                isasm = false;
                strLength = strLine.length();
                // objdump assembly lines start with  '     xxxx:'
                // routines entry points start with '0000xxxx'
                if (strLine.length() > 8) {
                    if ((strLine.charAt(1) == '0') && (strLine.charAt(1) == '0')) {
                        isasm = true;
                        /*
                        if (strLine.charAt(4) != ' ') {  //parseInt does not like leading spaces in xxxx
                            strPC = strLine.substring(4,8);
                        } else if (strLine.charAt(5) != ' ') {
                            strPC = strLine.substring(5,8);
                        } else if (strLine.charAt(6) != ' ') {
                            strPC = strLine.substring(6,8);
                        } else {
                            strPC = strLine.substring(7,8);
                        }
                       pc = Integer.parseInt(strPC, 16);
                       pc = pc/2;
                       asmStartPC[pc] = asmline;
                       asmStartLine = asmline;
                       */
                        srcStartLine = srcline;                    
                    } else if (strLine.charAt(8)==':') {
                        if (strLine.charAt(4) != ' ') {  //parseInt does not like leading spaces in xxxx
                            strPC = strLine.substring(4,8);
                        } else if (strLine.charAt(5) != ' ') {
                            strPC = strLine.substring(5,8);
                        } else if (strLine.charAt(6) != ' ') {
                            strPC = strLine.substring(6,8);
                        } else {
                            strPC = strLine.substring(7,8);
                        }
                        pc = Integer.parseInt(strPC, 16);
                        pc = pc/2;
                        isasm = true;
                    }
                }
                if (strLine.length() > 0 ) {  //ignore blank lines
                    if (asmPC[pc] == 0) {
                        asmPC[pc] = asmline;            //pc index into asm source
                 //     asmStartPC[pc] = asmStartLine;  //pc index into asm source routine starts
                    }
                    if (srcPC[pc] == 0) {
                        int temp = srcline - 1;
                        if (temp < 0) temp = 0;
                        while (lastSrcPc <= pc) {
                            srcPC[lastSrcPc] = temp;
                            srcStartPC[lastSrcPc++] = srcStartLine;
                        }
                    }    
                    asmline++;
                    asmText.append(" " + strLine + "\n");
                    if (!isasm) {
                        srcline++;
                        srcText.append(" " + strLine + "\n");
                     //   srcText.append(" " + strLine + " " + srcline + "\n");
                    }
                }
            }
          } catch (IOException e) {
            logger.warn("Error parsing avr-objdump output");
          }
          lastLineOfSource = srcStartLine;
        }
    }, "avr-objdump output thread");


    Thread readError = new Thread(new Runnable() {
        public void run() {
          try {
            String readLine;
            while ((readLine = processError.readLine()) != null) {
                logger.debug(readLine);
            }
          } catch (IOException e) {
            logger.warn("Error while reading error process");
          }
        }
    }, "avr-objdump error");


    Thread handleObjDumpThread = new Thread(new Runnable() {
        public void run() {
          // wait for thread to end
          try {
            objdumpProcess.waitFor();
          } catch (Exception e) {
            logger.debug(e.getMessage());
            return;
          }

          if (objdumpProcess.exitValue() != 0) {
            logger.debug("avr-objdump returned error code " + objdumpProcess.exitValue());
            return;
          }
        }
    }, "handle avr-objdump thread");

    readInput.start();
    readError.start();
    handleObjDumpThread.start();
  try {
    handleObjDumpThread.join();
  } catch (Exception e) {
    // make sure process has exited
    objdumpProcess.destroy();

    String msg = e.getMessage();
    if (e instanceof InterruptedException) {
        msg = "Aborted by user";
    }
  }

    jPanel.validate();
    jPanel.repaint();
 // try {Thread.sleep(20);}catch (Exception e){e.printStackTrace();}
    updatePanel(0);
}

  private void handleSearch() {try {
    prevButton.setEnabled(false);
    if (asmHilite != null)   asmHilite.removeAllHighlights();
    if (srcHilite != null)   srcHilite.removeAllHighlights();
    if (searchHilite != null)   searchHilite.removeAllHighlights();
    searchString = searchText.getText();
    if (searchString.length() <= 0) {
        nextButton.setEnabled(false);
        return;
    }
    int i;
    if (asmActive) {
        i = asmText.getText().indexOf(searchString,0);
        if (i < 0) {
            if (searchHilite == null) {
                searchHilite = new DefaultHighlighter();
                searchPainter = new DefaultHighlighter.DefaultHighlightPainter(Color.RED);
                searchText.setHighlighter(searchHilite);
            }
            searchHilite.addHighlight(0, searchString.length(), searchPainter);
            nextButton.setEnabled(false);
            searchText.selectAll();
            return;
        }
        currIndex = i;
        nextIndex = asmText.getText().indexOf(searchString,currIndex+searchString.length());
        if (nextIndex >= 0) nextButton.setEnabled(true);
        if (asmHilite == null) {
            asmHilite = new DefaultHighlighter();
            asmPainter = new DefaultHighlighter.DefaultHighlightPainter(Color.LIGHT_GRAY);
            asmText.setHighlighter(asmHilite);
        }
        asmScrollVis(currIndex);
        asmHilite.addHighlight(currIndex, currIndex+searchString.length(), asmPainter);
    } else {
        i = srcText.getText().indexOf(searchString,0);
        if (i < 0) {
            if (searchHilite == null) {
                searchHilite = new DefaultHighlighter();
                searchPainter = new DefaultHighlighter.DefaultHighlightPainter(Color.RED);
                searchText.setHighlighter(searchHilite);
            }
            searchHilite.addHighlight(0, searchString.length(), searchPainter);
            nextButton.setEnabled(false);
            searchText.selectAll();
            return;
        }
        currIndex = i;
        nextIndex = srcText.getText().indexOf(searchString,currIndex+searchString.length());
        if (nextIndex >= 0) nextButton.setEnabled(true);
        if (srcHilite == null) {
            srcHilite = new DefaultHighlighter();
            srcPainter = new DefaultHighlighter.DefaultHighlightPainter(Color.LIGHT_GRAY);
            srcText.setHighlighter(srcHilite);
        }
        srcScrollVis(currIndex);
        srcHilite.addHighlight(currIndex, currIndex+searchString.length(), srcPainter);
    }
  } catch(Exception ex){System.err.println("handleSearch: " + ex.getMessage());}}

  public JPanel getInterfaceVisualizer() {
    jPanel = new JPanel(new BorderLayout());
    final Box boxn = Box.createVerticalBox();
    final Box boxn1 = Box.createHorizontalBox();
    final Box boxn2 = Box.createHorizontalBox();
    final Box boxn3 = Box.createHorizontalBox();
    final Box boxn4 = Box.createHorizontalBox();
    final Box boxs = Box.createVerticalBox();
    final Box boxd = Box.createHorizontalBox();
    
    timeLabel = new JLabel();
    watchLabel = new JLabel();
    cyclesLabel = new JLabel();
    stateLabel = new JLabel();

    final JButton updateButton = new JButton("Update");
    final JButton resetButton = new JButton("Reset");
    final JToggleButton liveButton = new JToggleButton("Live",false);
    final JToggleButton asmButton = new JToggleButton("Asm",false);
    final JToggleButton srcButton = new JToggleButton("Src",false);
    final JToggleButton disableBPButton = new JToggleButton("Disable BPs");
    final JButton clearBPButton = new JButton("Clear BPs");
    final JButton stepOverButton = new JButton("Step over");
    final JButton stepIntoButton = new JButton("Step into");
    runButton = new JToggleButton("Run");
    final JSlider delaySlider = new JSlider(0,50,5);
    searchText = new JTextField(12);
    prevButton = new JButton("Prev");
    nextButton = new JButton("Next");
    
    boxn1.add(timeLabel);boxn1.add(Box.createHorizontalGlue());boxn1.add(updateButton);
    boxn2.add(watchLabel);boxn2.add(Box.createHorizontalGlue());boxn2.add(resetButton);
    boxn3.add(cyclesLabel);boxn3.add(Box.createHorizontalGlue());boxn3.add(liveButton);
    boxn4.add(stateLabel);boxn4.add(Box.createHorizontalGlue());boxn4.add(srcButton);
    boxn.add(boxn1);
    boxn.add(boxn2);
    boxn.add(boxn3);
    boxn.add(boxn4);
    jPanel.add(BorderLayout.NORTH, boxn);
    jPanel.add(BorderLayout.SOUTH, boxs);

    clearBPButton.setEnabled(false);
    disableBPButton.setEnabled(false);
    prevButton.setEnabled(false);
    nextButton.setEnabled(false);
    runButton.setSelected(true);
    boxd.add(prevButton);
    boxd.add(searchText);
    boxd.add(nextButton);
    boxd.add(Box.createHorizontalGlue());
    delaySlider.setPreferredSize(new Dimension(50,0));
    delaySlider.setInverted(true);
    delaySlider.setEnabled(false);
    displayDelay = 25; //msec
    boxd.add(delaySlider);
    boxd.add(asmButton);
    boxd.add(clearBPButton);
    boxd.add(disableBPButton);
    boxd.add(stepIntoButton);
    boxd.add(stepOverButton);
    boxd.add(runButton);
    
    updatePanel(0);
    // increase initial width to prevent scrollbar with longer strings
    stateLabel.setText(stateLabel.getText() + "              ");

    // search for string
    searchText.getDocument().addDocumentListener(new DocumentListener() {
        public void insertUpdate(DocumentEvent e) {
            handleSearch();
        }
        public void removeUpdate(DocumentEvent e) {
            handleSearch();
        }
        public void changedUpdate(DocumentEvent e) {
            logger.debug("change " + e);
        }
    });

    // next occurance of string
    nextButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {try {
            prevIndex = currIndex;
            prevButton.setEnabled(true);
            currIndex = nextIndex;
            if (asmActive) {
                nextIndex = asmText.getText().indexOf(searchString,currIndex+searchString.length());
                if (nextIndex < 0) nextButton.setEnabled(false);
                asmHilite.removeAllHighlights();
                asmHilite.addHighlight(currIndex, currIndex+searchString.length(), asmPainter);
                asmScrollVis(currIndex);
            } else {
                nextIndex = srcText.getText().indexOf(searchString,currIndex+searchString.length());
                if (nextIndex < 0) nextButton.setEnabled(false);
                srcHilite.removeAllHighlights();
                srcHilite.addHighlight(currIndex, currIndex+searchString.length(), srcPainter);
                srcScrollVis(currIndex);
            }
        } catch(Exception ex){System.err.println("nextButton: " + ex.getMessage());}}
    });

    // previous occurance of string
    prevButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {try {
            nextIndex = currIndex;
            nextButton.setEnabled(true);
            currIndex = prevIndex;
            if (asmActive) {
                prevIndex = asmText.getText().lastIndexOf(searchString,currIndex-searchString.length());
                if (prevIndex < 0) prevButton.setEnabled(false);
                asmHilite.removeAllHighlights();
                asmHilite.addHighlight(currIndex, currIndex+searchString.length(), asmPainter);
                asmScrollVis(currIndex);
            } else {
                prevIndex = srcText.getText().lastIndexOf(searchString,currIndex-searchString.length());
                if (prevIndex < 0) prevButton.setEnabled(false);
                srcHilite.removeAllHighlights();
                srcHilite.addHighlight(currIndex, currIndex+searchString.length(), srcPainter);
                srcScrollVis(currIndex);
            }
        } catch(Exception ex){System.err.println("prevButton: " + ex.getMessage());}}
    });
    
    // one time update
    updateButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            updatePanel(0);
        }
    });
    
    // reset stopwatch and cycle counter
    resetButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            lastTime = simulation.getSimulationTime();
            lastCycles = interpreter.getState().getCycles();
            updatePanel(0);
        }
    });
 
    // insert avrora probe when live update or breakpoints enabled.
    // Avrora calls fireBefore and fireAfter when program counter changes
    liveButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            liveUpdate = liveButton.isSelected();
            delaySlider.setEnabled(liveUpdate);
            setProbeState();
        }
    });

    // show source code
    srcButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            if (srcButton.isSelected()) {            
                if (srcText == null) {
                    asmText = new JTextArea(33, 24);
                    asmText.setEditable(false);
                    ((DefaultCaret)asmText.getCaret()).setUpdatePolicy(DefaultCaret.NEVER_UPDATE);
                    asmPane = new JScrollPane(asmText);

                    srcText = new JTextArea(33, 24);
                    srcText.setEditable(false);
                    ((DefaultCaret)srcText.getCaret()).setUpdatePolicy(DefaultCaret.NEVER_UPDATE);
                    srcPane = new JScrollPane(srcText);

                 // double click in source window sets breakpoint
                srcText.addMouseListener(new MouseAdapter() {
                public void mouseClicked(MouseEvent evt) {try{
                    if (evt.getClickCount() == 2) {
                        int linenumber = srcText.getLineOfOffset(srcText.getSelectionStart());
                        int pc = 0;
                        // a faster search could be done here...
                        while (srcPC[pc] < linenumber) pc++;
                        while (asmPC[pc] == 0) pc++;
                        linenumber = srcPC[pc]; //set bp at actual instruction                          
                        pc = pc * 2;
                        if (srcBPIndicatorToggle(linenumber)) {
                            logger.debug("Add src breakpoint at 0x" + Integer.toHexString(pc));
                            if (srcNumBreaks < 0) srcNumBreaks = 0;
                            srcBreakPC[srcNumBreaks++] = pc;
                            if (!asmActive) {
                                clearBPButton.setEnabled(true);
                                disableBPButton.setEnabled(true);
                            }
                        } else {
                            for (int i = 0; i<srcNumBreaks; i++) {
                                if (srcBreakPC[i] == pc) {
                                    logger.debug("Clear src breakpoint at 0x" + Integer.toHexString(pc));
                                    while (i < srcNumBreaks) srcBreakPC[i] = srcBreakPC[++i];
                                    if (--srcNumBreaks == 0) {
                                        if (!asmActive) {
                                            clearBPButton.setEnabled(false);
                                            disableBPButton.setEnabled(false);
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                        setProbeState();
                    }
                }catch(Exception e) {System.err.println("srcClick: " + e.getMessage());}}
                });

                // double click in asm window sets breakpoint
                asmText.addMouseListener(new MouseAdapter() {
                public void mouseClicked(MouseEvent evt) {try{
                    if (evt.getClickCount() == 2) {
                        int linenumber = asmText.getLineOfOffset(asmText.getSelectionStart());
                        int pc = 0;
                        while (asmPC[pc] < linenumber) pc++;
                        linenumber = asmPC[pc];
                        //toggle if it is the first only
                       // srcBPIndicatorToggle(srcPC[pc]);
                        pc = pc * 2;
                        if (asmBPIndicatorToggle(linenumber)) {
                            logger.debug("Add asm breakpoint at 0x" + Integer.toHexString(pc));
                            asmBreakPC[asmNumBreaks++] = pc;
                            if (asmActive) {
                                clearBPButton.setEnabled(true);
                                disableBPButton.setEnabled(true);
                            }
                        } else {
                            for (int i = 0; i<asmNumBreaks; i++) {
                                if (asmBreakPC[i] == pc) {
                                    logger.debug("Clear asm breakpoint at 0x" + Integer.toHexString(pc));
                                    while (i < asmNumBreaks) asmBreakPC[i] = asmBreakPC[++i];
                                    if (--asmNumBreaks == 0) {
                                        if (asmActive) {
                                        logger.debug("disablebp");
                                            clearBPButton.setEnabled(false);
                                            disableBPButton.setEnabled(false);
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                        setProbeState();
                    }
                }catch(Exception e) {System.err.println("asmClick: " + e.getMessage());}}
                });

                    splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, asmPane, srcPane);
                    splitPane.setOneTouchExpandable(true);
                    // splitPane.setDividerLocation(0);                   
                    if (objdumpFile == null) indexObjdumpFile();
                }
                if (objdumpFile != null) {
                    boxn.add(boxd);
                    boxs.add(splitPane);
                    sourceActive = true;
                    runButton.setSelected(true);
                }
            } else {
                sourceActive = false;
                halted = false;
                setProbeState();
                boxn.remove(boxd);
                boxs.remove(splitPane);
            }
            // add whitespace during resize so panel will not get scrollbar on text increase
            stateLabel.setText(stateLabel.getText() + "              ");
          //  ((MoteInterfaceViewer)jPanel.getRootPane().getParent()).pack();
        }
    });

    // activate assembly pane for search and stepping
    asmButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            asmActive = !asmActive;
            // no more asm actions if panel is invisible
            if (splitPane.getDividerLocation() < 10) {
                asmButton.setSelected(false);
                if (asmActive) {
                    asmActive = false;
                    return;
                }
            }

            if (searchText.getText().length() != 0) handleSearch();
            boolean bpactive;
            if (asmActive) bpactive = asmNumBreaks > 0;
            else bpactive = srcNumBreaks > 0;
            clearBPButton.setEnabled(bpactive);
            disableBPButton.setEnabled(bpactive);
            if (bpactive && !probeInserted) setProbeState();
            currIndex = nextIndex;
            // cancel any stepover action
            stepOverPC = 0;
            srcRoutineStart = 0;
            srcLastLine = 0;
            // check for BP switch between asm and src
            if (lastSrcLineStart >=0) {
                String tag = srcText.getText().substring(lastSrcLineStart,lastSrcLineStart+1);
                if (asmActive) {
                    if (tag.equals("#")) srcText.replaceRange("@",lastSrcLineStart,lastSrcLineStart+1);   
                } else {
                    if (tag.equals("@")) {
                        srcText.replaceRange("#",lastSrcLineStart,lastSrcLineStart+1);
                        srcLastTag = "@";
                    }
                }
            }
            if (lastAsmLineStart >=0) {
                String tag = asmText.getText().substring(lastAsmLineStart,lastAsmLineStart+1);
                if (!asmActive) {
                    if (tag.equals("#")) asmText.replaceRange("@",lastAsmLineStart,lastAsmLineStart+1);
                } else {
                    if (tag.equals("@")) {
                        asmText.replaceRange("#",lastAsmLineStart,lastAsmLineStart+1);
                        asmLastTag = "@";
                    }
                }
            }
        }
    });

    // clear all breakpoints
    clearBPButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            clearBPButton.setEnabled(false);
            disableBPButton.setEnabled(false);
            if (asmActive) {
                while (--asmNumBreaks >= 0) asmBPIndicatorToggle(asmPC[asmBreakPC[asmNumBreaks]/2]);
                asmNumBreaks = 0;
            } else {
                while (--srcNumBreaks >= 0) srcBPIndicatorToggle(srcPC[srcBreakPC[srcNumBreaks]/2]);
                srcNumBreaks = 0;
            }
            setProbeState();
        }
    });

    // disable all breakpoints
    disableBPButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            disableBreaks = !disableBreaks;
            setProbeState();
        }
    });
 
 // step to next source line
    stepIntoButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            stepIntoPress = true;
            halted = true;
            setProbeState();
        }
    });
    
    // step over subroutine call
    stepOverButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            if (halted) stepOverPress = true;
            halted = true;
            setProbeState();
        }
    });
    
    // run
    runButton.addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent e) {
            halted = !runButton.isSelected();
            if (halted) {
            } else {
                         //   updatePanel(0);
                if (lastSrcLineStart >= 0) srcText.replaceRange(srcLastTag,lastSrcLineStart,lastSrcLineStart+1);
                if (lastAsmLineStart >= 0) asmText.replaceRange(asmLastTag,lastAsmLineStart,lastAsmLineStart+1);
                lastSrcLineStart = -1;
                lastAsmLineStart = -1;
             //   updatePanel(0);
            }
           // if (!halted) updatePanel(0);
            setProbeState();
            // cancel any stepover action
            stepOverPress = false;
            stepOverPC = 0;
            srcRoutineStart = 0;
            srcLastLine = 0;
        }
    });
    
    // delay during run
    delaySlider.addChangeListener(new ChangeListener() {
        public void stateChanged(ChangeEvent e) {
            JSlider source = (JSlider)e.getSource();
          //  if (!source.getValueIsAdjusting()) {
                displayDelay = (int)source.getValue();
                displayDelay = displayDelay * displayDelay;
          //  }
        }
    });

	simulation.addObserver(observer = new Observer() {
		public void update(Observable obs, Object obj) {
		}
	});
    jPanel.putClientProperty("intf_obs", observer);
 //  observer.update(null, null);

	return jPanel;
	}

	public void releaseInterfaceVisualizer(JPanel panel) {
  //  logger.debug("release visualizer");
		Observer observer = (Observer) panel.getClientProperty("intf_obs");
		if (observer == null) {
			logger.fatal("Error when releasing panel, observer is null");
			return;
		}
        if (liveProbe != null) interpreter.removeProbe(liveProbe);
        sourceActive = false;
        liveUpdate = false;
        probeInserted = false;
                halted = false;
                //    runButton.setEnabled(true);
        /*
    //    MoteInterfaceViewer parent = (MoteInterfaceViewer)panel.getRootPane().getParent();
        if (panel.getRootPane()!=null) {
            MoteInterfaceViewer parent = (MoteInterfaceViewer)panel.getRootPane().getParent();
            if (parent != null) parent.setPreferredSize(originalViewerDimension);
        }
        */
		simulation.deleteObserver(observer);
	}

  public Collection<Element> getConfigXML() {
    return null;
  }


  public boolean setConfigXML(Collection<Element> configXML, boolean visAvailable) {
    boolean clearedRules = false;
    for (Element element : configXML) {
      if (element.getName().equals("tab")) {
        mainPane.setSelectedIndex(Integer.parseInt(element.getText()));
      } else if (element.getName().equals("rule")) {
        if (!clearedRules) {
          rules.clear();
          clearedRules = true;
        }
        rules.add(new Rule(element.getAttributeValue("from"), element.getAttributeValue("to")));
      }
    }
    return true;
  }

  private AbstractAction currentFileAction = new AbstractAction() {
    private static final long serialVersionUID = -3218306989816724883L;

    public void actionPerformed(ActionEvent e) {
   //   if (currentCodeFile == null) {
        return;
  //    }
  //    displaySourceFile(currentCodeFile, currentLineNumber, true);
    }
  };

  private AbstractAction mapAction = new AbstractAction("Locate sources...") {
    private static final long serialVersionUID = -3929432830596292495L;

    public void actionPerformed(ActionEvent e) {
   //   tryMapDebugInfo();
    }
  };

  private AbstractAction stepAction = new AbstractAction("Step instruction") {
    private static final long serialVersionUID = 3520750710757816575L;
    public void actionPerformed(ActionEvent e) {
      try {
      //  mspMote.getCPU().stepInstructions(1);
      } catch (Exception ex) {
        logger.fatal("Error: ", ex);
      }
    //  showCurrentPC();
    }
  };
    public Mote getMote() {
    return myMote;
  }

}
