# RIME unicast test
alltests: udp_ipv6_client udp_ipv6_server
	
TARGET=inga

PROJECT_SOURCEFILES += ../test.c

APPS = settings_set settings_delete

CONTIKI = ../../..
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
