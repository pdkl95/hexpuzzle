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
#include "level.h"
#include "collection.h"
#include "gui_browser.h"

extern char *home_dir;

void open_game_file(const char *path);
void open_classics_game_pack(int n);
bool draw_level_preview(level_t *level, Rectangle bounds);

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

    Rectangle *list_rect;
    Rectangle *list_rect_preview;
    Rectangle *btn_rect;
    Rectangle *btn_rect_preview;

    int scroll_index;
    int active;
    int focus;
};
typedef struct gui_list_vars gui_list_vars_t;

Rectangle browser_panel_rect;
Rectangle browser_preview_rect;
Rectangle browser_tabbar_rect;
Rectangle browser_area_rect;
Rectangle browser_list_rect;
Rectangle browser_play_button_rect;
Rectangle browser_list_with_preview_rect;
Rectangle browser_play_button_with_preview_rect;
Rectangle local_files_list_rect;
Rectangle local_files_list_with_preview_rect;
Rectangle local_files_dir_label_rect;
Rectangle local_files_dir_rect;
Rectangle local_files_up_button_rect;
Rectangle local_files_home_button_rect;
Rectangle local_files_local_saved_levels_button_rect;

char browser_panel_text[] = "Browse Levels";
char browser_play_button_text[] = "Play";
char browser_open_button_text[] = "Open";

char local_files_dir_label_text[] = "Directory";
char local_files_up_button_text_str[] = "Up";
char local_files_local_saved_levels_button_text_str[] = "Local Saved Levels";
char local_files_home_button_text_str[] = "Home";

char *local_files_up_button_text = NULL;
char *local_files_home_button_text = NULL;
char *local_files_local_saved_levels_button_text = NULL;

gui_list_entry_t classics_entries[] = {
    { .name = "01 - 10 (red)",    .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL },
    { .name = "11 - 20 (blue)",   .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL },
    { .name = "21 - 30 (green)",  .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL },
    { .name = "31 - 40 (yellow)", .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL }
};
#define NUM_CLASSICS_NAMES (sizeof(classics_entries)/sizeof(gui_list_entry_t))

gui_list_vars_t classics = {
    .names             = NULL,
    .entries           = classics_entries,
    .count             = NUM_CLASSICS_NAMES,
    .list_rect         = &browser_list_rect,
    .list_rect_preview = &browser_list_with_preview_rect,
    .btn_rect          = &browser_play_button_rect,
    .btn_rect_preview  = &browser_play_button_with_preview_rect,
    .scroll_index      = -1,
    .active            = -1,
    .focus             = -1
};

extern char *nvdata_default_browse_path;

char *browse_path = NULL;
FilePathList browse_file_list;

gui_list_vars_t local_files = {
    .names             = NULL,
    .entries           = NULL,
    .count             = 0,
    .list_rect         = &local_files_list_rect,
    .list_rect_preview = &local_files_list_with_preview_rect,
    .btn_rect          = &browser_play_button_rect,
    .btn_rect_preview  = &browser_play_button_with_preview_rect,
    .scroll_index      = -1,
    .active            = -1,
    .focus             = -1
};

#define NUM_TABS 3
const char *browser_tabbar_text[NUM_TABS];

int active_tab = 0;

level_t *browse_preview_level;

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
    if (home_dir) {
        if (DirectoryExists(home_dir)) {
            change_gui_browser_path(home_dir);
        }
    }
}

void change_gui_browser_path_to_local_saved_levels(void)
{
    assert_not_null(nvdata_default_browse_path);
    change_gui_browser_path(nvdata_default_browse_path);
}

