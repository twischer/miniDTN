MODULES+= examples/uDTN/pingpong
CFLAGS+=-DENABLE_LOGGING

ROOT_DIR = $(CURDIR)

include $(ROOT_DIR)/Makefile.include
