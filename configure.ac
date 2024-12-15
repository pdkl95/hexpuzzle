dnl -*- Autoconf -*-

m4_define([project_name], [hexpuzzle])
m4_define([project_url], [https://github.com/pdkl95/]project_name)
m4_define([project_email], pdkl95@gmail.com)
m4_define([project_author], [Brent Sanders])
m4_define([project_copyright_year], [2023])
m4_define([project_licence], [GPLv3+])
m4_define([project_licence_desc], [GNU GPL version 3 or later <http://gnu.org/licences/gpl.html>])


dnl Release Version
m4_define([project_major_version], [0])
m4_define([project_minor_version], [1])
m4_define([project_micro_version], [0])
m4_define([project_version], project_major_version.project_minor_version.project_micro_version)

AC_PREREQ([2.64])

AC_INIT(project_name,project_version,project_email,project_name,project_url)

dnl http://xkcd.com/371/
m4_append([AM_SANITY_CHECK],[
AS_ECHO(["Build environment is grinning and holding a spatula. Guess not."])
])


dnl =================================
dnl ===  general Autotools Setup  ===
dnl =================================

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.13 foreign color-tests dist-xz subdir-objects -Wall -Werror])
AM_MAINTAINER_MODE([enable])

dnl =======================
dnl ===  early options  ===
dnl =======================

OPTMODE="-O2"

dnl >>> Build in debug mode

AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [Enable normal debug features (-g, -O0)]))


AS_IF([test "x$enable_debug" = "xyes"], [
    CFLAGS="$CFLAGS -ggdb3"
    LDFLAGS="$LDFLAGS -ggdb3"
    OPTMODE="-O0"
],[
    enable_debug="no"
])

AM_CONDITIONAL([BUILD_DEBUG], [test "x$enable_debug" = "xyes"])

AH_TEMPLATE([DEBUG_BUILD], [Defined to 1 when --enable-debug is active])
AS_IF([test "x$enable_debug" = "xyes"],[AC_DEFINE([DEBUG_BUILD],[1])])


dnl >>> Build web targets instead of an executable binary

AC_ARG_ENABLE([web],
    AS_HELP_STRING([--enable-web], [Build html/js/wasm for web (run configure with emconfigure!)]))

AS_IF([test "x$enable_web" = "xyes"], [
    AS_IF([test "x$enable_debug" = "xyes"], [
        OPTMODE="-O0"
    ],[
        OPTMODE="-Os"
    ])
],[
    enable_web="no"
])

AM_CONDITIONAL([BUILD_WEB], [test "x$enable_web" = "xyes"])

AH_TEMPLATE([WEB_BUILD], [Defined to 1 when --enable-web is active])
AS_IF([test "x$enable_web" = "xyes"],[AC_DEFINE([WEB_BUILD],[1])])

CFLAGS="$CFLAGS $OPTMODE"

dnl >>> Use a locally installed version of chipmunk instead of
dnl     the provided version in external/chipmunk

AC_ARG_ENABLE([chipmunk],
    AS_HELP_STRING([--disable-chipmunk], [Disable the Chipmunk physics engine)]))

AS_IF([test "x$enable_chipmunk" != "xno"], [
    enable_chipmunk="yes"
],[
    enable_chipmunk="no"
])

AM_CONDITIONAL([USE_CHIPMUNK], [test "x$enable_chipmunk" != "xno"])

AH_TEMPLATE([USE_CHIPMUNK], [Defined to 1 if chipmunk is enabled])
AS_IF([test "x$enable_chipmunk" != "xno"],[AC_DEFINE([USE_CHIPMUNK],[1])])

dnl >>> any physics engine
AC_MSG_CHECKING([if a physics engine is available])
AS_IF([test "x$enable_chipmunk" != "xno"], [
    enable_physics="yes"
    AC_MSG_RESULT([chipmunk])
],[
    enable_physics="no"
    AC_MSG_WARN([No physics engine present - disabling physics features])
])

AM_CONDITIONAL([USE_PHYSICS], [test "x$enable_physics" = "xyes"])

AH_TEMPLATE([USE_PHYSICS], [Defined to 1 if a physics engine is available])
AS_IF([test "x$enable_physics" = "xyes"],[AC_DEFINE([USE_PHYSICS],[1])])


dnl >>> Use a locally installed version of raylib instead of
dnl     the provided version in external/raylib

AC_ARG_ENABLE([system-raylib],
    AS_HELP_STRING([--enable-system-raylib], [Use a locally installed version of raylib instead of the version iincluded with project_name)]))

AS_IF([test "x$enable_system_raylib" = "xyes"], [
],[
    enable_system_raylib="no"
])

AM_CONDITIONAL([USE_SYSTEM_RAYLIB], [test "x$enable_system_raylib" = "xyes"])


dnl ========================================
dnl ===  Compiler/Language Dependencies  ===
dnl ========================================

AC_PROG_CC
AM_PROG_CC_C_O

AC_USE_SYSTEM_EXTENSIONS

AX_CHECK_STDC

AM_PROG_AR
AC_PROG_RANLIB


dnl ===============================================================
dnl ===  Export the project configuration into config.h macros  ===
dnl ==============================================================-

dnl push this all into the headers
AH_TEMPLATE([PACKAGE_AUTHOR],
        [Author(s) of the project, and holder of the copyright])