void open_entry(gui_list_entry_t *entry)
{
    switch (entry->type) {
    case ENTRY_TYPE_DIR:
        if (DirectoryExists(entry->path)) {
            change_gui_browser_path(entry->path);
        }
        break;

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
    browser_tabbar_text[1] = "Local Files";
    browser_tabbar_text[2] = "Add File";

    change_gui_browser_path_to_local_saved_levels();

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

    SAFEFREE(local_files_up_button_text);
    SAFEFREE(local_files_home_button_text);
    SAFEFREE(local_files_local_saved_levels_button_text);

    SAFEFREE(browse_path);
}

void resize_gui_browser(void)
{
    browser_panel_rect = main_gui_area_rect;

    browser_preview_rect.width  = BROWSER_LEVEL_PREVIEW_SIZE;
    browser_preview_rect.height = BROWSER_LEVEL_PREVIEW_SIZE;
    browser_preview_rect.x =
        main_gui_area_rect.x
        + main_gui_area_rect.width
        - browser_preview_rect.width
        - PANEL_INNER_MARGIN;
    browser_preview_rect.y =
        main_gui_area_rect.y
        + main_gui_area_rect.height
        - browser_preview_rect.height
        - PANEL_INNER_MARGIN;

    float panel_bottom = browser_panel_rect.y + browser_panel_rect.height;

    browser_tabbar_rect.width  = browser_panel_rect.width - (2 * PANEL_INNER_MARGIN);
    browser_tabbar_rect.height = TOOL_BUTTON_HEIGHT;
    browser_tabbar_rect.x = browser_panel_rect.x + PANEL_INNER_MARGIN;
    browser_tabbar_rect.y = browser_panel_rect.y + PANEL_INNER_MARGIN + TOOL_BUTTON_HEIGHT;

    browser_area_rect.x      = browser_tabbar_rect.x + PANEL_INNER_MARGIN;
    browser_area_rect.y      = browser_tabbar_rect.y + browser_tabbar_rect.height + PANEL_INNER_MARGIN;
    browser_area_rect.width  = browser_tabbar_rect.width - (2 * PANEL_INNER_MARGIN);
    browser_area_rect.height = panel_bottom - (2 * PANEL_INNER_MARGIN) - browser_area_rect.y;

    float area_bottom = browser_area_rect.y + browser_area_rect.height;

    Vector2 browser_play_button_text_size = measure_big_button_text(browser_play_button_text);
    Vector2 browser_open_button_text_size = measure_big_button_text(browser_open_button_text);

    Vector2 button_max_size = Vector2Max(browser_play_button_text_size,
                                         browser_open_button_text_size);

    browser_play_button_rect.height = button_max_size.y + (5 * BUTTON_MARGIN);
    browser_play_button_rect.x = browser_area_rect.x;
    browser_play_button_rect.y = area_bottom - browser_play_button_rect.height;
    browser_play_button_rect.width  = browser_area_rect.width;

    browser_play_button_with_preview_rect = browser_play_button_rect;
    browser_play_button_with_preview_rect.width -= browser_preview_rect.width + (2 * PANEL_INNER_MARGIN);

    area_bottom -= browser_play_button_rect.height;
    area_bottom -= 2 * PANEL_INNER_MARGIN;

    browser_list_rect.x      = browser_area_rect.x;
    browser_list_rect.y      = browser_area_rect.y + PANEL_INNER_MARGIN;
    browser_list_rect.width  = browser_area_rect.width;
    browser_list_rect.height = area_bottom - browser_list_rect.y;

    browser_list_with_preview_rect = browser_list_rect;
    browser_list_with_preview_rect.height -= browser_preview_rect.height + (2 * PANEL_INNER_MARGIN);

    Vector2 local_files_dir_label_text_size = measure_gui_text(local_files_dir_label_text);

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

    if (local_files_up_button_text) {
        FREE(local_files_up_button_text);
    }
    if (local_files_home_button_text) {
        FREE(local_files_home_button_text);
    }
    local_files_up_button_text   = strdup(GuiIconText(ICON_ARROW_LEFT, local_files_up_button_text_str));
    local_files_home_button_text = strdup(GuiIconText(ICON_HOUSE,      local_files_home_button_text_str));
    local_files_local_saved_levels_button_text = strdup(GuiIconText(ICON_FOLDER_FILE_OPEN, local_files_local_saved_levels_button_text_str));

    Vector2 local_files_up_button_text_size   = measure_gui_text(local_files_up_button_text);
    Vector2 local_files_home_button_text_size = measure_gui_text(local_files_home_button_text);
    Vector2 local_files_local_saved_levels_button_text_size = measure_gui_text(local_files_local_saved_levels_button_text);

    local_files_up_button_rect.x      = browser_area_rect.x;
    local_files_up_button_rect.y      = browser_area_rect.y;
    local_files_up_button_rect.width  = local_files_up_button_text_size.x;
    local_files_up_button_rect.height = TOOL_BUTTON_HEIGHT;

    local_files_home_button_rect.width  = local_files_home_button_text_size.x;
    local_files_home_button_rect.x      = browser_area_rect.x + browser_area_rect.width - local_files_home_button_rect.width;
    local_files_home_button_rect.y      = local_files_up_button_rect.y;
    local_files_home_button_rect.height = local_files_up_button_rect.height;

    local_files_local_saved_levels_button_rect.width  = local_files_local_saved_levels_button_text_size.x;
    local_files_local_saved_levels_button_rect.x      = local_files_home_button_rect.x - local_files_local_saved_levels_button_rect.width - RAYGUI_ICON_SIZE;
    local_files_local_saved_levels_button_rect.y      = local_files_up_button_rect.y;
    local_files_local_saved_levels_button_rect.height = local_files_up_button_rect.height;

    browser_area_rect.y      += local_files_up_button_rect.height + RAYGUI_ICON_SIZE;
    browser_area_rect.height -= local_files_up_button_rect.height + RAYGUI_ICON_SIZE;

    local_files_list_rect.x      = browser_area_rect.x;
    local_files_list_rect.y      = browser_area_rect.y;
    local_files_list_rect.width  = browser_area_rect.width;
    local_files_list_rect.height = area_bottom - local_files_list_rect.y;

    local_files_list_with_preview_rect = local_files_list_rect;
    local_files_list_with_preview_rect.height -= browser_preview_rect.height + (2 * PANEL_INNER_MARGIN);
}

void draw_gui_browser_list(gui_list_vars_t *list)
{
    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    int save_bg_color = GuiGetStyle(DEFAULT, BACKGROUND_COLOR);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, GuiGetStyle(STATUSBAR, BASE_COLOR_NORMAL));

    Rectangle *list_rect =
        browse_preview_level
        ? list->list_rect_preview
        : list->list_rect;

    GuiListViewEx(*list_rect,
                  (const char **)list->names,
                  list->count,
                  &list->scroll_index,
                  &list->active,
                  &list->focus);

    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, save_bg_color);
}

