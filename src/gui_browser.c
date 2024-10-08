/****************************************************************************
 *                                                                          *
 * gui_browser.c                                                            *
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
#include "collection.h"
#include "gui_browser.h"

void open_game_file(const char *path);
void open_classics_game_pack(int n);

struct gui_list_vars {
    const char **names;
    int count;

    int scroll_index;
    int active;
    int focus;
};
typedef struct gui_list_vars gui_list_vars_t;

Rectangle browser_panel_rect;
Rectangle browser_tabbar_rect;
Rectangle browser_area_rect;
Rectangle browser_list_rect;
Rectangle browser_play_button_rect;

char browser_panel_text[] = "Browser";
char browser_play_button_text[] = "Play";

const char *classics_names[] = {
    "01 - 10 (red)",
    "11 - 20 (blue)",
    "21 - 30 (green)",
    "31 - 40 (yellow)"
};
#define NUM_CLASSICS_NAMES (sizeof(classics_names)/sizeof(char *))

gui_list_vars_t classics = {
    .names        = classics_names,
    .count        = NUM_CLASSICS_NAMES,
    .scroll_index = -1,
    .active       = -1,
    .focus        = -1
};

extern char *nvdata_default_browse_path;

char *browse_path = NULL;
FilePathList browse_file_list;

gui_list_vars_t local_files = {
    .names        = NULL,
    .count        = 0,
    .scroll_index = -1,
    .active       = -1,
    .focus        = -1
};

char ext[] = "Reset Level Finished Data";

#define NUM_TABS 3
const char *browser_tabbar_text[NUM_TABS];

int active_tab = 0;

void setup_browse_dir(void)
{
    assert_not_null(browse_path);

    if (!DirectoryExists(browse_path)) {
        errmsg("Cannot browse \"%s\": not a directory", browse_path);
        return;
    }

    if (local_files.names) {
        FREE(local_files.names);
        UnloadDirectoryFiles(browse_file_list);
    };

    browse_file_list = LoadDirectoryFilesEx(browse_path, ext, false);
    local_files.names = calloc(browse_file_list.count, sizeof(char *));

    for (int i=0; i<(int)browse_file_list.count; i++) {
        char *p = browse_file_list.paths[i];
        local_files.names[i] = p;
        while (*p) {
            if (*p == '/') {
                local_files.names[i] = p + 1;
            }
            p++;
        }
    }

    local_files.scroll_index = -1;
    local_files.active       = -1;
    local_files.focus        = -1;
}

void change_gui_browser_path(char *dir)
{
    SAFEFREE(browse_path);

    browse_path = strdup(dir);
    setup_browse_dir();
}

void init_gui_browser(void)
{
    browser_tabbar_text[0] = "Clsssics";
    browser_tabbar_text[1] = "Local Level Files";
    browser_tabbar_text[2] = "Add File";

    assert_not_null(nvdata_default_browse_path);
    change_gui_browser_path(nvdata_default_browse_path);

    resize_gui_browser();
}

void resize_gui_browser(void)
{
    browser_panel_rect.width  = window_size.x * 0.7;
    browser_panel_rect.height = window_size.y * 0.8;

    MINVAR(browser_panel_rect.width,  400);
    MINVAR(browser_panel_rect.height, 450);

    browser_panel_rect.x = (window_size.x / 2) - (browser_panel_rect.width  / 2);
    browser_panel_rect.y = (window_size.y / 2) - (browser_panel_rect.height / 2);

    float panel_bottom = browser_panel_rect.y + browser_panel_rect.height;

    browser_tabbar_rect.width  = browser_panel_rect.width - (2 * PANEL_INNER_MARGIN);
    browser_tabbar_rect.height = TOOL_BUTTON_HEIGHT;
    browser_tabbar_rect.x = browser_panel_rect.x + PANEL_INNER_MARGIN;
    browser_tabbar_rect.y = browser_panel_rect.y + PANEL_INNER_MARGIN + TOOL_BUTTON_HEIGHT;

    browser_area_rect.x      = browser_tabbar_rect.x + PANEL_INNER_MARGIN;
    browser_area_rect.y      = browser_tabbar_rect.y + browser_tabbar_rect.height + (2 * RAYGUI_ICON_SIZE);
    browser_area_rect.width  = browser_tabbar_rect.width - (2 * PANEL_INNER_MARGIN);
    browser_area_rect.height = panel_bottom - (2 * RAYGUI_ICON_SIZE) - browser_area_rect.y;

    float area_bottom = browser_area_rect.y + browser_area_rect.height;

    browser_play_button_rect.x = browser_area_rect.x;
    browser_play_button_rect.y = area_bottom - TOOL_BUTTON_HEIGHT;
    browser_play_button_rect.width  = browser_area_rect.width;
    browser_play_button_rect.height = TOOL_BUTTON_HEIGHT;    \

    area_bottom -= browser_play_button_rect.height;
    area_bottom -= RAYGUI_ICON_SIZE;

    browser_list_rect.x      = browser_area_rect.x;
    browser_list_rect.y      = browser_area_rect.y;
    browser_list_rect.width  = browser_area_rect.width;
    browser_list_rect.height = area_bottom - browser_list_rect.y;
}

int draw_gui_browser_list(gui_list_vars_t *list)
{
    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    GuiListViewEx(browser_list_rect,
                  list->names,
                  list->count,
                  &list->scroll_index,
                  &list->active,
                  &list->focus);


    if (GuiButton(browser_play_button_rect, browser_play_button_text)) {
        if (list->active >= 0 && list->active < list->count) {
            return list->active;
        }
    }

    return -1;
}

void draw_gui_browser_classics(void)
{
    int selected = draw_gui_browser_list(&classics);

    if (selected > -1) {
        open_classics_game_pack(selected);
    }
}

void draw_gui_browser_local_level_filee(void)
{
    int selected = draw_gui_browser_list(&local_files);

    if (selected > -1) {
        char *path = browse_file_list.paths[selected];
        open_game_file(path);
    }
}

void draw_gui_browser_add_filee(void)
{
}

void draw_gui_browser(void)
{
    GuiPanel(browser_panel_rect, browser_panel_text);

    int old_active_tab = active_tab;
    GuiSimpleTabBar(browser_tabbar_rect, browser_tabbar_text, 2, &active_tab);

    if (old_active_tab != active_tab) {
        switch (active_tab) {
        case 0:
            draw_gui_browser_classics();
            break;

        case 1:
            draw_gui_browser_local_level_filee();
            break;

        case 2:
            draw_gui_browser_add_filee();
            break;
        }
    }
}

