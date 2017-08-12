include config.mk

NAME = tscrape
VERSION = 0.1
BIN = \
	tscrape\
	tscrape_plain

SRC = ${BIN:=.c}

LIBUTIL = libutil.a
LIBUTILSRC = \
	strlcat.c\
	strlcpy.c\
	util.c
LIBUTILOBJ = ${LIBUTILSRC:.c=.o}

LIBXML = libxml.a
LIBXMLSRC = \
	xml.c
LIBXMLOBJ = ${LIBXMLSRC:.c=.o}

LIB = ${LIBUTIL} ${LIBXML}

MAN1 = ${BIN:=.1}

DOC = \
	LICENSE\
	README
HDR = \
	xml.h

all: $(BIN)

${BIN}: ${LIB} ${@:=.o}

OBJ = ${SRC:.c=.o} ${LIBUTILOBJ} ${LIBXMLOBJ}

${OBJ}: config.mk ${HDR}

.o:
	${CC} ${LDFLAGS} -o $@ $< ${LIB}

.c.o:
	${CC} -c ${CFLAGS} ${CPPFLAGS} -o $@ -c $<

${LIBUTIL}: ${LIBUTILOBJ}
	${AR} rc $@ $?
	${RANLIB} $@

${LIBXML}: ${LIBXMLOBJ}
	${AR} rc $@ $?
	${RANLIB} $@

dist: $(BIN)
	rm -rf release/${VERSION}
	mkdir -p release/${VERSION}
	cp -f ${MAN1} ${DOC} ${HDR} \
		${SRC} ${LIBXMLSRC} ${LIBUTILSRC} \
		Makefile config.mk \
		release/${VERSION}/
	# make tarball
	rm -f tscrape-${VERSION}.tar.gz
	(cd release/${VERSION}; \
	tar -czf ../../tscrape-${VERSION}.tar.gz .)

clean:
	rm -f ${BIN} ${OBJ} ${LIB}

install: all
	# installing executable files.
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${BIN} ${SCRIPTS} ${DESTDIR}${PREFIX}/bin
	for f in $(BIN); do chmod 755 ${DESTDIR}${PREFIX}/bin/$$f; done
	# installing manual pages for tools.
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	cp -f ${MAN1} ${DESTDIR}${MANPREFIX}/man1
	for m in $(MAN1); do chmod 644 ${DESTDIR}${MANPREFIX}/man1/$$m; done

uninstall:
	# removing executable files.
	for f in $(BIN); do rm -f ${DESTDIR}${PREFIX}/bin/$$f; done
	# removing manual pages.
	for m in $(MAN1); do rm -f ${DESTDIR}${MANPREFIX}/man1/$$m; done

.PHONY: all clean dist install uninstall
