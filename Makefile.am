AUTOMAKE_OPTIONS = subdir-objects

SUFFIXES = .glsl

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Wall -Wextra

XXD ?= xxd

CLASSICS_LEVELS = \
	levels/classics/1-red.hexlevelpack \
	levels/classics/2-blue.hexlevelpack \
	levels/classics/3-green.hexlevelpack \
	levels/classics/4-yellow.hexlevelpack


EXTRA_DIST = \
	README.md \
	lib/gnugetopt.h \
	lib/getopt.c \
	lib/getopt1.c \
	$(CLASSICS_LEVELS) \
	shaders/win_border.frag.glsl

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

hexpuzzle_SOURCES =                                    \
	src/anim_fsm.h          src/anim_fsm.c         \
	src/classics.h          src/classics.c         \
	src/collection.h        src/collection.c       \
	src/color.h             src/color.c            \
	src/const.h                                    \
	src/gui_browser.h       src/gui_browser.c      \
	src/gui_options.h       src/gui_options.c      \
	src/gui_random.h        src/gui_random.c       \
	src/hex.h               src/hex.c              \
	src/level.h             src/level.c            \
	src/logging.h           src/logging.c          \
	src/nvdata.h            src/nvdata.c           \
	src/nvdata_finished.h   src/nvdata_finished.c  \
	src/options.h           src/options.c          \
	src/raygui_impl.c            		       \
	src/raylib_helper.h     src/raylib_helper.c    \
	src/shader.h            src/shader.c           \
	src/tile.h              src/tile.c             \
	src/tile_draw.h         src/tile_draw.c        \
	src/tile_pos.h          src/tile_pos.c         \
	src/util.h              src/util.c             \
	src/win_anim.h          src/win_anim.c         \
	src/common.h            src/main.c

hexpuzzle_SOURCES +=             		   \
	external/raygui/raygui.h  		   \
	external/tinyfiledialogs/tinyfiledialogs.c \
	external/tinyfiledialogs/tinyfiledialogs.h \
	external/cJSON/cJSON.c                     \
	external/cJSON/cJSON.h

dist_EXTRA_hexpuzzle_SOURCES = \
	src/classics_levels.h  \
	shaders/win_border.frag.h

.glsl.h:
	$(AM_V_GEN)$(XXD) -i $^ > $@

win_border.frag.h: win_border.frag.glsl

src/classics_levels.h: $(CLASSICS_LEVELS)
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)cd $(top_srcdir) && for file in $^ ; do \
	    $(XXD) -i "$${file}" >> $@ ; \
	done

src/hexpuzzle-raygui_impl.$(OBJEXT): CFLAGS += -Wno-unused-parameter
src/hexpuzzle-nvdata_finished.$(OBJEXT): CFLAGS += -Wno-unused-but-set-variable -Wno-unused-variable -Wno-incompatible-pointer-types

BUILT_SOURCES = \
	$(dist_EXTRA_hexpuzzle_SOURCES)

MAINTAINERCLEANFILES = \
	$(dist_EXTRA_hexpuzzle_SOURCES)
