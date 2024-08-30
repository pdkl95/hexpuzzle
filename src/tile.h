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

struct tile {
    bool enabled;

    hex_axial_t position;
    path_type_t path[6];

    bool hover;
};
typedef struct tile tile_t;

tile_t *init_tile(tile_t *tile, hex_axial_t pos);
tile_t *create_tile(void);
void destroy_tile(tile_t *tile);

void tile_draw(tile_t *tile, float tile_size, bool drqg);

Vector2 *tile_corners(tile_t *tile, float tile_size);

#endif /*TILE_H*/

