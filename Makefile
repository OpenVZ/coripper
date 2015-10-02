INSTALL = install

default: all

debug:
	(cd src && $(MAKE) $(MAKEOPTS) $@)

all:
	(cd src && $(MAKE) $(MAKEOPTS) $@)

install:
	(cd src && ${MAKE} $@ DESTDIR="$(DESTDIR)")
	$(INSTALL) -d $(DESTDIR)/usr/share/man/man8
	$(INSTALL) -m 644 man/coripper.8 $(DESTDIR)/usr/share/man/man8

clean:
	(cd src && ${MAKE} $@)

.PHONY: clean all install debug
