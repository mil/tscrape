#include <stdint.h>
#include <time.h>

#ifdef __OpenBSD__
#include <unistd.h>
#else
#define pledge(p1,p2) 0
#endif

#undef strlcat
size_t strlcat(char *, const char *, size_t);
#undef strlcpy
size_t strlcpy(char *, const char *, size_t);

/* feed info */
struct feed {
	char *        name;     /* feed name */
	unsigned long totalnew; /* amount of new items per feed */
	unsigned long total;    /* total items */
	time_t        timenewest;
	char          timenewestformat[64];
};

enum {
	FieldUnixTimestamp = 0,
	FieldUsername, FieldFullname,
	FieldText, FieldItemid,
	FieldItemUsername, FieldItemFullname,
	FieldRetweetid, FieldIspinned,
	FieldLast
};

size_t  parseline(char *, char *[FieldLast]);
int     parsetime(const char *, time_t *, char *, size_t);
void    printescape(const char *);
void    printutf8pad(FILE *, const char *, size_t, int);
int     strtotime(const char *, time_t *);
void    xmlencode(const char *, FILE *);
