
noinst_LIBRARIES = libraylib.a

PLATFORM = PLATFORM_DESKTOP
GRAPHICS = GRAPHICS_API_OPENGL_33

libraylib_a_CFLAGS = \
	$(ALL_PKG_CFLAGS) \
	$(APP_CFLAGS) \
	-Wall \
	-D_GNU_SOURCE \
	-D$(PLATFORM) \
	-D$(GRAPHICS) \
	-Wno-missing-braces \
	-Werror=pointer-arith \
	-fno-strict-aliasing \
	-Werror=implicit-function-declaration \
	-I. \
	-Iexternal/glfw/include \
	-Iexternal/glfw/deps/mingw \
	-fPIC

if BUILD_DEBUG
libraylib_a_CFLAGS += -D_DEBUG
endif

GLFW_SRC = \
	external/glfw/include/GLFW/glfw3.h \
	external/glfw/include/GLFW/glfw3native.h

RAYLIB_EXTERNAL_SRC = \
	external/cgltf.h \
	external/dirent.h \
	external/dr_flac.h \
	external/dr_mp3.h \
	external/dr_wav.h \
	external/glad.h \
	external/glad_gles2.h \
	external/jar_mod.h \
	external/jar_xm.h \
	external/m3d.h \
	external/miniaudio.h \
	external/msf_gif.h \
	external/nanosvg.h \
	external/nanosvgrast.h \
	external/par_shapes.h \
	external/qoa.h \
	external/qoi.h \
	external/rl_gputex.h \
	external/rprand.h \
	external/sdefl.h \
	external/sinfl.h \
	external/stb_image.h \
	external/stb_image_resize2.h \
	external/stb_image_write.h \
	external/stb_perlin.h \
	external/stb_rect_pack.h \
	external/stb_truetype.h \
	external/tinyobj_loader_c.h \
	external/vox_loader.h

libraylib_a_SOURCES = \
	config,h \
	raudio.c \
	raylib.h \
	rcore.c \
	rglfw.c \
	rlgl.h \
	rmodels.c \
	rshapes.c \
	rtext.c \
	rtextures.c \
	utils.c \
	utils.h


EXTRA_DIST = \
	$(GLFW_SRC) \
	$(RAYLIB_EXTERNAL_SRC) \
	external/glfw/LICENSE.md
