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
#include "startup_action.h"

#define OPTIONS_DEFAULT_VERBOSE false
#define OPTIONS_DEFAULT_WAIT_EVENTS false
#define OPTIONS_DEFAULT_ANIMATE_BG  true
#define OPTIONS_DEFAULT_ANIMATE_WIN true
#define OPTIONS_DEFAULT_FPS 30
#define OPTIONS_DEFAULT_MAX_FPS 30
#define OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH  800
#define OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT 800

#define OPTIONS_DEFAULT_STARTUP_ACTION STARTUP_ACTION_NONE

#define OPTIONS_DEFAULT_CREATE_LEVEL_MODE CREATE_LEVEL_MODE_DFS
#define OPTIONS_DEFAULT_CREATE_LEVEL_MODE_STR "dfs"
#define OPTIONS_DEFAULT_CREATE_LEVEL_RADIUS 2
#define OPTIONS_DEFAULT_CREATE_LEVEL_EASY_MIN_PATH 2
#define OPTIONS_DEFAULT_CREATE_LEVEL_EASY_MAX_PATH 3
#define OPTIONS_DEFAULT_CREATE_LEVEL_MEDIUM_MIN_PATH 3
#define OPTIONS_DEFAULT_CREATE_LEVEL_MEDIUM_MAX_PATH 5
#define OPTIONS_DEFAULT_CREATE_LEVEL_HARD_MIN_PATH 4
#define OPTIONS_DEFAULT_CREATE_LEVEL_HARD_MAX_PATH 6
#define OPTIONS_DEFAULT_CREATE_LEVEL_MIN_PATH OPTIONS_DEFAULT_CREATE_LEVEL_MEDIUM_MIN_PATH
#define OPTIONS_DEFAULT_CREATE_LEVEL_MAX_PATH OPTIONS_DEFAULT_CREATE_LEVEL_MEDIUM_MAX_PATH

#define OPTIONS_WINDOW_MIN_WIDTH  500
#define OPTIONS_WINDOW_MIN_HEIGHT 500
#define OPTIONS_WINDOW_MAX_WIDTH  (MAX(GetScreenWidth(),  OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH ))
#define OPTIONS_WINDOW_MAX_HEIGHT (MAX(GetScreenHeight(), OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT))

struct options {

    bool safe_mode;
    bool verbose;
    bool wait_events;

    bool animate_bg;
    bool animate_win;
    bool load_state_animate_bg;
    bool load_state_animate_win;

    long max_fps;
    long initial_window_width;
    long initial_window_height;

    char *nvdata_dir;

    /* action */
    startup_action_t startup_action;

    /* action options */
    bool force;

    create_level_mode_t create_level_mode;
    long create_level_radius;
    long create_level_min_path;
    long create_level_max_path;

    /* remaining args */
    char **extra_argv;
    int extra_argc;
};
typedef struct options options_t;

options_t *create_options();
void destroy_options(options_t *options);

void options_set_defaults(options_t *options);
bool options_parse_args(options_t *options, int argc, char *argv[]);

extern options_t *options;

#endif /*OPTIONS_H*/
