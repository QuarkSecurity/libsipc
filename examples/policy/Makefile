AWK ?= gawk

EXAMPLESDIR = $(DATAROOTDIR)/sipc
FC_ENTRIES := $(shell $(AWK) '/^\/.*/{ print $$1 }' *.fc )
FC_ENTRY = $(shell $(AWK) '/^\/.*/{ print $$1 }' $(@:.pp=.fc) )

UID = $(shell id -u)

all:

install:

install-example:
	test -d $(DESTDIR)/$(EXAMPLESDIR)/policy || install -m 755 -d $(DESTDIR)/$(EXAMPLESDIR)/policy
	install -m 644 *.te $(DESTDIR)/$(EXAMPLESDIR)/policy
	install -m 644 *.fc $(DESTDIR)/$(EXAMPLESDIR)/policy
	install -m 644 *.if $(DESTDIR)/$(EXAMPLESDIR)/policy
	install -m 644 *.spt $(DESTDIR)/$(EXAMPLESDIR)/policy
	install -m 644 Makefile $(DESTDIR)/$(EXAMPLESDIR)/policy
	install -m 644 README $(DESTDIR)/$(EXAMPLESDIR)/policy

setup:
ifneq ($(UID),0)
	@echo "make $@" must be run as root since this target manipulates policy && false
endif
	@cp /usr/share/selinux/devel/include/system/unconfined.if unconfined.if.bak && \
	cp unconfined.spt /usr/share/selinux/devel/include/system/unconfined.if

install-policy: setup
ifneq ($(UID),0)
	@echo "make $@" must be run as root since this target manipulates policy && false
endif
	@echo "Building policy modules..."
	@$(MAKE) -f /usr/share/selinux/devel/include/Makefile 1>/dev/null
	@echo "Inserting policy modules..."
	@/usr/sbin/semodule `ls *.pp|sed 's/^\(.*\)$$/ -i \1/g'`
	@echo "Labeling files..."
	@for file in $(FC_ENTRIES); do \
		/usr/sbin/setfiles -v /etc/selinux/targeted/contexts/files/file_contexts $$file 1>/dev/null; \
	done

%.pp: setup %.te %.if %.fc
	@$(MAKE) -f /usr/share/selinux/devel/include/Makefile $@
	/usr/sbin/semodule -i $@
	for file in $(FC_ENTRY); do \
		/usr/sbin/setfiles -v /etc/selinux/targeted/contexts/files/file_contexts $$file; \
	done

remove-policy:
ifneq ($(UID),0)
	@echo "make $@" must be run as root since this target manipulates policy && false
endif
	@echo "Removing modules..."
	@/usr/sbin/semodule `ls *.te|sed 's/^\(.*\)\.te$$/ -r \1/g'` || true
	@echo "Resetting file labels..."
	@for file in $(FC_ENTRIES); do \
		/usr/sbin/setfiles -v /etc/selinux/targeted/contexts/files/file_contexts $$file 2>&1 1>/dev/null; \
	done
	echo "Finishing clean up..."
	@[ -f unconfined.if.bak ] && \
	cp unconfined.if.bak /usr/share/selinux/devel/include/system/unconfined.if && \
	rm unconfined.if.bak || true

clean:

.PHONY: all install clean setup remove-policy install-policy
