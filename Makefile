OBJS = newt.o test.o button.o form.o checkbox.o entry.o label.o listbox.o
LIBS = -lslang -lm -lefence
CFLAGS = -g -Wall

PROGS = test

SOURCES = $(subst .o,.c,$(OBJS))

ifeq (.depend,$(wildcard .depend))
TARGET=$(PROGS)
else
TARGET=depend $(PROGS)
endif

all:	$(TARGET)

test:	$(OBJS)
	gcc -g -o test $(OBJS) $(LIBS)

veryclean: clean
	rm -f .depend

clean:
	rm -f $(PROGS) $(OBJS) core

depend:
	$(CPP) $(CFLAGS) -M $(SOURCES) > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

