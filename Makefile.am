SUFFIXES = .ttf .otf

AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = -I m4 -I m4/gnulib

SUBDIRS =

AM_CPPFLAGS = -Wall -Wextra

LDADD =

XXD ?= xxd

CLASSICS_LEVELS = \
	levels/classics/1-red.hexlevelpack \
	levels/classics/2-blue.hexlevelpack \
	levels/classics/3-green.hexlevelpack \
	levels/classics/4-yellow.hexlevelpack

FONTS = \
	fonts/cabin_condensed/cabin_condensed_bold.ttf \
	fonts/jockeyone/jockeyone_regular.ttf \
	fonts/ubuntu/ubuntu_medium.ttf

SHADERS = \
	shaders/background.vert.glsl \
	shaders/background.frag.glsl \
	shaders/postprocessing.frag.glsl \
	shaders/win_border.frag.glsl

TEXTURES = \
	textures/bg_grid.png

EXTRA_DIST = \
	README.md \
	lib/gnugetopt.h \
	lib/getopt.c \
	lib/getopt1.c \
	m4/gnulib/gnulib-cache.m4 \
	external/sglib/LICENSE \
	fonts/cabin_condensed/OFL.txt \
	fonts/jockeyone/OFL.txt \
	fonts/ubuntu/LICENCE.txt \
	$(CLASSICS_LEVELS) \
	$(FONTS) \
	$(SHADERS) \
	$(TEXTURES)

CHIPMUNK_DIR = external/chipmunk

if USE_CHIPMUNK
	MAYBE_USE_CHIPMUNK = $(CHIPMUNK_DIR)
endif
SUBDIRS += $(MAYBE_USE_CHIPMUNK)


if BUILD_WEB
RAYLIB_PLATFORM = PLATFORM_WEB
RAYLIB_GRAPHICS = GRAPHICS_API_OPENGL_ES2

COMMON_PROG_CFLAGS = \
	-DPLATFORM=PLATFORM_WEB \
	-DPLATFORM_WEB \
	-flto
else
RAYLIB_PLATFORM = PLATFORM_DESKTOP
RAYLIB_GRAPHICS = GRAPHICS_API_OPENGL_33

COMMON_PROG_CFLAGS = \
	-D$(RAYLIB_PLATFORM) \
	-D$(RAYLIB_GRAPHICS) \
	-DPLATFORM=PLATFORM_DESKTOP \
	-DPLATFORM_DESKTOP
endif

noinst_LIBRARIES = \
	external/libcombined_deps.a \
	external/raylib/libraylib.a

bin_PROGRAMS = hexpuzzle

if ! BUILD_DEBUG
all-local: hexpuzzle$(EXEEXT)
	$(STRIP) -R .comment hexpuzzle$(EXEEXT)
endif

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
	external/libcombined_deps.a \
	external/raylib/libraylib.a \
	$(PLATFORM_LIBS)
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

if USE_CHIPMUNK
hexpuzzle_CFLAGS += \
	-I$(CHIPMUNK_DIR)/include/chipmunk

hexpuzzle_LDADD += \
	$(CHIPMUNK_DIR)/libchipmunk.a
endif

