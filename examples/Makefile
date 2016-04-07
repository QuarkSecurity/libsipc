# Author: David Windsor <dwindsor@tresys.com>
#
# Copyright (C) 2006, 2007 Tresys Technology, LLC
# Developed Under US JFCOM Sponsorship
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

BINS = mq_creator mq_destroyer mq_sender mq_reader \
	shm_creator shm_destroyer shm_sender shm_reader
EXAMPLESDIR = $(DATAROOTDIR)/sipc

TOP_SRCDIR ?= ..
INCLUDEDIRS = -I$(TOP_SRCDIR)/include
TOP_BUILDDIR ?= ..
LIBDIRS = -L$(TOP_BUILDDIR)/src

SUBDIRS = policy

all: $(BINS)
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) $@) || exit 1;\
	done

install:
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) $@) || exit 1;\
	done

mq_bin_reader: mq_bin_reader.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

mq_bin_sender: mq_bin_sender.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

mq_creator: mq_creator.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

mq_destroyer: mq_destroyer.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

mq_sender: mq_sender.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

mq_reader: mq_reader.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

shm_creator: shm_creator.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

shm_destroyer: shm_destroyer.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

shm_sender: shm_sender.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

shm_reader: shm_reader.o
	$(CC) $(LDFLAGS) -lsipc $(LIBDIRS) -o $@ $^

%.o: %.c
	$(CC) $(AM_CFLAGS) $(CFLAGS) $(INCLUDEDIRS) -c -o $@ $<

install-example: $(BINS)
	test -d $(EXAMPLESDIR) || install -m 755 -d $(EXAMPLESDIR)
	install -m 644 *.c $(EXAMPLESDIR)
	install -m 755 *.sh $(EXAMPLESDIR)
	install -m 755 $(BINS) $(EXAMPLESDIR)
	install -m 644 data.txt $(EXAMPLESDIR)
	install -m 644 Makefile $(EXAMPLESDIR)
	install -m 644 README $(EXAMPLESDIR)
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) $@) || exit 1;\
	done

clean:
	-rm -f $(BINS) *.o
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) $@) || exit 1;\
	done

.PHONY: all install clean install-examples
