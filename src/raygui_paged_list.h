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

struct raygui_paged_list_column {
    Rectangle bounds;
    char name[NAME_MAXLEN];
    float max_width;
    bool active;
};
typedef struct raygui_paged_list_column raygui_paged_list_column_t;

struct raygui_paged_list {
    Rectangle bounds;
    Rectangle gui_list_bounds;
    Rectangle gui_item_bounds;
    Rectangle gui_sidebar_bounds;
    Rectangle gui_prev_button_bounds;
    Rectangle gui_next_button_bounds;

    IconStr prev_button_text;
    IconStr next_button_text;

    int row_height;
    int items_per_page;
    int page;
    int page_count;
    int first_page;
    int last_page;

    int active_columns;

    const char **text;
    int text_count;

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

void raygui_paged_list_resize(raygui_paged_list_t *list, Rectangle bounds);

void raygui_paged_list_set_selected_callback(raygui_paged_list_t *list, raygui_paged_list_selected_cb_t selected_callback);

void raygui_paged_list_set_text(raygui_paged_list_t *list, const char **text, int count);
void raygui_paged_list_draw(raygui_paged_list_t *list);

int raygui_paged_list_draw_as_listview(Rectangle bounds, const char **text, int count, int *scrollIndex, int *active, int *focus);

void raygui_paged_list_select_active_page(raygui_paged_list_t *list);

extern float raygui_paged_list_sidebar_width;

#endif /*RAYGUI_PAGED_LIST_H*/

