/****************************************************************************
 *                                                                          *
 * level.h                                                                  *
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
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.             *
 *                                                                          *
 ****************************************************************************/

#ifndef LEVEL_H
#define LEVEL_H

#define LEVEL_FILENAME_EXT "hexlevel"
#define IS_LEVEL_FILENAME(filename) \
    (0 == strcmp(filename_ext(filename), LEVEL_FILENAME_EXT))

#include "tile.h"
#include "grid.h"

struct level {
    char *id;
    char *name;

    int radius;

    int tile_count;
    tile_t *tiles;

    char *filename;

    bool finished;
    char *ui_name;
    int ui_name_length;

    struct level *prev, *next;
};
typedef struct level level_t;

level_t *create_level(void);
void destroy_level(level_t *level);

bool level_parse_string(level_t *level, char *str);
level_t *load_level_file(char *filename);

grid_t *level_create_grid(level_t *level);

void level_update_ui_name(level_t *level);

void level_play(level_t *level);

#endif /*LEVEL_H*/
