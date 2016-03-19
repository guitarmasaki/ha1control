#
# HA-1 HA1Control
#
# Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
# License: GNU GPL v2
#

#.SILENT:

target_program = $(shell pwd | sed 's!.*/!!')
cc_srcs = $(sort $(shell ls *.cc 2> /dev/null))
cc_header = $(sort $(shell ls *.h 2> /dev/null))
target_os = $(shell uname)
LIBS = -L/usr/local/lib -lgif -lpng -lcurl -lssl -lcrypto -ljansson
CC_FLAGS = -I. -I/usr/local/include
ifeq ($(target_os),Linux)
  CC_FLAGS += -DENABLE_DAEMON -DENABLE_IRREMOTE
endif
ifeq ($(target_os),Darwin)
  CC_FLAGS += -I/usr/local/openssl/include
  LIBS += -liconv -L/usr/local/openssl/lib
endif


# CC_FLAGS += -DDEBUG


all: $(target_program)

$(target_program) : $(cc_srcs) $(cc_header)
	g++ $(CC_FLAGS) $(LIBS) -Wno-deprecated-declarations -o $@ $(cc_srcs)

clean:
	rm -f $(target_program)

install: all
	- [ ! -f /etc/init.d/ha1control ] || /etc/init.d/ha1control stop
	mkdir -p /var/ha1/irremote
	cp ha1control /usr/local/bin
	[ -f /var/ha1/ha1control.conf ] || ( [ -f ha1control.conf ] && cp ha1control.conf /var/ha1 )
	[ -f /var/ha1/ha1control.conf ] || cp sample0_ha1control.conf /var/ha1/ha1control.conf
	[ -f /var/ha1/client.ui ] || ( [ -f client.ui ] && cp client.ui /var/ha1 )
	cp ha2module.ha /var/ha1
	cp irremote/* /var/ha1/irremote
	cp ha1control.rc /etc/init.d/ha1control
	cp ha1control.rotate /etc/logrotate.d/ha1control
	update-rc.d ha1control defaults 3 1
	/etc/init.d/ha1control start

