CONTIKI_PROJECT = inga_demo
all: $(CONTIKI_PROJECT)
CFLAGS += -DWITH_NODE_ID=1
#UIP_CONF_IPV6=1
ifdef NODE_ID
	CFLAGS +=-DNODEID=$(NODE_ID)
endif

CONTIKI = ../..
include $(CONTIKI)/Makefile.include
