/****************************************************************************
 *                                                                          *
 * path.h                                                                   *
 *                                                                          *
 * This file is part of hexpuzzle.                                          *
 *                                                                          *
 * hexpuzzle is free software: you can redistribute it and/or               *
 * modify it under the terms of the GNU General Public License as published *
 * by the Free Software Foundation, either version 3 of the License,        *
 * or (at your option) any later version.                                   *
 *                                                                          *
 * hexpuzzle is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General *
 * Public License for more details.                                         *
 *                                                                          *
 * You should have received a copy of the GNU General Public License along  *
 * with rocks. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PATH_H
#define PATH_H

#include "const.h"
#include "options.h"

typedef struct path path_t;
enum path_type {
    PATH_TYPE_NONE   = 0,
    PATH_TYPE_RED    = 1,
    PATH_TYPE_BLUE   = 2,
    PATH_TYPE_YELLOW = 3,
    PATH_TYPE_GREEN  = 4
};
typedef enum path_type path_type_t;
#if (PATH_TYPE_COUNT <= PATH_TYPE_GREEN)
# error "PATH_TYPE_COUNT does not match enum path_type"
#endif

#define PATH_TYPE_MIN (PATH_TYPE_NONE)
#define PATH_TYPE_MAX (PATH_TYPE_COUNT - 1)

char *path_type_name(path_type_t type);

struct path_int {
    int path[PATH_TYPE_COUNT];
};
typedef struct path_int path_int_t;

static inline Color path_type_color(path_type_t type)
{
    return options->path_color[type].color;
}

static inline Color path_type_highlight_color(path_type_t type)
{
    return options->path_color[type].highlight_color;
}

#endif /*PATH_H*/

