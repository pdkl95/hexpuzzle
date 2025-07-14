/****************************************************************************
 *                                                                          *
 * raygui_paged_list.h                                                      *
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

#ifndef RAYGUI_PAGED_LIST_H
#define RAYGUI_PAGED_LIST_H

#include "raygui_cell.h"

struct raygui_paged_list;

typedef void (*raygui_paged_list_selected_cb_t)(struct raygui_paged_list *list);

#define MAX_PAGED_LIST_COLUMNS 8
#define MAX_PAGED_LIST_COLUMN_TEXT_WIDTH UI_NAME_MAXLEN

enum raygui_paged_list_mode {
    RAYGUI_PAGED_LIST_MODE_PLAIN_TEXT   = 0,
    RAYGUI_PAGED_LIST_MODE_CELL_COLUMNS = 1
};
typedef enum raygui_paged_list_mode raygui_paged_list_mode_t;

struct raygui_paged_list {
    raygui_paged_list_mode_t mode;

    float sidebar_width;
    float sidebar_margin_width;

    Rectangle bounds;
    Rectangle header_bounds;
    Rectangle gui_list_bounds;
    Rectangle gui_item_bounds;
    Rectangle gui_sidebar_bounds;
    Rectangle gui_prev_button_bounds;
    Rectangle gui_next_button_bounds;

    IconStr prev_button_text;
    IconStr next_button_text;

    int header_height;
    int row_height;
    int items_per_page;
    int page;
    int page_count;
    int first_page;
    int last_page;

    int active_columns;

    const char **text;
    int row_count;
    raygui_cell_grid_t *cell_grid;

    int *page_index;
    int *active;
    int *focus;

    raygui_paged_list_selected_cb_t selected_callback;
};
typedef struct raygui_paged_list raygui_paged_list_t;

static inline bool raygui_paged_list_on_first_page(raygui_paged_list_t *list)
{
    return list->page == list->first_page;
}

static inline bool raygui_paged_list_on_last_page(raygui_paged_list_t *list)
{
    return list->page == list->last_page;
}

raygui_paged_list_t *alloc_raygui_paged_list(void);
raygui_paged_list_t *init_raygui_paged_list(raygui_paged_list_t *list,
                                            int *page_index,
                                            int *active,
                                            int *focus);
raygui_paged_list_t *create_raygui_paged_list(int *page_index,
                                              int *active,
                                              int *focus);
void cleanup_raygui_paged_list(raygui_paged_list_t *list);
void destroy_raygui_paged_list(raygui_paged_list_t *list);

void raygui_paged_list_use_columns(raygui_paged_list_t *list, raygui_cell_header_t *headers, int column_count);

void raygui_paged_list_alloc_column_cells(raygui_paged_list_t *list, int row_count);
void raygui_paged_list_free_column_cells(raygui_paged_list_t *list);

static inline raygui_cell_t *raygui_paged_list_get_cell(raygui_paged_list_t *list, int row, int column)
{
    return raygui_cell_grid_get_cell(list->cell_grid, row, column);
}

void raygui_paged_list_resize(raygui_paged_list_t *list, Rectangle bounds);

void raygui_paged_list_set_selected_callback(raygui_paged_list_t *list, raygui_paged_list_selected_cb_t selected_callback);

void raygui_paged_list_set_text(raygui_paged_list_t *list, const char **text, int count);
void raygui_paged_list_draw(raygui_paged_list_t *list);

int raygui_paged_list_draw_as_listview(Rectangle bounds, const char **text, int count, int *scrollIndex, int *active, int *focus);

void raygui_paged_list_select_active_page(raygui_paged_list_t *list);

extern float raygui_paged_list_sidebar_width;

#endif /*RAYGUI_PAGED_LIST_H*/

