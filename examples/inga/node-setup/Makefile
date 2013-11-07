CONTIKI_PROJECT = inga-setup
all: clean $(CONTIKI_PROJECT)
	
TARGET=inga
APPS = settings_set 

ifndef NODE_ID
  $(error You must specify a node id with NODE_ID=value)
endif

PAN_ID = 0xABCD
RADIO_CHANNEL = 0x1A
RADIO_TX_POWER = 0x00

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
