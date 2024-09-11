/****************************************************************************
 *                                                                          *
 * grid.h                                                                   *
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

#ifndef GRID_H
#define GRID_H

#include "const.h"
#include "hex.h"
#include "tile.h"

struct grid {
    int radius;
    int max_radius;

    int tile_radius;
    int tile_grid_width;
    int tile_grid_height;
    int maxtiles;

    hex_axial_t center;
    tile_t *center_tile;

    tile_t tiles[TILE_GRID_WIDTH][TILE_GRID_HEIGHT];
    char *name;

    float req_tile_size;
    float tile_size;

    Vector2 px_offset;
    Vector2 px_grid_size;
    Vector2 px_min;
    Vector2 px_max;
    Rectangle px_bounding_box;

    tile_t *hover;
    hex_direction_t hover_section;
    tile_t *hover_adjacent;
    hex_direction_t hover_adjacent_section;
    float hover_section_adjacency_radius;

    Vector2 mouse_pos;

    tile_t *drag_target;
    Vector2 drag_start;
    Vector2 drag_offset;
    int drag_reset_frames;
    int drag_reset_total_frames;
    Vector2 drag_reset_vector;
};
typedef struct grid grid_t;

grid_t *create_grid(int radius);
void destroy_grid(grid_t *grid);
void grid_resize(grid_t *grid);
void grid_draw(grid_t *grid);
tile_t *grid_get_tile(grid_t *grid, hex_axial_t axial);
tile_t *grid_find_neighbor_tile(grid_t *grid, tile_t *tile, hex_direction_t section);
void grid_set_hover(grid_t *grid, IVector2 mouse_position);
void grid_drag_start(grid_t *grid);
void grid_drag_stop(grid_t *grid);
void grid_modify_hovered_feature(grid_t *grid);
void grid_serialize(grid_t *grid, FILE *f);
void grid_change_radius(grid_t *grid, int new_radius);
void grid_enable_ring(grid_t *grid, int radius);
void grid_disable_ring(grid_t *grid, int radius);

extern grid_t *current_grid;

#endif /*GRID_H*/

