/****************************************************************************
 *                                                                          *
 * gui_collection.c                                                         *
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
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#include "common.h"

#include "sglib/sglib.h"

#include "level.h"
#include "collection.h"
#include "gui_collection.h"

bool draw_level_preview(level_t *level, Rectangle bounds);

Rectangle collection_panel_rect;
Rectangle collection_preview_rect;
Rectangle collection_list_rect;
Rectangle collection_new_button_rect;
Rectangle collection_play_button_rect;
Rectangle collection_edit_button_rect;
Rectangle collection_back_button_rect;
Rectangle collection_name_label_rect;
Rectangle collection_name_value_rect;

Vector2 collection_name_value_location;

Color name_value_bg_color;

char collection_panel_text[] = "Level Collection";
char collection_new_button_text[] = "New\nLevel";
char collection_play_button_text[] = "Play";
char collection_edit_button_text[] = "Edit";
char collection_name_label_text[] = "Name";

#define COLLECTION_BROWSER_BUTTON_LINE_COUNT 2
char *collection_browser_button_lines[COLLECTION_BROWSER_BUTTON_LINE_COUNT] = {
    "Back To",
    "Collections"
};

level_t *collection_preview_level;

void init_gui_collection(void)
{
    name_value_bg_color = ColorBrightness(tile_bg_color, -0.25);
    resize_gui_collection();
}

void cleanup_gui_collection(void)
{
}

void resize_gui_collection(void)
{
    collection_panel_rect = main_gui_area_rect;

    Vector2 collection_new_button_text_size  = measure_big_button_text(collection_new_button_text);
    Vector2 collection_play_button_text_size = measure_big_button_text(collection_play_button_text);
    Vector2 collection_edit_button_text_size = measure_big_button_text(collection_edit_button_text);

    float preview_play_width = MAX(COLLECTION_LEVEL_PREVIEW_SIZE,
                                   collection_play_button_text_size.x);
    preview_play_width = MAX(preview_play_width, collection_new_button_text_size.x);
    preview_play_width = MAX(preview_play_width, collection_edit_button_text_size.x);

    collection_preview_rect.height = COLLECTION_LEVEL_PREVIEW_SIZE;
    collection_preview_rect.width  = MIN(collection_preview_rect.height,
                                         preview_play_width);

    collection_preview_rect.height -= 2.0f * PANEL_INNER_MARGIN;
    collection_preview_rect.width  -= 2.0f * PANEL_INNER_MARGIN;

    collection_preview_rect.x =
        main_gui_area_rect.x
        + main_gui_area_rect.width
        - collection_preview_rect.width
        - PANEL_INNER_MARGIN;
    collection_preview_rect.y =
        main_gui_area_rect.y
        + main_gui_area_rect.height
        - collection_preview_rect.height
        - PANEL_INNER_MARGIN;

    collection_preview_rect.y = (collection_preview_rect.y + main_gui_area_rect.y) / 2.0f;

    collection_play_button_rect.width  = collection_preview_rect.width;
    collection_play_button_rect.height = collection_play_button_text_size.y + WINDOW_MARGIN;
    collection_play_button_rect.x = collection_preview_rect.x;
    collection_play_button_rect.y =
        collection_preview_rect.y
        + collection_preview_rect.height
        + PANEL_INNER_MARGIN;

    collection_edit_button_rect.width  = collection_preview_rect.width;
    collection_edit_button_rect.height = collection_edit_button_text_size.y + WINDOW_MARGIN;
    collection_edit_button_rect.x = collection_play_button_rect.x;
    collection_edit_button_rect.y =
        collection_play_button_rect.y
        + collection_play_button_rect.height
        + PANEL_INNER_MARGIN;

    collection_new_button_rect.width  = collection_preview_rect.width;
    collection_new_button_rect.height = (1 * collection_new_button_text_size.y) + WINDOW_MARGIN;
    collection_new_button_rect.x = collection_preview_rect.x;
    collection_new_button_rect.y =
        collection_preview_rect.y
        - collection_new_button_rect.height
        - (3 * PANEL_INNER_MARGIN);

    collection_back_button_rect.width  = collection_preview_rect.width;
    collection_back_button_rect.height = ICON_BUTTON_SIZE;
    collection_back_button_rect.x = collection_preview_rect.x;
    collection_back_button_rect.y =
        collection_panel_rect.y
        + collection_panel_rect.height
        - (2 * collection_back_button_rect.height)
        - (2 *PANEL_INNER_MARGIN);

    Vector2 collection_name_label_text_size = measure_gui_text(collection_name_label_text);

    collection_name_label_rect.x      = collection_panel_rect.x + PANEL_INNER_MARGIN;
    collection_name_label_rect.y      = collection_panel_rect.y + PANEL_INNER_MARGIN + TOOL_BUTTON_HEIGHT;
    collection_name_label_rect.width  = collection_name_label_text_size.x + 5;
    collection_name_label_rect.height = collection_name_label_text_size.y;

    collection_name_value_rect.x      = collection_name_label_rect.x + collection_name_label_rect.width + PANEL_INNER_MARGIN;
    collection_name_value_rect.y      = collection_name_label_rect.y;
    collection_name_value_rect.width  = collection_panel_rect.width - collection_name_label_rect.width - (3 * PANEL_INNER_MARGIN);
    collection_name_value_rect.height = collection_name_label_rect.height + 7;

    collection_name_value_location = getVector2FromRectangle(collection_name_value_rect);
    collection_name_value_location.x += 4;
    collection_name_value_location.y += 1;
    collection_name_label_rect.y += 3;

    collection_list_rect.x      = collection_name_label_rect.x;
    collection_list_rect.y      = collection_name_label_rect.y + collection_name_label_rect.height + PANEL_INNER_MARGIN;
    collection_list_rect.width  = collection_play_button_rect.x - collection_list_rect.x - PANEL_INNER_MARGIN;
    collection_list_rect.height = collection_panel_rect.height - (collection_list_rect.y - collection_panel_rect.y) - PANEL_INNER_MARGIN;
}

static level_t *collection_find_level_by_idx(collection_t *collection, int level_idx)
{
    level_t *p = collection->levels;

    while (p && level_idx > 0) {
        p = p->next;
        level_idx--;
    }

    return p;
}

static level_t *collection_find_active_levwl(collection_t *collection)
{
    return collection_find_level_by_idx(collection,
                                        collection->gui_list_active);
}

static void collection_check_gui_list_active_bounds(collection_t *collection)
{
    if (collection->gui_list_active < 0) {
        collection->gui_list_active = 0;
    }
    if (collection->gui_list_active >= collection->level_count) {
        collection->gui_list_active = collection->level_count - 1;
    }
}

static void collection_move_level_earlier(collection_t *collection, int level_idx)
{
    level_t *level = collection_find_level_by_idx(collection, level_idx);
    if (!level) {
        return;
    }

    level_t *target = level->prev;
    if (!target) {
        return;
    }

    SGLIB_DL_LIST_DELETE(level_t, collection->levels, level, prev, next);
    SGLIB_DL_LIST_ADD_BEFORE(level_t, target, level, prev, next);

    level_t *first;
    SGLIB_DL_LIST_GET_FIRST(level_t, target, prev, next, first);
    collection->levels = first;

    collection_update_level_names(collection);

    collection->gui_list_active = level_idx - 1;
    collection_check_gui_list_active_bounds(collection);

    collection->changed = true;
}

static void collection_move_level_later(collection_t *collection, int level_idx)
{
    level_t *level = collection_find_level_by_idx(collection, level_idx);
    if (!level) {
        return;
    }

    level_t *target = level->next;
    if (!target) {
        return;
    }

    SGLIB_DL_LIST_DELETE(level_t, collection->levels, level, prev, next);
    SGLIB_DL_LIST_ADD_AFTER(level_t, target, level, prev, next);

    level_t *first;
    SGLIB_DL_LIST_GET_FIRST(level_t, target, prev, next, first);
    collection->levels = first;

    collection_update_level_names(collection);

    collection->gui_list_active = level_idx + 1;
    collection_check_gui_list_active_bounds(collection);

    collection->changed = true;
}

static void draw_move_buttons(Rectangle bounds)
{
    collection_t *collection = current_collection;

    int count = collection->level_count;
    int *scrollIndex = &collection->gui_list_scroll_index;

    /* adapted from GuiListViewEx() */
    bool useScrollBar = false;
    if ((GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING))*count > bounds.height) useScrollBar = true;

    Rectangle itemBounds = { 0 };
    itemBounds.x = bounds.x + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING);
    itemBounds.y = bounds.y + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING) + GuiGetStyle(DEFAULT, BORDER_WIDTH);
    itemBounds.width = bounds.width - 2*GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING) - GuiGetStyle(DEFAULT, BORDER_WIDTH);
    itemBounds.height = (float)GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT);
    if (useScrollBar) itemBounds.width -= GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH);

    int visibleItems = (int)bounds.height/(GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING));
    if (visibleItems > count) visibleItems = count;

    int startIndex = (scrollIndex == NULL)? 0 : *scrollIndex;
    if ((startIndex < 0) || (startIndex > (count - visibleItems))) startIndex = 0;

    char btn_text[1024];

    Rectangle defered_draw_btn_rect;
    char defered_draw_btn_text[1024];
    char *defered_draw_btn_tt   = NULL;

    itemBounds.x -= 64;

    for (int i = 0; i < visibleItems; i++) {
        float ymargin = (itemBounds.height - RAYGUI_ICON_SIZE) / 2.0f;
        Rectangle up_btn_rect = {
            .x = itemBounds.x + itemBounds.width + 1 * (RAYGUI_ICON_SIZE),
            .y = itemBounds.y + ymargin,
            .width  = RAYGUI_ICON_SIZE,
            .height = RAYGUI_ICON_SIZE
        };
        Rectangle down_btn_rect = {
            .x = itemBounds.x + itemBounds.width + 2.5 * (RAYGUI_ICON_SIZE),
            .y = itemBounds.y + ymargin,
            .width  = RAYGUI_ICON_SIZE,
            .height = RAYGUI_ICON_SIZE
        };

        if (i > 0) {
            strncpy(btn_text, GuiIconText(ICON_ARROW_UP, NULL), 1024);
            btn_text[1023] = '\0';

            if (CheckCollisionPointRec(mouse_positionf, up_btn_rect)) {
                defered_draw_btn_rect = up_btn_rect;
                memcpy(defered_draw_btn_text, btn_text, 1024);
                defered_draw_btn_tt = "Move Up";
            }

            if (GuiButton(up_btn_rect, btn_text)) {
                collection_move_level_earlier(collection, i);
            }
        }

        if (i < visibleItems - 1) {
            strncpy(btn_text, GuiIconText(ICON_ARROW_DOWN, NULL), 1024);
            btn_text[1023] = '\0';

            if (CheckCollisionPointRec(mouse_positionf, down_btn_rect)) {
                defered_draw_btn_rect = down_btn_rect;
                memcpy(defered_draw_btn_text, btn_text, 1024);
                defered_draw_btn_tt = "Move Down";
            }

            if (GuiButton(down_btn_rect, btn_text)) {
                collection_move_level_later(collection, i);
            }
        }

        if (defered_draw_btn_tt) {
            GuiEnableTooltip();
            GuiSetTooltip(defered_draw_btn_tt);

            GuiButton(defered_draw_btn_rect, defered_draw_btn_text);

            GuiDisableTooltip();
            GuiSetTooltip(NULL);
        }

        itemBounds.y += (GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING));
    }
}

