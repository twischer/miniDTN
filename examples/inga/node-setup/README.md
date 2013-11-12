Set up INGA
===

Use this to setup nodes with default values for node id, pan id, channel and tx power.
Settings will be written to EEPROM when the program starts.

Example
---

If only one node is connected to your compute, you can run

    make NODE_ID=<your-id> setup

If multiple nodes are connected, you have to specify which node to set

    make NODE_ID=<your-id> MOTES=/dev/ttyUSBx setup

To check your settings instantly after programming, you can add the login target

    make NODE_ID=<your-id> MOTES=/dev/ttyUSBx setup login

