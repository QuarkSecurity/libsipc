all:

install:
	test -d $(DESTDIR)/$(INCLUDEDIR)/sipc || install -m 755 -d $(DESTDIR)/$(INCLUDEDIR)/sipc
	install -m 644 $(wildcard sipc/*.h) $(DESTDIR)/$(INCLUDEDIR)/sipc

clean:

.PHONY: all install clean
