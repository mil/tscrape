#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

/* data */
static char fullname[128];
static char timestamp[16];
static char text[256];
static char username[128];

static char      classname[256];
static char      datatime[16];
static int       state;
static XMLParser p;

static void
printescape(const char *s)
{
	size_t i;

	for (i = 0; *s; s++) {
		if (iscntrl(*s)) {
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

static void
printtimeformat(const char *s)
{
	time_t t = 0;
	struct tm *tm;
	char buf[32];

	if (strtotime(s, &t))
		return;
	if (!(tm = gmtime(&t)))
		return;
	if (!strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", tm))
		return;
	fputs(buf, stdout);
}

static void
printtweet(void)
{
	printescape(timestamp);
	putchar('\t');
	printtimeformat(timestamp);
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

static void
xmltagend(XMLParser *x, const char *t, size_t tl, int isshort)
{
	(void)x;
	(void)tl;
	(void)isshort;

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

	(void)x;
	(void)tl;
	(void)isshort;

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
	(void)x;
	(void)t;
	(void)tl;
	(void)al;
	(void)vl;

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
	if ((len = xml_entitytostr(v, buf, sizeof(buf))) > 0)
		xmlattr(x, t, tl, a, al, buf, (size_t)len);
	else
		xmlattr(x, t, tl, a, al, v, vl);
}

static void
xmldata(XMLParser *x, const char *d, size_t dl)
{
	(void)x;
	(void)dl;

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

	(void)x;

	if (!(state & (Text|Username|Fullname)))
		return;
	if ((len = xml_entitytostr(d, buf, sizeof(buf))) > 0)
		xmldata(x, buf, (size_t)len);
	else
		xmldata(x, d, dl);
}

static void
xmlcdata(XMLParser *x, const char *d, size_t dl)
{
	xmldata(x, d, dl);
}

int
main(void)
{
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
