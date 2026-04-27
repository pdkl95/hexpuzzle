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

#include <ctype.h>

#ifdef DEBUG_BUILD
# include <signal.h>
#endif

#include "logging.h"
#include "ansi_colors.h"
#include "options.h"

bool color_logs = false;

const char *color_reset = "";
const char *color_separator = "";
const char *color_progname = "";
const char *color_warn_prefix = "";
const char *color_err_prefix = "";

const char *color_raylib_prefix = "";
int raylib_log_level = LOG_WARNING;

struct ignore_gl_debug_message {
    int count;
    int capacity;
    GLuint *list;
};
struct ignore_gl_debug_message ignore_dbg_msg = {0};

#ifdef DEBUG_BUILD
bool break_on_gl_error = false;
#endif /*DEBUG_BUILD*/

void use_color_logs(void)
{
    color_logs = true;

    color_reset            = ANSI_COLOR_COLOR_RESET;
    color_separator        = ANSI_COLOR_WHITE;
    color_progname         = ANSI_COLOR_GREEN;
    color_warn_prefix      = ANSI_COLOR_UNDERLINE_HIGH_YELLOW;
    color_err_prefix       = ANSI_COLOR_BOLD_HIGH_YELLOW ANSI_COLOR_RED_HIGH_BG;
    color_raylib_prefix    = ANSI_COLOR_HIGH_GREEN;
}

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
fvmsg_prefix_color(
    FILE *fh,
    const char *prefix,
    const char *prefix_color
) {
    assert_not_null(fh);

    if (prefix) {
        fprintf(fh, "%s%s%s:%s %s%s%s %s-%s ",
                color_progname,
                progname,
                color_separator,
                color_reset,
                prefix_color ? prefix_color : "",
                prefix,
                color_reset,
                color_separator,
                color_reset);
    } else {
        fprintf(fh, "%s%s%s:%s ",
                color_progname,
                progname,
                color_separator,
                color_reset);
    }
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
fvmsg_color(
    FILE *fh,
    const char *prefix,
    const char *prefix_color,
    const char *fmt,
    va_list     ap
) {
    int rv = 0;

    fvmsg_prefix_color(fh, prefix, prefix_color);
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
vmsg_color(
    const char *prefix,
    const char *prefix_color,
    const char *fmt,
    va_list     ap
) {
    return fvmsg_color(VMSG_IO_STREAM, prefix, prefix_color, fmt, ap);
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
    return vmsg_color(NULL, NULL, fmt, ap);
}

int
vwarnmsg(
    const char *fmt,
    va_list     ap
) {
    return vmsg_color("WARNING",
                      color_warn_prefix,
                      fmt,
                      ap);
}

int
verrmsg(
    const char *fmt,
    va_list     ap
) {
    return vmsg_color("ERROR",
                      color_err_prefix,
                      fmt,
                      ap);
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



/**********
 * Raylib *
 **********/

static const char *raylib_log_type_string(int logLevel)
{
    switch (logLevel) {
    case LOG_TRACE:   return "TRACE";
    case LOG_DEBUG:   return "DEBUG";
    case LOG_INFO:    return "INFO";
    case LOG_WARNING: return "WARNING";
    case LOG_ERROR:   return "ERROR";
    case LOG_FATAL:   return "FATAL";
    default:
        assert_never_reached();
        return "(NULL)";
    }
}

static const char *raylib_section_color(const char *section)
{
#define sec(name, color) do {                   \
        if (0 == strcmp(#name, section)) {      \
            return color;                       \
        }                                       \
    } while(0)

    sec(SHADER,   ANSI_COLOR_HIGH_MAGENTA);
    sec(TEXTURE,  ANSI_COLOR_HIGH_BLUE);
    sec(FBO,      ANSI_COLOR_HIGH_GREEN);
    sec(FONT,     ANSI_COLOR_BOLD_GREEN);
    sec(FILEIO,   ANSI_COLOR_BOLD_HIGH_BLACK);
    sec(SYSTEM,   ANSI_COLOR_HIGH_WHITE);
    sec(RLGL,     ANSI_COLOR_HIGH_CYAN);
    sec(GLDEBUG,  ANSI_COLOR_BOLD_CYAN);
    sec(GL,       ANSI_COLOR_CYAN);
    sec(GLAD,     ANSI_COLOR_CYAN);
    //sec(PLATFORM, ANSI_COLOR_BLUE);
    //sec(DISPLAY,  ANSI_COLOR_BLUE);

#undef sec

    return ANSI_COLOR_BLUE;
}

const char *raylib_log_level_color(int logLevel)
{
    switch (logLevel) {
    case LOG_TRACE:   return ANSI_COLOR_BOLD_BLUE;
    case LOG_DEBUG:   return ANSI_COLOR_BOLD_CYAN;
    case LOG_INFO:    return ANSI_COLOR_BOLD_GREEN;
    case LOG_WARNING: return ANSI_COLOR_BOLD_YELLOW;
    case LOG_ERROR:   return ANSI_COLOR_BOLD_RED;
    case LOG_FATAL:   return ANSI_COLOR_BOLD_HIGH_RED;
    default:
        assert_never_reached();
        return "";
    }
}

void ignore_gl_debug_message(GLuint id)
{
    int old_count = ignore_dbg_msg.count;
    ignore_dbg_msg.count++;

    int old_capacity = ignore_dbg_msg.capacity;
    if (ignore_dbg_msg.capacity < 1) {
        ignore_dbg_msg.capacity = 4;
    }

    while (ignore_dbg_msg.count >= ignore_dbg_msg.capacity) {
        ignore_dbg_msg.capacity *= 2;
    }

    if (old_capacity != ignore_dbg_msg.capacity) {
        ignore_dbg_msg.list = realloc(ignore_dbg_msg.list,
                                      ignore_dbg_msg.capacity * sizeof(int));
    }

    ignore_dbg_msg.list[old_count] = id;

#if 0
    printf("Ignoring GL debug messsage type %u\n", id);
    printf("    (count=%d, capacity=%d)\n",
           ignore_dbg_msg.count,
           ignore_dbg_msg.capacity);
#endif
}

#ifdef DEBUG_BUILD
static void APIENTRY gl_debug_message_cb(GLenum source, GLenum type, GLuint id, GLenum severity, UNUSED GLsizei length, const GLchar* message, UNUSED void* userParam)
{

    if ((id == 131169) || (id == 131185) || (id == 131218) || (id == 131204)) {
        return;
    }

    for (int i=0; i<ignore_dbg_msg.count; i++) {
        if (id == ignore_dbg_msg.list[i]) {
            return;
        }
    }

    const char *msgSource = NULL;
    const char *source_color = ANSI_COLOR_UNDERLINE_HIGH_WHITE;
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        msgSource = "API";
        source_color = ANSI_COLOR_BOLD_CYAN;
        break;

    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        msgSource = "WINDOW_SYSTEM";
        source_color = ANSI_COLOR_BOLD_MAGENTA;
        break;

    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        msgSource = "SHADER_COMPILER";
        source_color = ANSI_COLOR_BOLD_YELLOW;
        break;

    case GL_DEBUG_SOURCE_THIRD_PARTY:
        msgSource = "THIRD_PARTY";
        source_color = ANSI_COLOR_BOLD_BLUE;
        break;

    case GL_DEBUG_SOURCE_APPLICATION:
        msgSource = "APPLICATION";
        source_color = ANSI_COLOR_BOLD_RED;
        break;

    case GL_DEBUG_SOURCE_OTHER:
        msgSource = "OTHER";
        source_color = ANSI_COLOR_BOLD_WHITE;
        break;

    default:
        break;
    }

    const char *msgType = NULL;
    const char *type_color = ANSI_COLOR_UNDERLINE_HIGH_WHITE;
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        msgType = "ERROR";
        type_color = ANSI_COLOR_HIGH_RED;
        break;

    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        msgType = "DEPRECATED_BEHAVIOR";
        type_color = ANSI_COLOR_HIGH_YELLOW;
        break;

    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        msgType = "UNDEFINED_BEHAVIOR";
        type_color = ANSI_COLOR_HIGH_RED;
        break;

    case GL_DEBUG_TYPE_PORTABILITY:
        msgType = "PORTABILITY";
        type_color = ANSI_COLOR_HIGH_YELLOW;
        break;

    case GL_DEBUG_TYPE_PERFORMANCE:
        msgType = "PERFORMANCE";
        type_color = ANSI_COLOR_HIGH_CYAN;
        break;

    case GL_DEBUG_TYPE_MARKER:
        msgType = "MARKER";
        type_color = ANSI_COLOR_HIGH_BLUE;
        break;

    case GL_DEBUG_TYPE_PUSH_GROUP:
        msgType = "PUSH_GROUP";
        type_color = ANSI_COLOR_HIGH_GREEN;
        break;

    case GL_DEBUG_TYPE_POP_GROUP:
        msgType = "POP_GROUP";
        type_color = ANSI_COLOR_HIGH_GREEN;
        break;

    case GL_DEBUG_TYPE_OTHER:
        msgType = "OTHER";
        type_color = ANSI_COLOR_HIGH_WHITE;
        break;

    default:
        break;
    }

    const char *msgSeverity = "DEFAULT";
    const char *severity_color = ANSI_COLOR_UNDERLINE_HIGH_WHITE;
    switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
        msgSeverity = "LOW";
        severity_color = ANSI_COLOR_HIGH_GREEN;
        break;

    case GL_DEBUG_SEVERITY_MEDIUM:
        msgSeverity = "MEDIUM";
        severity_color = ANSI_COLOR_HIGH_YELLOW;
        break;

    case GL_DEBUG_SEVERITY_HIGH:
        msgSeverity = "HIGH";
        severity_color = ANSI_COLOR_HIGH_RED;
        break;

    case GL_DEBUG_SEVERITY_NOTIFICATION:
        msgSeverity = "NOTIFICATION";
        severity_color = ANSI_COLOR_HIGH_BLUE;
        break;

    default:
        break;
    }

    TRACELOG(LOG_WARNING, "GL: %sOpenGL debug message%s (type%s=%s%s%s source%s=%s%s%s severity%s=%s%s%s id%s=%s%d%s)%s:%s %s",
             ANSI_COLOR_UNDERLINE_WHITE,
             color_reset,

             color_separator,
             type_color,
             msgType,
             color_reset,

             color_separator,
             source_color,
             msgSource,
             color_reset,

             color_separator,
             severity_color,
             msgSeverity,
             color_reset,

             color_separator,
             ANSI_COLOR_BOLD_HIGH_WHITE,
             id,
             color_reset,

             color_separator,
             color_reset,

             message);

    if (break_on_gl_error) {
        if ((severity == GL_DEBUG_SEVERITY_MEDIUM) ||
            (severity == GL_DEBUG_SEVERITY_HIGH)) {
            if ((type == GL_DEBUG_TYPE_ERROR) ||
                (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) ||
                (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)) {
                fprintf(VMSG_IO_STREAM, "\nDetected GL error - raising SIGINT\n");
                raise(SIGINT);
            }
        }
    }
}

