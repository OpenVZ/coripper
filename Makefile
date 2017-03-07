include Makefile.incl

INSTALL = install
MANDIR ?= /usr/share/man

define do_rebrand
	sed -e "s,@PRODUCT_NAME_SHORT@,$(PRODUCT_NAME_SHORT),g" -i $(1) || exit 1;
endef

default: all

debug:
	(cd src && $(MAKE) $(MAKEOPTS) $@)

all:
	(cd src && $(MAKE) $(MAKEOPTS) $@)

install:
	(cd src && ${MAKE} $@ DESTDIR="$(DESTDIR)")
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m 644 man/coripper.8 $(DESTDIR)$(MANDIR)/man8
	$(call do_rebrand,$(DESTDIR)$(MANDIR)/man8/coripper.8)

clean:
	(cd src && ${MAKE} $@)

.PHONY: clean all install debug
