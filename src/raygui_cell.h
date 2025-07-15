/****************************************************************************
 *                                                                          *
 * raygui_cell.h                                                            *
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

#ifndef RAYGUI_CELL_H
#define RAYGUI_CELL_H

struct raygui_cell;
struct raygui_cell_header;

#define RAYGUI_CELL_TEXT_MAXLEN NAME_MAXLEN
#define RAYGUI_CELL_TEXT_AND_ICON_MAXLEN \
    (RAYGUI_CELL_TEXT_MAXLEN + ICON_STR_MAXLEN)

/*********************************************************/

enum raygui_cell_flag {
    RAYGUI_CELL_FLAG_NULL           =      0,
    RAYGUI_CELL_FLAG_ERROR          = 0x0001,
    RAYGUI_CELL_FLAG_NO_DATA        = 0x0002,
    RAYGUI_CELL_FLAG_INACTIVE       = 0x0004,
    RAYGUI_CELL_FLAG_TOOLTIP        = 0x0008,
    RAYGUI_CELL_FLAG_BORDER_LEFT    = 0x0010,
    RAYGUI_CELL_FLAG_BORDER_BOTTOM  = 0x0020,
    RAYGUI_CELL_FLAG_BORDER_RIGHT   = 0x0040,
    RAYGUI_CELL_FLAG_BORDER_TOP     = 0x0080
};
typedef enum raygui_cell_flag raygui_cell_flag_t;

enum raygui_cell_mode {
    RAYGUI_CELL_MODE_NULL = 0,
    RAYGUI_CELL_MODE_TEXT,
    RAYGUI_CELL_MODE_ICON,
    RAYGUI_CELL_MODE_TEXT_AND_ICON
};
typedef enum raygui_cell_mode raygui_cell_mode_t;

struct raygui_cell_padding {
    float left;
    float bottom;
    float right;
    float top;
};
typedef struct raygui_cell_padding raygui_cell_padding_t;

struct raygui_cell {
    raygui_cell_mode_t mode;
    const char *tooltip;

    uint16_t flags;

    char *text;
    Color text_color;

    int icon;
    Color icon_color;

    Color border_color;

    struct raygui_cell_header *header;
};
typedef struct raygui_cell raygui_cell_t;

struct raygui_cell_header {
    raygui_cell_mode_t mode;
    bool hide;
    Rectangle bounds;
    Rectangle inner_bounds;
    float width;
    raygui_cell_padding_t padding;
    char text[RAYGUI_CELL_TEXT_MAXLEN];
};
typedef struct raygui_cell_header raygui_cell_header_t;

struct raygui_cell_grid {
    raygui_cell_header_t *headers;
    raygui_cell_t *cells;

    int columns;
    int rows;

    int cell_count;
};
typedef struct raygui_cell_grid raygui_cell_grid_t;

static inline int raygui_cell_grid_cell_index(raygui_cell_grid_t *grid, int row, int column)
{
    assert_not_null(grid);
    assert(   row >= 0);
    assert(column >= 0);
    assert(   row < grid->rows);
    assert(column < grid->columns);

    return (grid->columns * row) + column;
}

static inline raygui_cell_t *raygui_cell_grid_get_cell(raygui_cell_grid_t *grid, int row, int column)
{
    assert_not_null(grid);
    assert_not_null(grid->cells);

    int index = raygui_cell_grid_cell_index(grid, row, column);

    return &(grid->cells[index]);
}

static inline raygui_cell_header_t *raygui_cell_grid_get_header(raygui_cell_grid_t *grid, int column)
{
    assert_not_null(grid);
    assert_not_null(grid->headers);

    return &(grid->headers[column]);
}

/**************************************************************/

#define CELL_BIT_FLAG_FUNCS(upname, lowname)                                 \
    static inline void raygui_cell_set_##lowname(struct raygui_cell *cell)   \
    {                                                                        \
        cell->flags |= RAYGUI_CELL_FLAG_##upname;                            \
    }                                                                        \
                                                                             \
    static inline void raygui_cell_clear_##lowname(struct raygui_cell *cell) \
    {                                                                        \
        cell->flags &= ~RAYGUI_CELL_FLAG_##upname;                           \
    }                                                                        \
                                                                             \
    static inline bool raygui_cell_has_##lowname(struct raygui_cell *cell)   \
    {                                                                        \
        return cell->flags & RAYGUI_CELL_FLAG_##upname;                      \
    }


CELL_BIT_FLAG_FUNCS(ERROR,    error)
CELL_BIT_FLAG_FUNCS(NO_DATA,  no_data)
CELL_BIT_FLAG_FUNCS(INACTIVE, inactive)
CELL_BIT_FLAG_FUNCS(TOOLTIP,  tooltip)

CELL_BIT_FLAG_FUNCS(BORDER_LEFT,   border_left)
CELL_BIT_FLAG_FUNCS(BORDER_BOTTOM, border_bottom)
CELL_BIT_FLAG_FUNCS(BORDER_RIGHT,  border_right)
CELL_BIT_FLAG_FUNCS(BORDER_TOP,    border_top)

#undef CELL_BIT_FLAG_FUNCS

void raygui_cell_use_tooltip(struct raygui_cell *cell, const char *str);

/**************************************************************/

void draw_raygui_cell_at(struct raygui_cell *cell, Rectangle bounds, int state);

raygui_cell_grid_t *create_raygui_cell_grid(raygui_cell_header_t *headers, int header_count);
void destroy_raygui_cell_grid(raygui_cell_grid_t *grid);

void raygui_cell_grid_alloc_cells(raygui_cell_grid_t *grid, int rows);
void raygui_cell_grid_free_cells(raygui_cell_grid_t *grid);

void raygui_cell_grid_draw_row(raygui_cell_grid_t *grid, int row, float y, int state);
void raygui_cell_grid_draw_headers(raygui_cell_grid_t *grid);

#endif /*RAYGUI_CELL_H*/

