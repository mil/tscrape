include config.mk

NAME = tscrape
VERSION = 0.1

BIN = \
	tscrape\
	tscrape_html\
	tscrape_plain
SCRIPTS = \
	tscrape_update

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

MAN1 = tscrape.1

DOC = \
	LICENSE\
	README
HDR = \
	util.h\
	xml.h

all: $(BIN)

${BIN}: ${LIB} ${@:=.o}

OBJ = ${SRC:.c=.o} ${LIBXMLOBJ} ${LIBUTILOBJ}

${OBJ}: config.mk ${HDR}

.o:
	${CC} ${TSCRAPE_LDFLAGS} -o $@ $< ${LIB}

.c.o:
	${CC} ${TSCRAPE_CFLAGS} ${TSCRAPE_CPPFLAGS} -o $@ -c $<

${LIBUTIL}: ${LIBUTILOBJ}
	${AR} rc $@ $?
	${RANLIB} $@

${LIBXML}: ${LIBXMLOBJ}
	${AR} rc $@ $?
	${RANLIB} $@

dist:
	rm -rf "${NAME}-${VERSION}"
	mkdir -p "${NAME}-${VERSION}"
	cp -f ${MAN1} ${DOC} ${HDR} \
		${SRC} ${LIBXMLSRC} ${LIBUTILSRC} ${SCRIPTS} \
		Makefile config.mk \
		tscraperc.example style.css \
		"${NAME}-${VERSION}"
	# make tarball
	tar -cf - "${NAME}-${VERSION}" | \
		gzip -c > "${NAME}-${VERSION}.tar.gz"
	rm -rf "${NAME}-${VERSION}"

clean:
	rm -f ${BIN} ${OBJ} ${LIB}

install: all
	# installing executable files and scripts.
	mkdir -p "${DESTDIR}${PREFIX}/bin"
	cp -f ${BIN} ${SCRIPTS} "${DESTDIR}${PREFIX}/bin"
	for f in $(BIN) $(SCRIPTS); do chmod 755 "${DESTDIR}${PREFIX}/bin/$$f"; done
	# installing example files.
	mkdir -p "${DESTDIR}${DOCPREFIX}"
	cp -f tscraperc.example\
		style.css\
		README\
		"${DESTDIR}${DOCPREFIX}"
	# installing manual pages for tools.
	mkdir -p "${DESTDIR}${MANPREFIX}/man1"
	cp -f ${MAN1} "${DESTDIR}${MANPREFIX}/man1"
	for m in $(MAN1); do chmod 644 "${DESTDIR}${MANPREFIX}/man1/$$m"; done

uninstall:
	# removing executable files and scripts.
	for f in $(BIN) $(SCRIPTS); do rm -f "${DESTDIR}${PREFIX}/bin/$$f"; done
	# removing example files.
	rm -f \
		"${DESTDIR}${DOCPREFIX}/tscraperc.example"\
		"${DESTDIR}${DOCPREFIX}/style.css"\
		"${DESTDIR}${DOCPREFIX}/README"
	-rmdir "${DESTDIR}${DOCPREFIX}"
	# removing manual pages.
	for m in $(MAN1); do rm -f "${DESTDIR}${MANPREFIX}/man1/$$m"; done

.PHONY: all clean dist install uninstall
