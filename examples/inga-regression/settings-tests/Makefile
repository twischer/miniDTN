# RIME unicast test
alltests: udp_ipv6_client udp_ipv6_server
	
TARGET=inga

UIP_CONF_IPV6=1

PROJECT_SOURCEFILES += ../test.c

APPS = settings_set settings_delete

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
