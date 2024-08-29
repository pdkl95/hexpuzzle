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

static int grid_tile_storage_location(grid_t *grid, hex_axial_t axial)
{
    assert_not_null(grid);

    if (hex_axial_distance(axial, grid->center) > grid->radius) {
        return -1;
    }

    if ((axial.r < 0) || (axial.r >= grid->tile_grid_height) ||
        (axial.q < 0) || (axial.q >= grid->tile_grid_width)) {
        return -1;
    }
    int loc = (axial.r * grid->tile_grid_width) + axial.q;

    assert(loc >= 0);
    assert(loc < grid->maxtiles);

    return loc;
}

static void grid_add_to_bounding_box(grid_t *grid, tile_t *tile)
{
    Vector2 *corners = tile_corners(tile, grid->tile_size);
    for (int i=0; i<6; i++) {
        grid->px_min.x = MIN(grid->px_min.x, corners[i].x);
        grid->px_min.y = MIN(grid->px_min.y, corners[i].y);

        grid->px_max.x = MAX(grid->px_max.x, corners[i].x);
        grid->px_max.y = MAX(grid->px_max.y, corners[i].y);
   }
}

grid_t *create_grid(int radius)
{
    assert(radius > 0);
    assert(radius < 10);

    grid_t *grid = calloc(1, sizeof(grid_t));

    grid->radius = radius;

    grid->tile_grid_width  = (2 * radius) + 1;
    grid->tile_grid_height = (2 * radius) + 1;
    grid->maxtiles = grid->tile_grid_width * grid->tile_grid_height;

    grid->tile_size = 40.0f;

    grid->hover = NULL;

    grid->tiles = calloc(grid->maxtiles, sizeof(tile_t));

    grid->center.q = radius;
    grid->center.r = radius;

    grid->px_min.x = (float)window_size.x;
    grid->px_min.y = (float)window_size.y;

    grid->px_max.x = 0.0f;
    grid->px_max.y = 0.0f;

    for (int q=0; q<grid->tile_grid_width; q++) {
        for (int r=0; r<grid->tile_grid_height; r++) {
            hex_axial_t pos = {
                .q = q,
                .r = r
            };
            if (hex_axial_distance(pos, grid->center) <= radius) {
                tile_t *tile = grid_get_tile(grid, pos);
                init_tile(tile, pos);
                grid_add_to_bounding_box(grid, tile);
            }
        }
    }

    grid->px_bounding_box.x = grid->px_min.x;
    grid->px_bounding_box.y = grid->px_min.y;
    grid->px_bounding_box.width  = grid->px_max.x - grid->px_min.x;
    grid->px_bounding_box.height = grid->px_max.y - grid->px_min.y;

    grid->px_offset.x = ((float)window_size.x - grid->px_bounding_box.width)  / 2;
    grid->px_offset.y = ((float)window_size.y - grid->px_bounding_box.height) / 2;

    return grid;
}

void destroy_grid(grid_t *grid)
{
    assert_not_null(grid);

    SAFEFREE(grid->tiles);
    SAFEFREE(grid);
}

tile_t *grid_get_tile(grid_t *grid, hex_axial_t axial)
{
    assert_not_null(grid);

    int loc = grid_tile_storage_location(grid, axial);
    if ((loc >= 0) && (loc < grid->maxtiles)) {
        return &grid->tiles[loc];
    } else {
        return NULL;
    }
}

void grid_set_hover(grid_t *grid, hex_axial_t axial)
{
    if (grid) {
        if (grid->hover) {
            grid->hover->hover = false;
        }
        grid->hover = grid_get_tile(grid, axial);
        if (grid->hover) {
            grid->hover->hover = true;
        }
    }
}

void grid_draw(grid_t *grid)
{
    assert_not_null(grid);

    rlPushMatrix();

    rlTranslatef(grid->px_offset.x,
                 grid->px_offset.y,
                 0.0);

    for (int i=0; i<grid->maxtiles; i++) {
        tile_t *tile = &grid->tiles[i];
        assert_not_null(tile);
        if (tile->enabled) {
            tile_draw(tile, grid->tile_size);
        }
    }

    rlPopMatrix();
}