static inline void draw_level_list_colored_status(void)
{
    const char **text = current_collection->level_names;
    Rectangle bounds = collection_list_rect;
    int count = current_collection->level_count;
    int startIndex = current_collection->gui_list_scroll_index;

    bool useScrollBar = false;
    if ((GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) +
         GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING)) * count > bounds.height) {
        useScrollBar = true;
    }

    Rectangle itemBounds = { 0 };
    itemBounds.x = bounds.x + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING);
    itemBounds.y = bounds.y + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING) + GuiGetStyle(DEFAULT, BORDER_WIDTH);
    itemBounds.width = bounds.width - 2*GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING) - GuiGetStyle(DEFAULT, BORDER_WIDTH);
    itemBounds.height = (float)GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT);
    if (useScrollBar) itemBounds.width -= GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH);

     // Get items on the list
    int visibleItems = (int)bounds.height/(GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) +
                                           GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING));
    if (visibleItems > count) {
        visibleItems = count;
    }

    if ((startIndex < 0) || (startIndex > (count - visibleItems))) {
        startIndex = 0;
    }
    level_t *level = current_collection->levels;
    int nskip = startIndex;
    while (nskip > 0) {
        assert_not_null(level->next);
        level = level->next;
        nskip--;
    }

    int align = GuiGetStyle(LISTVIEW, TEXT_ALIGNMENT);
    for (int i = 0; ((i < visibleItems) && (text != NULL)); i++) {
        assert_not_null(level);

        GuiDrawText(level->finished_status_icon,
                    GetTextBounds(DEFAULT, itemBounds),
                    align,
                    level->finished_status_color);

        itemBounds.y += GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING);
        level = level->next;
    }
}

