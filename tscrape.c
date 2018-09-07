#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "xml.h"
#include "util.h"

#define STRP(s) s,sizeof(s)-1

/* states */
enum {
	Item      = 1,
	Stream    = 2,
	Header    = 4,
	Timestamp = 8,
	Text      = 16
};

/* data */
static char fullname[1024];
static int  ispinned;
static char itemusername[1024];
static char itemfullname[1024];
static char timestamp[16];
static char text[4096];
static char username[1024];

static char      classname[256];
static char      datatime[16];
static char      itemid[64];
static char      retweetid[64];
static int       state;
static XMLParser p;

static void
printtweet(void)
{
	char buf[32];
	time_t t;

	if (parsetime(timestamp, &t, buf, sizeof(buf)) != -1)
		printf("%lld", (long long)t);
	putchar('\t');
	printescape(username);
	putchar('\t');
	printescape(fullname);
	putchar('\t');
	printescape(text);
	putchar('\t');
	printescape(itemid);
	putchar('\t');
	printescape(itemusername);
	putchar('\t');
	printescape(itemfullname);
	putchar('\t');
	printescape(retweetid);
	putchar('\t');
	printf("%d", ispinned);
	putchar('\n');
}

static int
isclassmatch(const char *classes, const char *clss, size_t len)
{
	const char *p;

	if (!(p = strstr(classes, clss)))
		return 0;
	return (p == classes || isspace((unsigned char)p[-1])) &&
	        (isspace((unsigned char)p[len]) || !p[len]);
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
		state &= ~(Timestamp);
}

static char ignoretag[8];
static XMLParser xo; /* old context */

static void
xmlignoretagend(XMLParser *p, const char *t, size_t tl, int isshort)
{
	if (!strcasecmp(t, ignoretag))
		memcpy(p, &xo, sizeof(*p)); /* restore context */
}

static void
xmltagstart(XMLParser *x, const char *t, size_t tl)
{
	classname[0] = '\0';
}

static void
xmltagstartparsed(XMLParser *x, const char *t, size_t tl, int isshort)
{
	/* temporary replace the callback except the reader and end of tag
	   restore the context once we receive the same ignored tag in the
	   end tag handler */
	if (!strcasecmp(t, "script") || !strcasecmp(t, "style")) {
		strlcpy(ignoretag, t, sizeof(ignoretag));
		memcpy(&xo, x, sizeof(xo)); /* store old context */
		memset(x, 0, sizeof(*x));
		x->xmltagend = xmlignoretagend;
		x->getnext = xo.getnext;
		return;
	}

	if (!strcmp(t, "p") && isclassmatch(classname, STRP("js-tweet-text"))) {
		if (state & (Item | Stream | Header))
			state |= Text;
	} else if (!strcmp(t, "div") &&
	           isclassmatch(classname, STRP("stream-item-footer"))) {
		if (text[0] && username[0])
			printtweet();
		state = 0;
	} else if (!strcmp(t, "li") &&
	           isclassmatch(classname, STRP("js-stream-item"))) {
		state |= Item;
		datatime[0] = text[0] = timestamp[0] = itemfullname[0] = '\0';
		itemid[0] = itemusername[0] = retweetid[0] = '\0';
		ispinned = 0;
		if (isclassmatch(classname, STRP("js-pinned")))
			ispinned = 1;
	} else if (state & Item) {
		if (!strcmp(t, "div") &&
		    isclassmatch(classname, STRP("js-stream-tweet"))) {
			state &= ~(Text|Header);
			state |= Stream;
		} else if (!strcmp(t, "a") &&
		           isclassmatch(classname, STRP("js-action-profile"))) {
			state |= Header;
		} else if (!strcmp(t, "span") &&
		          isclassmatch(classname, STRP("js-short-timestamp"))) {
			state |= Timestamp;
			strlcpy(timestamp, datatime, sizeof(timestamp));
			datatime[0] = '\0';
		}
	}
	if ((state & Text) && !strcmp(t, "a") && !isspace((unsigned char)text[0]))
		strlcat(text, " ", sizeof(text));
}

static void
xmlattr(XMLParser *x, const char *t, size_t tl, const char *a, size_t al,
        const char *v, size_t vl)
{
	/* NOTE: assumes classname attribute is set before data-* in current tag */
	if (!state && !strcmp(t, "div") && isclassmatch(classname, STRP("user-actions"))) {
		if (!strcmp(a, "data-screen-name")) {
			strlcat(username, " ", sizeof(username));
			strlcat(username, v, sizeof(username));
		} else if (!strcmp(a, "data-name")) {
			strlcat(fullname, " ", sizeof(fullname));
			strlcat(fullname, v, sizeof(fullname));
		}
	}

	if (!strcmp(a, "class")) {
		strlcat(classname, v, sizeof(classname));
	} else if (state & Item) {
		if (!strcmp(t, "div")) {
			if (!strcmp(a, "data-item-id"))
				strlcpy(itemid, v, sizeof(itemid));
			else if (!strcmp(a, "data-retweet-id"))
				strlcpy(retweetid, v, sizeof(retweetid));

			if (isclassmatch(classname, STRP("js-stream-tweet"))) {
				if (!strcmp(a, "data-screen-name")) {
					strlcat(itemusername, " ", sizeof(itemusername));
					strlcat(itemusername, v, sizeof(itemusername));
				} else if (!strcmp(a, "data-name")) {
					strlcat(itemfullname, " ", sizeof(itemfullname));
					strlcat(itemfullname, v, sizeof(itemfullname));
				}
			}
		} else if (!strcmp(t, "span") && !strcmp(a, "data-time")) {
			/* UNIX timestamp */
			strlcpy(datatime, v, sizeof(datatime));
		}
		/* NOTE: can be <div data-image-url>. */
		if (!strcmp(a, "data-image-url")) {
			strlcat(text, " ", sizeof(text));
			strlcat(text, v, sizeof(text));
		}

		/* indication it has a video */
		if (itemid[0] && !strcmp(a, "data-playable-media-url")) {
			strlcat(text, " ", sizeof(text));
			strlcat(text, "https://twitter.com/i/videos/", sizeof(text));
			strlcat(text, itemid, sizeof(text));
		}
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
	if (state & Text) {
		if (!isclassmatch(classname, STRP("u-hidden")))
			strlcat(text, d, sizeof(text));
	}
}

static void
xmldataentity(XMLParser *x, const char *d, size_t dl)
{
	char buf[16];
	ssize_t len;

	if (!(state & Text))
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
	p.xmltagstart       = xmltagstart;
	p.xmltagend         = xmltagend;
	p.xmltagstartparsed = xmltagstartparsed;
	/* reader (stdin) */
	p.getnext           = getchar;

	xml_parse(&p);

	return 0;
}
