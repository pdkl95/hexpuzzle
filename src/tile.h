/****************************************************************************
 *                                                                          *
 * tile.h                                                                   *
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

#ifndef TILE_H
#define TILE_H

#include "cJSON/cJSON.h"

#include "hex.h"

struct tile_pos;
struct level;

enum path_type {
    PATH_TYPE_NONE   = 0,
    PATH_TYPE_RED    = 1,
    PATH_TYPE_BLUE   = 2,
    PATH_TYPE_YELLOW = 3,
    PATH_TYPE_GREEN  = 4
};
typedef enum path_type path_type_t;
#define PATH_TYPE_COUNT (PATH_TYPE_GREEN + 1)

char *path_type_name(path_type_t type);

struct path_int {
    int path[PATH_TYPE_COUNT];
};
typedef struct path_int path_int_t;

struct tile {
    /*
     * game attr
     */
    bool enabled;
    bool fixed;
    bool hidden;
    path_type_t path[6];
    path_type_t saved_path[6];

    struct tile_pos *solved_pos;
    struct tile_pos *unsolved_pos;
};
typedef struct tile tile_t;

int compare_tiles(const void *p1, const void *p2);

char *tile_flag_string(tile_t *tile);
char *tile_path_string(tile_t *tile);
char *tile_our_pos_string(struct tile_pos *pos);

void print_tile(tile_t *tile);
tile_t *init_tile(tile_t *tile);
tile_t *create_tile(void);
void destroy_tile(tile_t *tile);

bool tile_eq(tile_t *dst, tile_t *other);
void tile_copy_attributes(tile_t *dst, tile_t *src);
void tile_copy_attributes_except_enabled(tile_t *dst, tile_t *src);
void tile_swap_attributes(tile_t *a, tile_t *b);

path_int_t tile_count_path_types(tile_t *tile);

bool tile_from_json(tile_t *tile, struct level *level, cJSON *json);
cJSON *tile_to_json(tile_t *tile);

static inline bool tile_dragable(tile_t *tile)
{
    return tile->enabled && !tile->fixed && !tile->hidden;
}

#endif /*TILE_H*/

