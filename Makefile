# darkmoon window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.cpp darkmoon.cpp util.cpp
OBJ = ${SRC:.cpp=.o}

all: options darkmoon

options:
	@echo darkmoon build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

darkmoon: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f darkmoon ${OBJ} darkmoon-${VERSION}.tar.gz

dist: clean
	mkdir -p darkmoon-${VERSION}
	cp -R LICENSE Makefile README config.def.h config.mk\
		darkmoon.1 drw.h util.h ${SRC} darkmoon.png transient.c darkmoon-${VERSION}
	tar -cf darkmoon-${VERSION}.tar darkmoon-${VERSION}
	gzip darkmoon-${VERSION}.tar
	rm -rf darkmoon-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f darkmoon ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/darkmoon
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < darkmoon.1 > ${DESTDIR}${MANPREFIX}/man1/darkmoon.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/darkmoon.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/darkmoon\
		${DESTDIR}${MANPREFIX}/man1/darkmoon.1

.PHONY: all options clean dist install uninstall
