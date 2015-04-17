CONTIKI = ../..

MODULES=core/cfs
MODULES+=core/net/tdma
MODULES+=core/net/mac/tdma_mac



all:  example-unicast 
UIP_CONF_IPV6=0

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
include $(CONTIKI)/Makefile.include
