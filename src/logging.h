/******************************************************************************
 *                                                                            *
 *  logging.h                                                                 *
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

#ifndef LOGGING_H
#define LOGGING_H

#define VMSG_IO_STREAM stdout
#define VINFOMSG_IO_STREAM VMSG_IO_STREAM

void use_color_logs(void);

int infoprintf(const char *fmt, ...);

void fvmsg_prefix(FILE *fh, const char *prefix);
void vmsg_prefix(const char *prefix);
void infomsg_prefix(const char *prefix);

/****************************
 * generic vararg messaging *
 ****************************/

int fvmsg(
    FILE *fh,
    const char *prefix,
    const char *fmt,
    va_list ap);

int vmsg(
    const char *prefix,
    const char *fmt,
    va_list ap);

int fvmsg_with_location(
    FILE *fh,
    const char *loc_file,
    int         loc_line,
    const char *prefix,
    const char *fmt,
    va_list ap);

int vmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *prefix,
    const char *fmt,
    va_list ap);

/********************************
 * message-typed vararg methods *
 ********************************/

int vinfomsg(
    const char *fmt,
    va_list ap);

int vwarnmsg(
    const char *fmt,
    va_list ap);

int verrmsg(
    const char *fmt,
    va_list ap);

int vinfomsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    va_list ap);

int vwarnmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    va_list ap);

int verrmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    va_list ap);

/****************************************************
 * standard printf(3) style message-typed functions *
 ****************************************************/

int infomsg(
    const char *fmt,
    ...);

int warnmsg(
    const char *fmt,
    ...);

int errmsg(
    const char *fmt,
    ...);

int infomsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    ...);

int warnmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    ...);

int errmsg_with_location(
    const char *loc_file,
    int         loc_line,
    const char *fmt,
    ...);

#define fatal_errmsg(...) do { \
        errmsg(__VA_ARGS__);   \
        hcf();                 \
    } while(0)

extern int raylib_log_level;
void raylib_trace_log_cb(int logLevel, const char *text, va_list args);

#ifdef DEBUG_BUILD
void use_gl_debug_callback(void);
void ignore_gl_debug_message(GLuint id);

extern bool break_on_gl_error;
#endif

void log_show_version(void);

#endif /*LOGGING_H*/
