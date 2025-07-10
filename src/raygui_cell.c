/****************************************************************************
 *                                                                          *
 * raygui_cell.c                                                            *
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
#include "raygui_cell.h"

void draw_raygui_cell_at(struct raygui_cell *cell, Vector2 position)
{
    Rectangle bounds = {
        .x = position.x,
        .y = position.y,
        .width  = cell->size.x,
        .height = cell->size.y
    };

    if (raygui_cell_has_tooltip(cell)) {
        tooltip(bounds, cell->tooltip);
    }
}


raygui_cell_grid_t *create_raygui_cell_grid(raygui_cell_header_t *headers, int header_count)
{
    raygui_cell_grid_t *grid = calloc(1, sizeof(raygui_cell_grid_t));

    grid->headers = headers;
    grid->columns = header_count;

    return grid;
}

void destroy_raygui_cell_grid(raygui_cell_grid_t *grid)
{
    if (grid) {
        SAFEFREE(grid->cell);
        SAFEFREE(grid);
    }
}

raygui_cell_t **raygui_cell_grid_alloc_rows(raygui_cell_grid_t *grid, int rows)
{
    if (grid->cell) {
        SAFEFREE(grid->cell);
    }

    grid->cell = calloc(rows, sizeof(raygui_cell_t *));

    return grid->cell;
}

