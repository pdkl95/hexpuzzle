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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

#include "util.h"

char last_char(char *str)
{
    int idx = strlen(str);
    if (idx < 1) {
        return '\0';
    }

    return str[idx - 1];
}

#if defined(PLATFORM_DESKTOP)
bool file_exists(const char *file)
{
    struct stat sb;
    return (stat(file, &sb) == 0);
}

int mkdir_p(const char *dir, mode_t mode)
{
	struct stat sb;

	if (!dir) {
		errno = EINVAL;
		return -1;
	}

	if (!stat(dir, &sb))
		return 0;

	mkdir_p(dirname(strdupa(dir)), mode);

	return mkdir(dir, mode);
}
#endif

const char *directory_without_end_separator(const char *path)
{
    static char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, "%s", path);
    int last = strlen(buf) - 1;
    if (is_dir_separator(buf[last])) {
        buf[last] = '\0';
    }
    return buf;
}

const char *concat_dir_and_filename(const char *dir, const char *filename)
{
    assert_not_null(dir);
    assert_not_null(filename);

    static char buf[PATH_MAX];
    int dirlast = strlen(dir) - 1;
    if (is_dir_separator(dir[dirlast])) {
        snprintf(buf, PATH_MAX, "%s%s", dir, filename);
    } else {
        snprintf(buf, PATH_MAX, "%s/%s", dir, filename);
    }
    return buf;
}

const char *concat_dir_and_filename_and_ext(const char *dir, const char *filename, const char *ext)
{
    assert_not_null(dir);
    assert_not_null(filename);
    assert_not_null(ext);

    static char buf[PATH_MAX];
    int dirlast = strlen(dir) - 1;
    if (ext[0] == '.') {
        if (is_dir_separator(dir[dirlast])) {
            snprintf(buf, PATH_MAX, "%s%s%s", dir, filename, ext);
        } else {
            snprintf(buf, PATH_MAX, "%s/%s%s", dir, filename, ext);
        }
    } else {
        if (is_dir_separator(dir[dirlast])) {
            snprintf(buf, PATH_MAX, "%s%s.%s", dir, filename, ext);
        } else {
            snprintf(buf, PATH_MAX, "%s/%s.%s", dir, filename, ext);
        }
    }
    return buf;
}

