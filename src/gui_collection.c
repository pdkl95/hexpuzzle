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
#include "level.h"
#include "collection.h"
#include "gui_collection.h"

bool draw_level_preview(level_t *level, Rectangle bounds);

Rectangle collection_panel_rect;
Rectangle collection_preview_rect;
Rectangle collection_list_rect;
Rectangle collection_play_button_rect;

char collection_panel_text[] = "Level Collection";
char collection_play_button_text[] = "Play";

level_t *collection_preview_level;

void init_gui_collection(void)
{
    resize_gui_collection();
}

void cleanup_gui_collection(void)
{
}

void resize_gui_collection(void)
{
    collection_panel_rect = main_gui_area_rect;

    Vector2 collection_play_button_text_size = measure_big_button_text(collection_play_button_text);

    float preview_play_width = MAX(COLLECTION_LEVEL_PREVIEW_SIZE,
                                   collection_play_button_text_size.x);

    collection_preview_rect.width  = preview_play_width;
    collection_preview_rect.height = COLLECTION_LEVEL_PREVIEW_SIZE;
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

    collection_play_button_rect.width  = preview_play_width;
    collection_play_button_rect.height = collection_play_button_text_size.y + WINDOW_MARGIN;
    collection_play_button_rect.x = collection_preview_rect.x;
    collection_play_button_rect.y =
        collection_preview_rect.y
        + collection_preview_rect.height
        + PANEL_INNER_MARGIN;
}

static int draw_play_button(void)
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

    if (GuiButton(collection_play_button_rect, collection_play_button_text)) {
        if (current_collection->gui_list_active >= 0 && current_collection->gui_list_active < current_collection->level_count) {
            rv = current_collection->gui_list_active;
            goto play_button_cleanup;
        }
    }

    if (mouse_left_doubleclick) {
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

void draw_gui_collection(void)
{
    GuiPanel(collection_panel_rect, collection_panel_text);

    int selected = draw_play_button();
    bool preview_clicked = draw_level_preview(collection_preview_level, collection_preview_rect);

    if ((selected > -1) || preview_clicked) {
        cellection_launch_active_gui_list_level(current_collection,
                                                game_mode == GAME_MODE_EDIT_COLLECTION);
    }
}
