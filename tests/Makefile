TESTS = test_ipc ipc_creator ipc_destroyer
AM_CFLAGS += -I../include
AM_LDFLAGS += -lsipc -lcunit -Wl,-rpath=$(TOP_BUILDDIR)/src -L$(TOP_BUILDDIR)/src
TOP_BUILDDIR := $(shell pwd)/..
CFLAGS += -O0 -g3 -gdwarf-2

all:

install:

tests: $(TESTS)

test_ipc: test_ipc.c shm.h shm.c mqueue.h mqueue.c
	$(CC) $(AM_CFLAGS) $(CFLAGS) $(INCLUDEDIRS) $^ $(AM_LDFLAGS) $(LDFLAGS) -o $@

ipc_creator: ipc_creator.c
	$(CC) $(AM_CFLAGS) $(CFLAGS) $(INLCUDEDIRS) $(AM_LDFLAGS) $(LDFLAGS) $^ -o $@

ipc_destroyer: ipc_destroyer.c
	$(CC) $(AM_CFLAGS) $(CFLAGS) $(INCLUDEDIRS) $(AM_LDFLAGS) $(LDFLAGS) $^ -o $@

clean:
	-rm -rf $(TESTS)

.PHONY: all install tests clean
