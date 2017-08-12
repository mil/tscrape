#include <ctype.h>
#include <err.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"

static time_t comparetime;
static char *line;
static size_t linesize;

static void
printfeed(FILE *fp, const char *feedname)
{
	char *fields[FieldLast];
	struct tm *tm;
	time_t parsedtime;
	ssize_t linelen;

	while ((linelen = getline(&line, &linesize, fp)) > 0) {
		if (line[linelen - 1] == '\n')
			line[--linelen] = '\0';
		if (!parseline(line, fields))
			break;

		parsedtime = 0;
		strtotime(fields[FieldUnixTimestamp], &parsedtime);
	        if (!(tm = localtime(&parsedtime)))
			err(1, "localtime");

		if (parsedtime >= comparetime)
			putchar('N');
		else
			putchar(' ');
		if (fields[FieldRetweetid][0])
			putchar('R');
		else
			putchar(' ');
		putchar(' ');

		if (feedname[0])
			printf("%-15.15s  ", feedname);

	        fprintf(stdout, "%04d-%02d-%02d %02d:%02d  ",
		        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		        tm->tm_hour, tm->tm_min);

		printutf8pad(stdout, fields[FieldItemFullname], 25, ' ');
		fputs("  ", stdout);
		printescape(fields[FieldText]);
		putchar('\n');
	}
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	char *name;
	int i;

	if (pledge("stdio rpath", NULL) == -1)
		err(1, "pledge");

	setlocale(LC_CTYPE, "");

	if (pledge(argc == 1 ? "stdio" : "stdio rpath", NULL) == -1)
		err(1, "pledge");

	if ((comparetime = time(NULL)) == -1)
		err(1, "time");
	/* 1 day is old news */
	comparetime -= 86400;

	if (argc == 1) {
		printfeed(stdin, "");
	} else {
		for (i = 1; i < argc; i++) {
			if (!(fp = fopen(argv[i], "r")))
				err(1, "fopen: %s", argv[i]);
			name = ((name = strrchr(argv[i], '/'))) ? name + 1 : argv[i];
			printfeed(fp, name);
			if (ferror(fp))
				err(1, "ferror: %s", argv[i]);
			fclose(fp);
		}
	}
	return 0;
}
