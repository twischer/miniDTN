CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI_PROJECT = dtntpnode
all: $(CONTIKI_PROJECT)

DTN_APPS += dtntp dtnpingecho

CONF_DTN=1

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
