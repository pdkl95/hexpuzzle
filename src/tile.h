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

#include "hex.h"

enum path_type {
    PATH_TYPE_NONE   = 0,
    PATH_TYPE_RED    = 1,
    PATH_TYPE_BLUE   = 2,
    PATH_TYPE_YELLOW = 3,
    PATH_TYPE_GREEN  = 4
};
typedef enum path_type path_type_t;
#define PATH_TYPE_COUNT (PATH_TYPE_GREEN + 1)

struct path_int {
    int path[PATH_TYPE_COUNT];
};
typedef struct path_int path_int_t;

Color path_type_color(path_type_t type);

struct tile_section {
    Vector2 corners[3];
};
typedef struct tile_section tile_section_t;

struct tile {
    /*
     * game attr
     */
    bool enabled;
    bool fixed;
    bool hidden;
    path_type_t path[6];

    /*
     * drawing/ui attr
     */
    hex_axial_t position;

    float size;
    float line_width;
    float center_circle_draw_radius;
    float center_circle_hover_radius;

    Vector2 center;
    Vector2 corners[7];
    Vector2 midpoints[7];
    tile_section_t sections[6];
    struct tile *neighbors[6];

    bool hover;
    bool hover_center;
    hex_direction_t hover_section;

    struct tile *hover_adjacent;

    struct tile *solved;
    struct tile *unsolved;
};
typedef struct tile tile_t;

int compare_tiles(const void *p1, const void *p2);

void print_tile(tile_t *tile);
tile_t *init_tile(tile_t *tile, hex_axial_t pos);
tile_t *create_tile(void);
void destroy_tile(tile_t *tile);

bool tile_eq(tile_t *dst, tile_t *other);
void tile_copy_attributes(tile_t *dst, tile_t *src);
void tile_copy_attributes_except_enabled(tile_t *dst, tile_t *src);
void tile_swap_attributes(tile_t *a, tile_t *b);

bool tile_check(tile_t *tile);
path_int_t tile_count_path_types(tile_t *tile);
void tile_set_size(tile_t *tile, float tile_size);
void tile_draw(tile_t *tile, tile_t *drag_target, bool finished, Color finished_color);

void tile_set_hover(tile_t *tile, Vector2 mouse_pos);
void tile_unset_hover(tile_t *tile);

void tile_set_hover_adjacent(tile_t *tile, hex_direction_t section, tile_t *adjacent_tile);
void tile_unset_hover_adjacent(tile_t *tile);

void tile_cycle_path_section(tile_t *tile, hex_direction_t section;);
void tile_modify_hovered_feature(tile_t *tile);

void tile_serialize(tile_t *tile, FILE *f);
void tile_set_flag_from_char(tile_t *tile, char c);

#endif /*TILE_H*/

