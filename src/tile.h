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
#include "path.h"

struct tile_pos;
struct level;

struct tile_flags {
    bool enabled;
    bool fixed;
    bool hidden;
};
typedef struct tile_flags tile_flags_t;

struct tile {
    int id;

    /*
     * game attr
     */
    bool enabled;
    bool fixed;
    bool hidden;

    int path_count;
    path_type_t path[6];
    path_type_t saved_path[6];

    path_type_t start_for_path_type;

    struct tile_pos *solved_pos;
    struct tile_pos *unsolved_pos;
};
typedef struct tile tile_t;

struct tile_neighbor_paths {
    path_type_t path[6];
    path_type_t saved_path[6];
    path_type_t neighbor_path[6];
    tile_t     *neighbors[6];
};
typedef struct tile_neighbor_paths tile_neighbor_paths_t;

int compare_tiles(const void *p1, const void *p2);

char *tile_flag_string(tile_t *tile);
char *tile_path_string(tile_t *tile);
char *tile_our_pos_string(struct tile_pos *pos);

void print_tile(tile_t *tile);
tile_t *init_tile(tile_t *tile);
tile_t *create_tile(void);
void destroy_tile(tile_t *tile);

tile_flags_t tile_get_flags(tile_t *tile);
void tile_set_flags(tile_t *tile, tile_flags_t flags);

tile_neighbor_paths_t tile_get_neighbor_paths(tile_t *tile);
void tile_set_neighbor_paths(tile_t *tile, tile_neighbor_paths_t paths);

bool tile_has_path_type(tile_t *tile, path_type_t type);

bool tile_eq(tile_t *dst, tile_t *other);
void tile_copy_attributes(tile_t *dst, tile_t *src);
void tile_copy_attributes_except_enabled(tile_t *dst, tile_t *src);
void tile_swap_attributes(tile_t *a, tile_t *b);

void tile_update_path_count(tile_t *tile);
path_int_t tile_count_path_types(tile_t *tile);
bool tile_is_blank(tile_t *tile);

bool tile_from_json(tile_t *tile, struct level *level, cJSON *json);
cJSON *tile_to_json(tile_t *tile);

static inline bool tile_dragable(tile_t *tile)
{
    return tile->enabled && !tile->fixed && !tile->hidden;
}

bool tile_is_solved(tile_t *tile);

#endif /*TILE_H*/

