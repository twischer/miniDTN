CONTIKI_PROJECT = inga-setup

TARGET=inga

APPS += settings_set
APPS += settings_delete

# Preset default values
EUI64 =
PAN_ADDR =
PAN_ID =
RADIO_CHANNEL = 
RADIO_TX_POWER = 

setup: SETTINGS_SET_LOAD = 1
setup: SETTINGS_DELETE_LOAD = 0
setup: test-single $(CONTIKI_PROJECT).upload

setup.jtag: SETTINGS_SET_LOAD = 1
setup.jtag: SETTINGS_DELETE_LOAD = 0
setup.jtag: test-single $(CONTIKI_PROJECT).jtag

setup.bang: SETTINGS_SET_LOAD = 1
setup.bang: SETTINGS_DELETE_LOAD = 0
setup.bang: test-single $(CONTIKI_PROJECT).bang

delete: SETTINGS_SET_LOAD = 0
delete: SETTINGS_DELETE_LOAD = 1
delete: test-single $(CONTIKI_PROJECT).upload

delete.jtag: SETTINGS_SET_LOAD = 0
delete.jtag: SETTINGS_DELETE_LOAD = 1
delete.jtag: test-single $(CONTIKI_PROJECT).jtag

delete.bang: SETTINGS_SET_LOAD = 0
delete.bang: SETTINGS_DELETE_LOAD = 1
delete.bang: test-single $(CONTIKI_PROJECT).bang

# Note: currently, deleting single items is not fully supported by the settings library!
#delete: #TODO

# tests for single MOTE
test-single: NR_OF_MOTES = $(shell echo $(MOTES) | wc -w)
test-single:
	@test ! $(NR_OF_MOTES) -eq 0 || (echo "*** At least one MOTE required. Specify with MOTES=<device>" && false)
	@test $(NR_OF_MOTES) -eq 1 || (echo "*** Only single MOTE allowed. Specify with MOTES=<device>" && false)

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include

