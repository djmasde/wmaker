#
# $Log: Makefile,v $
# Revision 1.2  1999/08/19 17:58:51  carsten
# Almost final version
#
# Revision 1.1  1999/08/15 15:38:09  carsten
# Added wmwave project to repository

LIBDIR = -L/usr/X11R6/lib
LIBS   = -lXpm -lXext -lX11 -lm
FLAGS = -O6
OBJS =	 \
		wmgeneral.o \

default:all

.c.o:
	cc -I/usr/X11R6/share/include $(FLAGS) -c -Wall $< -o $*.o

wmwave.o: wmwave.c wmwave-master.xpm
	cc -I/usr/X11R6/share/include $(FLAGS) -c -Wall wmwave.c -o $*.o

wmwave: $(OBJS) wmwave.o
	cc $(FLAGS) -o wmwave $(OBJS) -lXext $(LIBDIR) $(LIBS) wmwave.o

all:: wmwave

clean::
	rm -f *.o
	rm -f wmwave 
	rm -f *~

install:: wmwave
	strip wmwave
	cp -f wmwave /usr/bin/
	chmod 755 /usr/bin/wmwave
	chown root:root /usr/bin/wmwave
	cp -f wmwave.1 /usr/man/man1
	chmod 644 /usr/man/man1/wmwave.1
	chown root:root /usr/man/man1/wmwave.1
	@echo "wmwave Installation finished..."

uninstall:
	rm /usr/bin/wmwave
	rm /usr/man/man1/wmwave.1
