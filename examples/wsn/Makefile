CONTIKI = ../..
#für udp:
APPS=servreg-hack

WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL

all: rime_toggle udp_toggle

include $(CONTIKI)/Makefile.include