static void draw_level_list(void)
{
    assert_not_null(current_collection);

    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    int save_bg_color = GuiGetStyle(DEFAULT, BACKGROUND_COLOR);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, GuiGetStyle(STATUSBAR, BASE_COLOR_NORMAL));

    GuiListViewEx(collection_list_rect,
                  current_collection->level_names,
                  current_collection->level_count,
                  &current_collection->gui_list_scroll_index,
                  &current_collection->gui_list_active,
                  &current_collection->gui_list_focus);

    draw_level_list_colored_status();

#if 0
    printf("LIST[n=%d] scroll_index=%d, active=%d, focus=%d\n",
           current_collection->level_count,
           current_collection->gui_list_scroll_index,
           current_collection->gui_list_active,
           current_collection->gui_list_focus);
#endif

    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, save_bg_color);
}

static int draw_big_button(Rectangle bounds, const char *text, bool do_doubleclick)
{
    int rv = -1;

    set_big_button_font();

    bool disable_button = true;
    if (current_collection) {
        if (current_collection->gui_list_active != -1) {
            disable_button = false;
        }
    }

    if (disable_button) {
        GuiDisable();
    }

    if (GuiButton(bounds, text)) {
        if (current_collection->gui_list_active >= 0 && current_collection->gui_list_active < current_collection->level_count) {
            rv = current_collection->gui_list_active;
            goto play_button_cleanup;
        }
    }

    if (do_doubleclick && mouse_left_doubleclick) {
        if (current_collection->gui_list_active >= 0 && current_collection->gui_list_active < current_collection->level_count) {
            rv = current_collection->gui_list_active;
            goto play_button_cleanup;
        }
    }

  play_button_cleanup:
    if (disable_button) {
        GuiEnable();
    }

    set_default_font();

    return rv;
}

