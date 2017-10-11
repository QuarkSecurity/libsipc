all:

install:
	test -d $(INCLUDEDIR)/sipc || install -m 755 -d $(INCLUDEDIR)/sipc
	install -m 644 $(wildcard sipc/*.h) $(INCLUDEDIR)/sipc

clean:

.PHONY: all install clean
