/******************************************************************************
 *                                                                            *
 *  util.c                                                                    *
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
#include <math.h>

#include "util.h"

char *strcat_alloc(const char *first, const char *second)
{
    size_t s1 = strlen(first);
    size_t s2 = strlen(second);

    size_t total_size = s1 + s2 + 1;
    if (total_size < s2 + 1) {
        return NULL;
    }

    char * ret = calloc(1, total_size);
    if(ret == NULL) {
        return NULL;
    }

    strcpy(ret, first);
    strcpy(ret + s1, second);
    return ret;
}

float
ease_circular_in(
    float t
) {
    return 1 - sqrtf(1 - (t * t));
}

float
ease_exponential_in(
    float t
) {
    if (t == 0.0) {
        return t;
    } else {
        return powf(2, 10 * (t - 1));
    }
}


void
halt_and_catch_fire(
    int rc
) {
#ifdef DEBUG_BUILD
    if (rc) {
        warnmsg("Exiting with code %d.", "rc");
    } else {
        warnmsg("Exiting successfully!");
    }
#endif

    exit(rc);
}

void
hcf(
    void
) {
    halt_and_catch_fire(EXIT_FAILURE);
}

void
exit_success(
    void
) {
    halt_and_catch_fire(EXIT_SUCCESS);
}

void
die(
    const char *fmt,
    ...
) {
    va_list ap;

    va_start(ap, fmt);
    verrmsg(fmt, ap);
    va_end(ap);

    hcf();
}

void
die_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    ...
) {
    va_list ap;

    va_start(ap, fmt);
    verrmsg_with_location(loc_file, loc_line, fmt, ap);
    va_end(ap);

    hcf();
}

int
vasprintf_or_die(
    const char       *loc_file,
    int         loc_line,
    char      **strp,
    const char *fmt,
    va_list     ap
) {
    int rv = vasprintf(strp, fmt, ap);
    if (-1 == rv) {
        die_with_location(loc_file, loc_line,
                          "vasprintf failed to format  \"%s\"",
                          fmt);
    }
    return rv;
}

int
asprintf_or_die(
    const char       *loc_file,
    int         loc_line,
    char      **strp,
    const char *fmt,
    ...
) {
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vasprintf_or_die(loc_file, loc_line, strp, fmt, ap);
    va_end(ap);

    return rv;
}
