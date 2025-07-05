/******************************************************************************
 *                                                                            *
 *  options.h                                                                 *
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "common.h"
#include "range.h"
#include "color.h"
//#include "path.h"
#include "startup_action.h"

#define OPTIONS_DEFAULT_VERBOSE false
#define OPTIONS_DEFAULT_WAIT_EVENTS false
#define OPTIONZ_DEFAULT_ALLOW_EDIT_MODE false
#define OPTIONS_DEFAULT_ANIMATE_BG  true
#define OPTIONS_DEFAULT_ANIMATE_WIN true
#define OPTIONS_DEFAULT_USE_PHYSICS true
#define OPTIONS_DEFAULT_USE_POSTPROCESSING true
#define OPTIONS_DEFAULT_USE_SOLVE_TIMER true
#define OPTIONS_DEFAULT_SHOW_LEVEL_PREVIEWS true
#define OPTIONS_DEFAULT_SHOW_TOOLTIPS true
#define OPTIONS_DEFAULT_LOG_FINISHED_LEVELS true
#define OPTIONS_DEFAULT_EXTRA_RAINBOWS false
#define OPTIONS_DEFAULT_FPS 30
#define OPTIONS_DEFAULT_MAX_FPS 30
#define OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH  800
#define OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT 800
#define OPTIONS_DEFAULT_CURSOR_SCALE 1
#define OPTIONS_DEFAULT_DOUBLE_CLICK_MS 250
#define OPTIONS_DEFAULT_MAX_WIN_RADIUS LEVEL_MIN_RADIUS
#define OPTIONS_DEFAULT_STARTUP_ACTION STARTUP_ACTION_NONE

#define OPTIONS_DEFAULT_CREATE_LEVEL_MODE CREATE_LEVEL_MODE_RANDOM
#define OPTIONS_DEFAULT_CREATE_LEVEL_RADIUS 2
#define OPTIONS_DEFAULT_CREATE_LEVEL_MIN_FIXED 2
#define OPTIONS_DEFAULT_CREATE_LEVEL_MAX_FIXED 4
#define OPTIONS_DEFAULT_CREATE_LEVEL_FIXED  ((int_range_t){ .min = OPTIONS_DEFAULT_CREATE_LEVEL_MIN_FIXED, .max = OPTIONS_DEFAULT_CREATE_LEVEL_MAX_FIXED })
#define OPTIONS_DEFAULT_CREATE_LEVEL_MIN_HIDDEN 1
#define OPTIONS_DEFAULT_CREATE_LEVEL_MAX_HIDDEN 4
#define OPTIONS_DEFAULT_CREATE_LEVEL_HIDDEN ((int_range_t){ .min = OPTIONS_DEFAULT_CREATE_LEVEL_MIN_HIDDEN, .max = OPTIONS_DEFAULT_CREATE_LEVEL_MAX_HIDDEN })
#define OPTIONS_DEFAULT_CREATE_LEVEL_EXPOINTS 3
#define OPTIONS_DFFAULT_CREATE_LEVEL_SYMMETRY_MODE SYMMETRY_MODE_REFLECT
#define OPTIONS_DFFAULT_CREATE_LEVEL_MINIMUM_PATH_DENSITY 250
#define OPTIONS_DFFAULT_CREATE_LEVEL_MINIMUM_PATH_DENSITY_FLOAT 2.5

#define OPTIONS_DEFAULT_PATH_COLOR_0 (Color){ 0, 0, 0, 0 }
#define OPTIONS_DEFAULT_PATH_COLOR_1 RED
#define OPTIONS_DEFAULT_PATH_COLOR_2 BLUE
#define OPTIONS_DEFAULT_PATH_COLOR_3 YELLOW
#define OPTIONS_DEFAULT_PATH_COLOR_4 GREEN
#define OPTIONS_DEFAULT_PATH_HL_COLOR_0 (Color){ 0, 0, 0, 0 }
#define OPTIONS_DEFAULT_PATH_HL_COLOR_1 (Color){ 255,  65,  81, 255 }
#define OPTIONS_DEFAULT_PATH_HL_COLOR_2 (Color){ 70,  166, 255, 255 }
#define OPTIONS_DEFAULT_PATH_HL_COLOR_3 (Color){ 255, 253, 127, 255 }
#define OPTIONS_DEFAULT_PATH_HL_COLOR_4 (Color){  67, 255, 105, 255 }

#define OPTIONS_WINDOW_MIN_WIDTH  OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH
#define OPTIONS_WINDOW_MIN_HEIGHT OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT
#define OPTIONS_WINDOW_MAX_WIDTH  (MAX(GetMonitorWidth(GetCurrentMonitor()),  OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH ))
#define OPTIONS_WINDOW_MAX_HEIGHT (MAX(GetMonitorHeight(GetCurrentMonitor()), OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT))

enum symmetry_mode {
    SYMMETRY_MODE_NONE = 0,
    SYMMETRY_MODE_REFLECT,
    SYMMETRY_MODE_ROTATE
};
typedef enum symmetry_mode symmetry_mode_t;

struct options {
    bool safe_mode;
    bool verbose;
    bool wait_events;

    color_option_t path_color[PATH_TYPE_COUNT];

    bool load_color_opt;

    bool allow_edit_mode;

    bool animate_bg;
    bool animate_win;
    bool use_physics;
    bool use_postprocessing;
    bool use_solve_timer;
    bool show_level_previews;
    bool show_tooltips;
    bool log_finished_levels;
    bool extra_rainbows;
    bool load_state_animate_bg;
    bool load_state_animate_win;
    bool load_state_use_physics;
    bool load_state_use_postprocessing;
    bool load_state_use_solve_timer;
    bool load_state_show_level_previews;
    bool load_state_show_tooltips;
    bool load_state_log_finished_levels;

    long max_fps;
    long initial_window_width;
    long initial_window_height;
    long cursor_scale;
    long double_click_ms;
    long max_win_radius;

    char *nvdata_dir;

    /* action */
    startup_action_t startup_action;

    /* action options */
    bool force;

    char *rng_seed_str;
    create_level_mode_t create_level_mode;
    long create_level_radius;
    int_range_t create_level_fixed;
    int_range_t create_level_hidden;
    long create_level_expoints;
    symmetry_mode_t create_level_symmetry_mode;
    int create_level_minimum_path_density;

    bool load_state_create_level_mode;
    bool load_state_create_level_radius;
    bool load_state_create_level_fixed_min;
    bool load_state_create_level_fixed_max;
    bool load_state_create_level_hidden_min;
    bool load_state_create_level_hidden_max;
    bool load_state_create_level_symmetry_mode;
    bool load_state_create_level_minimum_path_density;

    char *file_path;

    /* cheat (debug) options */
    bool cheat_autowin;
    bool cheat_solver;

    /* remaining args */
    char **extra_argv;
    int extra_argc;
};
typedef struct options options_t;

extern options_t *options;

options_t *create_options();
void destroy_options(options_t *options);

void options_set_defaults(options_t *options);
bool options_parse_args(options_t *options, int argc, char *argv[]);

extern options_t *options;

#endif /*OPTIONS_H*/
