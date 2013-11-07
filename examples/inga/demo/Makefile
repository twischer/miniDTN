CONTIKI_PROJECT = inga_demo
all: $(CONTIKI_PROJECT)
uip-unicast: uip_unicast
	
CFLAGS += -DWITH_NODE_ID=1

#CFLAGS += ‐DWITH_UIP=1
UIP_CONF_IPV6=1

ifdef NODE_ID
	CFLAGS +=-DNODEID=$(NODE_ID)
endif
#INGA_BAUDRATE = 38400
TARGET=inga
APPS = settings_set settings_delete

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
