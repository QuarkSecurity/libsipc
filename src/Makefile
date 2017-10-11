SOVERSION=1
RELEASE=0

DESTDIR ?= /usr/local
LIBDIR ?= $(DESTDIR)/lib/

TARGET=libsipc.so
SONAME=$(TARGET).$(SOVERSION)
LIBSO=$(TARGET).$(SOVERSION)
RELEASESO=$(LIBSO).$(RELEASE)

TOP_SRCDIR ?= ..
INCLUDEDIRS = -I$(TOP_SRCDIR)/include

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))

all: $(RELEASESO)

$(RELEASESO): $(OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^ -Wl,-soname,$(SONAME),--version-script=libsipc.map,-z,defs $(LIBDIRS)
	ln -sf $@ $(LIBSO)
	ln -sf $@ $(TARGET)

%.o: %.c sipc_internal.h
	$(CC) $(AM_CFLAGS) $(CFLAGS) $(INCLUDEDIRS) -c -o $@ $<

install: $(RELEASESO)
	test -d $(LIBDIR) || install -m 755 -d $(LIBDIR)
	install -m 644 $(RELEASESO) $(LIBDIR)
	cd $(LIBDIR) && ln -sf $(RELEASESO) $(TARGET)
	cd $(LIBDIR) && ln -sf $(RELEASESO) $(LIBSO)

clean:
	-rm -f $(OBJS) $(RELEASESO) $(LIBSO) $(TARGET)

.PHONY: all install clean
