#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#include "util.h"

/* Read a field-separated line from 'fp',
 * separated by a character 'separator',
 * 'fields' is a list of pointers with a size of FieldLast (must be >0).
 * 'line' buffer is allocated using malloc, 'size' will contain the allocated
 * buffer size.
 * returns: amount of fields read (>0) or -1 on error. */
size_t
parseline(char *line, char *fields[FieldLast])
{
	char *prev, *s;
	size_t i;

	for (prev = line, i = 0;
	    (s = strchr(prev, '\t')) && i < FieldLast - 1;
	    i++) {
		*s = '\0';
		fields[i] = prev;
		prev = s + 1;
	}
	fields[i++] = prev;
	/* make non-parsed fields empty. */
	for (; i < FieldLast; i++)
		fields[i] = "";

	return i;
}

/* Parse time to time_t, assumes time_t is signed, ignores fractions. */
int
strtotime(const char *s, time_t *t)
{
	long long l;
	char *e;

	errno = 0;
	l = strtoll(s, &e, 10);
	if (errno || *s == '\0' || *e)
		return -1;
	/* NOTE: assumes time_t is 64-bit on 64-bit platforms:
	         long long (atleast 32-bit) to time_t. */
	if (t)
		*t = (time_t)l;

	return 0;
}

/* Escape characters below as HTML 2.0 / XML 1.0. */
void
xmlencode(const char *s, FILE *fp)
{
	for (; *s; s++) {
		switch(*s) {
		case '<':  fputs("&lt;",   fp); break;
		case '>':  fputs("&gt;",   fp); break;
		case '\'': fputs("&#39;",  fp); break;
		case '&':  fputs("&amp;",  fp); break;
		case '"':  fputs("&quot;", fp); break;
		default:   fputc(*s, fp);
		}
	}
}

/* print `len' columns of characters. If string is shorter pad the rest
 * with characters `pad`. */
void
printutf8pad(FILE *fp, const char *s, size_t len, int pad)
{
	wchar_t w;
	size_t col = 0, i, slen;
	int rl, wc;

	if (!len)
		return;

	slen = strlen(s);
	for (i = 0; i < slen && col < len + 1; i += rl) {
		if ((rl = mbtowc(&w, &s[i], slen - i < 4 ? slen - i : 4)) <= 0)
			break;
		if ((wc = wcwidth(w)) == -1)
			wc = 1;
		col += (size_t)wc;
		if (col >= len && s[i + rl]) {
			fputs("\xe2\x80\xa6", fp);
			break;
		}
		fwrite(&s[i], 1, rl, fp);
	}
	for (; col < len; col++)
		putc(pad, fp);
}

void
printescape(const char *s)
{
	size_t i;
	const char *e;

	/* strip leading and trailing white-space */
	for (; *s && isspace((unsigned char)*s); s++)
		;
	for (e = s + strlen(s); e > s && isspace((unsigned char)*(e - 1)); e--)
		;

	for (i = 0; *s && s < e; s++) {
		if (iscntrl((unsigned char)*s) || isspace((unsigned char)*s)) {
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

int
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
