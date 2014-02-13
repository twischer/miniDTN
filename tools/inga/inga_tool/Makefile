# Use pkg-config to determine which libftdi version we have

FTDI_VERSION := -1

HASFTDI  := $(shell pkg-config --exists libftdi ; echo "$$?" )
HASFTDI0 := $(shell pkg-config --exists libftdi0; echo "$$?" )
HASFTDI1 := $(shell pkg-config --exists libftdi1; echo "$$?" )

# libftdi1 uses libusb-1.0, libftdi0 uses libusb (0.1)
ifeq (0, ${HASFTDI1})
	FTDI_VERSION := 1
	FTDI_CFLAGS  := `pkg-config --cflags libftdi1 libusb-1.0`
	FTDI_LDLIBS  := `pkg-config --libs libftdi1 libusb-1.0`
else
ifeq (0, ${HASFTDI0})
	FTDI_VERSION := 0
	FTDI_CFLAGS  := `pkg-config --cflags libftdi0 libusb`
	FTDI_LDLIBS  := `pkg-config --libs libftdi0 libusb`
else
ifeq (0, ${HASFTDI})
	FTDI_VERSION := 0
	FTDI_CFLAGS  := `pkg-config --cflags libftdi libusb`
	FTDI_LDLIBS  := `pkg-config --libs libftdi libusb`
endif
endif
endif

# Use pkg-config to determine the compiler and linker flag for popt and libudev

POPT_CFLAGS := `pkg-config --cflags popt`
POPT_LDLIBS := `pkg-config --libs popt`

LIBUDEV_CFLAGS := `pkg-config --cflags libudev`
LIBUDEV_LDLIBS := `pkg-config --libs libudev`

CFLAGS=${FTDI_CFLAGS} ${POPT_CFLAGS} ${LIBUDEV_CFLAGS} -DFTDI_VERSION="${FTDI_VERSION}" -ggdb
LDLIBS=${FTDI_LDLIBS} ${POPT_LDLIBS} ${LIBUDEV_LDLIBS}

# Standard recipes to make inga_tool

NAME    := inga_tool
SOURCES := inga_tool.c
HEADERS :=
OBJECTS := $(shell echo "${SOURCES}" | sed -e 's/\.c/\.o/g')

all: ${NAME}

${NAME}:${OBJECTS}
	${CC} ${OBJECTS} ${LDLIBS} -o $@

${OBJECTS}:%.o:%.c ${HEADERS}
	${CC} ${CFLAGS} $< -c -o $@

clean:
	rm -f ${OBJECTS} ${NAME}

.PHONY: clean