AH_TEMPLATE([PACKAGE_COPYRIGHT_YEAR],
        [The year(s) that the project's copyright was established])
AH_TEMPLATE([PACKAGE_COPYRIGHT_STRING],
        [Official string to use when printing the project's copyright])
AH_TEMPLATE([PACKAGE_LICENCE],
        [Short name or abbreviation for the project's licencse])
AH_TEMPLATE([PACKAGE_LICENCE_DESC],
        [Full, descriptive name of the project's licencse])
AC_DEFINE([PACKAGE_AUTHOR],           "project_author")
AC_DEFINE([PACKAGE_COPYRIGHT_YEAR],   "project_copyright_year")
AC_DEFINE([PACKAGE_COPYRIGHT_STRING], "Copyright (c) project_copyright_year project_author")
AC_DEFINE([PACKAGE_LICENCE],          "project_licence")
AC_DEFINE([PACKAGE_LICENCE_DESC],     "project_licence_desc")


dnl ====================================
dnl ===  General Requirement Checks  ===
dnl ====================================

AC_DEFINE([_GNU_SOURCE], [], [We want GNU extensions])

AC_DEFUN([ERR_NOT_FOUND],
	[AC_MSG_ERROR([$1 not found], [1])])

dnl Checks for compiler features
AX_C___ATTRIBUTE__
AC_C_CONST

dnl Checks for header files.
AC_CHECK_HEADERS([ctype.h], [], [ERR_NOT_FOUND([ctype.h])])
AC_CHECK_HEADERS([dirent.h], [], [ERR_NOT_FOUND([dirent.h])])
AC_CHECK_HEADERS([libgen.h], [], [ERR_NOT_FOUND([libgen.h])])
AC_CHECK_HEADERS([limits.h], [], [ERR_NOT_FOUND([limits.h])])
AC_CHECK_HEADERS([math.h], [], [ERR_NOT_FOUND([math.h])])
AC_CHECK_HEADERS([stdint.h], [], [ERR_NOT_FOUND([stdint.h])])
AC_CHECK_HEADERS([stdlib.h], [], [ERR_NOT_FOUND([stdlib.h])])
AC_CHECK_HEADERS([string.h], [], [ERR_NOT_FOUND([string.h])])
AC_CHECK_HEADERS([unistd.h], [], [ERR_NOT_FOUND([unistd.h])])
AC_CHECK_HEADERS([arpa/inet.h], [], [ERR_NOT_FOUND([arpa/inet/h.h])])
AC_HEADER_ASSERT
AC_HEADER_DIRENT

dnl Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T

dnl Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNC([asprintf], [], [ERR_NOT_FOUND([asprintf])])
AC_CHECK_FUNC([getline], [], [ERR_NOT_FOUND([getline])])
AC_CHECK_FUNC([isdigit], [], [ERR_NOT_FOUND([isdigit])])
AC_CHECK_FUNC([vasprintf], [], [ERR_NOT_FOUND([vasprintf])])
AC_CHECK_FUNC([scandir], [], [ERR_NOT_FOUND([scandir])])
AC_CHECK_FUNC([strcspn], [], [ERR_NOT_FOUND([strcspn])])
AC_CHECK_FUNC([strpbrk], [], [ERR_NOT_FOUND([strpbrk])])
AC_CHECK_FUNC([strsep], [], [ERR_NOT_FOUND([strsep])])
AX_FUNC_GETOPT_LONG

dnl Checks for other declarations
dnl AC_CHECK_DECLS([])

dnl Export any extra build options
APP_CFLAGS="-I\$(top_srcdir)/external"
APP_LIBS=""

dnl =================================
dnl ===  pkg-config dependencies  ===
dnl =================================

ALL_PKG_CFLAGS=""
ALL_PKG_LIBS=""

AS_IF([test "x$enable_web" != "xyes"], [
    PKG_CHECK_MODULES([GL],     [gl     >= 10.0.4])

    ALL_PKG_CFLAGS="$ALL_PKG_CFLAGS $GL_CFLAGS"
    ALL_PKG_LIBS="$ALL_PKG_LIBS $GL_LIBS"
])

AS_IF([test "x$enable_system_raylib" = "xyes"], [
    PKG_CHECK_MODULES([RAYLIB], [raylib >=  5.0.0])

    ALL_PKG_CFLAGS="$ALL_PKG_CFLAGS $RAYLIB_CFLAGS"
    ALL_PKG_LIBS="$ALL_PKG_LIBS $RAYLIB_LIBS"
],[
    APP_CFLAGS="$APP_CFLAGS -I\$(top_srcdir)/external/raylib"
    APP_LIBS="$APP_LIBS -L\$(top_srcdir)/external/raylib/src -ldl -lpthread"
])


dnl ================
dnl ===  Output  ===
dnl ================

AS_IF([test "x$enable_web" = "xyes"], [
    EXEEXT=".html"
    AC_SUBST(EXEEXT)
])

AC_SUBST(APP_CFLAGS)
AC_SUBST(APP_LIBS)
AC_SUBST(ALL_PKG_CFLAGS)
AC_SUBST(ALL_PKG_LIBS)

AC_MSG_NOTICE([])
AC_MSG_NOTICE([----------------------])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([        DEBUG = ${enable_debug}])
AC_MSG_NOTICE([          WEB = ${enable_web}])
AC_MSG_NOTICE([     CHIPMUNK = ${enable_chipmunk}])
AC_MSG_NOTICE([      PHYSICS = ${enable_physics}])
AC_MSG_NOTICE([SYSTEM_RAYLIB = ${enable_system_raylib}])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([      AR = ${AR}])
AC_MSG_NOTICE([     CPP = ${CPP}])
AC_MSG_NOTICE([      CC = ${CC}])
AC_MSG_NOTICE([  CFLAGS = ${APP_CFLAGS} ${ALL_PKG_CFLAGS}])
AC_MSG_NOTICE([    LIBS = ${APP_LIBS} ${ALL_PKG_LIBS}])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([----------------------])
AC_MSG_NOTICE([])

AC_CONFIG_FILES([
        external/chipmunk/Makefile
        Makefile
])

AC_OUTPUT

