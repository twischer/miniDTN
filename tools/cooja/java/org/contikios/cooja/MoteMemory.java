package org.contikios.cooja;

/**
 * @TODO: move...
 */
import java.util.HashMap;
import java.util.Map;
import org.contikios.cooja.MoteMemory.UnknownVariableException;
import org.contikios.cooja.MemMonitor.MonitorType;

/**
 *
 * @author Enrico Joerns
 */
public abstract class MoteMemory extends NewAddressMemory {

  public MoteMemory(MemoryLayout layout) {
    super(layout);
  }

  /**
   * @return All variable names known and residing in this memory
   */
  public abstract String[] getVariableNames();

  /**
   * Checks if given variable exists in memory.
   *
   * @param varName Variable name
   * @return True if variable exists, false otherwise
   */
  public abstract boolean variableExists(String varName);

  /**
   * Returns address of variable with given name.
   *
   * @param varName Variable name
   * @return Variable address
   */
  public abstract long getVariableAddress(String varName) throws UnknownVariableException;

  /**
   *
   * @param varName
   * @return
   * @throws org.contikios.cooja.AddressMemory.UnknownVariableException
   */
  public abstract int getVariableSize(String varName) throws UnknownVariableException;

  /**
   *
   * @param varName
   * @return
   */
  public byte getByteValueOf(String varName)
          throws UnknownVariableException {
    return getByteValueOf(getVariableAddress(varName));
  }

  /**
   *
   * @param varName
   * @return
   */
  public short getShortValueOf(String varName)
          throws UnknownVariableException {
    return getShortValueOf(getVariableAddress(varName));
  }

  /**
   *
   * @param varName
   * @return
   */
  public int getIntValueOf(String varName)
          throws UnknownVariableException {
    return getIntValueOf(getVariableAddress(varName));
  }

  /**
   *
   * @param varName
   * @return
   */
  public long getLongValueOf(String varName)
          throws UnknownVariableException {
    return getLongValueOf(getVariableAddress(varName));
  }

  /**
   *
   * @param varName
   * @return
   */
  public long getAddrValueOf(String varName)
          throws UnknownVariableException {
    return getAddrValueOf(getVariableAddress(varName));
  }
  
  /**
   * 
   * @param varName
   * @param length
   * @return
   * @throws org.contikios.cooja.AddressMemory.UnknownVariableException 
   */
  public byte[] getByteArray(String varName, int length)
      throws UnknownVariableException {
    return getMemorySegment(getVariableAddress(varName), length);
  }
  
  /**
   *
   * @param varName
   * @param value
   */
  public void setByteValueOf(String varName, byte value)
          throws UnknownVariableException {
    setByteValueOf(getVariableAddress(varName), value);
  }

  /**
   *
   * @param varName
   * @param value
   */
  public void setShortValueOf(String varName, short value)
          throws UnknownVariableException {
    setShortValueOf(getVariableAddress(varName), value);
  }

  /**
   *
   * @param varName
   * @param value
   */
  public void setIntValueOf(String varName, int value)
          throws UnknownVariableException {
    setIntValueOf(getVariableAddress(varName), value);
  }

  /**
   *
   * @param varName
   * @param value
   */
  public void setLongValueOf(String varName, long value)
          throws UnknownVariableException {
    setLongValueOf(getVariableAddress(varName), value);
  }

  /**
   *
   * @param varName
   * @param value
   */
  public void setAddrValueOf(String varName, long value)
          throws UnknownVariableException {
    setAddrValueOf(getVariableAddress(varName), value);
  }

  /**
   * 
   * @param varName
   * @param data
   * @throws org.contikios.cooja.AddressMemory.UnknownVariableException 
   */
  public void setByteArray(String varName, byte[] data)
          throws UnknownVariableException {
    setMemorySegment(getVariableAddress(varName), data);
  }


  /**
   * Unknown variable name exception.
   */
  public class UnknownVariableException extends RuntimeException {

    public UnknownVariableException(String varName) {
      super("Unknown variable name: " + varName);
    }
  }
  
  
    
//  public enum MemoryEventType {
//
//    READ, WRITE
//  };

  /**
   * Monitor to listen for memory updates.
   */
  public interface VarMonitor extends MemMonitor {

    public void varChanged(MoteMemory memory, MemoryEventType type, String varName);
  }

  /**
   * Maps VarMonitor to their internal created AddressMonitor.
   * Require for removing appropriate monitor.
   */
  Map<VarMonitor, AddressMonitor> monitorMapping = new HashMap<>();
  
  /**
   * Adds a AddressMonitor for the specified address region.
   *
   * @param flag Select memory operation(s) to listen for (read, write,
   * read/write)
   * @param varName
   * @param vm
   * @return
   */
  public boolean addVarMonitor(MonitorType flag, final String varName, final VarMonitor vm) {
    AddressMonitor mm = new AddressMonitor() {
      @Override
      public void memoryChanged(NewAddressMemory memory, MemoryEventType type, long address) {
        vm.varChanged(MoteMemory.this, type, varName);
      }
    };
    monitorMapping.put(vm, mm);
    return addMemoryMonitor(
            flag,
            getVariableAddress(varName),
            getVariableSize(varName),
            mm);
  }

  /**
   * Removes AddressMonitor assigned to the specified region.
   *
   * @param varName
   * @param mm AddressMonitor to remove
   */
  public void removeVarMonitor(String varName, VarMonitor mm) {
    removeMemoryMonitor(getVariableAddress(varName), getVariableSize(varName), monitorMapping.get(mm));
  }
}