hexpuzzle_SOURCES =                                           \
	src/background.h           src/background.c           \
	src/blueprint_string.h     src/blueprint_string.c     \
	src/classics.h             src/classics.c             \
	src/collection.h           src/collection.c           \
	src/color.h                src/color.c                \
	src/const.h                                           \
	src/fonts.h                src/fonts.c                \
	src/fsdir.h                src/fsdir.c                \
	src/game_mode.h            src/game_mode.c            \
	src/generate_level.h       src/generate_level.c       \
	src/gui_browser.h          src/gui_browser.c          \
	src/gui_collection.h       src/gui_collection.c       \
	src/gui_dialog.h           src/gui_dialog.c           \
	src/gui_help.h             src/gui_help.c             \
	src/gui_options.h          src/gui_options.c          \
	src/gui_popup_message.h    src/gui_popup_message.c    \
	src/gui_random.h           src/gui_random.c           \
	src/gui_title.h            src/gui_title.c            \
	src/hex.h                  src/hex.c                  \
	src/level.h                src/level.c                \
	src/level_draw.h           src/level_draw.c           \
	src/level_undo.h           src/level_undo.c           \
	src/logging.h              src/logging.c              \
	src/numeric.h              src/numeric.c              \
	src/nvdata.h               src/nvdata.c               \
	src/nvdata_finished.h      src/nvdata_finished.c      \
	src/options.h              src/options.c              \
	src/path.h                 src/path.c                 \
	src/range.h                src/range.c                \
	src/raygui_cell.h          src/raygui_cell.c          \
	src/raygui_impl.c 			              \
	src/raygui_paged_list.h    src/raygui_paged_list.c    \
	src/raylib_gui_numeric.h   src/raylib_gui_numeric.c   \
	src/raylib_helper.h        src/raylib_helper.c        \
	src/startup_action.h       src/startup_action.c       \
	src/shader.h               src/shader.c               \
	src/solve_timer.h          src/solve_timer.c          \
	src/solver.h               src/solver.c               \
	src/textures.h             src/textures.c             \
	src/tile.h                 src/tile.c                 \
	src/tile_draw.h            src/tile_draw.c            \
	src/tile_pos.h             src/tile_pos.c             \
	src/util.h                 src/util.c                 \
	src/win_anim.h             src/win_anim.c             \
	src/win_anim_mode_config.h src/win_anim_mode_config.c \
	src/common.h               src/main.c

if USE_PHYSICS
hexpuzzle_SOURCES += \
	src/physics.h           src/physics.c
endif

dist_EXTRA_hexpuzzle_SOURCES =      \
	src/data_classics_levels.h  \
	src/data_fonts.h            \
	src/data_shaders.h	    \
	src/data_textures.h

EXTRA_PROGRAMS = util/compile_to_c_src
util_compile_to_c_src_SOURCES = util/compile_to_c_src.c
util_compile_to_c_src_CFLAGS = $(COMMON_PROG_CFLAGS)
util_compile_to_c_src_LDADD = $(COMMON_PROG_LDADD) \
	external/raylib/libraylib.a \
	$(PLATFORM_LIBS)

src/data_classics_levels.h: $(CLASSICS_LEVELS)
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)cd $(top_srcdir) && for file in $^ ; do \
	    $(XXD) -i "$${file}" >> $@ ; \
	done

src/data_fonts.h: $(FONTS) | util/compile_to_c_src
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)cd $(top_srcdir) && for file in $^ ; do \
	    $(builddir)/util/compile_to_c_src -c "$${file}" "$${file}".c ; \
	    cat "$${file}".c >> $@ ; \
	    rm -f "$${file}".c ; \
	done

src/data_shaders.h: $(SHADERS)
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)cd $(top_srcdir) && for file in $^ ; do \
	    $(XXD) -i "$${file}" >> $@ ; \
	done

src/data_textures.h: $(TEXTURES)
	$(AM_V_at)rm -f $@
	$(AM_V_GEN)cd $(top_srcdir) && for file in $^ ; do \
	    $(XXD) -i "$${file}" >> $@ ; \
	done

NO_UNUSED_BUT_SET_OPT =
if CC_IS_GCC
NO_UNUSED_BUT_SET_OPT += -Wno-unused-but-set-variable
endif

src/hexpuzzle-physac_impl.$(OBJEXT): CFLAGS += -Wno-sign-compare
src/hexpuzzle-raygui_impl.$(OBJEXT): CFLAGS += -Wno-unused-parameter -Wno-unused-result
src/hexpuzzle-nvdata_finished.$(OBJEXT): CFLAGS += $(NO_UNUSED_BUT_SET_OPT) -Wno-unused-variable -Wno-incompatible-pointer-types
src/hexpuzzle-gui_popup_message.$(OBJEXT): CFLAGS += $(NO_UNUSED_BUT_SET_OPT) -Wno-unused-variable -Wno-incompatible-pointer-types -Wno-misleading-indentation
src/hexpuzzle-fsdir.$(OBJEXT): CFLAGS += -Wno-unused-variable -Wno-misleading-indentation

