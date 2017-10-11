all:

install:
	test -d $(MANDIR)/man3 || mkdir -m 755 -p $(MANDIR)/man3
	install -m 644 $(wildcard man3/*.3) $(MANDIR)/man3

clean:

.PHONY: all install clean
