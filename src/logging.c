/******************************************************************************
 *                                                                            *
 *  logging.c                                                                 *
 *                                                                            *
 *  This file is part of hexpuzzle.                                           *
 *                                                                            *
 *  hexpuzzle is free software: you can redistribute it and/or                *
 *  modify it under the terms of the GNU General Public License as published  *
 *  by the Free Software Foundation, either version 3 of the License,         *
 *  or (at your option) any later version.                                    *
 *                                                                            *
 *  hexpuzzle is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General  *
 *  Public License for more details.                                          *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License along   *
 *  with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.              *
 *                                                                            *
 ******************************************************************************/

#include "common.h"

#include <stdarg.h>

#include "logging.h"

int
infoprintf(
    const char *fmt,
    ...
) {
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vfprintf(VINFOMSG_IO_STREAM, fmt, ap);
    va_end(ap);

    return rv;
}

void
fvmsg_prefix(
    FILE *fh,
    const char *prefix
) {
    assert_not_null(fh);
    assert_not_null(prefix);

    fprintf(fh, "%s: %s", progname, prefix);    
}

void
vmsg_prefix(
    const char *prefix
) {
    fvmsg_prefix(VMSG_IO_STREAM, prefix);
}

void
infomsg_prefix(
    const char *prefix
) {
    fvmsg_prefix(VINFOMSG_IO_STREAM, prefix);
}


/****************************
 * generic vararg messaging *
 ****************************/

int
fvmsg(
    FILE *fh,
    const char *prefix,
    const char *fmt,
    va_list     ap
) {
    int rv = 0;

    fvmsg_prefix(fh, prefix);
    rv = vfprintf(fh, fmt, ap);
    fprintf(fh, "\n");
    fflush(fh);

    return rv;
}

int
vmsg(
    const char *prefix,
    const char *fmt,
    va_list     ap
) {
    return fvmsg(VMSG_IO_STREAM, prefix, fmt, ap);
}

int
fvmsg_with_location(
    FILE *fh,
    const char *loc_file,
    int         loc_line,
    const char *prefix,
    const char *fmt,
    va_list     ap
) {
    fprintf(fh, "[%s:%d] ", loc_file, loc_line);
    return fvmsg(fh, prefix, fmt, ap);
}

int
vmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *prefix,
    const char *fmt,
    va_list     ap
) {
    return fvmsg_with_location(VMSG_IO_STREAM, loc_file, loc_line, prefix, fmt, ap);
}


/********************************
 * message-typed vararg methods *
 ********************************/

int
vinfomsg(
    const char *fmt,
    va_list     ap
) {
    return vmsg("", fmt, ap);
}

int
vwarnmsg(
    const char *fmt,
    va_list     ap
) {
    return vmsg("WARNING - ", fmt, ap);
}

int
verrmsg(
    const char *fmt,
    va_list     ap
) {
    return vmsg("ERROR - ", fmt, ap);
}

int
vinfomsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    va_list     ap
) {
    return vmsg_with_location(loc_file, loc_line, "", fmt, ap);
}

int
vwarnmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    va_list     ap
) {
    return vmsg_with_location(loc_file, loc_line, "WARNING - ", fmt, ap);
}

int
verrmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    va_list     ap
) {
    return vmsg_with_location(loc_file, loc_line, "ERROR - ", fmt, ap);
}


/****************************************************
 * standard printf(3) style message-typed functions *
 ****************************************************/

int
infomsg(
    const char *fmt,
    ...
) {
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vinfomsg(fmt, ap);
    va_end(ap);

    return rv;
}

int
warnmsg(
    const char *fmt,
    ...
) {
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vwarnmsg(fmt, ap);
    va_end(ap);

    return rv;
}

int
errmsg(
    const char *fmt,
    ...
) {
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = verrmsg(fmt, ap);
    va_end(ap);

    return rv;
}

int
infomsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    ...
) {
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vwarnmsg_with_location(loc_file, loc_line, fmt, ap);
    va_end(ap);

    return rv;
}

int
warnmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    ...
) {
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vwarnmsg_with_location(loc_file, loc_line, fmt, ap);
    va_end(ap);

    return rv;
}

int
errmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    ...
) {
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = verrmsg_with_location(loc_file, loc_line, fmt, ap);
    va_end(ap);

    return rv;
}
