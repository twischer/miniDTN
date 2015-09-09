MODULES+= examples/uDTN/dtnping
CFLAGS+= -DPROJECT_CONF_H=\"project-conf.h\"
# Uncomment it for enabling debug messages
#CFLAGS+= -DENABLE_LOGGING

DTN_APPS += dtnpingecho

ROOT_DIR = $(CURDIR)

include $(ROOT_DIR)/Makefile.include
