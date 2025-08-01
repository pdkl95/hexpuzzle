/******************************************************************************
 *                                                                            *
 *  util.h                                                                    *
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

#ifndef UTIL_H
#define UTIL_H

#if defined(PLATFORM_DESKTOP)
#include <libgen.h>
#endif

#ifndef strdupa
# define strdupa(s)                                         \
    (__extension__                                          \
     ({                                                     \
         const char *__old = (s);                           \
         size_t __len = strlen (__old) + 1;                 \
         char *__new = (char *) __builtin_alloca (__len);   \
         (char *) memcpy (__new, __old, __len);             \
     }))
#endif

char last_char(char *str);
#if defined(PLATFORM_DESKTOP)
bool file_exists(const char *file);
int mkdir_p(const char *dir, mode_t mode);
#endif

static inline bool is_dir_separator(char c)
{
#if defined(_WIN32)
    return (c == '\\');
#else
    return (c == '/');
#endif
}

const char *directory_without_end_separator(const char *path);
const char *concat_dir_and_filename(const char *dir, const char *filename);
const char *concat_dir_and_filename_and_ext(const char *dir, const char *filename, const char *ext);
const char *filename_ext(const char *filename);
char *strcat_alloc(const char *first, const char *second);

char *strdup_xxd_include(unsigned char *buf, unsigned int len);

const char *generate_unique_id(void);

static inline const char *booltext(bool value)
{
    return value ? "true" : "false";
}

bool is_number(const char *str);

double normal_rng(void);

float slew_limit(float current, float target, float step);
float slew_limit_up(float current, float target, float step);
float slew_limit_down(float current, float target, float step);

/********************
 * easing functions *
 ********************/

#define ease_quartic_in(t)  ((t) * (t) * (t) * (t))
#define ease_quintic_in(t)  ((t) * (t) * (t) * (t) * (t))

float ease_sine_in(float t);
float ease_sine_out(float t);
float ease_circular_in(float t);
float ease_circular_out(float t);
float ease_quad_in(float t);
float ease_quad_out(float t);
float ease_cuhic_in(float t);
float ease_cubic_out(float t);
float ease_quint_in(float t);
float ease_quint_out(float t);
float ease_exponential_in(float t);
float ease_exponential_out(float t);
float ease_back_in(float t);
float ease_back_out(float t);
float ease_bounce_in(float t);
float ease_bounce_out(float t);

float ease_quartic_inout(float t);

static inline float clamp(float x, float lowerlimit, float upperlimit) {
  if (x < lowerlimit) return lowerlimit;
  if (x > upperlimit) return upperlimit;
  return x;
}

static inline float saturate(float x)
{
    return clamp(x, 0.0f, 1.0f);
}

float smoothstep(float edge0, float edge1, float x);
float smootherstep(float edge0, float edge1, float x);
float exp_sustained_impulse(float x, float f, float k);

/****************
 * fatal errors *
 ****************/

void halt_and_catch_fire(int rc) NORETURN;
void hcf(void) NORETURN;
void exit_success(void) NORETURN;


void
die(
    const char *fmt,
    ...) NORETURN;

void
die_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    ...) NORETURN;

#define WARN(msg)                               \
    warnmsg_with_location(__FILE__, __LINE__,   \
                         "WARNING: %s",         \
                         (msg));

#define DIE(errmsg)                             \
    die_with_location(__FILE__, __LINE__,       \
                      "FATAL ERROR: %s",        \
                      (errmsg));


int
asprintf_or_die(
    const char  *loc_file,
    int          loc_line,
    char **strp,
    const char  *fmt,
    ...);

int
vasprintf_or_die(
    const char *loc_file,
    int loc_line,
    char **strp,
    const char *fmt,
    va_list ap);

#define safe_asprintf(strp, fmt, ...)                           \
    asprintf_or_die(__FILE__, __LINE__, strp, fmt, __VA_ARGS__)

#define safe_vasprintf(strp, fmt, ap)                   \
    vasprintf_or_die(__FILE__, __LINE__, strp, fmt, ap)


#endif /*UTIL_H*/
