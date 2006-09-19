INSTALL= /usr/bin/install -c
INSTALL_PROGRAM= ${INSTALL}
INSTALL_DATA= ${INSTALL} -m 644
INSTALLNLSDIR=$(datadir)/locale

MSGMERGE = msgmerge

NLSPACKAGE = newt

CATALOGS = $(shell ls *.po)
FMTCATALOGS = $(patsubst %.po,%.mo,$(CATALOGS))

POTFILES  = ../dialogboxes.c

all: $(NLSPACKAGE).pot $(FMTCATALOGS)

$(NLSPACKAGE).pot: $(POTFILES)
	xgettext --default-domain=$(NLSPACKAGE) \
		--add-comments --keyword=_ --keyword=N_ $(POTFILES)
	if cmp -s $(NLSPACKAGE).po $(NLSPACKAGE).pot; then \
	    rm -f $(NLSPACKAGE).po; \
	else \
	    mv $(NLSPACKAGE).po $(NLSPACKAGE).pot; \
	fi

refresh-po: Makefile
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
		lang=`echo $$cat | sed 's/.po//'`; \
		if $(MSGMERGE) $$lang.po $(NLSPACKAGE).pot > $$lang.pot ; then \
			mv -f $$lang.pot $$lang.po ; \
			echo "$(MSGMERGE) of $$lang succeeded" ; \
		else \
			echo "$(MSGMERGE) of $$lang failed" ; \
			rm -f $$lang.pot ; \
		fi \
	done

update-po: $(NLSPACKAGE).pot Makefile refresh-po

report:
	@for cat in $(CATALOGS); do \
		echo -n "$$cat: "; \
		msgfmt -v --statistics -o /dev/null $$cat; \
	done

clean:
	rm -f *mo

distclean: clean
	rm -f .depend Makefile

depend:

install: all
	mkdir -p $(INSTALLNLSDIR)
	for n in $(CATALOGS); do \
	    l=`basename $$n .po`; \
	    mo=$$l.mo; \
	    $(INSTALL) -m 755 -d $(INSTALLNLSDIR)/$$l; \
	    $(INSTALL) -m 755 -d $(INSTALLNLSDIR)/$$l/LC_MESSAGES; \
	    if [ -f $$n ]; then \
	        $(INSTALL) -m 644 $$mo $(INSTALLNLSDIR)/$$l/LC_MESSAGES/$(NLSPACKAGE).mo; \
	    fi; \
	done

%.mo: %.po
	msgfmt -v -o $@ $<
