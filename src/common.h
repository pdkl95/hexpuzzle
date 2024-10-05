/******************************************************************************
 *                                                                            *
 *  common.h                                                                  *
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


#ifndef COMMON_H
#define COMMON_H

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#define TRACELOG(level, ...) TraceLog(level, __VA_ARGS__)
#include "raylib.h"
#include "rlgl.h"

#include <math.h>
#define TAU (2.0 * M_PI)
#define TO_RADIANS(deg) ((deg) * (TAU/360.0))
#define TO_DEGREES(rad) ((rad) * (360.0/TAU))

#include "raymath.h"
#include "raygui/raygui.h"

#define QUOTE(x) #x
#define STR(macro) QUOTE(macro)

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define CLAMPVAR(var, minvalue, maxvalue) do { \
        var = MAX(var, minvalue);              \
        var = MIN(var, maxvalue);              \
    } while(0)

#define FREE(ptr) do { \
        free(ptr);     \
        ptr = NULL;    \
    } while(0)

#define SAFEFREE(ptr) do { \
        if (ptr) {         \
            FREE(ptr);     \
        }                  \
    } while(0)

extern const char *progname;
extern const char *progversion;

/***********************************************/
/***       error reporting & asserts         ***/

#define assert_null(x)     assert((x) == NULL)
#define assert_not_null(x) assert((x) != NULL)
#define assert_this(x)     assert_not_null(x)

#define assert_never_reached() do {                     \
        assert(false && "This shouldn't ever happen!"); \
        __builtin_unreachable();                        \
    } while(0)

#ifdef __GNUC__
# define NORETURN __attribute__ ((noreturn))
# define UNUSED   __attribute__ ((__unused__))
# define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#else
# define NORETURN
# define UNUSED
# define UNUSED_FUNCTION(x) UNUSED_ ##x
#endif

#include "const.h"
#include "raylib_helper.h"
#include "util.h"
#include "logging.h"

void disable_automatic_events(void);
void enable_automatic_events(void);

enum ui_result {
    UI_RESULT_PENDING = -1,
    UI_RESULT_NULL    = 0,
    UI_RESULT_CANCEL  = 1,
    UI_RESULT_OK      = 2
};
typedef enum ui_result ui_result_t;

typedef struct IVector2 {
    int x;
    int y;
} IVector2;

static inline Vector2 ivector2_to_vector2(IVector2 ivec)
{
    Vector2 vec = {
        .x = (float)ivec.x,
        .y = (float)ivec.y
    };
    return vec;
}

extern IVector2 window_size;

enum game_mode {
    GAME_MODE_NULL = 0,
    GAME_MODE_PLAY_COLLECTION,
    GAME_MODE_EDIT_COLLECTION,
    GAME_MODE_PLAY_LEVEL,
    GAME_MODE_EDIT_LEVEL,
    GAME_MODE_BROWSER,
    GAME_MODE_OPTIONS
};
typedef enum game_mode game_mode_t;

extern game_mode_t game_mode;

extern IVector2 mouse_position;
extern Vector2 mouse_positionf;

extern float current_time;
extern double double_current_time;

extern Font font20, font18, font16;
void set_gui_font20(void);
void set_gui_font18(void);
void set_gui_font16(void);
void set_default_gui_font(void);

#define MeasureGuiText(str)                  \
    MeasureTextEx(DEFAULT_GUI_FONT,          \
                  str,                       \
                  DEFAULT_GUI_FONT_SIZE,     \
                  DEFAULT_GUI_FONT_SPACING)

#define play_mode    (game_mode == GAME_MODE_PLAY_LEVEL)
#define edit_mode    (game_mode == GAME_MODE_EDIT_LEVEL)
#define browser_mode (game_mode == GAME_MODE_BROWSER)
#define options_mode (game_mode == GAME_MODE_OPTIONS)

#endif /*COMMON_H*/
