LIBS = -lslang -lm #-lefence
SHLIBS = -lslang -lm -lc

CFLAGS = $(RPM_OPT_FLAGS) -Wall -I/usr/include/slang
ifeq ($(RPM_OPT_FLAGS),)
CFLAGS += -g # -O2 -I/usr/include/slang
endif

VERSION = 0.22
CVSTAG = r$(subst .,-,$(VERSION))
SONAME = 0.20

PROGS = test whiptail whiptcl.so testgrid
TESTOBJS = test.o 
NDIALOGOBJS = whiptail.o dialogboxes.o 
WHIPTCLOBJS = whiptcl.o dialogboxes.o
LIBNEWT = libnewt.a
LIBNEWTSH = libnewt.so.$(VERSION)
LIBNEWTSONAME = libnewt.so.$(SONAME)
LIBOBJS = newt.o button.o form.o checkbox.o entry.o label.o listbox.o \
          scrollbar.o textbox.o scale.o grid.o windows.o buttonbar.o

SHCFLAGS = -fPIC

prefix = /usr
includedir = $(prefix)/include
libdir = $(prefix)/lib
bindir = $(prefix)/bin
ARCHNAME = $(shell uname -m | sed 's/i.86/i386/')
pythondir = $(prefix)/lib/python1.4
pythonbindir = $(prefix)/lib/python1.4/linux-$(ARCHNAME)

#--------------------------------------

SOURCES = $(subst .o,.c,$(TESTOBJS) $(NDIALOGOBJS) $(LIBOBJS)) 

SHAREDDIR = shared
SHAREDOBJS = $(patsubst %,$(SHAREDDIR)/%, $(LIBOBJS))

ifeq (.depend,$(wildcard .depend))
TARGET=$(PROGS)
else
TARGET=depend $(PROGS)
endif

all:	$(TARGET) _snackmodule.so

test:	$(TESTOBJS) $(LIBNEWT)
	gcc -g -o test $(TESTOBJS) $(LIBNEWT) $(LIBS)

testgrid:	testgrid.o $(LIBNEWT)
	gcc -g -o testgrid testgrid.o $(LIBNEWT) $(LIBS)

_snackmodule.so:   snackmodule.o $(LIBNEWTSH)
	gcc --shared -o _snackmodule.so snackmodule.o -L . $(LIBNEWTSH)

snackmodule.o:   snackmodule.c
	gcc -I/usr/include/python1.4 -fPIC $(CFLAGS) -c snackmodule.c

whiptail: $(NDIALOGOBJS) $(LIBNEWTSH)
	gcc -g -o whiptail $(NDIALOGOBJS) -L . $(LIBNEWTSH) $(LIBS) -lpopt

whiptcl.so: $(WHIPTCLOBJS) $(LIBNEWTSH)
	gcc -shared -o whiptcl.so $(WHIPTCLOBJS) -L . $(LIBNEWTSH) -ltcl -lslang -lpopt -lm

$(LIBNEWT): $(LIBNEWT)($(LIBOBJS))

newt.o: newt.c Makefile
	$(CC) $(CFLAGS) -DVERSION=\"$(VERSION)\" -c -o $@ $<

veryclean: clean
	rm -f .depend

clean:
	rm -f $(PROGS) *.o $(LIBNEWT) core $(LIBNEWTSH)  \
		$(SHAREDOBJS) *.so*

depend:
	$(CPP) $(CFLAGS) -M $(SOURCES) > .depend

$(SHAREDDIR):
	mkdir -p $(SHAREDDIR)

sharedlib: $(LIBNEWTSH)

$(LIBNEWTSH): $(SHAREDDIR) $(SHAREDOBJS)
	gcc -shared -o $(LIBNEWTSH) -Wl,-soname,$(LIBNEWTSONAME) $(SHAREDOBJS) $(SHLIBS)

$(SHAREDDIR)/%.o : %.c
	$(CC) $(SHCFLAGS) -c $(CFLAGS) -o $@ $<

$(SHAREDDIR)/newt.o: newt.c Makefile
	$(CC) $(SHCFLAGS) $(CFLAGS) -DVERSION=\"$(VERSION)\" -c -o $@ $<


install: $(LIBNEWT) whiptail
	[ -d $(instroot)/$(bindir) ] || install -m 755 -d $(instroot)/$(bindir)
	[ -d $(instroot)/$(libdir) ] || install -m 755 -d $(instroot)/$(libdir)
	[ -d $(instroot)/$(includedir) ] || install -m 755 -d $(instroot)/$(includedir)
	install -m 644 newt.h $(instroot)/$(includedir)
	install -m 644 $(LIBNEWT) $(instroot)/$(libdir)
	install -s -m 755 whiptail $(instroot)/$(bindir)

install-sh: sharedlib whiptcl.so _snackmodule.so
	install -m 755 $(LIBNEWTSH) $(instroot)/$(libdir)
	ln -sf $(LIBNEWTSH) $(instroot)/$(libdir)/libnewt.so
	install -m 755 whiptcl.so $(instroot)/$(libdir)
	[ -d $(instroot)/$(pythonbindir) ] || install -m 755 -d $(instroot)/$(pythonbindir)
	install -m 755 _snackmodule.so $(instroot)/$(pythonbindir)
	install -m 755 snack.py $(instroot)/$(pythondir)

archive: 
	@cvs tag -F $(CVSTAG)
	@rm -rf /tmp/newt-$(VERSION)
	@mkdir /tmp/newt-$(VERSION)
	@cvs export -r$(CVSTAG) -d /tmp/newt-$(VERSION) newt
	@cd /tmp; tar czSpf newt-$(VERSION).tar.gz newt-$(VERSION)
	@rm -rf /tmp/newt-$(VERSION)
	@cp /tmp/newt-$(VERSION).tar.gz .
	@rm -f /tmp/newt-$(VERSION).tar.gz 
	@echo " "
	@echo "The final archive is ./newt-$(VERSION).tar.gz."

ifeq (.depend,$(wildcard .depend))
include .depend
endif

