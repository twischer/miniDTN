CONTIKI_PROJECT = inga-setup

TARGET=inga

APPS += settings_set
APPS += settings_delete

# Preset default values
NODE_ID = 
PAN_ID = 
RADIO_CHANNEL = 
RADIO_TX_POWER = 
EUI64 = 

setup: SETTINGS_SET_LOAD = 1
setup: SETTINGS_DELETE_LOAD = 0
setup: test $(CONTIKI_PROJECT).upload

delete: SETTINGS_SET_LOAD = 0
delete: SETTINGS_DELETE_LOAD = 1
delete: test-single $(CONTIKI_PROJECT).upload

# Note: currently, deleting single items is not fully supported by the settings library!
#delete: #TODO

# tests for var NODE_ID set
test: test-single
	@echo NODE_ID is $(NODE_ID)
	@test ! -z $(NODE_ID) || (echo "*** You must specify a node id with NODE_ID=<value>" && false)

# tests for single MOTE
test-single: NR_OF_MOTES = $(shell echo $(MOTES) | wc -w)
test-single:
	@test ! $(NR_OF_MOTES) -eq 0 || (echo "*** At least one MOTE required. Specify with MOTES=<device>" && false)
	@test $(NR_OF_MOTES) -eq 1 || (echo "*** Only single MOTE allowed. Specify with MOTES=<device>" && false)

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include

