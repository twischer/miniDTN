CONTIKI_PROJECT = default_app
all: $(CONTIKI_PROJECT)
CFLAGS += -DWITH_NODE_ID=1
#UIP_CONF_IPV6=1
ifdef NODE_ID
	CFLAGS +=-DNODEID=$(NODE_ID)
endif
#INGA_BAUDRATE = 38400
FAT=1
TARGET=inga
CFLAGS += -DCONFIG_GROUPS=10 -DMAX_KEY_SIZE=5
CONTIKI_SOURCEFILES += ini_parser.c config_mapping.c app_config.c sd_mount.c logger.c uart_handler.c sensor_fetch.c

CONTIKI = ../..
include $(CONTIKI)/Makefile.include
