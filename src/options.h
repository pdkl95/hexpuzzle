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
#define OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT 600

#define OPTIONS_WINDOW_MIN_WIDTH  200
#define OPTIONS_WINDOW_MIN_HEIGHT 150
#define OPTIONS_WINDOW_MAX_WIDTH  1920
#define OPTIONS_WINDOW_MAX_HEIGHT 1200

struct options {
    bool verbose;
    bool wait_events;
    long max_fps;
    long initial_window_width;
    long initial_window_height;
};
typedef struct options options_t;

options_t *create_options();
void destroy_options(options_t *options);

void options_set_defaults(options_t *options);
bool options_parse_args(options_t *options, int argc, char *argv[]);

extern options_t *options;

#endif /*OPTIONS_H*/
