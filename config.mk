# customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/man
DOCPREFIX = ${PREFIX}/share/doc/tscrape

# compiler and linker
CC = cc
AR = ar
RANLIB = ranlib

# use system flags.
TSCRAPE_CFLAGS = ${CFLAGS}
TSCRAPE_LDFLAGS = ${LDFLAGS}
TSCRAPE_CPPFLAGS = -D_DEFAULT_SOURCE

# debug
#TSCRAPE_CFLAGS = -fstack-protector-all -O0 -g -std=c99 -Wall -Wextra -pedantic
#TSCRAPE_LDFLAGS =

# optimized
#TSCRAPE_CFLAGS = -O2 -std=c99
#TSCRAPE_LDFLAGS = -s

# optimized static
#TSCRAPE_CFLAGS = -static -O2 -std=c99
#TSCRAPE_LDFLAGS = -static -s
