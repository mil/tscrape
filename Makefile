include config.mk

NAME = tscrape
VERSION = 0.1
SRC = \
	tscrape.c\
	xml.c
COMPATSRC = \
	strlcat.c\
	strlcpy.c
BIN = \
	tscrape
MAN1 = \
	tscrape.1
DOC = \
	LICENSE\
	README
HDR = \
	compat.h\
	xml.h

OBJ = ${SRC:.c=.o} ${EXTRAOBJ}

all: $(BIN)

.c.o:
	${CC} -c ${CFLAGS} $<

dist: $(BIN)
	rm -rf release/${VERSION}
	mkdir -p release/${VERSION}
	cp -f ${MAN1} ${HDR} ${SRC} ${COMPATSRC} ${DOC} \
		Makefile config.mk \
		release/${VERSION}/
	# make tarball
	rm -f sfeed-${VERSION}.tar.gz
	(cd release/${VERSION}; \
	tar -czf ../../sfeed-${VERSION}.tar.gz .)

${OBJ}: config.mk ${HDR}

tscrape: tscrape.o xml.o ${EXTRAOBJ}
	${CC} -o $@ tscrape.o xml.o ${EXTRAOBJ} ${LDFLAGS}

clean:
	rm -f ${BIN} ${OBJ}

install: all
	# installing executable files.
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${BIN} ${DESTDIR}${PREFIX}/bin
	for f in $(BIN); do chmod 755 ${DESTDIR}${PREFIX}/bin/$$f; done
	# installing example files.
	mkdir -p ${DESTDIR}${PREFIX}/share/${NAME}
	cp -f README\
		${DESTDIR}${PREFIX}/share/${NAME}
	# installing manual pages.
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	cp -f ${MAN1} ${DESTDIR}${MANPREFIX}/man1
	for m in $(MAN1); do chmod 644 ${DESTDIR}${MANPREFIX}/man1/$$m; done

uninstall:
	# removing executable files.
	for f in $(BIN); do rm -f ${DESTDIR}${PREFIX}/bin/$$f; done
	# removing example files.
	rm -f \
		${DESTDIR}${PREFIX}/share/${NAME}/README
	-rmdir ${DESTDIR}${PREFIX}/share/${NAME}
	# removing manual pages.
	for m in $(MAN1); do rm -f ${DESTDIR}${MANPREFIX}/man1/$$m; done

.PHONY: all clean dist install uninstall
