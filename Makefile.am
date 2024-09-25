AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Wall -Wextra

EXTRA_DIST = \
	README.md \
	lib/gnugetopt.h \
	lib/getopt.c \
	lib/getopt1.c \
	external/raygui/raygui.h \
	external/raygui/gui_window_file_dialog.h

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
	src/anim_fsm.h        src/anim_fsm.c       \
	src/collection.h      src/collection.c     \
	src/color.h           src/color.c          \
	src/const.h                                \
	src/hex.h             src/hex.c            \
	src/level.h           src/level.c          \
	src/logging.h         src/logging.c        \
	src/options.h         src/options.c        \
	src/raygui_file_dialog_impl.c              \
	src/raylib_helper.h   src/raylib_helper.c  \
	src/shader.h          src/shader.c         \
	src/tile.h            src/tile.c           \
	src/tile_draw.h       src/tile_draw.c      \
	src/tile_pos.h        src/tile_pos.c       \
	src/util.h            src/util.c           \
	src/win_anim.h        src/win_anim.c       \
	src/common.h          src/main.c

hexpuzzle_SOURCES +=              \
	external/zip-kuba/miniz.h \
	external/zip-kuba/zip.h   \
	external/zip-kuba/zip.c