BUILT_SOURCES = \
	util/compile_to_c_src \
	$(dist_EXTRA_hexpuzzle_SOURCES)

DISTCLEANFILES = \
	util/compile_to_c_src

MAINTAINERCLEANFILES = \
	util/compile_to_c_src \
	$(dist_EXTRA_hexpuzzle_SOURCES)


######################################################################
# libcombined_deps.a

#external_libcombined_deps_a_LIBADD = \
#	external/raylib/libraylib.a

external_libcombined_deps_a_SOURCES =	 \
	external/raygui/raygui.h 	 \
	external/raygui/style/dark_alt.h \
	external/stb/stb_perlin.h	 \
	external/sglib/sglib.h		 \
	external/pcg/pcg_basic.c	 \
	external/pcg/pcg_basic.h	 \
	external/cJSON/cJSON.c		 \
	external/cJSON/cJSON.h


######################################################################
# libraylib.a

external_raylib_libraylib_a_CFLAGS = \
	$(ALL_PKG_CFLAGS) \
	$(APP_CFLAGS) \
	-Wall \
	-D_GNU_SOURCE \
	-D$(RAYLIB_PLATFORM) \
	-D$(RAYLIB_GRAPHICS) \
	-Wno-missing-braces \
	-Werror=pointer-arith \
	-fno-strict-aliasing \
	-Werror=implicit-function-declaration \
	-I$(top_srcdir)/external/raylib \
	-I$(top_srcdir)/external/raylib/external/glfw/include \
	-I$(top_srcdir)/external/raylib/external/glfw/deps/mingw \
	-fPIC \
	-Wno-unused-function \
	-Wno-unused-parameter \
	-Wno-sign-compare

if BUILD_DEBUG
external_raylib_libraylib_a_CFLAGS += -D_DEBUG
endif

GLFW_SRC = \
	external/raylib/external/glfw/src/cocoa_joystick.h \
	external/raylib/external/glfw/src/cocoa_platform.h \
	external/raylib/external/glfw/src/cocoa_time.c \
	external/raylib/external/glfw/src/cocoa_time.h \
	external/raylib/external/glfw/src/context.c \
	external/raylib/external/glfw/src/egl_context.c \
	external/raylib/external/glfw/src/glx_context.c \
	external/raylib/external/glfw/src/init.c \
	external/raylib/external/glfw/src/input.c \
	external/raylib/external/glfw/src/internal.h \
	external/raylib/external/glfw/src/linux_joystick.c \
	external/raylib/external/glfw/src/linux_joystick.h \
	external/raylib/external/glfw/src/mappings.h \
	external/raylib/external/glfw/src/monitor.c \
	external/raylib/external/glfw/src/null_init.c \
	external/raylib/external/glfw/src/null_joystick.c \
	external/raylib/external/glfw/src/null_joystick.h \
	external/raylib/external/glfw/src/null_monitor.c \
	external/raylib/external/glfw/src/null_platform.h \
	external/raylib/external/glfw/src/null_window.c \
	external/raylib/external/glfw/src/osmesa_context.c \
	external/raylib/external/glfw/src/platform.c \
	external/raylib/external/glfw/src/platform.h \
	external/raylib/external/glfw/src/posix_module.c \
	external/raylib/external/glfw/src/posix_poll.c \
	external/raylib/external/glfw/src/posix_poll.h \
	external/raylib/external/glfw/src/posix_thread.c \
	external/raylib/external/glfw/src/posix_thread.h \
	external/raylib/external/glfw/src/posix_time.c \
	external/raylib/external/glfw/src/posix_time.h \
	external/raylib/external/glfw/src/vulkan.c \
	external/raylib/external/glfw/src/wgl_context.c \
	external/raylib/external/glfw/src/win32_init.c \
	external/raylib/external/glfw/src/win32_joystick.c \
	external/raylib/external/glfw/src/win32_joystick.h \
	external/raylib/external/glfw/src/win32_module.c \
	external/raylib/external/glfw/src/win32_monitor.c \
	external/raylib/external/glfw/src/win32_platform.h \
	external/raylib/external/glfw/src/win32_thread.c \
	external/raylib/external/glfw/src/win32_thread.h \
	external/raylib/external/glfw/src/win32_time.c \
	external/raylib/external/glfw/src/win32_time.h \
	external/raylib/external/glfw/src/win32_window.c \
	external/raylib/external/glfw/src/window.c \
	external/raylib/external/glfw/src/wl_init.c \
	external/raylib/external/glfw/src/wl_monitor.c \
	external/raylib/external/glfw/src/wl_platform.h \
	external/raylib/external/glfw/src/wl_window.c \
	external/raylib/external/glfw/src/x11_init.c \
	external/raylib/external/glfw/src/x11_monitor.c \
	external/raylib/external/glfw/src/x11_platform.h \
	external/raylib/external/glfw/src/x11_window.c \
	external/raylib/external/glfw/src/xkb_unicode.c \
	external/raylib/external/glfw/src/xkb_unicode.h \
	external/raylib/external/glfw/include/GLFW/glfw3.h \
	external/raylib/external/glfw/include/GLFW/glfw3native.h

