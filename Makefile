PREFIX = /usr
LD = ld
CC = gcc
INSTALL = install
STRIP = strip
CFLAGS = -pipe -O2 -Wall -Wextra -std=gnu99 -DPIC -fPIC -I.
LDFLAGS = -Wl,-no-undefined,-z,defs

VLC_PLUGIN_CFLAGS := $(shell pkg-config --cflags vlc-plugin)
VLC_PLUGIN_LIBS := $(shell pkg-config --libs vlc-plugin)

LIBVLC_CFLAGS := $(shell pkg-config --cflags libvlc)
LIVVLC_LIBS := $(shell pkg-config --libs libvlc)

libdir = $(PREFIX)/lib
plugindir = $(libdir)/vlc/plugins

override CFLAGS += -DMODULE_STRING=\"omem\" $(VLC_PLUGIN_CFLAGS) $(LIBVLC_CFLAGS) 
override LDFLAGS += $(VLC_PLUGIN_LIBS)

TARGETS = libstream_out_omem_plugin.so test
C_SOURCES = omem.c
C_TESTS = test.c

all: libstream_out_omem_plugin.so test

install: all
	mkdir -p -- $(DESTDIR)$(plugindir)/access_output
	$(INSTALL) --mode 0755 libstream_out_omem_plugin.so $(DESTDIR)$(plugindir)/access_output

install-strip:
	$(MAKE) install INSTALL="$(INSTALL) -s"

uninstall:
	rm -f $(DESTDIR)$(plugindir)/access_output/libstream_out_omem_plugin.so

clean:
	rm -f -- libstream_out_omem_plugin.{dll,so} *.o

mostlyclean: clean

%.o: %.c
	$(CC) $(CFLAGS) -c $<

libstream_out_omem_plugin.so: $(C_SOURCES:%.c=%.o)
	$(CC) -shared -o $@ $(C_SOURCES:%.c=%.o) $(LDFLAGS)
	$(STRIP) --strip-unneeded $@

test: $(C_TESTS:%.c=%.o)
	$(CC) -o $@ $(C_TESTS:%.c=%.o) $(LIVVLC_LIBS)

.PHONY: all install install-strip uninstall clean mostlyclean

