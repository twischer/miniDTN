package org.contikios.cooja;

/**
 *
 * @author Enrico Joerns
 */
public interface MemMonitor {

  /**
   * 
   */
  public enum MemoryEventType {

    READ, WRITE
  };

  /**
   * Memory access type to listen for.
   */
  public enum MonitorType {

    R, W, RW
  };

}
