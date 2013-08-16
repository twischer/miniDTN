TARGET=inga

EXAMPLES = hello-world \
	   udp-ipv6 \
	   eeprom-test \
	   rime
	   #ipv6/rpl-border-router \

MAKE_EXAMPLES = $(patsubst %, %.make, $(EXAMPLES))

all: $(MAKE_EXAMPLES)

%.make:
	@echo It is ../../$(patsubst %.make,%,$@)
	make -C ../../$(patsubst %.make,%,$@) TARGET=inga clean
	make -C ../../$(patsubst %.make,%,$@) TARGET=inga

	
#	make -C ../../hello-world
#	make -C ../../ipv6/rpl-border-router
#	make -C ../../udp-ipv6
#	make -c ../../eeprom-test
#	make -C ../../examples/rime/ all


CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
