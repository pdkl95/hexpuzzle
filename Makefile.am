AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Wall -Wextra

EXTRA_DIST = \
	README.md \
	lib/gnugetopt.h \
	lib/getopt.c \
	lib/getopt1.c

bin_PROGRAMS = hexpuzzle

COMMON_PROG_CFLAGS = \
	-DPLATFORM=PLATFORM_DESKTOP \
	-DPLATFORM_DESKTOP \
	-I$(top_srcdir)/src \
	$(ALL_PKG_CFLAGS) \
	$(APP_CFLAGS)

COMMON_PROG_LDADD = \
	$(ALL_PKG_LIBS) \
	$(APP_LIBS) \
	-lm

hexpuzzle_CFLAGS = $(COMMON_PROG_CFLAGS)
hexpuzzle_LDADD = $(COMMON_PROG_LDADD)

hexpuzzle_SOURCES =                                \
	src/grid.h            src/grid.c           \
	src/hex.h             src/hex.c            \
	src/level.h           src/level.c          \
	src/logging.h         src/logging.c        \
	src/options.h         src/options.c        \
	src/raylib_helper.h   src/raylib_helper.c  \
	src/tile.h            src/tile.c           \
	src/util.h            src/util.c           \
	src/common.h          src/main.c