RAYLIB_EXTERNAL_SRC = \
	external/raylib/external/cgltf.h \
	external/raylib/external/dirent.h \
	external/raylib/external/dr_flac.h \
	external/raylib/external/dr_mp3.h \
	external/raylib/external/dr_wav.h \
	external/raylib/external/glad.h \
	external/raylib/external/glad_gles2.h \
	external/raylib/external/jar_mod.h \
	external/raylib/external/jar_xm.h \
	external/raylib/external/m3d.h \
	external/raylib/external/miniaudio.h \
	external/raylib/external/msf_gif.h \
	external/raylib/external/nanosvg.h \
	external/raylib/external/nanosvgrast.h \
	external/raylib/external/par_shapes.h \
	external/raylib/external/qoa.h \
	external/raylib/external/qoi.h \
	external/raylib/external/rl_gputex.h \
	external/raylib/external/rprand.h \
	external/raylib/external/sdefl.h \
	external/raylib/external/sinfl.h \
	external/raylib/external/stb_image.h \
	external/raylib/external/stb_image_resize2.h \
	external/raylib/external/stb_image_write.h \
	external/raylib/external/stb_perlin.h \
	external/raylib/external/stb_rect_pack.h \
	external/raylib/external/stb_truetype.h \
	external/raylib/external/tinyobj_loader_c.h \
	external/raylib/external/vox_loader.h

external_raylib_libraylib_a_SOURCES = \
	external/raylib/raylib_config.h \
	external/raylib/raudio.c \
	external/raylib/raylib.h \
	external/raylib/rcore.c \
	external/raylib/rlgl.h \
	external/raylib/rmodels.c \
	external/raylib/rshapes.c \
	external/raylib/rtext.c \
	external/raylib/rtextures.c \
	external/raylib/utils.c \
	external/raylib/utils.h

if ! BUILD_WEB
external_raylib_libraylib_a_SOURCES += \
	external/raylib/rglfw.c
endif

EXTRA_DIST += \
	$(GLFW_SRC) \
	$(RAYLIB_EXTERNAL_SRC) \
	external/raylib/platforms/rcore_desktop.c \
	external/raylib/platforms/rcore_web.c \
	external/raylib/external/glfw/LICENSE.md

external/raylib/external_raylib_libraylib_a-rglfw.$(OBJEXT): CFLAGS += $(GLFW_OSX) -Wno-missing-field-initializers
external/raylib/external_raylib_libraylib_a-rtextures.$(OBJEXT): CFLAGS += -Wno-array-bounds


#########################################################
# gnulib

AM_CPPFLAGS += -I$(top_builddir)/lib -I$(top_srcdir)/lib

hexpuzzle_LDADD += lib/gnulib/libgnu.a
hexpuzzle_LDADD += $(CLOCK_TIME_LIB)

MOSTLYCLEANFILES =
MOSTLYCLEANDIRS =

include lib/gnulib/Makefile.gnulib
