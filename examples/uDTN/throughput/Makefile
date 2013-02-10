CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = uDTN-sender
all: $(CONTIKI_PROJECT)

# If there is something about coffee in the config, use it
ifneq (,$(findstring storage_coffee,$(CFLAGS)))
  # Compiling with COFFEE Storage
  COFFEE_FILES=5
endif

CONF_DTN=1

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
