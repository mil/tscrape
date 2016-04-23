#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "xml.h"

#define STRP(s) s,sizeof(s)-1

/* states */
enum {
	Item      = 1,
	Stream    = 2,
	Header    = 4,
	Timestamp = 8,
	Text      = 16,
	Fullname  = 32,
	Username  = 64
};

/* for compatibility with libc's that don't have strlcat or strlcpy. The
 * functions are synced from OpenBSD */
#undef strlcat
size_t strlcat(char *, const char *, size_t);
#undef strlcpy
size_t strlcpy(char *, const char *, size_t);

/* data */
static char fullname[128];
static char timestamp[16];
static char text[1024];
static char username[128];

static char      classname[256];
static char      datatime[16];
static int       state;
static XMLParser p;

static void
printescape(const char *s)
{
	size_t i;
	char *e;

	/* strip leading and trailing white-space */
	for (; *s && isspace(*s); s++)
		;
	for (e = s + strlen(s); e > s && isspace(*(e - 1)); e--)
		;

	for (i = 0; *s && s < e; s++) {
		if (iscntrl(*s) || isspace(*s)) {
			i++;
			continue;
		}
		if (i) {
			i = 0;
			putchar(' ');
		}
		putchar(*s);
	}
}

/* Parse time to time_t, assumes time_t is signed. */
int
strtotime(const char *s, time_t *t)
{
	long l;
	char *e;

	errno = 0;
	l = strtol(s, &e, 10);
	if (*s == '\0' || *e != '\0')
		return -1;
	if (t)
		*t = (time_t)l;

	return 0;
}

static int
parsetime(const char *s, time_t *t, char *buf, size_t bufsiz)
{
	struct tm *tm;

	if (strtotime(s, t))
		return -1;
	if (!(tm = localtime(t)))
		return -1;
	if (!strftime(buf, bufsiz, "%Y-%m-%d %H:%M", tm))
		return -1;

	return 0;
}

static void
printtweet(void)
{
	char buf[32];
	time_t t;
	int r;

	r = parsetime(timestamp, &t, buf, sizeof(buf));
	if (r != -1)
		printf("%ld", (long)t);
	putchar('\t');
	if (r != -1)
		fputs(buf, stdout);
	putchar('\t');
	printescape(text);
	putchar('\t');
	printescape(username);
	putchar('\t');
	printescape(fullname);
	putchar('\n');
}

static int
isclassmatch(const char *classes, const char *clss, size_t len)
{
	const char *p;

	if (!(p = strstr(classes, clss)))
		return 0;
	return (p == classes || isspace(p[-1])) && (isspace(p[len]) || !p[len]);
}

/* convert XML and some HTML entities */
static ssize_t
html_entitytostr(const char *s, char *buf, size_t bufsiz)
{
	ssize_t len;

	if ((len = xml_entitytostr(s, buf, bufsiz)) > 0)
		return len;
	else if (!strcmp(s, "&nbsp;"))
		return (ssize_t)strlcpy(buf, " ", bufsiz);
	return len;
}

static void
xmltagend(XMLParser *x, const char *t, size_t tl, int isshort)
{
	if (!strcmp(t, "p"))
		state &= ~Text;
	else if (!strcmp(t, "span"))
		state &= ~(Timestamp|Username);
	else if (!strcmp(t, "strong"))
		state &= ~Fullname;
}

static void
xmltagstartparsed(XMLParser *x, const char *t, size_t tl, int isshort)
{
	const char *v = classname;

	if (!strcmp(t, "p") && isclassmatch(v, STRP("js-tweet-text"))) {
		if (state & (Item | Stream | Header))
			state |= Text;
	} else if (!strcmp(t, "div") && isclassmatch(v, STRP("stream-item-footer"))) {
		printtweet();
		state = 0;
	} else if (!strcmp(t, "li") && isclassmatch(v, STRP("js-stream-item"))) {
		state |= Item;
	} else if (state & Item) {
		if (!strcmp(t, "div") && isclassmatch(v, STRP("js-stream-tweet"))) {
			state &= ~(Text|Header);
			state |= Stream;
			datatime[0] = text[0] = timestamp[0] = fullname[0] = username[0] = '\0';
		} else if (!strcmp(t, "a") && isclassmatch(v, STRP("js-action-profile"))) {
			state |= Header;
		} else if (!strcmp(t, "strong") && isclassmatch(v, STRP("fullname"))) {
			state |= Fullname;
		} else if (!strcmp(t, "span") && isclassmatch(v, STRP("js-short-timestamp"))) {
			state |= Timestamp;
			strlcpy(timestamp, datatime, sizeof(timestamp));
			datatime[0] = '\0';
		} else if (!strcmp(t, "span") && isclassmatch(v, STRP("username"))) {
			state |= Username;
		}
	}
	classname[0] = '\0';
}

static void
xmlattr(XMLParser *x, const char *t, size_t tl, const char *a, size_t al,
        const char *v, size_t vl)
{
	if (!strcmp(a, "class")) {
		strlcat(classname, v, sizeof(classname));
	} else if ((state & Item) && !strcmp(t, "span") && !strcmp(a, "data-time")) {
		/* UNIX timestamp */
		strlcat(datatime, v, sizeof(datatime));
	}
}

static void
xmlattrentity(XMLParser *x, const char *t, size_t tl, const char *a, size_t al,
              const char *v, size_t vl)
{
	char buf[16];
	ssize_t len;

	if (!state)
		return;
	if ((len = html_entitytostr(v, buf, sizeof(buf))) > 0)
		xmlattr(x, t, tl, a, al, buf, (size_t)len);
	else
		xmlattr(x, t, tl, a, al, v, vl);
}

static void
xmldata(XMLParser *x, const char *d, size_t dl)
{
	if (state & Username)
		strlcat(username, d, sizeof(username));
	else if (state & Fullname)
		strlcat(fullname, d, sizeof(fullname));
	else if (state & Text)
		strlcat(text, d, sizeof(text));
}

static void
xmldataentity(XMLParser *x, const char *d, size_t dl)
{
	char buf[16];
	ssize_t len;

	if (!(state & (Text|Username|Fullname)))
		return;
	if ((len = html_entitytostr(d, buf, sizeof(buf))) > 0)
		xmldata(x, buf, (size_t)len);
	else
		xmldata(x, d, dl);
}

static void
xmlcdata(XMLParser *x, const char *d, size_t dl)
{
	xmldata(x, d, dl);
}

#ifndef USE_PLEDGE
int
pledge(const char *promises, const char *paths[])
{
	return 0;
}
#endif

int
main(void)
{
	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");

	/* handlers */
	p.xmlattr           = xmlattr;
	p.xmlattrentity     = xmlattrentity;
	p.xmlcdata          = xmlcdata;
	p.xmldata           = xmldata;
	p.xmldataentity     = xmldataentity;
	p.xmltagend         = xmltagend;
	p.xmltagstartparsed = xmltagstartparsed;
	/* reader (stdin) */
	p.getnext           = getchar;

	xml_parse(&p);

	return 0;
}
