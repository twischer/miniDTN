package org.contikios.cooja.plugins.analyzers;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import org.apache.log4j.Logger;

public class PcapExporter {

  private static final Logger logger = Logger.getLogger(PcapExporter.class);

  DataOutputStream out;
  File file;

  public PcapExporter(File file) throws IOException {
    this.file = file;
  }

  public void openPcap() throws IOException {
    out = new DataOutputStream(new FileOutputStream(file));
    /* pcap header */
    out.writeInt(0xa1b2c3d4);
    out.writeShort(0x0002);
    out.writeShort(0x0004);
    out.writeInt(0);
    out.writeInt(0);
    out.writeInt(4096);
    out.writeInt(195); /* 195 for LINKTYPE_IEEE802_15_4 */

    out.flush();
    logger.info("Opened pcap file " + file.getAbsolutePath());
  }

  public void closePcap() throws IOException {
    out.close();
  }

  public void exportPacketData(byte[] data) throws IOException {
    if (out == null) {
      openPcap();
    }
    try {
      /* pcap packet header */
      out.writeInt((int) (System.currentTimeMillis() / 1000));
      out.writeInt((int) ((System.currentTimeMillis() % 1000) * 1000));
      out.writeInt(data.length);
      out.writeInt(data.length + 2);
      /* and the data */
      out.write(data);
      out.flush();
    }
    catch (IOException ex) {
      logger.error(ex);
    }
  }

}
