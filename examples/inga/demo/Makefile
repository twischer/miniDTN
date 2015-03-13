CONTIKI_PROJECT = inga-demo
all: $(CONTIKI_PROJECT)
	
CFLAGS += -DWITH_NODE_ID=1

ifdef NODE_ID
	CFLAGS +=-DNODEID=$(NODE_ID)
endif
#INGA_BAUDRATE = 38400
TARGET=inga
APPS = settings_set settings_delete

CONTIKI = ../../..
# Choose NETSTACK here, IPV6 is default 
# CONTIKI_WITH_RIME = 1
include $(CONTIKI)/Makefile.include
