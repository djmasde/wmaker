all: memtest

install:

clean:
	rm -f memtest *.o

memtest: memtest.o ../libPropList.a
	gcc -o memtest -L.. memtest.o -lPropList

../libPropList.a:
	make -C ..

%.o: %.c
	gcc -g -c -I.. -o $@ $<