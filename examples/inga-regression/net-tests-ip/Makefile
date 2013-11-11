# RIME unicast test
alltests: udp_ipv6_client udp_ipv6_server
	
TARGET=inga

UIP_CONF_IPV6=1

APPS = settings_set  
EUI64=00:00:00:00:00:00:00:00

PROJECT_SOURCEFILES += ../test.c

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