static void draw_edit_mode_buttons(void)
{
    int selected = draw_big_button(collection_edit_button_rect, collection_edit_button_text, false);
    if (selected > -1) {
        level_t *level = collection_find_active_levwl(current_collection);
        if (level) {
            level_edit(level);
        }
    }

    set_panel_font();

    if (GuiButton(collection_new_button_rect, collection_new_button_text)) {
        printf("new level\n");
    }

    set_default_font();
}

void gui_collection_update_level_preview(void)
{
    assert_not_null(current_collection);
    if (current_collection->gui_list_active < 0) {
        current_collection->gui_list_active = 0;
    }
    collection_preview_level = collection_find_active_levwl(current_collection);
}

static void draw_back_to_collections_button(void)
{
    //set_big_button_font();

    const char **lines = (const char **)collection_browser_button_lines;
    if (GuiButtonMultiLine(collection_back_button_rect,
                           lines,
                           COLLECTION_BROWSER_BUTTON_LINE_COUNT,
                           ICON_ARROW_LEFT)) {
        set_game_mode(GAME_MODE_BROWSER);
    }

    //set_default_font();
}

void draw_gui_collection(void)
{
    assert_not_null(current_collection);

    GuiPanel(collection_panel_rect, collection_panel_text);

    GuiLabel(collection_name_label_rect, collection_name_label_text);
    DrawRectangleRec(collection_name_value_rect, name_value_bg_color);
    draw_panel_text(collection_name(current_collection), collection_name_value_location, panel_header_text_color);

    int old_active = current_collection->gui_list_active;

    draw_level_list();

    if (game_mode == GAME_MODE_EDIT_COLLECTION) {
        draw_move_buttons(collection_list_rect);
        draw_edit_mode_buttons();
    }

    draw_back_to_collections_button();

    int selected = draw_big_button(collection_play_button_rect, collection_play_button_text, true);
    bool preview_clicked = draw_level_preview(collection_preview_level, collection_preview_rect);

    if ((selected > -1) || preview_clicked) {
        level_t *level = collection_find_active_levwl(current_collection);
        if (level) {
            level_play(level);
        }
    } else if (current_collection->gui_list_active != old_active) {
        gui_collection_update_level_preview();
    }
}