int draw_gui_browser_big_button(gui_list_vars_t *list, const char *button_text)
{
    int rv = -1;

    set_big_button_font();

    bool disable_button = list->active == -1;

    if (disable_button) {
        GuiDisable();
    }

    Rectangle *btn_rect =
        browse_preview_level
        ? list->btn_rect_preview
        : list->btn_rect;

    if (GuiButton(*btn_rect, button_text)) {
        if (list->active >= 0 && list->active < list->count) {
            rv = list->active;
            goto big_button_cleanup;
        }
    }

    if (mouse_left_doubleclick) {
        if (list->active >= 0 && list->active < list->count) {
            rv = list->active;
            goto big_button_cleanup;
        }
    }

  big_button_cleanup:
    if (disable_button) {
        GuiEnable();
    }

    set_default_font();
    return rv;
}

void draw_gui_browser_classics(void)
{
    draw_gui_browser_list(&classics);

    int selected = draw_gui_browser_big_button(&classics, browser_play_button_text);

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

    if (GuiButton(local_files_local_saved_levels_button_rect, local_files_local_saved_levels_button_text)) {
        change_gui_browser_path_to_local_saved_levels();
    }

    if (GuiButton(local_files_home_button_rect, local_files_home_button_text)) {
        change_gui_browser_path_to_home();
    }

    draw_gui_browser_list(&local_files);

    char *button_text =  browser_play_button_text;
    gui_list_entry_t *entry = &(local_files.entries[local_files.active]);

    switch (entry->type) {
    case ENTRY_TYPE_DIR:
        button_text = browser_open_button_text;
        break;

    case ENTRY_TYPE_LEVEL_FILE:
        /* fall through */
    case ENTRY_TYPE_COLLECTION_FILE:
        /* fall through */
    case ENTRY_TYPE_COLLECTION_DIR:
        break;

    default:
        break;
    }

    int selected = draw_gui_browser_big_button(&local_files, button_text);

    if (selected > -1) {
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

    if (browse_preview_level) {
        draw_level_preview(browse_preview_level, browser_preview_rect);
    }
}