void use_gl_debug_callback(void)
{
    glDebugMessageCallback(gl_debug_message_cb, 0);
}
#endif /*DEBUG_BUILD*/

void raylib_trace_log_cb(int logLevel, const char *text, va_list args)
{
    if (logLevel < raylib_log_level) {
        return;
    }

    const char *type = raylib_log_type_string(logLevel);

    if (color_logs) {
        char *sep = strstr(text, ": ");

        const char *level_color = raylib_log_level_color(logLevel);

        fvmsg_prefix_color(VMSG_IO_STREAM, NULL, NULL);
        fprintf(VMSG_IO_STREAM,
                "%s%s%s:%s %s%s%s:%s ",

                color_raylib_prefix,
                "raylib",
                color_separator,
                color_reset,

                level_color,
                type,
                color_separator,
                color_reset);

        if (sep) {
            bool have_section = true;
            const char *p = text;
            while (p < sep) {
                if (!isupper(*p)) {
                    have_section = false;
                    break;
                }
                p++;
            }
            if (have_section) {
                char *fmt = sep + 2;
#define SECTION_BUFSIZE 64
                char section[SECTION_BUFSIZE];
                snprintf(section, SECTION_BUFSIZE, "%s", text);
                sep = strstr(section, ": ");
                sep[0] = '\0';
#undef SECTION_BUFSIZE

                const char *sec_color = raylib_section_color(section);

                fprintf(VMSG_IO_STREAM,
                        "%s%s%s:%s ",

                        sec_color,
                        section,
                        color_separator,
                        color_reset);

                vprintf(fmt, args);
            } else {
                vprintf(text, args);
            }

        } else {
            vprintf(text, args);
        }
    } else {
        printf("raylib: %s: ", type);
        vprintf(text, args);
    }

    putchar('\n');
    fflush(VMSG_IO_STREAM);

    if (logLevel == LOG_FATAL) {
        exit(EXIT_FAILURE);
    }
}

void log_show_version(void)
{
#define BUM_MAXLEN 2048
    char buf[BUM_MAXLEN];

    snprintf(buf, BUM_MAXLEN,
             "%s%s%s:%s ",
             color_progname,
             progname,
             color_separator,
             color_reset);

    show_version(buf);

#undef BUM_MAXLEN
}
