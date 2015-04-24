CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = uDTN-generate-bundles
all: $(CONTIKI_PROJECT)

# If there is something about coffee in the config, use it
ifneq (,$(findstring storage_coffee,$(CFLAGS)))
  # Compiling with COFFEE Storage
  MODULES += core/cfs/coffee
  COFFEE_DEVICE=5
  CFS=coffee
endif
#UIP_CONF_IPV6=1

CONTIKI_WITH_DTN=1
CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
