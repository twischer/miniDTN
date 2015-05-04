# RIME unicast test
alltests: udp_ipv6_client udp_ipv6_server
	
TARGET=inga

APPS = settings_set  
EUI64=00:00:00:00:00:00:00:00

PROJECT_SOURCEFILES += ../test.c

CONTIKI = ../../..
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
