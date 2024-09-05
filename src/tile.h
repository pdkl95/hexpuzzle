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
    PATH_TYPE_GREEN  = 4,
    PATH_TYPE_MAX    = 5
};
typedef enum path_type path_type_t;

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

    bool hover;
    bool hover_center;
    hex_direction_t hover_section;

    struct tile *next;
};
typedef struct tile tile_t;

tile_t *init_tile(tile_t *tile, hex_axial_t pos);
tile_t *create_tile(void);
tile_t *create_tile_from_serialized_strings(char *addr, char *path, char *flags);
void destroy_tile(tile_t *tile);

void tile_copy_attributes(tile_t *dst, tile_t *src);
void tile_swap_attributes(tile_t *a, tile_t *b);

void tile_set_size(tile_t *tile, float tile_size);
void tile_draw(tile_t *tile, tile_t *drag_target);

void tile_set_hover(tile_t *tile, Vector2 mouse_pos);
void tile_unset_hover(tile_t *tile);

void tile_cycle_path_section(tile_t *tile, hex_direction_t section;);
void tile_modify_hovered_feature(tile_t *tile);

void tile_serialize(tile_t *tile, FILE *f);

#endif /*TILE_H*/

