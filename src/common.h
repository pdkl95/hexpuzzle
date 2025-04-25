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

#define MAXVAR(var, maxvalue) do { \
        var = MIN(var, maxvalue);  \
    } while(0)

#define MINVAR(var, minvalue) do { \
        var = MAX(var, minvalue);  \
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

#define NUM_ELEMENTS(type,arr) (sizeof(arr)/sizeof(type))

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

//#define RANDOM_GEN_DEBUG
//#define DEBUG_ID_AND_DIR
//#define DEBUG_MOUSE_INPUT_STATE

#include "const.h"
#include "raylib_helper.h"
#include "util.h"
#include "logging.h"

#ifdef DEBUG_ID_AND_DIR
extern int debug_id;
extern int debug_dir;
#endif

#define print_cjson(json) do {                \
        char *str = cJSON_Print(json);        \
        if (str) {                            \
            printf("JSON>>>\n%s\n<<<JSON\n",  \
                   str);                      \
            free(str);                        \
        }                                     \
    } while(0)

int global_rng_get(int bound);
bool global_rng_bool(int true_chances, int false_chances);
int global_rng_sign(int pos_chances, int neg_chances);

void disable_automatic_events(void);
void enable_automatic_events(void);

extern bool mouse_input_accepted;

static inline void enable_mouse_input(void)
{
#ifdef DEBUG_MOUSE_INPUT_STATE
    printf("mouse input: ENABLED\n");
#endif
    mouse_input_accepted = true;
}

static inline void disable_mouse_input(void)
{
#ifdef DEBUG_MOUSE_INPUT_STATE
    printf("mouse input: DISABLED\n");
#endif
    mouse_input_accepted = false;
}

static inline bool mouse_input_is_enabled(void)
{
#ifdef DEBUG_MOUSE_INPUT_STATE
    printf("mouse input TEST -> %s\n",
           mouse_input_accepted ? "ENABLED" : "DISABLED");
#endif
    return mouse_input_accepted;
}

void set_mouse_position(int new_x, int new_y);

extern bool do_postprocessing;

static inline void enable_postprocessing(void)
{
    do_postprocessing = true;
}

static inline void disable_postprocessing(void)
{
    do_postprocessing = false;
}


enum ui_result {
    UI_RESULT_PENDING = -1,
    UI_RESULT_NULL    = 0,
    UI_RESULT_CANCEL  = 1,
    UI_RESULT_OK      = 2
};
typedef enum ui_result ui_result_t;

extern ui_result_t modal_ui_result;

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

static inline IVector2 vector2_to_ivector2(Vector2 vec)
{
    IVector2 ivec = {
        .x = (int)vec.x,
        .y = (int)vec.y
    };
    return ivec;
}

extern IVector2 window_size;
extern Vector2 window_sizef;
extern Vector2 window_center;
extern float window_corner_dist;
extern Rectangle main_gui_area_rect;

extern IVector2 mouse_position;
extern Vector2 mouse_positionf;
extern bool mouse_left_click;
extern bool mouse_left_release;
extern bool mouse_right_click;
extern bool mouse_left_doubleclick;
extern int current_mouse_cursor;

static inline void set_mouse_cursor(int cursor)
{
    current_mouse_cursor = cursor;
}

extern float current_time;
extern double double_current_time;

extern Rectangle tooltip_bounds;
extern const char *tooltip_text;

static inline void tooltip(Rectangle bounds, const char *text)
{
    if (CheckCollisionPointRec(mouse_positionf, bounds)) {
        tooltip_bounds = bounds;
        tooltip_text   = text;
    }
}

#include "fonts.h"
#include "game_mode.h"

#endif /*COMMON_H*/
