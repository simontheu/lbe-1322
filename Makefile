CFLAGS=-g
LBPROGRAMS=lbe-1322

HIDLIB=-L. -lhidapi-hidraw -Wl,-rpath,.

all: lbe-1322-utils

libhidapi-hidraw.so:
	ln -s libhidapi-hidraw.so.0 libhidapi-hidraw.so

lbe-1322-utils: lbe-1322.c
	gcc ${CFLAGS} -o lbe-1322 lbe-1322.c -I.

all-clean:
	rm ${LBPROGRAMS} libhidapi-hidraw.so

clean:
	rm ${PROGRAMS}


