AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Wall -Wextra

RAYLIB_SRC_DIR = external/raylib

if ! USE_SYSTEM_RAYLIB
MAYBE_RAYLIB_SRC_DIR = $(RAYLIB_SRC_DIR)
endif

SUBDIRS = $(MAYBE_RAYLIB_SRC_DIR)

XXD ?= xxd

CLASSICS_LEVELS = \
	levels/classics/1-red.hexlevelpack \
	levels/classics/2-blue.hexlevelpack \
	levels/classics/3-green.hexlevelpack \
	levels/classics/4-yellow.hexlevelpack

FONTS = \
	fonts/PalanquinDark-Regular.ttf

SHADERS = \
	shaders/postprocessing.frag.glsl \
	shaders/win_border.frag.glsl

EXTRA_DIST = \
	README.md \
	lib/gnugetopt.h \
	lib/getopt.c \
	lib/getopt1.c \
	external/sglib/LICENSE \
	$(CLASSICS_LEVELS) \
	$(FONTS) \
	$(SHADERS)

if BUILD_WEB
COMMON_PROG_CFLAGS = \
	-DPLATFORM=PLATFORM_WEB \
	-DPLATFORM_WEB \
	-flto
else
COMMON_PROG_CFLAGS = \
	-DPLATFORM=PLATFORM_DESKTOP \
	-DPLATFORM_DESKTOP
endif

bin_PROGRAMS = hexpuzzle

COMMON_PROG_CFLAGS += \
	-I$(top_srcdir)/src \
	$(ALL_PKG_CFLAGS) \
	$(APP_CFLAGS)

COMMON_PROG_LDADD = \
	$(ALL_PKG_LIBS) \
	$(APP_LIBS) \
	-lm

hexpuzzle_CFLAGS = $(COMMON_PROG_CFLAGS)
hexpuzzle_LDADD = $(COMMON_PROG_LDADD)

if ! USE_SYSTEM_RAYLIB
hexpuzzle_LDADD += \
	$(RAYLIB_SRC_DIR)/libraylib.a \
	-lGL -lc -lm -lpthread -ldl -lrt
endif

if BUILD_WEB
hexpuzzle_LDFLAGS = \
	-flto \
	-s MAX_WEBGL_VERSION=1 \
	-s USE_GLFW=3 \
	-s WASM=1 \
	-s GL_ENABLE_GET_PROC_ADDRESS=1 \
	-s ALLOW_MEMORY_GROWTH=1
#	-s ASYNCIFY

if BUILD_DEBUG
hexpuzzle_LDFLAGS += \
	-s ASSERTIONS=1
endif

hexpuzzle_LDFLAGS += \
	-lidbfs.js \
	--shell-file minshell.html

clean-local:
	rm -f hexpuzzle.js hexpuzzle.wasm
endif

hexpuzzle_SOURCES =                                    \
	src/anim_fsm.h          src/anim_fsm.c         \
	src/classics.h          src/classics.c         \
	src/collection.h        src/collection.c       \
	src/color.h             src/color.c            \
	src/const.h                                    \
	src/fonts.h             src/fonts.c            \
	src/game_mode.h         src/game_mode.c        \
	src/gui_browser.h       src/gui_browser.c      \
	src/gui_options.h       src/gui_options.c      \
	src/gui_random.h        src/gui_random.c       \
	src/hex.h               src/hex.c              \
	src/level.h             src/level.c            \
	src/level_draw.h        src/level_draw.c       \
	src/logging.h           src/logging.c          \
	src/nvdata.h            src/nvdata.c           \
	src/nvdata_finished.h   src/nvdata_finished.c  \
	src/options.h           src/options.c          \
	src/path.h              src/path.c             \
	src/raygui_impl.c 			       \
	src/raylib_helper.h     src/raylib_helper.c    \
	src/startup_action.h    src/startup_action.c   \
	src/shader.h            src/shader.c           \
	src/solver.h            src/solver.c           \
	src/tile.h              src/tile.c             \
	src/tile_draw.h         src/tile_draw.c        \
	src/tile_pos.h          src/tile_pos.c         \
	src/util.h              src/util.c             \
	src/win_anim.h          src/win_anim.c         \
	src/common.h            src/main.c

hexpuzzle_SOURCES +=             		   \
	external/raygui/raygui.h 		   \
	external/raygui/style/dark_alt.h           \
	external/stb/stb_perlin.h                  \
	external/sglib/sglib.h                     \
	external/pcg/pcg_basic.c                   \
	external/pcg/pcg_basic.h                   \
	external/cJSON/cJSON.c                     \
	external/cJSON/cJSON.h

if ! BUILD_WEB
hexpuzzle_SOURCES += 				   \
	external/tinyfiledialogs/tinyfiledialogs.c \
	external/tinyfiledialogs/tinyfiledialogs.h
endif

dist_EXTRA_hexpuzzle_SOURCES =      \
	src/data_classics_levels.h  \
	src/data_fonts.h            \
	src/data_shaders.h

src/data_classics_levels.h: $(CLASSICS_LEVELS)
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)cd $(top_srcdir) && for file in $^ ; do \
	    $(XXD) -i "$${file}" >> $@ ; \
	done

src/data_fonts.h: $(FONTS)
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)cd $(top_srcdir) && for file in $^ ; do \
	    $(XXD) -i "$${file}" >> $@ ; \
	done

src/data_shaders.h: $(SHADERS)
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)cd $(top_srcdir) && for file in $^ ; do \
	    $(XXD) -i "$${file}" >> $@ ; \
	done

src/hexpuzzle-physac_impl.$(OBJEXT): CFLAGS += -Wno-sign-compare
src/hexpuzzle-raygui_impl.$(OBJEXT): CFLAGS += -Wno-unused-parameter -Wno-unused-result
src/hexpuzzle-nvdata_finished.$(OBJEXT): CFLAGS += -Wno-unused-but-set-variable -Wno-unused-variable -Wno-incompatible-pointer-types

BUILT_SOURCES = \
	$(dist_EXTRA_hexpuzzle_SOURCES)

MAINTAINERCLEANFILES = \
	$(dist_EXTRA_hexpuzzle_SOURCES)
