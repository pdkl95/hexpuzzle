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

void raygui_cell_use_tooltip(struct raygui_cell *cell, const char *str)
{
    if (str) {
        raygui_cell_set_tooltip(cell);
        cell->tooltip = str;
    } else {
        raygui_cell_clear_tooltip(cell);
        cell->tooltip = NULL;
    }
}

void draw_raygui_cell_border_at(struct raygui_cell *cell, Rectangle bounds, UNUSED int state)
{
    if (raygui_cell_has_border_left(cell)) {
        DrawLine(bounds.x,
                 bounds.y,
                 bounds.x,
                 bounds.y + bounds.height,
                 cell->border_color);
    }
    if (raygui_cell_has_border_bottom(cell)) {
        DrawLine(bounds.x,
                 bounds.y + bounds.height,
                 bounds.x + bounds.width,
                 bounds.y + bounds.height,
                 cell->border_color);
    }
    if (raygui_cell_has_border_right(cell)) {
        DrawLine(bounds.x + bounds.width,
                 bounds.y,
                 bounds.x + bounds.width,
                 bounds.y + bounds.height,
                 cell->border_color);
    }
    if (raygui_cell_has_border_top(cell)) {
        DrawLine(bounds.x,
                 bounds.y,
                 bounds.x + bounds.width,
                 bounds.y,
                 cell->border_color);
    }
}

void draw_raygui_cell_content_at(struct raygui_cell *cell, Rectangle bounds, int state)
{
    Vector2 position = {
        .x = bounds.x,
        .y = bounds.y
    };

    Color state_color = GetColor(GuiGetStyle(LISTVIEW, state));

    BeginScissorMode(bounds.x, bounds.y, bounds.width, bounds.height);
    {
        switch (cell->mode) {
        case RAYGUI_CELL_MODE_NULL:
            break;

        case RAYGUI_CELL_MODE_TEXT:
            draw_gui_text(cell->text, position, state_color);
            break;

        case RAYGUI_CELL_MODE_ICON:
            GuiDrawIcon(cell->icon, bounds.x, bounds.y, 1, cell->icon_color);
            break;

        case RAYGUI_CELL_MODE_TEXT_AND_ICON:
            GuiDrawIcon(cell->icon, bounds.x, bounds.y, 1, cell->icon_color);
            draw_gui_text(cell->text, position, state_color);
            break;

        default:
            assert(false && "bad cell mode");
        }
    }
    EndScissorMode();

    if (raygui_cell_has_tooltip(cell)) {
        tooltip(bounds, cell->tooltip);
    }
}


raygui_cell_grid_t *create_raygui_cell_grid(raygui_cell_header_t *headers, int header_count)
{
    raygui_cell_grid_t *grid = calloc(1, sizeof(raygui_cell_grid_t));

    grid->headers = headers;
    grid->columns = header_count;

    grid->rows       = -1;
    grid->cell_count = -1;

    return grid;
}

void destroy_raygui_cell_grid(raygui_cell_grid_t *grid)
{
    if (grid) {
        SAFEFREE(grid->cells);
        SAFEFREE(grid);
    }
}

void raygui_cell_grid_alloc_cells(raygui_cell_grid_t *grid, int rows)
{
    assert_not_null(grid);
    assert(grid->columns > 0);

    grid->rows = rows;

    if (rows < 1) {
        return;
    }

    int new_cell_count = grid->rows * grid->columns;

    if (new_cell_count == grid->cell_count) {
        memset(grid->cells, 0, new_cell_count * sizeof(raygui_cell_t));
    } else {
        if (grid->cells) {
            SAFEFREE(grid->cells);
        }
        grid->cells = calloc(new_cell_count, sizeof(raygui_cell_t));
    }

    grid->cell_count = new_cell_count;
}

void raygui_cell_grid_free_cells(raygui_cell_grid_t *grid)
{
    assert_not_null(grid);
    SAFEFREE(grid->cells);
    grid->rows = -1;
}

void raygui_cell_grid_draw_row(raygui_cell_grid_t *grid, int row, float y, int state)
{
    assert_not_null(grid);
    assert(row >= 0);
    assert(row < grid->rows);

    for (int col=0; col<grid->columns; col++) {
        raygui_cell_t *cell = raygui_cell_grid_get_cell(grid, row, col);
        if (cell->header->hide) {
            //continue;
            break;
        }

        Rectangle bounds = cell->header->bounds;
        float header_outer_y = bounds.y;
        bounds.y = y;

        Rectangle inner_bounds = cell->header->inner_bounds;
        float header_inner_y = inner_bounds.y;
        inner_bounds.y = y + (header_inner_y - header_outer_y);

        draw_raygui_cell_border_at( cell,       bounds, state);
        draw_raygui_cell_content_at(cell, inner_bounds, state);
        //DrawRectangleRec(      bounds, ColorAlpha(LIME,0.3));
        //DrawRectangleRec(inner_bounds, ColorAlpha(YELLOW,0.3));
    }
}

static void grid_draw_header(raygui_cell_header_t *header)
{
    if (header->hide) {
        return;
    }

    GuiStatusBar(header->bounds, header->text);
}

void raygui_cell_grid_draw_headers(raygui_cell_grid_t *grid)
{
    assert_not_null(grid);

    for (int col=0; col<grid->columns; col++) {
        raygui_cell_header_t *header = raygui_cell_grid_get_header(grid, col);
        grid_draw_header(header);
    }
}
