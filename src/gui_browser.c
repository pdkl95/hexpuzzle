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
#include "options.h"
#include "collection.h"
#include "gui_browser.h"

void open_game_file(const char *path);
void open_classics_game_pack(int n);

enum gui_list_entry_type {
    ENTRY_TYPE_NULL = 0,
    ENTRY_TYPE_DIR,
    ENTRY_TYPE_LEVEL_FILE,
    ENTRY_TYPE_COLLECTION_FILE,
    ENTRY_TYPE_COLLECTION_DIR
};
typedef enum gui_list_entry_type gui_list_entry_type_t;

struct gui_list_entry {
    const char *name;
    const char *path;

    int icon;

    char *icon_name;

    gui_list_entry_type_t type;
};
typedef struct gui_list_entry gui_list_entry_t;

struct gui_list_vars {
    char **names;
    gui_list_entry_t *entries;

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
Rectangle local_files_list_rect;
Rectangle local_files_dir_label_rect;
Rectangle local_files_dir_rect;
Rectangle local_files_up_button_rect;
Rectangle local_files_home_button_rect;

char browser_panel_text[] = "Browser";
char browser_play_button_text[] = "Play";

char local_files_dir_label_text[] = "Directory";
char local_files_up_button_text_str[] = "Up";
char local_files_home_button_text_str[] = "Home";

char *local_files_up_button_text = NULL;
char *local_files_home_button_text = NULL;

gui_list_entry_t classics_entries[] = {
    { .name = "01 - 10 (red)",    .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL },
    { .name = "11 - 20 (blue)",   .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL },
    { .name = "21 - 30 (green)",  .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL },
    { .name = "31 - 40 (yellow)", .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL }
};
#define NUM_CLASSICS_NAMES (sizeof(classics_entries)/sizeof(gui_list_entry_t))

gui_list_vars_t classics = {
    .names        = NULL,
    .entries      = classics_entries,
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
    .entries      = NULL,
    .count        = 0,
    .scroll_index = -1,
    .active       = -1,
    .focus        = -1
};

#define NUM_TABS 3
const char *browser_tabbar_text[NUM_TABS];

int active_tab = 0;

const char *entry_type_str(gui_list_entry_type_t type)
{
    switch (type) {
    case ENTRY_TYPE_DIR:
        return "DIR";
    case ENTRY_TYPE_LEVEL_FILE:
        return "LEVEL_FILE";
    case ENTRY_TYPE_COLLECTION_FILE:
        return "COLLECTION_FILE";
    case ENTRY_TYPE_COLLECTION_DIR:
        return "COLLECTION_DIR";
    default:
        return "NULL";
    }
}

#if defined(PLATFORM_DESKTOP)
static void free_local_files_data(void)
{
    for (int i=0; i<local_files.count; i++) {
        SAFEFREE(local_files.entries[i].icon_name);
    }

    SAFEFREE(local_files.names);
    SAFEFREE(local_files.entries);

    UnloadDirectoryFiles(browse_file_list);
}
#endif
int compare_entries(const void *p1, const void *p2)
{
    gui_list_entry_t *e1 = (gui_list_entry_t *)p1;
    gui_list_entry_t *e2 = (gui_list_entry_t *)p2;

    assert_not_null(e1);
    assert_not_null(e2);
    assert_not_null(e1->name);
    assert_not_null(e1->name);

    int rv = e1->icon - e2->icon;
    if (rv) { return rv; }

    return strcmp(e1->name, e2->name);
}

static void prepare_gui_list_names(gui_list_vars_t *list)
{
    assert_not_null(list);
    assert_not_null(list->entries);
    assert(list->count >= 0);

    SAFEFREE(list->names);
    list->names = calloc(list->count, sizeof(char *));

    for (int i=0; i<list->count; i++) {
        gui_list_entry_t *entry = &(list->entries[i]);
        const char *icon_name = GuiIconText(entry->icon, entry->name);
        entry->icon_name = strdup(icon_name);
    }

    qsort(list->entries, list->count, sizeof(gui_list_entry_t), compare_entries);

    for (int i=0; i<list->count; i++) {
        gui_list_entry_t *entry = &(list->entries[i]);
        list->names[i] = entry->icon_name;
    }
}

#if defined(PLATFORM_DESKTOP)
static bool should_skip_file(const char *name)
{
    // skip hidden files
    if (name[0] == '.') {
        return true;
    }

    return false;
}

void setup_browse_dir(void)
{
    assert_not_null(browse_path);

    if (!DirectoryExists(browse_path)) {
        errmsg("Cannot browse \"%s\": not a directory", browse_path);
        return;
    }

    int len = strlen(browse_path);
    assert(len > 0);
    int last = len - 1;
    if ((last > 0) && ((browse_path[last] == '\\') || (browse_path[last] == '/'))) {
        browse_path[last] = '\0';
    }

    if (local_files.names) {
        free_local_files_data();
    }

    if (options->verbose) {
        infomsg("Scanning directory: \"%s\"", browse_path);
    }

    browse_file_list = LoadDirectoryFilesEx(browse_path, NULL, false);
    local_files.entries = calloc(browse_file_list.count, sizeof(gui_list_entry_t));

    local_files.count = 0;
    int icon = ICON_NONE;
    for (int i=0; i<(int)browse_file_list.count; i++) {
        gui_list_entry_t *entry = &(local_files.entries[local_files.count]);
        char *path = browse_file_list.paths[i];
        const char *name = GetFileName(path);

        if (should_skip_file(name)) {
            if (options->verbose) {
                infomsg("SCAN> %s (SKIP, FLAGGED)", name);
            }
            continue;
        }

        char *fd;
        gui_list_entry_type_t type = ENTRY_TYPE_NULL;
        if (IsPathFile(path)) {
            fd = "FILE";

            if (IsFileExtension(name, "." COLLECTION_FILENAME_EXT)) {
                type = ENTRY_TYPE_COLLECTION_FILE;
                icon = ICON_SUITCASE;
            } else if (IsFileExtension(name, "." LEVEL_FILENAME_EXT)) {
                type = ENTRY_TYPE_LEVEL_FILE;
                icon = ICON_FILE;
            } else {
                if (options->verbose) {
                    infomsg("SCAN> %s (SKIP, UNKNOWN EXT)", name);
                }
                continue;
            }
        } else {
            fd = "DIR";

            const char *index_file = TextFormat("%s/%s", path, COLLECTION_ZIP_INDEX_FILENAME);
            if (FileExists(index_file)) {
                type = ENTRY_TYPE_COLLECTION_DIR;
                icon = ICON_SUITCASE;
            } else {
                type = ENTRY_TYPE_DIR;
                icon = ICON_FOLDER;
            }
        }

        if (options->verbose) {
            infomsg("SCAN> %s (%s, %s)", name, fd, entry_type_str(type));
        }

        entry->name = name;
        entry->path = path;
        entry->icon = icon;
        entry->type = type;

        local_files.count++;
    }

    local_files.scroll_index = -1;
    local_files.active       = -1;
    local_files.focus        = -1;

    prepare_gui_list_names(&local_files);
}

void change_gui_browser_path(const char *dir)
{
    SAFEFREE(browse_path);

    browse_path = strdup(dir);
    setup_browse_dir();
}

void change_gui_browser_path_up(void)
{
    if (!browse_path) {
        return;
    }

    const char *parent = GetPrevDirectoryPath(browse_path);
    change_gui_browser_path(parent);
}

void change_gui_browser_path_to_home(void)
{
    assert_not_null(nvdata_default_browse_path);
    change_gui_browser_path(nvdata_default_browse_path);
}

void open_entry(gui_list_entry_t *entry)
{
    switch (entry->type) {
    /* case ENTRY_TYPE_DIR: */
    /*     if (DirectoryExists(entry->path)) { */
    /*         change_gui_browser_path(entry->path); */
    /*     } */
    /*     break; */

    case ENTRY_TYPE_COLLECTION_DIR:
        /* fall through */
    case ENTRY_TYPE_LEVEL_FILE:
        /* fall through */
    case ENTRY_TYPE_COLLECTION_FILE:
        if (FileExists(entry->path)) {
            open_game_file(entry->path);
        }
        break;

    default:
        errmsg("Cannot open \"%s\": NULL entr6y type.", entry->path);
    }
}
#endif

void init_gui_browser(void)
{
    browser_tabbar_text[0] = "Classics";
#if defined(PLATFORM_DESKTOP)
    browser_tabbar_text[1] = "Local Level Files";
    browser_tabbar_text[2] = "Add File";

    change_gui_browser_path_to_home();

#else
    browser_tabbar_text[1] = NULL;
    browser_tabbar_text[2] = NULL;
#endif

    prepare_gui_list_names(&classics);

    resize_gui_browser();
}

void cleanup_gui_browser(void)
{
#if defined(PLATFORM_DESKTOP)
    if (local_files.names) {
        free_local_files_data();
    }
#endif

    SAFEFREE(browse_path);
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

    Vector2 local_files_dir_label_text_size = MeasureGuiText(local_files_dir_label_text);

    local_files_dir_label_rect.x      = browser_area_rect.x;
    local_files_dir_label_rect.y      = browser_area_rect.y;
    local_files_dir_label_rect.width  = local_files_dir_label_text_size.x;
    local_files_dir_label_rect.height = TOOL_BUTTON_HEIGHT;

    local_files_dir_rect.x      = local_files_dir_label_rect.x + local_files_dir_label_rect.width + RAYGUI_ICON_SIZE;
    local_files_dir_rect.y      = local_files_dir_label_rect.y;
    local_files_dir_rect.width  = browser_area_rect.width - local_files_dir_label_rect.width - RAYGUI_ICON_SIZE;
    local_files_dir_rect.height = local_files_dir_label_rect.height;

    browser_area_rect.y      += local_files_dir_label_rect.height + RAYGUI_ICON_SIZE;
    browser_area_rect.height -= local_files_dir_label_rect.height + RAYGUI_ICON_SIZE;

    local_files_up_button_text   = strdup(GuiIconText(ICON_ARROW_LEFT, local_files_up_button_text_str));
    local_files_home_button_text = strdup(GuiIconText(ICON_HOUSE,      local_files_home_button_text_str));
    Vector2 local_files_up_button_text_size   = MeasureGuiText(local_files_up_button_text);
    Vector2 local_files_home_button_text_size = MeasureGuiText(local_files_home_button_text);

    local_files_up_button_rect.x      = browser_area_rect.x;
    local_files_up_button_rect.y      = browser_area_rect.y;
    local_files_up_button_rect.width  = local_files_up_button_text_size.x;
    local_files_up_button_rect.height = TOOL_BUTTON_HEIGHT;

    local_files_home_button_rect.width  = local_files_home_button_text_size.x;
    local_files_home_button_rect.x      = browser_area_rect.x + browser_area_rect.width - local_files_home_button_rect.width;
    local_files_home_button_rect.y      = local_files_up_button_rect.y;
    local_files_home_button_rect.height = local_files_up_button_rect.height;

    browser_area_rect.y      += local_files_up_button_rect.height + RAYGUI_ICON_SIZE;
    browser_area_rect.height -= local_files_up_button_rect.height + RAYGUI_ICON_SIZE;

    local_files_list_rect.x      = browser_area_rect.x;
    local_files_list_rect.y      = browser_area_rect.y;
    local_files_list_rect.width  = browser_area_rect.width;
    local_files_list_rect.height = area_bottom - local_files_list_rect.y;
}

int draw_gui_browser_list(gui_list_vars_t *list, Rectangle list_rect)
{
    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    GuiListViewEx(list_rect,
                  (const char **)list->names,
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
    int selected = draw_gui_browser_list(&classics, browser_list_rect);

    if (selected > -1) {
        open_classics_game_pack(selected + 1);
    }
}

#if defined(PLATFORM_DESKTOP)
void draw_gui_browser_local_level_file(void)
{
    GuiLabel(local_files_dir_label_rect, local_files_dir_label_text);
    GuiStatusBar(local_files_dir_rect, browse_path);

    if (GuiButton(local_files_up_button_rect, local_files_up_button_text)) {
        change_gui_browser_path_up();
    }

    if (GuiButton(local_files_home_button_rect, local_files_home_button_text)) {
        change_gui_browser_path_to_home();
    }

    int selected = draw_gui_browser_list(&local_files, local_files_list_rect);

    if (selected > -1) {
        gui_list_entry_t *entry = &(local_files.entries[selected]);
        if (entry) {
            open_entry(entry);
        }
    }
}
#endif

void draw_gui_browser(void)
{
    GuiPanel(browser_panel_rect, browser_panel_text);

    GuiSimpleTabBar(browser_tabbar_rect, browser_tabbar_text, 2, &active_tab);

    switch (active_tab) {
    case 0:
        draw_gui_browser_classics();
        break;

#if defined(PLATFORM_DESKTOP)
    case 1:
        draw_gui_browser_local_level_file();
        break;
#endif
    }
}

