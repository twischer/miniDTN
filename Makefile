MODULES+= examples/uDTN/dtnping
CFLAGS+= -DPROJECT_CONF_H=\"project-conf.h\"
CFLAGS+= -DINGA_CONF_PAN_ID=0x0780
CFLAGS+= -DINGA_CONF_PAN_ADDR=1466


# Uncomment it for enabling debug messages
#CFLAGS+= -DENABLE_LOGGING
CFLAGS+= -DCONF_LOGLEVEL=LOGL_INF

DTN_APPS += dtnpingecho

ROOT_DIR = $(CURDIR)

include $(ROOT_DIR)/Makefile.include
