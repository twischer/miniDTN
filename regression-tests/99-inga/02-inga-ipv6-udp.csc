<?xml version="1.0" encoding="UTF-8"?>
<simconf>
  <project EXPORT="discard">[CONTIKI_DIR]/tools/cooja/apps/mrm</project>
  <project EXPORT="discard">[CONTIKI_DIR]/tools/cooja/apps/mspsim</project>
  <project EXPORT="discard">[CONTIKI_DIR]/tools/cooja/apps/avrora</project>
  <simulation>
    <title>My simulation</title>
    <delaytime>0</delaytime>
    <randomseed>generated</randomseed>
    <motedelay_us>1000000</motedelay_us>
    <radiomedium>
      org.contikios.cooja.radiomediums.UDGM
      <transmitting_range>50.0</transmitting_range>
      <interference_range>100.0</interference_range>
      <success_ratio_tx>1.0</success_ratio_tx>
      <success_ratio_rx>1.0</success_ratio_rx>
    </radiomedium>
    <events>
      <logoutput>40000</logoutput>
    </events>
    <motetype>
      org.contikios.cooja.avrmote.IngaMoteType
      <identifier>inga1</identifier>
      <description>Inga Mote Type #1</description>
      <source EXPORT="discard">[CONTIKI_DIR]/examples/udp-ipv6/udp-client.c</source>
      <commands EXPORT="discard">make clean TARGET=inga
make udp-client.inga TARGET=inga DEFINES=UDP_CONNECTION_ADDR=fe80::2</commands>
      <firmware EXPORT="copy">[CONTIKI_DIR]/examples/udp-ipv6/udp-client.inga</firmware>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraLED</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AT86RF23xRadio</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraClock</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraUsart0</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvrDebugger</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraADC</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
    </motetype>
    <motetype>
      org.contikios.cooja.avrmote.IngaMoteType
      <identifier>inga2</identifier>
      <description>Inga Mote Type #2</description>
      <source EXPORT="discard">[CONTIKI_DIR]/examples/udp-ipv6/udp-server.c</source>
      <commands EXPORT="discard">make udp-server.inga TARGET=inga</commands>
      <firmware EXPORT="copy">[CONTIKI_DIR]/examples/udp-ipv6/udp-server.inga</firmware>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraLED</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AT86RF23xRadio</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraClock</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraUsart0</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvrDebugger</moteinterface>
      <moteinterface>org.contikios.cooja.avrmote.interfaces.AvroraADC</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
    </motetype>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>65.934608127183</x>
        <y>63.70462190529231</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.avrmote.interfaces.AvroraMoteID
        <id>1</id>
      </interface_config>
      <motetype_identifier>inga1</motetype_identifier>
    </mote>
    <mote>
      <breakpoints />
      <interface_config>
        org.contikios.cooja.interfaces.Position
        <x>67.66105781539623</x>
        <y>63.13924301161143</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        org.contikios.cooja.avrmote.interfaces.AvroraMoteID
        <id>2</id>
      </interface_config>
      <motetype_identifier>inga2</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.SimControl
    <width>248</width>
    <z>0</z>
    <height>200</height>
    <location_x>0</location_x>
    <location_y>0</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.LogListener
    <plugin_config>
      <filter />
    </plugin_config>
    <width>816</width>
    <z>3</z>
    <height>333</height>
    <location_x>1</location_x>
    <location_y>365</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.Visualizer
    <plugin_config>
      <skin>org.contikios.cooja.plugins.skins.IDVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.AddressVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.UDGMVisualizerSkin</skin>
      <viewport>123.21660699387752 0.0 0.0 123.21660699387752 -8113.602333266065 -7760.635326525308</viewport>
    </plugin_config>
    <width>246</width>
    <z>2</z>
    <height>167</height>
    <location_x>0</location_x>
    <location_y>198</location_y>
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>TIMEOUT(500000, log.log("last msg: " + msg + "\n")); /* print last msg at timeout */
count = 0;
while (count++ &lt; 5) {
  /* Message from sender process to receiver process */
  YIELD_THEN_WAIT_UNTIL(msg.contains("Client sending"));
  YIELD_THEN_WAIT_UNTIL(msg.contains("Server received"));
  log.log(count + ": Sender -&gt; Receiver OK\n");

  /* Message from receiver process to sender process */
  YIELD_THEN_WAIT_UNTIL(msg.contains("Responding with"));
  YIELD_THEN_WAIT_UNTIL(msg.contains("Response from"));
  log.log(count + ": Receiver -&gt; Sender OK\n");
}

log.testOK(); /* Report test success and quit */</script>
      <active>true</active>
    </plugin_config>
    <width>572</width>
    <z>1</z>
    <height>700</height>
    <location_x>441</location_x>
    <location_y>2</location_y>
  </plugin>
</simconf>

