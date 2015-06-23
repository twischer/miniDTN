MODULES+= examples/uDTN/dtnping
CFLAGS+= -DPROJECT_CONF_H=\"project-conf.h\"
# Uncomment it for enabling debug messages
#CFLAGS+= -DENABLE_LOGGING

ROOT_DIR = $(CURDIR)

include $(ROOT_DIR)/Makefile.include
