BINDIR = /usr/bin

PROG = pclock
OBJS = Main.o Graphics.o Clock.o transform.o

DEFAULT_XPM = round-3d.xpm

INCDIR = -I/usr/X11R6/include
LIBDIR = -L/usr/X11R6/lib
CFLAGS = -O2 -s -fomit-frame-pointer -Wall
LIBS = -lXpm -lXext -lX11 -lm

###############################################################################

all: $(PROG)

install: all
	install -s -m 111 -o bin -g bin $(PROG) $(BINDIR)

clean:
	rm -f *.o Makefile.bak Default.xpm *~ tags $(PROG)

clobber: clean
	rm -f $(PROG)

###############################################################################

Default.xpm:
	ln -s ../XPM/$(DEFAULT_XPM) $@

Graphics.o: PClock.h Default.xpm Clock.h transform.h

Main.o: PClock.h Defaults.h Version.h

Clock.o: transform.h

$(PROG): $(OBJS)
	gcc $(CFLAGS) -o $@ $^ $(LIBDIR) $(LIBS)

###############################################################################

.c.o:
	gcc $(CFLAGS) $(INCDIR) -c $< -o $@
