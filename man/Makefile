all:

install:
	test -d $(DESTDIR)/$(MANDIR)/man3 || mkdir -m 755 -p $(DESTDIR)/$(MANDIR)/man3
	install -m 644 $(wildcard man3/*.3) $(DESTDIR)/$(MANDIR)/man3

clean:

.PHONY: all install clean
