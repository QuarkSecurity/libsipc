# Installation directories
PREFIX ?= /usr/local
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib
DATAROOTDIR ?= $(PREFIX)/share
MANDIR ?= $(DATAROOTDIR)/man

ifeq ($(DEBUG), 1)
  # if DEBUG is set, then explicitly compile with -O0
  AM_CFLAGS ?= -O0 -g3 -gdwarf-2
  LDFLAGS ?= -g3
else
  # if no DEBUG and no CFLAGS are already set, then compile with -O3
  CFLAGS ?= -O3
endif

AM_CFLAGS += -Wall -Wundef -Wmissing-noreturn -D_GNU_SOURCE -fpic
# -Werror -Wwrite-strings  -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith -Wreturn-type -Wcast-qual -Wswitch -Wshadow -Wcast-align
INCLUDEDIRS := -I$(shell pwd)/include
LIBDIRS := -L$(shell pwd)/src

export PREFIX INCLUDEDIR LIBDIR DATAROOTDIR MANDIR \
	AM_CFLAGS LDFLAGS

# This library can optionally build bindings for other langugages.
# Set these variables to non-empty (either by editing this file or
# setting them on the command line) to enable the bindings.
# ENABLE_JAVA = 1

ifdef ENABLE_JAVA
    MAYBE_JAVA = bindings/java
endif

SUBDIRS = src include $(MAYBE_JAVA) examples man tests
ALL_SUBDIRS = src include bindings/java examples man tests

C_INDENT_OPTS   = -npro -nbad -bap -nbbb -nbbo -nbc -br -bli0 -bls -c40 -cbi0 -cd40 -cdw  -ce -ci8 -cli0 -cp40 -ncs -d0 -nbfda -di1 -nfc1 -nfca -i8 -ip0 -l132 -lp -nlps -npcs -pi0 -nprs -npsl -saf -sai -sbi0 -sob -ss -ts8 -ut -T sipc_t

all:
	for i in $(SUBDIRS) ; do \
		($(MAKE) -C $$i all) || exit 1;\
	done

example:
	$(MAKE) -C examples $@

tests:
	$(MAKE) -C tests $@

install:
	for i in $(SUBDIRS) ; do \
		($(MAKE) -C $$i install) || exit 1;\
	done

install-example:
	$(MAKE) -C examples $@

javadoc:
	$(MAKE) -C bindings/java $@

indent:
	find src include tests \
		\( -name '*.[ch]' -type f -exec indent $(C_INDENT_OPTS) '{}' \; \)

clean:
	for i in $(ALL_SUBDIRS) ; do \
		($(MAKE) -C $$i clean);\
	done

.PHONY: all example tests install install-example javadoc indent clean
