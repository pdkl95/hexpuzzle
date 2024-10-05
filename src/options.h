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

#define OPTIONS_DEFAULT_VERBOSE false
#define OPTIONS_DEFAULT_WAIT_EVENTS false
#define OPTIONS_DEFAULT_FPS 30
#define OPTIONS_DEFAULT_MAX_FPS 30
#define OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH  800
#define OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT 800

#define OPTIONS_WINDOW_MIN_WIDTH  500
#define OPTIONS_WINDOW_MIN_HEIGHT 500
#define OPTIONS_WINDOW_MAX_WIDTH  (MAX(GetScreenWidth(),  OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH ))
#define OPTIONS_WINDOW_MAX_HEIGHT (MAX(GetScreenHeight(), OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT))

struct options {
    bool safe_mode;
    bool verbose;
    bool wait_events;
    long max_fps;
    long initial_window_width;
    long initial_window_height;

    char *nvdata_dir;

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
