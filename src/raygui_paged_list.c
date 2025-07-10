/****************************************************************************
 *                                                                          *
 * raygui_paged_list.c                                                      *
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
#include "raygui_paged_list.h"

char prev_button_tooltip[] = "Prev Page";
char next_button_tooltip[] = "Next Page";

float raygui_paged_list_sidebar_width = 0.0f;

raygui_paged_list_t *alloc_raygui_paged_list(void)
{
    return calloc(1, sizeof(raygui_paged_list_t));
}

raygui_paged_list_t *init_raygui_paged_list(raygui_paged_list_t *list, int *page_index, int *active, int *focus)
{
    assert_not_null(list);
    assert_not_null(page_index);
    assert_not_null(active);
    assert_not_null(focus);

    memset(list, 0, sizeof(raygui_paged_list_t));

    list->page_index = page_index;
    list->active     = active;
    list->focus      = focus;

    memcpy(list->prev_button_text, GuiIconText(ICON_ARROW_UP_FILL,   NULL), ICON_STR_MAXLEN);
    memcpy(list->next_button_text, GuiIconText(ICON_ARROW_DOWN_FILL, NULL), ICON_STR_MAXLEN);

    return list;
}

raygui_paged_list_t *create_raygui_paged_list(int *page_index, int *active, int *focus)
{
    raygui_paged_list_t *list = alloc_raygui_paged_list();
    return init_raygui_paged_list(list, page_index, active, focus);
}

void cleanup_raygui_paged_list(UNUSED raygui_paged_list_t *list)
{
}

void destroy_raygui_paged_list(raygui_paged_list_t *list)
{
    if (list) {
        cleanup_raygui_paged_list(list);
        SAFEFREE(list);
    }
}

void raygui_paged_list_goto_prev_page(raygui_paged_list_t *list)
{
    list->page--;
    if (list->page < 0) {
        list->page = 0;
    }
}

void raygui_paged_list_goto_next_page(raygui_paged_list_t *list)
{
    list->page++;
    if (list->page > list->page_count) {
        list->page = list->page_count;
    }
}

static void raygui_paged_list_prepare_text(raygui_paged_list_t *list)
{
    if (!list->text || (list->items_per_page < 1)) {
        return;
    }

    assert(list->items_per_page > 1);

    if (list->text_count > 0) {
        list->page_count = (int)ceilf(((float)list->text_count) / ((float)list->items_per_page));
    } else {
        list->page_count = 1;
    }

    list->first_page = 0;
    list->last_page  = list->page_count - 1;

    list->gui_list_bounds = list->bounds;
    list->gui_sidebar_bounds = list->gui_list_bounds;

    list->gui_sidebar_bounds.width = ICON_BUTTON_SIZE;

    float sidebar_size = list->gui_sidebar_bounds.width + PANEL_INNER_MARGIN;
    raygui_paged_list_sidebar_width = sidebar_size;

    list->gui_list_bounds.width -= sidebar_size;
    list->gui_sidebar_bounds.x  += list->gui_list_bounds.width + PANEL_INNER_MARGIN;

    list->gui_prev_button_bounds = list->gui_sidebar_bounds;
    list->gui_next_button_bounds = list->gui_sidebar_bounds;

    float extra_button_height = 4 * ICON_BUTTON_SIZE;

    list->gui_prev_button_bounds.height = ICON_BUTTON_SIZE + extra_button_height;
    list->gui_next_button_bounds.height = ICON_BUTTON_SIZE + extra_button_height;
    list->gui_next_button_bounds.y =
        list->gui_sidebar_bounds.y
        + list->gui_sidebar_bounds.height
        - list->gui_next_button_bounds.height;

    list->gui_item_bounds.x = list->gui_list_bounds.x + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING);
    list->gui_item_bounds.y = list->gui_list_bounds.y + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING) + GuiGetStyle(DEFAULT, BORDER_WIDTH);
    list->gui_item_bounds.width = list->gui_list_bounds.width - 2*GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING) - GuiGetStyle(DEFAULT, BORDER_WIDTH);
    list->gui_item_bounds.height = (float)GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT);
}

void raygui_paged_list_resize(raygui_paged_list_t *list, Rectangle bounds)
{
    list->bounds = bounds;

    list->row_height = GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT)
        + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING);

    list->items_per_page = (int)floorf(list->bounds.height / ((float)list->row_height));

    assert(list->items_per_page > 0);

    raygui_paged_list_prepare_text(list);
}

void raygui_paged_list_set_selected_callback(raygui_paged_list_t *list, raygui_paged_list_selected_cb_t selected_callback)
{
    assert_not_null(list);

    list->selected_callback = selected_callback;
}

void raygui_paged_list_set_text(raygui_paged_list_t *list, const char **text, int count)
{
    assert_not_null(list);

    list->text = text;
    list->text_count = count;

    raygui_paged_list_prepare_text(list);
}

static inline void raygui_paged_list_draw_sidebar(raygui_paged_list_t *list)
{

    bool show_prev = !raygui_paged_list_on_first_page(list);
    bool show_next = !raygui_paged_list_on_last_page(list);

    if (show_prev) {
        tooltip(list->gui_prev_button_bounds, prev_button_tooltip);
        if (GuiButton(list->gui_prev_button_bounds, list->prev_button_text)) {
            raygui_paged_list_goto_prev_page(list);
        }
    } else {
        GuiDisable();
        GuiButton(list->gui_prev_button_bounds, list->prev_button_text);
        GuiEnable();
    }

    if (show_next) {
        tooltip(list->gui_next_button_bounds, next_button_tooltip);
        if (GuiButton(list->gui_next_button_bounds, list->next_button_text)) {
            raygui_paged_list_goto_next_page(list);
        }
    } else {
        GuiDisable();
        GuiButton(list->gui_next_button_bounds, list->next_button_text);
        GuiEnable();
    }
}

static void raygui_paged_list_draw_text_rows(raygui_paged_list_t *list)
{
    assert_not_null(list);

    assert(list->page_count > 0);

    int start_idx = list->page * list->items_per_page;
    int end_idx = start_idx + list->items_per_page;
    end_idx = MIN(end_idx, list->text_count);
    int visible_items = end_idx - start_idx;

    GuiState state = GuiGetState();
        
    bool locked = GuiIsLocked();
    bool slider_dragging = GuiIsSliderDragging();

    Rectangle item_bounds = list->gui_item_bounds;

    if ((state != STATE_DISABLED) && !locked && !slider_dragging) {
        if (CheckCollisionPointRec(mouse_positionf, list->gui_list_bounds)) {
            state = STATE_FOCUSED;
            *list->focus = -1;
            for (int i = 0; i < visible_items; i++) {
                if (CheckCollisionPointRec(mouse_positionf, item_bounds)) {
                    *list->focus = start_idx + i;
                    if (mouse_left_click) {
                        *list->active = start_idx + i;
                    }
                    break;
                } else {
                    if ((*list->active == (start_idx + i)) &&
                        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        *list->active = -1;
                    }
                }

                item_bounds.y += (GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) +
                                  GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING));
            }
        } else {
            *list->focus = -1;
        }

        item_bounds.y = list->gui_list_bounds.y +
            GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING) +
            GuiGetStyle(DEFAULT, BORDER_WIDTH);
    }

    // assuming typedef enum { BORDER = 0, BASE, TEXT, OTHER } GuiPropertyElement;
    int   border_width = GuiGetStyle(DEFAULT, BORDER_WIDTH);
    Color border_color = GetColor(GuiGetStyle(LISTVIEW, /*BORDER*/ 0 + state*3));
    Color color        = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));

    GuiDrawRectangle(list->gui_list_bounds, border_width, border_color, color);

    const char **text = list->text;

    for (int i = 0; ((i < visible_items) && (text != NULL)); i++) {
        if (state == STATE_DISABLED) {
            if ((start_idx + i) == *list->active) {
                GuiDrawRectangle(item_bounds,
                                 GuiGetStyle(LISTVIEW, BORDER_WIDTH),
                                 GetColor(GuiGetStyle(LISTVIEW, BORDER_COLOR_DISABLED)),
                                 GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_DISABLED)));
            }

            GuiDrawText(text[start_idx + i],
                        GetTextBounds(DEFAULT, item_bounds),
                        GuiGetStyle(LISTVIEW, TEXT_ALIGNMENT),
                        GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_DISABLED)));
        } else {
            if ((start_idx + i) == *list->active) {
                GuiDrawRectangle(item_bounds,
                                 GuiGetStyle(LISTVIEW, BORDER_WIDTH),
                                 GetColor(GuiGetStyle(LISTVIEW, BORDER_COLOR_PRESSED)),
                                 GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_PRESSED)));

                GuiDrawText(text[start_idx + i],
                            GetTextBounds(DEFAULT, item_bounds),
                            GuiGetStyle(LISTVIEW, TEXT_ALIGNMENT),
                            GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_PRESSED)));
                
            } else if (((start_idx + i) == *list->focus)) {
                GuiDrawRectangle(item_bounds,
                                 GuiGetStyle(LISTVIEW, BORDER_WIDTH),
                                 GetColor(GuiGetStyle(LISTVIEW, BORDER_COLOR_FOCUSED)),
                                 GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_FOCUSED)));

                GuiDrawText(text[start_idx + i],
                            GetTextBounds(DEFAULT, item_bounds),
                            GuiGetStyle(LISTVIEW, TEXT_ALIGNMENT),
                            GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_FOCUSED)));
            } else {
                GuiDrawText(text[start_idx + i],
                            GetTextBounds(DEFAULT, item_bounds),
                            GuiGetStyle(LISTVIEW, TEXT_ALIGNMENT),
                            GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_NORMAL)));
            }
        }

        item_bounds.y += (GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) +
                          GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING));
    }

    raygui_paged_list_draw_sidebar(list);

#if 0
    Vector2 textpos = getVector2FromRectangle(list->gui_list_bounds);
    draw_panel_text(TextFormat("page = %d", list->page), textpos, YELLOW);
#endif
}

void raygui_paged_list_draw(raygui_paged_list_t *list)
{
    raygui_paged_list_draw_text_rows(list);
}

int raygui_paged_list_draw_as_listview(Rectangle bounds, const char **text, int count, int *scrollIndex, int *active, int *focus)
{
    raygui_paged_list_t list = {0};

    init_raygui_paged_list(&list, scrollIndex, active, focus);
    raygui_paged_list_resize(&list, bounds);
    raygui_paged_list_set_text(&list, text, count);
    raygui_paged_list_draw(&list);

    return 0;
}

static int raygui_paged_list_get_pagenum(raygui_paged_list_t *list, int rowid)
{
    return (int)floorf(((float)rowid) / ((float)list->items_per_page));
}

void raygui_paged_list_select_active_page(raygui_paged_list_t *list)
{
    assert_not_null(list);

    int pagenum = raygui_paged_list_get_pagenum(list, *list->active);

    if ((pagenum >= list->first_page) && (pagenum <= list->last_page)) {
        list->page = pagenum;
    }
}
