CONTIKI = ../..
#für udp:
APPS=servreg-hack

WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL
CFLAGS += -DWITH_NODE_ID=1

all: rime_toggle udp_toggle present

include $(CONTIKI)/Makefile.include
