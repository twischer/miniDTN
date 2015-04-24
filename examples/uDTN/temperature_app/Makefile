CFLAGS += -DIEEE802154_CONF_PANID=0x1234
CFLAGS += -DRADIO_CONF_PAN_ID=0x1234
CFLAGS += -DRADIO_CONF_CHANNEL=21
CFLAGS += -DCONVERGENCE_LAYER_TIMEOUT=0.5
CFLAGS += -DBUNDLE_CONF_STORAGE_SIZE=57
CFLAGS += -DBUNDLE_CONF_STORAGE_BEHAVIOUR=4
CFLAGS += -DMMEM_CONF_SIZE=6700

CONTIKI_PROJECT = temperature
all: $(CONTIKI_PROJECT)

DTN_APPS += dtntp
DTN_APPS += dtnpingecho 
DTN_APPS += dtneepromsettings
# If there is something about coffee in the config, use it
ifneq (,$(findstring storage_coffee,$(CFLAGS)))
  # Compiling with COFFEE Storage
  MODULES += core/cfs/coffee
  COFFEE_DEVICE=5
  CFS=coffee
endif

CONTIKI_WITH_DTN = 1

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
