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

#define RAYGUI_CELL_TOOLTIP_MAXLEN NAME_MAXLEN

/*********************************************************/

enum raygui_cell_flag {
    RAYGUI_CELL_FLAG_NULL      =      0,
    RAYGUI_CELL_FLAG_ERROR     = 0x0001,
    RAYGUI_CELL_FLAG_NO_DATA   = 0x0002,
    RAYGUI_CELL_FLAG_INACTIVE  = 0x0004,
    RAYGUI_CELL_FLAG_TOOLTIP   = 0x0008
};
typedef enum raygui_cell_flag raygui_cell_flag_t;

enum raygui_cell_mode {
    RAYGUI_CELL_MODE_NULL = 0,
    RAYGUI_CELL_MODE_TEXT,
    RAYGUI_CELL_MODE_ICON,
    RAYGUI_CELL_MODE_TEXT_AND_ICON
};
typedef enum raygui_cell_mode raygui_cell_mode_t;

struct raygui_cell_text {
    char text[RAYGUI_CELL_TEXT_MAXLEN];
    char *text_src;
    Color color;
};
typedef struct raygui_cell_text raygui_cell_text_t;

struct raygui_cell_icon {
    int icon;
    IconStr icon_str;
    Color color;
};
typedef struct raygui_cell_icon raygui_cell_icon_t;

struct raygui_cell_text_and_icon {
    raygui_cell_text_t text;
    raygui_cell_icon_t icon;
};
typedef struct raygui_cell_text_and_icon raygui_cell_text_and_icon_t;

struct raygui_cell {
    raygui_cell_mode_t mode;

    union {
        raygui_cell_text_t          cell_text;
        raygui_cell_icon_t          cell_icon;
        raygui_cell_text_and_icon_t cell_text_and_icon;
    };
    
    Vector2 size;
    char *tooltip;

    uint16_t flags;

    struct raygui_cell_header *header;

    struct raygui_cell *next_row_cell;
    struct raygui_cell *next_column_cell;
};
typedef struct raygui_cell raygui_cell_t;

struct raygui_cell_header {
    raygui_cell_mode_t mode;
    Vector2 size;
    float width;
    char text[RAYGUI_CELL_TEXT_MAXLEN];
    char tooltip[RAYGUI_CELL_TOOLTIP_MAXLEN];

    struct raygui_cell *first_column_cell;
};
typedef struct raygui_cell_header raygui_cell_header_t;

struct raygui_cell_grid {
    raygui_cell_header_t *headers;
    raygui_cell_t **cell;

    int columns;
    int rows;
};
typedef struct raygui_cell_grid raygui_cell_grid_t;

/**************************************************************/

static inline void raygui_cell_set_error(struct raygui_cell *cell)
{
    cell->flags |= RAYGUI_CELL_FLAG_ERROR;
}

static inline void raygui_cell_clear_error(struct raygui_cell *cell)
{
    cell->flags &= ~RAYGUI_CELL_FLAG_ERROR;
}

static inline bool raygui_cell_has_error(struct raygui_cell *cell)
{
    return cell->flags & RAYGUI_CELL_FLAG_ERROR;
}


static inline void raygui_cell_set_no_data(struct raygui_cell *cell)
{
    cell->flags |= RAYGUI_CELL_FLAG_NO_DATA;
}

static inline void raygui_cell_clear_no_data(struct raygui_cell *cell)
{
    cell->flags &= ~RAYGUI_CELL_FLAG_NO_DATA;
}

static inline bool raygui_cell_has_no_data(struct raygui_cell *cell)
{
    return cell->flags & RAYGUI_CELL_FLAG_NO_DATA;
}


static inline void raygui_cell_set_inactive(struct raygui_cell *cell)
{
    cell->flags |= RAYGUI_CELL_FLAG_INACTIVE;
}

static inline void raygui_cell_clear_inactive(struct raygui_cell *cell)
{
    cell->flags &= ~RAYGUI_CELL_FLAG_INACTIVE;
}

static inline bool raygui_cell_has_inactive(struct raygui_cell *cell)
{
    return cell->flags & RAYGUI_CELL_FLAG_INACTIVE;
}


static inline void raygui_cell_set_tooltip(struct raygui_cell *cell, const char *str)
{
    snprintf(cell->tooltip, RAYGUI_CELL_TOOLTIP_MAXLEN, "%s", str);
    cell->flags |= RAYGUI_CELL_FLAG_TOOLTIP;
}

static inline void raygui_cell_clear_tooltip(struct raygui_cell *cell)
{
    cell->flags &= ~RAYGUI_CELL_FLAG_TOOLTIP;
}

static inline bool raygui_cell_has_tooltip(struct raygui_cell *cell)
{
    return cell->flags & RAYGUI_CELL_FLAG_TOOLTIP;
}

/**************************************************************/

void draw_raygui_cell_at(struct raygui_cell *cell, Vector2 position);

raygui_cell_grid_t *create_raygui_cell_grid(raygui_cell_header_t *headers, int header_count);
void destroy_raygui_cell_grid(raygui_cell_grid_t *grid);

raygui_cell_t **raygui_cell_grid_alloc_rows(raygui_cell_grid_t *grid, int rows);

#endif /*RAYGUI_CELL_H*/