const char *filename_ext(const char *filename)
{
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

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

char *strdup_xxd_include(unsigned char *buf, unsigned int len)
{
    char *str = malloc(len + 1);

    memcpy(str, buf, len);
    str[len] = '\0';

    return str;
}

static uint32_t rand32(void)
{
    return ((rand() & 0x3) << 30) | ((rand() & 0x7fff) << 15) | (rand() & 0x7fff);
}

const char *gen_unique_id(void)
{
    static char buf[37];
    snprintf(buf, 37, "%08x-%04x-%04x-%04x-%04x%08x",
             rand32(),
             rand32() & 0xffff,
             ((rand32() & 0x0fff) | 0x4000),
             (rand32() & 0x3fff) + 0x8000,
             rand32() & 0xffff, rand32());
    return buf;
}

bool is_number(const char *str)
{
    while (*str) {
        if (!isdigit(*str)) {
            return false;
        }
        str++;
    }
    return true;
}

double normal_rng(void)
{
	double a = ((double)(rand()))/((double)RAND_MAX);
	double b = ((double)(rand()))/((double)RAND_MAX);

	return  sqrt(-2.0 * log(a)) * cos(2 * M_PI * b);
}

float slew_limit(float current, float target, float step)
{
    if (current < target) {
        current += step;
    } else if (current > step) {
        current -= step;
    }

    if (fabsf(target - current) <= step) {
        return target;
    } else {
        return current;
    }
}

float slew_limit_up(float current, float target, float step)
{
    if (current > target) {
        return target;
    } else if (current < step) {
        current += step;
    }

    if (fabsf(target - current) <= step) {
        return target;
    } else {
        return current;
    }
}

float slew_limit_down(float current, float target, float step)
{
    if (current < target) {
        return target;
    } else if (current > step) {
        current -= step;
    }

    if (fabsf(target - current) <= step) {
        return target;
    } else {
        return current;
    }
}

float ease_circular_in(float t)
{
    return 1.0f - sqrtf(1.0f - (t * t));
}

float ease_circular_out(float t)
{
    return sqrtf(1.0f - powf(t - 1.0f, 2.0f));
}

float ease_sine_in(float t)
{
    return 1.0f - cosf((t * PI) / 2.0f);
}

float ease_sine_out(float t)
{
    return 1.0f - sinf((t * PI) / 2.0f);
}

float ease_quad_in(float t)
{
    return t * t;
}

float ease_quad_out(float t)
{
    return 1.0f - ((1.0 - t) * (1.0 - t));
}

float ease_cuhic_in(float t)
{
    return t * t * t;
}

float ease_cubic_out(float t)
{
    return 1.0f - powf(1.0f - t, 3.0f);
}

float ease_quint_in(float t) {
    return t * t * t * t * t;
}

float ease_quint_out(float t) {
    return 1.0f - powf(1 - t, 5);
}

float ease_exponential_in(float t) {
    if (t == 0.0f) {
        return t;
    } else {
        return powf(2.0f, 10.0f * (t - 1.0f));
    }
}

float ease_exponential_out(float t) {
    if (t == 1.0f) {
        return t;
    } else {
        return 1.0f - powf(2.0f, -10.0f * t);
    }
}

#define EASE_BACK_C1 1.70158f
#define EASE_BACK_C3 (EASE_BACK_C1 + 1.0f)

float ease_back_in(float t) {
    return 1.0f + EASE_BACK_C3 * powf(t - 1.0f, 3.0f) + EASE_BACK_C1 * powf(t - 1.0f, 2.0f);
}

float ease_back_out(float t) {
    return EASE_BACK_C3 * t * t * t - EASE_BACK_C1 * t * t;
}

float ease_bounce_in(float t)
{
    return 1.0f - ease_bounce_out(1.0 - t);
}

float ease_bounce_out(float t)
{
    /* const float n1 = 7.5625f; */
    /* const float d1 = 2.75f; */

    /* if (t < 1.0f / d1) { */
    /*     return n1 * t * t; */
    /* } else if (t < 2.0f / d1) { */
    /*     return n1 * (t -= 1.5f   / d1) * t + 0.75f; */
    /* } else if (t < 2.5f / d1) { */
    /*     return n1 * (t -= 2.25f  / d1) * t + 0.9375f; */
    /* } else { */
    /*     return n1 * (t -= 2.625f / d1) * t + 0.984375f; */
    /* } */

    if (t < 4.0f/11.0f) {
        return (121.0f * t * t) / 16.0f;
    } else if (t < 8.0f / 11.0f) {
        return ((363.0f / 40.0f) * t * t) - ((99.0f / 10.0f) * t) + (17.0f / 5.0f);
    } else if (t < (9.0f / 10.0f)) {
        return ((4356.0f / 361.0f) * t * t) - ((35442.0f / 1805.0f) * t) + (16061.0f / 1805.0f);
    } else {
        return ((54.0f / 5.0f) * t * t) - ((513.0f / 25.0f) * t) + (268.0f / 25.0f);
    }
}

float ease_quartic_inout(float t)
{
    if (t < 0.5f) {
		return 8.0f * t * t * t * t;
	} else {
		float f = t - 1.0f;
		return -8.0f * f * f * f * f + 1.0f;
	}
}

float smoothstep(float edge0, float edge1, float x) {
    x = saturate((x - edge0) / (edge1 - edge0));
    return x * x * (3.0f - 2.0f * x);
}

float smootherstep(float edge0, float edge1, float x) {
  x = saturate((x - edge0) / (edge1 - edge0));

  return x * x * x * (x * (6.0f * x - 15.0f) + 10.0f);
}

float exp_sustained_impulse(float x, float f, float k)
{
    float s = fmaxf(x-f, 0.0f);
    return fminf((x * x)/(f * f), 1.0f + (2.0f / f) * s * exp(-k * s));
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
