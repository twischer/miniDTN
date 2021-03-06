MODULES+= examples/uDTN/dtnping
#MODULES+= examples/uDTN/dtnsend
#MODULES+= examples/uDTN/throughput
#MODULES+= examples/uDTN/counter
#MODULES+= examples/uDTN/fatfs_test
#MODULES+= examples/uDTN/fatfs-storage-test

CFLAGS+= -DPROJECT_CONF_H=\"project-conf.h\"
CFLAGS+= -DINGA_CONF_PAN_ID=0x0780
CFLAGS+= -DINGA_CONF_PAN_ADDR=1466


#CFLAGS+= -DBUNDLE_CONF_STORAGE=storage_fatfs
# Enable for fromating the sd card on every start up
#CFLAGS+= -DBUNDLE_CONF_STORAGE_INIT=1


# compute static stack usage of each function
#CFLAGS+= -fstack-usage

# uncomment for enabling LwIP debug messages
# CFLAGS+= -DLWIP_DEBUG
# CFLAGS+= -DLWIP_DBG_MIN_LEVEL=LWIP_DBG_LEVEL_WARNING
# CFLAGS+= -DPBUF_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DAPI_LIB_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DAPI_MSG_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DIP_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DRAW_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DMEM_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DMEMP_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DSYS_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DTCP_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DTCP_INPUT_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DTCP_OUTPUT_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DUDP_DEBUG=LWIP_DBG_ON
# CFLAGS+= -DTCPIP_DEBUG=LWIP_DBG_ON


# Uncomment it for enabling debug messages
#CFLAGS+= -DENABLE_LOGGING
CFLAGS+= -DCONF_LOGLEVEL=LOGL_WRN
#CFLAGS+= -DUART_BAUDRATE=921600

DTN_APPS += dtnpingecho

ROOT_DIR = $(CURDIR)

include $(ROOT_DIR)/Makefile.include
