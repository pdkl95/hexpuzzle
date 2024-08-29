/****************************************************************************
 *                                                                          *
 * grid.c                                                                   *
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

#include "common.h"
#include "tile.h"
#include "grid.h"

static inline hex_axial_t grid_centerize_axial(grid_t *grid, hex_axial_t axial)
{
    return hex_axial_add(grid->center,  axial);
}

static inline int grid_tile_storage_location(grid_t *grid, hex_axial_t axial)
{
    axial = grid_centerize_axial(grid, axial);
    return (axial.r * grid->width) + axial.q;
}

grid_t *create_grid(int radius)
{
    grid_t *grid = calloc(1, sizeof(grid_t));

    grid->radius = radius;

    grid->width  = (2 * radius) + 1;
    grid->height = (2 * radius) + 1;
    grid->maxtiles = grid->width * grid->height;

    grid->tile_size = 20.0f;

    grid->hover = NULL;

    grid->tiles = calloc(grid->maxtiles, sizeof(tile_t));

    grid->center.q = radius;
    grid->center.r = radius;


    for (int q=0; q<grid->width; q++) {
        for (int r=0; r<grid->height; r++) {
            hex_axial_t pos = {
                .q = q,
                .r = r
            };
            hex_axial_t centered_pos = hex_axial_subtract(pos, grid->center);
            tile_t *tile = grid_get_tile(grid, centered_pos);
            init_tile(tile, pos);
        }
    }

    return grid;
}

void destroy_grid(grid_t *grid)
{
    if (grid) {
        SAFEFREE(grid->tiles);
        SAFEFREE(grid);
    }
}

tile_t *grid_get_tile(grid_t *grid, hex_axial_t axial)
{
    int loc = grid_tile_storage_location(grid, axial);
    return &grid->tiles[loc];
}

void grid_set_hover(grid_t *grid, hex_axial_t axial)
{
    if (grid->hover) {
        grid->hover->hover = false;
    }
    grid->hover = grid_get_tile(grid, axial);
    if (grid->hover) {
        grid->hover->hover = true;
    }
}

void grid_draw(grid_t *grid)
{
    Vector2 offset = {0};

    for (int i=0; i<grid->maxtiles; i++) {
        tile_t *tile = &grid->tiles[i];
        if (tile->enabled) {
            tile_draw(tile, grid->tile_size, offset);
        }
    }
}
