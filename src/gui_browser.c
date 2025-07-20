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
#include "fonts.h"
#include "level.h"
#include "collection.h"
#include "raygui_paged_list.h"
#include "gui_browser.h"
#include "gui_dialog.h"
#include "nvdata_finished.h"

extern char *home_dir;

bool rebuild_history_browser = true;

bool open_game_file(const char *path, bool edit);
void open_classics_game_pack(int n);
bool open_blueprint(const char *blueprint);
bool draw_level_preview(level_t *level, Rectangle bounds);
void edit_new_blank_level(void);

enum gui_list_entry_type {
    ENTRY_TYPE_NULL = 0,
    ENTRY_TYPE_DIR,
    ENTRY_TYPE_LEVEL_FILE,
    ENTRY_TYPE_COLLECTION_FILE,
    ENTRY_TYPE_COLLECTION_DIR,
    ENTRY_TYPE_FINISHED_LEVEL
};
typedef enum gui_list_entry_type gui_list_entry_type_t;

enum gui_list_entry_status {
    ENTRY_STATUS_NULL = 0,
    ENTRY_STATUS_NOT_LOADABLE,
    ENTRY_STATUS_UNLOADED,
    ENTRY_STATUS_LOAD_OK,
    ENTRY_STATUS_LOAD_ERROR
};
typedef enum gui_list_entry_status gui_list_entry_status_t;

struct gui_list_entry {
    int index;

    gui_list_entry_type_t type;
    gui_list_entry_status_t status;

    level_t *level;
};
typedef struct gui_list_entry gui_list_entry_t;

struct gui_list_fspath_entry {
    gui_list_entry_t common;

    char *name;
    char *path;

    int icon;

    char *icon_name;
};
typedef struct gui_list_fspath_entry gui_list_fspath_entry_t;

Rectangle browser_panel_rect;
Rectangle browser_preview_rect;
Rectangle browser_tabbar_rect;
Rectangle browser_area_rect;
Rectangle browser_list_rect;
Rectangle browser_play_button_rect;
Rectangle browser_edit_button_rect;
Rectangle browser_list_with_preview_rect;
Rectangle browser_play_button_with_preview_rect;
Rectangle browser_edit_button_with_preview_rect;
Rectangle local_files_list_rect;
Rectangle local_files_list_with_preview_rect;
Rectangle local_files_dir_label_rect;
Rectangle local_files_dir_rect;
Rectangle local_files_up_button_rect;
Rectangle local_files_refresh_button_rect;
Rectangle local_files_home_button_rect;
Rectangle local_files_local_saved_levels_button_rect;
Rectangle local_files_new_button_rect;
Rectangle local_files_rename_button_rect;
Rectangle local_files_new_button_with_preview_rect;
Rectangle local_files_rename_button_with_preview_rect;
Rectangle history_list_rect;
Rectangle history_list_with_preview_rect;

char browser_panel_text[] = "Browse Levels";
char browser_play_button_text[] = "Play";
char browser_edit_button_text[] = "Edit";
char browser_open_button_text[] = "Open";
char browser_have_blueprint_tooltip_text[] = "Have level-regen Blueprint";
char browser_have_classic_tooltip_text[] = "Have Classic Level";

char local_files_dir_label_text[] = "Directory";
char local_files_up_button_text_str[] = "Up";
char local_files_refresh_button_text_str[] = "Refresh";
char local_files_local_saved_levels_button_text_str[] = "Local Saved Levels";
char local_files_home_button_text_str[] = "Home";
char local_files_new_button_text_str[] = "New";
char local_files_rename_button_text_str[] = "Rename";

char *local_files_up_button_text = NULL;
char *local_files_refresh_button_text = NULL;
char *local_files_home_button_text = NULL;
char *local_files_local_saved_levels_button_text = NULL;
char *local_files_new_button_text = NULL;
char *local_files_rename_button_text = NULL;

raygui_paged_list_t classics_gui_list;
raygui_paged_list_t local_files_gui_list;
raygui_paged_list_t local_files_gui_list_preview;
raygui_paged_list_t history_gui_list;
raygui_paged_list_t history_gui_list_preview;

gui_list_fspath_entry_t classics_entries[] = {
    { .name = "01 - 10 (red)",    .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL },
    { .name = "11 - 20 (blue)",   .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL },
    { .name = "21 - 30 (green)",  .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL },
    { .name = "31 - 40 (yellow)", .path = NULL, .icon = ICON_SUITCASE, .icon_name = NULL }
};
#define NUM_CLASSICS_NAMES NUM_ELEMENTS(gui_list_fspath_entry_t, classics_entries)

enum history_column {
    HISTORY_COLUMN_PLAY_TYPE = 0,
    HISTORY_COLUMN_NAME      = 1,
    HISTORY_COLUMN_WIN_DATE  = 2,
    HISTORY_COLUMN_TIME      = 3
};
typedef enum history_column history_column_t;

raygui_cell_header_t history_headers[] = {
    { .mode = RAYGUI_CELL_MODE_ICON,   // play type icon
      .text = "" },
    { .mode = RAYGUI_CELL_MODE_TEXT,
      .text = "Name/Seed" },
    { .mode = RAYGUI_CELL_MODE_TEXT,
      .text = "Win Date" },
    { .mode = RAYGUI_CELL_MODE_TEXT,
      .text = "Time" }
};
#define NUM_HISTORY_COLUMNS ((int)NUM_ELEMENTS(raygui_cell_header_t, history_headers))

struct gui_list_history_entry {
    gui_list_entry_t common;

    struct finished_level *finished_level;

    raygui_cell_t cells[NUM_HISTORY_COLUMNS];

    char name[NAME_MAXLEN];
    char win_time[NAME_MAXLEN];
    char elapsed_time[NAME_MAXLEN];
};
typedef struct gui_list_history_entry gui_list_history_entry_t;

struct gui_list_vars {
    char **names;
    gui_list_entry_t *entries;

    raygui_cell_header_t *history_headers;

    int count;

    Rectangle *list_rect;
    Rectangle *list_rect_preview;
    Rectangle *btn_rect;
    Rectangle *btn_rect_preview;
    Rectangle *edit_btn_rect;
    Rectangle *edit_btn_rect_preview;

    raygui_paged_list_t *gui_list;
    raygui_paged_list_t *gui_list_preview;

    int scroll_index;
    int active;
    int focus;
};
typedef struct gui_list_vars gui_list_vars_t;

gui_list_vars_t classics = {
    .names                 = NULL,
    .entries               = (gui_list_entry_t *)classics_entries,
    .history_headers       = NULL,
    .count                 = NUM_CLASSICS_NAMES,
    .list_rect             = &browser_list_rect,
    .list_rect_preview     = &browser_list_with_preview_rect,
    .btn_rect              = &browser_play_button_rect,
    .btn_rect_preview      = &browser_play_button_with_preview_rect,
    .edit_btn_rect         = &browser_edit_button_rect,
    .edit_btn_rect_preview = &browser_edit_button_with_preview_rect,
    .gui_list              = &classics_gui_list,
    .gui_list_preview      = NULL,
    .scroll_index          = -1,
    .active                = -1,
    .focus                 = -1
};

extern char *nvdata_default_browse_path;

char *browse_path = NULL;
FilePathList browse_file_list;

gui_list_vars_t local_files = {
    .names                 = NULL,
    .entries               = NULL,
    .history_headers       = NULL,
    .count                 = 0,
    .list_rect             = &local_files_list_rect,
    .list_rect_preview     = &local_files_list_with_preview_rect,
    .btn_rect              = &browser_play_button_rect,
    .btn_rect_preview      = &browser_play_button_with_preview_rect,
    .edit_btn_rect         = &browser_edit_button_rect,
    .edit_btn_rect_preview = &browser_edit_button_with_preview_rect,
    .gui_list              = &local_files_gui_list,
    .gui_list_preview      = &local_files_gui_list_preview,
    .scroll_index          = -1,
    .active                = -1,
    .focus                 = -1
};

gui_list_vars_t history = {
    .names                 = NULL,
    .entries               = NULL,
    .history_headers       = history_headers,
    .count                 = 0,
    .list_rect             = &history_list_rect,
    .list_rect_preview     = &history_list_with_preview_rect,
    .btn_rect              = &browser_play_button_rect,
    .btn_rect_preview      = &browser_play_button_with_preview_rect,
    .edit_btn_rect         = &browser_edit_button_rect,
    .edit_btn_rect_preview = &browser_edit_button_with_preview_rect,
    .gui_list              = &history_gui_list,
    .gui_list_preview      = &history_gui_list_preview,
    .scroll_index          = -1,
    .active                = -1,
    .focus                 = -1
};

#define NUM_TABS 3
const char *browser_tabbar_text[NUM_TABS];

int browser_active_tab = 0;

level_t *browse_preview_level = NULL;
bool defer_setup_browse_dir = false;
bool defer_setup_browse_history = false;

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
    case ENTRY_TYPE_FINISHED_LEVEL:
        return "FINISHED_LEVEL";
    default:
        return "NULL";
    }
}

const char *entry_status_str(gui_list_entry_status_t status)
{
    switch (status) {
    case ENTRY_STATUS_NOT_LOADABLE:
        return "NOT_LOADABLE";
    case ENTRY_STATUS_UNLOADED:
        return "UNLOADED";
    case ENTRY_STATUS_LOAD_OK:
        return "LOAD_OK";
    case ENTRY_STATUS_LOAD_ERROR:
        return "LOAD_ERROR";
    default:
        return "NULL";
    }
}

static inline gui_list_fspath_entry_t *get_gui_list_fspath_entry(gui_list_vars_t *list, int index)
{
    gui_list_fspath_entry_t *entries = (gui_list_fspath_entry_t *)list->entries;
    return &(entries[index]);
}

static inline gui_list_history_entry_t *get_gui_list_history_entry(gui_list_vars_t *list, int index)
{
    gui_list_history_entry_t *entries = (gui_list_history_entry_t *)list->entries;
    return &(entries[index]);
}

#if defined(PLATFORM_DESKTOP)
static void free_gui_list_vars_fspath_data(gui_list_vars_t *list)
{
    for (int i=0; i<list->count; i++) {
        gui_list_fspath_entry_t *e = get_gui_list_fspath_entry(list, i);
        if (e->common.level) {
            destroy_level(e->common.level);
            e->common.level = NULL;
        }

        SAFEFREE(e->icon_name);
        SAFEFREE(e->name);
        SAFEFREE(e->path);
    }

    SAFEFREE(list->names);
    SAFEFREE(list->entries);
}

static void free_gui_list_vars_history_data(gui_list_vars_t *list)
{
    for (int i=0; i<list->count; i++) {
        gui_list_history_entry_t *e = get_gui_list_history_entry(list, i);
        if (e->common.level) {
            destroy_level(e->common.level);
            e->common.level = NULL;
        }
    }

    SAFEFREE(list->names);
    SAFEFREE(list->entries);
}

static void free_local_files_data(void)
{
    free_gui_list_vars_fspath_data(&local_files);

    UnloadDirectoryFiles(browse_file_list);
}

static void free_history_data(void)
{
    free_gui_list_vars_history_data(&history);
}
#endif

int compare_fspath_entries(const void *p1, const void *p2)
{
    gui_list_fspath_entry_t *e1 = (gui_list_fspath_entry_t *)p1;
    gui_list_fspath_entry_t *e2 = (gui_list_fspath_entry_t *)p2;

    assert_not_null(e1);
    assert_not_null(e2);
    //assert_not_null(e1->name);
    //assert_not_null(e1->name);

    int rv = e1->icon - e2->icon;
    if (rv) { return rv; }

    if (!e1->name) { return -1; }
    if (!e2->name) { return  1; }

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
        gui_list_fspath_entry_t *entry = get_gui_list_fspath_entry(list, i);
        const char *icon_name = GuiIconText(entry->icon, entry->name);
        if (entry->common.status == ENTRY_STATUS_LOAD_ERROR) {
            icon_name = GuiIconText(ICON_CRACK, entry->name);
        }
        entry->icon_name = strdup(icon_name);
        entry->common.index = i;
    }

    gui_list_fspath_entry_t *entries = (gui_list_fspath_entry_t *)list->entries;
    qsort(entries, list->count, sizeof(gui_list_fspath_entry_t), compare_fspath_entries);

    for (int i=0; i<list->count; i++) {
        gui_list_fspath_entry_t *entry = get_gui_list_fspath_entry(list, i);
        list->names[i] = entry->icon_name;
    }

    if (list->gui_list) {
        raygui_paged_list_set_text(list->gui_list,
                                   (const char **)list->names,
                                   list->count);
    }

    if (list->gui_list_preview) {
        raygui_paged_list_set_text(list->gui_list_preview,
                                   (const char **)list->names,
                                   list->count);
    }
}

static void rebuild_list_names(void)
{
    prepare_gui_list_names(&local_files);
}

static void fail_entry(gui_list_entry_t *entry)
{
    entry->status = ENTRY_STATUS_LOAD_ERROR;
    rebuild_list_names();
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
    local_files.entries = calloc(browse_file_list.count, sizeof(gui_list_fspath_entry_t));

    local_files.count = 0;
    int icon = ICON_NONE;
    for (int i=0; i<(int)browse_file_list.count; i++) {
        gui_list_fspath_entry_t *entry = get_gui_list_fspath_entry(&local_files, local_files.count);
        assert(entry >= ((gui_list_fspath_entry_t *)(local_files.entries)));
        assert(entry < ((gui_list_fspath_entry_t *)(local_files.entries)) + sizeof(gui_list_fspath_entry_t));

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
        gui_list_entry_status_t status = ENTRY_STATUS_UNLOADED;
        if (IsPathFile(path)) {
            fd = "FILE";

            if (IsFileExtension(name, "." COLLECTION_FILENAME_EXT)) {
                type = ENTRY_TYPE_COLLECTION_FILE;
                icon = ICON_SUITCASE;
            } else if (IsFileExtension(name, "." LEVEL_FILENAME_EXT)) {
                type = ENTRY_TYPE_LEVEL_FILE;
                icon = ICON_FILE;
            } else {
                status = ENTRY_STATUS_NOT_LOADABLE;
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
                status = ENTRY_STATUS_NOT_LOADABLE;
            }
        }

        if (options->verbose) {
            infomsg("SCAN> %s (%s, %s, %s)", name, fd, entry_type_str(type), entry_status_str(status));
        }

        entry->name   = strdup(name);
        entry->path   = strdup(path);
        entry->icon   = icon;
        entry->common.type   = type;
        entry->common.status = status;

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

    if (!defer_setup_browse_dir) {
        setup_browse_dir();
    }
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

static void open_dir_entry(gui_list_fspath_entry_t *entry, UNUSED bool edit)
{
    if (DirectoryExists(entry->path)) {
        change_gui_browser_path(entry->path);
    }
}

static void open_file_entry(gui_list_fspath_entry_t *entry, bool edit)
{
    if (FileExists(entry->path)) {
        if (!open_game_file(entry->path, edit)) {
            fail_entry((gui_list_entry_t *)entry);
        }
    } else {
        fail_entry((gui_list_entry_t *)entry);
    }
}

static void open_history_entry(gui_list_history_entry_t *entry, UNUSED bool edit)
{
    if (finished_level_has_blueprint(entry->finished_level)) {
        if (!open_blueprint(entry->finished_level->blueprint)) {
            fail_entry((gui_list_entry_t *)entry);
        }
    } else if (finished_level_has_classic(entry->finished_level)) {
        level_t *level = find_classic_level_by_nameref(&entry->finished_level->classic_nameref);
        if (level) {
            level_play(level);
        } else {
            fail_entry((gui_list_entry_t *)entry);
        }
    } else if (finished_level_has_fspath(entry->finished_level)) {
        fail_entry((gui_list_entry_t *)entry);
    } else {
        fail_entry((gui_list_entry_t *)entry);
    }
}

void open_entry(gui_list_entry_t *entry, bool edit)
{
    switch (entry->status) {
    case ENTRY_STATUS_NULL:
        assert(false && "gui_list_entry_status_t should never be NULL");
        return;

    case ENTRY_STATUS_NOT_LOADABLE:
        if (entry->type == ENTRY_TYPE_FINISHED_LEVEL) {
            errmsg("Cannot open entry \"%s\" - no blueprint",
                   ((gui_list_history_entry_t *)entry)->name);
        } else {
            errmsg("Cannot open entry \"%s\" - unknown file type",
                   ((gui_list_fspath_entry_t *)entry)->path);
        }
        return;

    case ENTRY_STATUS_LOAD_ERROR:
        if (entry->type == ENTRY_TYPE_FINISHED_LEVEL) {
            errmsg("Cannot open entry \"%s\" - blueprint load error",
                   ((gui_list_history_entry_t *)entry)->name);
        } else {
            errmsg("Cannot open entry \"%s\" - load error",
                   ((gui_list_fspath_entry_t *)entry)->path);
        }
        return;

    default:
        /* do nothing */
        break;
    }

    switch (entry->type) {
    case ENTRY_TYPE_DIR:
        open_dir_entry((gui_list_fspath_entry_t *)entry, edit);
        break;

    case ENTRY_TYPE_COLLECTION_DIR:
        fallthrough;
    case ENTRY_TYPE_LEVEL_FILE:
        fallthrough;
    case ENTRY_TYPE_COLLECTION_FILE:
        open_file_entry((gui_list_fspath_entry_t *)entry, edit);
        break;

    case ENTRY_TYPE_FINISHED_LEVEL:
        open_history_entry((gui_list_history_entry_t *)entry, edit);
        break;

    default:
        errmsg("Cannot open entry #%d - NULL entry type.", entry->index);
    }
}

static level_t *entry_load_finished_level(gui_list_history_entry_t *entry)
{
    if (finished_level_has_blueprint(entry->finished_level)) {
        return generate_level_from_blueprint(entry->finished_level->blueprint, "browser_preview");
    } else if (finished_level_has_classic(entry->finished_level)) {
        return find_classic_level_by_nameref(&entry->finished_level->classic_nameref);
    } else if (finished_level_has_fspath(entry->finished_level)) {
        return NULL;
    } else {
        return NULL;
    }
}

static level_t *entry_load_level(gui_list_entry_t *entry)
{
    switch (entry->type) {
    case ENTRY_TYPE_DIR:
        fallthrough;
    case ENTRY_TYPE_COLLECTION_DIR:
        fallthrough;
    case ENTRY_TYPE_LEVEL_FILE:
        fallthrough;
    case ENTRY_TYPE_COLLECTION_FILE:
        return load_level_file(((gui_list_fspath_entry_t *)entry)->path);

    case ENTRY_TYPE_FINISHED_LEVEL:
        return entry_load_finished_level((gui_list_history_entry_t *)entry);

    default:
        errmsg("Cannot load level for entry #%d - NULL entry type.", entry->index);
    }
    return NULL;
}

void preview_entry(gui_list_entry_t *entry)
{
    assert_not_null(entry);

    switch (entry->type) {
    case ENTRY_TYPE_LEVEL_FILE:
        break;

    case ENTRY_TYPE_FINISHED_LEVEL:
        break;

    default:
        browse_preview_level = NULL;
        return;
    }

    switch (entry->status) {
    case ENTRY_STATUS_LOAD_ERROR:
        return;

    case ENTRY_STATUS_NOT_LOADABLE:
        browse_preview_level = NULL;
        return;

    default:
        /* do nothing */
        break;
    }

    if (!entry->level) {
        entry->level = entry_load_level(entry);

        if (!entry->level) {
            fail_entry(entry);
            return;
        }
    }

    browse_preview_level = entry->level;
}

void disable_preview(void)
{
    browse_preview_level = NULL;;
}

gui_list_fspath_entry_t *find_entry_by_filename(gui_list_vars_t *list, const char *filename)
{
    for (int i=0; i<list->count; i++) {
        gui_list_fspath_entry_t *entry = get_gui_list_fspath_entry(list, i);

        if (entry->common.type == ENTRY_TYPE_FINISHED_LEVEL) {
            continue;
        }

        if (0 == strcmp(((gui_list_fspath_entry_t *)entry)->name, filename)) {
            return entry;
        }
    }

    return NULL;
}

void select_entry_by_filename(gui_list_vars_t *list, const char *filename)
{
    gui_list_fspath_entry_t *entry = find_entry_by_filename(list, filename);
    if (entry) {
        list->active = entry->common.index;
    } else {
        list->active = -1;
    }
}

void gui_browser_new(UNUSED gui_list_fspath_entry_t *entry)
{
    edit_new_blank_level();
}

void gui_browser_rename_string_finished_cb(struct gui_dialog *dialog, void *data)
{
    if (!dialog->status) {
        return;
    }

    gui_list_fspath_entry_t *entry = (gui_list_fspath_entry_t *)data;

    char *oldname = strdup(concat_dir_and_filename(
                               browse_path,
                               GetFileName(entry->path)));

    if (!FileExists(oldname)) {
        popup_bug_message("File \"%s\" does not exist", oldname);
        goto rename_string_finished_cleanup_old_only;
    }

    if (strlen(dialog->string) < 1) {
        warnmsg("Cannot rename to a zero-length filename");
        goto rename_string_finished_cleanup_old_only;
    }

    char *newname = strdup(concat_dir_and_filename_and_ext(
                               browse_path,
                               dialog->string,
                               GetFileExtension(oldname)));

    if (0 == strcmp(oldname, newname)) {
        if (options->verbose) {
            warnmsg("Skipping rename - old and new names are the same");
        }
        goto rename_string_finished_cleanup;
    }

    if (FileExists(newname)) {
        popup_error_message("Cannot rename %s to %s: would clobber existing file",
                            entry->name, dialog->string);
        goto rename_string_finished_cleanup;
    }

    pstr(oldname);
    pstr(newname);

    if (-1 == rename(oldname, newname)) {
        popup_error_message("Rename error: %d", strerror(errno));
        setup_browse_dir();
        goto rename_string_finished_cleanup;
    }

    popup_message("Renamed \"%s\" to \"%s\"",
                  entry->name, dialog->string);

    setup_browse_dir();
    select_entry_by_filename(&local_files, GetFileName(newname));

  rename_string_finished_cleanup:
    FREE(newname);
  rename_string_finished_cleanup_old_only:
    FREE(oldname);
}

void gui_browser_rename(gui_list_fspath_entry_t *entry)
{
    switch (entry->common.type) {
        //case ENTRY_TYPE_COLLECTION_DIR:
        //fallthrough;
    case ENTRY_TYPE_LEVEL_FILE:
        fallthrough;
    case ENTRY_TYPE_COLLECTION_FILE:
        if (FileExists(entry->path)) {
            const char *filename = GetFileNameWithoutExt(entry->path);
            gui_dialog_ask_for_string("Rename",
                                      NULL,
                                      filename,
                                      gui_browser_rename_string_finished_cb,
                                      entry);
        }
        break;

    default:
        /* do nothing */
        break;
    }
}

void setup_browse_history(void)
{
    if (history.names) {
        free_history_data();
    }

    if (options->verbose) {
        infomsg("Scanning finished level history (count = %d)", finished_levels.count);
    }

    history.count = finished_levels.count;

    if (history.entries) {
        SAFEFREE(history.entries);
    }
    history.entries = calloc(finished_levels.count, sizeof(gui_list_history_entry_t));

    raygui_paged_list_alloc_column_cells(history.gui_list,         finished_levels.count);
    raygui_paged_list_alloc_column_cells(history.gui_list_preview, finished_levels.count);

    struct finished_level * fl = NULL;
    struct sglib_finished_level_iterator it;

    int count = 0;

    for(fl = sglib_finished_level_it_init_inorder(&it, finished_levels.tree);
        fl != NULL;
        fl = sglib_finished_level_it_next(&it), count++
    ) {
        gui_list_history_entry_t *entry  = get_gui_list_history_entry(&history, count);
        //cell_rows[count] = entry->cells;

        entry->finished_level = fl;
        assert_not_null(entry->finished_level);

        gui_list_entry_type_t type = ENTRY_TYPE_FINISHED_LEVEL;
        gui_list_entry_status_t status = ENTRY_STATUS_LOAD_OK;

        for (int col=0; col<NUM_HISTORY_COLUMNS; col++) {
            raygui_cell_header_t *header = &(history.history_headers[col]);
            raygui_cell_t *cell          = raygui_paged_list_get_cell(history.gui_list, count, col);
            raygui_cell_t *cell_preview  = raygui_paged_list_get_cell(history.gui_list_preview, count, col);

            cell->mode   = header->mode;
            cell->header = header;

            cell_preview->mode   = cell->mode;
            cell_preview->header = cell->header;

            if (col > -10) {
                raygui_cell_set_border_left(cell);
                raygui_cell_set_border_left(cell_preview);
            }

#if 0
            if (count > -10) {
                raygui_cell_set_border_top(cell);
                raygui_cell_set_border_top(cell_preview);
            }

            raygui_cell_set_border_bottom(cell);
            raygui_cell_set_border_bottom(cell_preview);
            raygui_cell_set_border_right(cell);
            raygui_cell_set_border_right(cell_preview);
#endif
            cell->border_color = GRAY;
            cell_preview->border_color = cell->border_color;;
        }

        {
            raygui_cell_t *cell          = raygui_paged_list_get_cell(history.gui_list,
                                                                      count,
                                                                      HISTORY_COLUMN_PLAY_TYPE);
            raygui_cell_t *cell_preview  = raygui_paged_list_get_cell(history.gui_list_preview,
                                                                      count,
                                                                      HISTORY_COLUMN_PLAY_TYPE);
            if (finished_level_has_blueprint(fl)) {
                cell->icon       = ICON_FILETYPE_TEXT;
                cell->icon_color = blueprint_color;

                raygui_cell_use_tooltip(cell, browser_have_blueprint_tooltip_text);
            } else if (finished_level_has_classic(fl)) {
                cell->icon       = ICON_FILE;
                cell->icon_color = GREEN;

                raygui_cell_use_tooltip(cell, browser_have_classic_tooltip_text);
            } else if (finished_level_has_fspath(fl)) {
            } else {
                status = ENTRY_STATUS_NOT_LOADABLE;

                cell->icon       = ICON_CROSS_SMALL;
                cell->icon_color = RED;

                raygui_cell_use_tooltip(cell, NULL);
            }

            cell_preview->icon       = cell->icon;
            cell_preview->icon_color = cell->icon_color;
        }

        {
            raygui_cell_t *cell          = raygui_paged_list_get_cell(history.gui_list,
                                                                      count,
                                                                      HISTORY_COLUMN_NAME);
            raygui_cell_t *cell_preview  = raygui_paged_list_get_cell(history.gui_list_preview,
                                                                      count,
                                                                      HISTORY_COLUMN_NAME);
            cell->text = entry->name;
            cell_preview->text = entry->name;

            if (finished_level_has_name(fl)) {
                snprintf(entry->name, NAME_MAXLEN, "%s", fl->name);
            } else {
                entry->name[0] = '\0';
            }
        }

        {
            raygui_cell_t *cell          = raygui_paged_list_get_cell(history.gui_list,
                                                                      count,
                                                                      HISTORY_COLUMN_WIN_DATE);
            raygui_cell_t *cell_preview  = raygui_paged_list_get_cell(history.gui_list_preview,
                                                                      count,
                                                                      HISTORY_COLUMN_WIN_DATE);
            cell->text         = entry->win_time;
            cell_preview->text = entry->win_time;

            if (finished_level_has_win_time(fl)) {
                struct tm *tm = gmtime(&(fl->win_time));
                strftime(entry->win_time, NAME_MAXLEN, "%Y-%b-%d %H:%M", tm);
            } else {
                entry->win_time[0] = '\0';
            }
        }

        {
            raygui_cell_t *cell          = raygui_paged_list_get_cell(history.gui_list,
                                                                      count,
                                                                      HISTORY_COLUMN_TIME);
            raygui_cell_t *cell_preview  = raygui_paged_list_get_cell(history.gui_list_preview,
                                                                      count,
                                                                      HISTORY_COLUMN_TIME);

            cell->text         = entry->elapsed_time;
            cell_preview->text = entry->elapsed_time;

            if (finished_level_has_elapsed_time(fl)) {
                snprintf(entry->elapsed_time, NAME_MAXLEN, "%s",
                         elapsed_time_parts_to_readable_string(&fl->elapsed_time));
            } else {
                entry->elapsed_time[0] = '\0';
            }
        }

        if (options->verbose) {
            infomsg("HIST[%d] \"%s\" \"%s\" \"%s\" (%s, %s)",
                    count,
                    entry->name,
                    entry->win_time,
                    entry->elapsed_time,
                    entry_type_str(type),
                    entry_status_str(status));
        }

        entry->common.type   = type;
        entry->common.status = status;
    }

    history.scroll_index = -1;
    history.active       = -1;
    history.focus        = -1;

    raygui_paged_list_resize(history.gui_list_preview, *history.list_rect_preview);
    raygui_paged_list_resize(history.gui_list,         *history.list_rect);
}
#endif

void init_gui_browser(void)
{
    defer_setup_browse_dir = true;
    defer_setup_browse_history = true;

    browser_tabbar_text[0] = "Classics";
#if defined(PLATFORM_DESKTOP)
    browser_tabbar_text[1] = "Local Files";
    browser_tabbar_text[2] = "History";

    init_raygui_paged_list(local_files.gui_list,
                           &(local_files.scroll_index),
                           &(local_files.active),
                           &(local_files.focus));

    init_raygui_paged_list(local_files.gui_list_preview,
                           &(local_files.scroll_index),
                           &(local_files.active),
                           &(local_files.focus));

    change_gui_browser_path_to_local_saved_levels();

    init_raygui_paged_list(history.gui_list,
                           &(history.scroll_index),
                           &(history.active),
                           &(history.focus));

    init_raygui_paged_list(history.gui_list_preview,
                           &(history.scroll_index),
                           &(history.active),
                           &(history.focus));

    raygui_paged_list_use_columns(history.gui_list,         history_headers, NUM_HISTORY_COLUMNS);
    raygui_paged_list_use_columns(history.gui_list_preview, history_headers, NUM_HISTORY_COLUMNS);
#else
    browser_tabbar_text[1] = NULL;
    browser_tabbar_text[2] = NULL;
#endif

    init_raygui_paged_list(classics.gui_list,
                           &(classics.scroll_index),
                           &(classics.active),
                           &(classics.focus));

    prepare_gui_list_names(&classics);

    resize_gui_browser();
}

void cleanup_gui_browser(void)
{
#if defined(PLATFORM_DESKTOP)
    cleanup_raygui_paged_list(history.gui_list_preview);
    cleanup_raygui_paged_list(history.gui_list);

    cleanup_raygui_paged_list(local_files.gui_list_preview);
    cleanup_raygui_paged_list(local_files.gui_list);

    if (history.names) {
        free_history_data();
    }

    if (local_files.names) {
        free_local_files_data();
    }
#endif

    cleanup_raygui_paged_list(classics.gui_list);

    SAFEFREE(local_files_refresh_button_text);
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
        - (2*PANEL_INNER_MARGIN);
    browser_preview_rect.y =
        main_gui_area_rect.y
        + main_gui_area_rect.height
        - browser_preview_rect.height
        - (2*PANEL_INNER_MARGIN);

    float panel_bottom = browser_panel_rect.y + browser_panel_rect.height;

    browser_tabbar_rect.width  = browser_panel_rect.width - (2 * PANEL_INNER_MARGIN);
    browser_tabbar_rect.height = TOOL_BUTTON_HEIGHT;
    browser_tabbar_rect.x = browser_panel_rect.x + PANEL_INNER_MARGIN;
    browser_tabbar_rect.y = browser_panel_rect.y + PANEL_INNER_MARGIN + TOOL_BUTTON_HEIGHT;

    browser_area_rect.x      = browser_tabbar_rect.x + PANEL_INNER_MARGIN;
    browser_area_rect.y      = browser_tabbar_rect.y + browser_tabbar_rect.height + PANEL_INNER_MARGIN;
    browser_area_rect.width  = browser_tabbar_rect.width - (2 * PANEL_INNER_MARGIN);
    browser_area_rect.height = panel_bottom - (2 * PANEL_INNER_MARGIN) - browser_area_rect.y;

    float high_area_rect_y = browser_area_rect.y;

    float area_bottom = browser_area_rect.y + browser_area_rect.height;

    Vector2 browser_play_button_text_size = measure_big_button_text(browser_play_button_text);
    Vector2 browser_open_button_text_size = measure_big_button_text(browser_open_button_text);
    Vector2 browser_edit_button_text_size = measure_big_button_text(browser_edit_button_text);

    Vector2 button_max_size = Vector2Max(browser_play_button_text_size,
                                         browser_open_button_text_size);
    button_max_size = Vector2Max(button_max_size, browser_edit_button_text_size);

    browser_play_button_rect.height = button_max_size.y + (5 * BUTTON_MARGIN);
    browser_play_button_rect.x = browser_area_rect.x;
    browser_play_button_rect.y = area_bottom - browser_play_button_rect.height;
    browser_play_button_rect.width  = browser_area_rect.width;

    browser_play_button_with_preview_rect = browser_play_button_rect;
    browser_play_button_with_preview_rect.width -= browser_preview_rect.width + (2 * PANEL_INNER_MARGIN);

    if (options->allow_edit_mode) {
        browser_edit_button_rect              = browser_play_button_rect;
        browser_edit_button_with_preview_rect = browser_play_button_with_preview_rect;

        browser_edit_button_rect.width = browser_edit_button_text_size.x + (2 * PANEL_INNER_MARGIN);
        browser_edit_button_with_preview_rect.width = browser_edit_button_rect.width;

        browser_play_button_rect.width -= browser_edit_button_rect.width + PANEL_INNER_MARGIN;
        browser_play_button_with_preview_rect.width -= browser_edit_button_with_preview_rect.width + PANEL_INNER_MARGIN;

        browser_edit_button_rect.x              += browser_play_button_rect.width + PANEL_INNER_MARGIN;
        browser_edit_button_with_preview_rect.x += browser_play_button_with_preview_rect.width + PANEL_INNER_MARGIN;
    }

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
    local_files_dir_label_rect.width  = local_files_dir_label_text_size.x + PANEL_INNER_MARGIN;
    local_files_dir_label_rect.height = TOOL_BUTTON_HEIGHT;

    local_files_dir_rect.x      = local_files_dir_label_rect.x + local_files_dir_label_rect.width + RAYGUI_ICON_SIZE;
    local_files_dir_rect.y      = local_files_dir_label_rect.y;
    local_files_dir_rect.width  = browser_area_rect.width - local_files_dir_label_rect.width - RAYGUI_ICON_SIZE;
    local_files_dir_rect.height = local_files_dir_label_rect.height;

    browser_area_rect.y      += local_files_dir_label_rect.height + RAYGUI_ICON_SIZE;
    browser_area_rect.height -= local_files_dir_label_rect.height + RAYGUI_ICON_SIZE;

    if (local_files_refresh_button_text) {
        FREE(local_files_refresh_button_text);
    }
    if (local_files_up_button_text) {
        FREE(local_files_up_button_text);
    }
    if (local_files_home_button_text) {
        FREE(local_files_home_button_text);
    }
    if (local_files_local_saved_levels_button_text) {
        FREE(local_files_local_saved_levels_button_text);
    }
    if (local_files_new_button_text) {
        FREE(local_files_new_button_text);
    }
    if (local_files_rename_button_text) {
        FREE(local_files_rename_button_text);
    }
    local_files_refresh_button_text = strdup(GuiIconText(ICON_RESTART,    local_files_refresh_button_text_str));
    local_files_up_button_text      = strdup(GuiIconText(ICON_ARROW_LEFT, local_files_up_button_text_str));
    local_files_home_button_text    = strdup(GuiIconText(ICON_HOUSE,      local_files_home_button_text_str));
    local_files_local_saved_levels_button_text = strdup(GuiIconText(ICON_FOLDER_FILE_OPEN, local_files_local_saved_levels_button_text_str));
    local_files_new_button_text    = strdup(GuiIconText(ICON_FILE_ADD, local_files_new_button_text_str));
    local_files_rename_button_text = strdup(GuiIconText(ICON_PENCIL,   local_files_rename_button_text_str));

    Vector2 local_files_refresh_button_text_size = measure_gui_text(local_files_refresh_button_text);
    Vector2 local_files_up_button_text_size      = measure_gui_text(local_files_up_button_text);
    Vector2 local_files_home_button_text_size    = measure_gui_text(local_files_home_button_text);
    Vector2 local_files_local_saved_levels_button_text_size = measure_gui_text(local_files_local_saved_levels_button_text);

    local_files_up_button_rect.x      = browser_area_rect.x;
    local_files_up_button_rect.y      = browser_area_rect.y;
    local_files_up_button_rect.width  = local_files_up_button_text_size.x;
    local_files_up_button_rect.height = TOOL_BUTTON_HEIGHT;

    local_files_refresh_button_rect.x      = local_files_up_button_rect.x + local_files_up_button_rect.width + RAYGUI_ICON_SIZE;
    local_files_refresh_button_rect.y      = browser_area_rect.y;
    local_files_refresh_button_rect.width  = local_files_refresh_button_text_size.x;
    local_files_refresh_button_rect.height = TOOL_BUTTON_HEIGHT;

    local_files_home_button_rect.width  = local_files_home_button_text_size.x;
    local_files_home_button_rect.x      = browser_area_rect.x + browser_area_rect.width - local_files_home_button_rect.width;
    local_files_home_button_rect.y      = local_files_up_button_rect.y;
    local_files_home_button_rect.height = local_files_up_button_rect.height;
;
    local_files_local_saved_levels_button_rect.width  = local_files_local_saved_levels_button_text_size.x;
    local_files_local_saved_levels_button_rect.x      = local_files_home_button_rect.x - local_files_local_saved_levels_button_rect.width - RAYGUI_ICON_SIZE;
    local_files_local_saved_levels_button_rect.y      = local_files_up_button_rect.y;
    local_files_local_saved_levels_button_rect.height = local_files_up_button_rect.height;

    browser_area_rect.y      += local_files_up_button_rect.height + RAYGUI_ICON_SIZE;
    browser_area_rect.height -= local_files_up_button_rect.height + RAYGUI_ICON_SIZE;

    Vector2 local_files_new_button_text_size = measure_gui_text(local_files_new_button_text);
    Vector2 local_files_rename_button_text_size = measure_gui_text(local_files_rename_button_text);

    local_files_new_button_rect.width  = local_files_new_button_text_size.x;
    local_files_new_button_rect.height = TOOL_BUTTON_HEIGHT;
    local_files_rename_button_rect.width  = local_files_rename_button_text_size.x;
    local_files_rename_button_rect.height = TOOL_BUTTON_HEIGHT;

    local_files_new_button_rect.x    = browser_area_rect.x + browser_area_rect.width - local_files_new_button_rect.width;
    local_files_rename_button_rect.x = browser_area_rect.x;

    local_files_new_button_rect.y    = area_bottom - local_files_new_button_rect.height;
    local_files_rename_button_rect.y = local_files_new_button_rect.y;

    local_files_new_button_with_preview_rect = local_files_new_button_rect;
    local_files_rename_button_with_preview_rect = local_files_rename_button_rect;

    local_files_new_button_with_preview_rect.x -= browser_preview_rect.width + (2 * PANEL_INNER_MARGIN);
    local_files_new_button_with_preview_rect.y = browser_preview_rect.y;
    local_files_rename_button_with_preview_rect.y = browser_preview_rect.y;

    area_bottom -= local_files_new_button_rect.height;
    area_bottom -= PANEL_INNER_MARGIN;

    local_files_list_rect.x      = browser_area_rect.x;
    local_files_list_rect.y      = browser_area_rect.y;
    local_files_list_rect.width  = browser_area_rect.width;
    local_files_list_rect.height = area_bottom - local_files_list_rect.y;

    local_files_list_with_preview_rect = local_files_list_rect;
    float list_preview_delta =
        local_files_list_rect.y
        + local_files_list_rect.height
        - browser_preview_rect.y
        + PANEL_INNER_MARGIN;
    local_files_list_with_preview_rect.height -= list_preview_delta;

    history_list_rect              = local_files_list_rect;
    history_list_with_preview_rect = local_files_list_with_preview_rect;

    float upper_extra = history_list_rect.y - high_area_rect_y - PANEL_INNER_MARGIN;
    history_list_rect.y                   -= upper_extra;
    history_list_rect.height              += upper_extra;
    history_list_with_preview_rect.y      -= upper_extra;
    history_list_with_preview_rect.height += upper_extra;

    history_list_rect.y                   -= 1.0f;
    history_list_with_preview_rect.y      -= 1.0f;

    if (main_gui_area_rect.width < 1.0) {
        return;
    }

    raygui_paged_list_resize(classics.gui_list, *classics.list_rect);

#if defined(PLATFORM_DESKTOP)
    raygui_paged_list_resize(local_files.gui_list_preview, *local_files.list_rect_preview);
    raygui_paged_list_resize(local_files.gui_list,         *local_files.list_rect);

    char ex_name[]     = "12345678901234567890";
    char ex_win_date[] = "2000-JAN-30 12:34";

    Vector2 ex_name_size     = measure_gui_text(ex_name);
    Vector2 ex_win_date_size = measure_gui_text(ex_win_date);

    float text_padding = GuiGetStyle(DEFAULT, TEXT_PADDING);
    float top_bottom_padding = text_padding + GuiGetStyle(STATUSBAR, BORDER_WIDTH);

    for (int col=0; col<NUM_HISTORY_COLUMNS; col++) {
        raygui_cell_header_t *header = &(history.history_headers[col]);

        header->padding.left   = text_padding + 4.0f;
        header->padding.right  = text_padding;
        header->padding.top    = top_bottom_padding + 1.0f;
        header->padding.bottom = top_bottom_padding;
    }

    history_headers[HISTORY_COLUMN_PLAY_TYPE].width = RAYGUI_ICON_SIZE;
    history_headers[HISTORY_COLUMN_NAME     ].width = ex_name_size.x + 2.0;
    history_headers[HISTORY_COLUMN_WIN_DATE ].width = ex_win_date_size.x + 2.0;
    history_headers[HISTORY_COLUMN_TIME     ].width = -1.0;

    history_headers[HISTORY_COLUMN_PLAY_TYPE].padding.left -= 3.0f;

    raygui_paged_list_resize(history.gui_list_preview, *history.list_rect_preview);
    raygui_paged_list_resize(history.gui_list,         *history.list_rect);
#endif

    local_files_new_button_rect.x -= raygui_paged_list_sidebar_width;
    local_files_home_button_rect.x -= raygui_paged_list_sidebar_width;
    local_files_local_saved_levels_button_rect.x -= raygui_paged_list_sidebar_width;
}

static inline raygui_paged_list_t *get_current_gui_list(gui_list_vars_t *list)
{
    raygui_paged_list_t *gui_list = list->gui_list;
    if (browse_preview_level && list->gui_list_preview) {
        gui_list = list->gui_list_preview;
    }
    assert_not_null(gui_list);
    return gui_list;
}


int draw_gui_browser_list(gui_list_vars_t *list)
{
    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    int save_bg_color = GuiGetStyle(DEFAULT, BACKGROUND_COLOR);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, GuiGetStyle(STATUSBAR, BASE_COLOR_NORMAL));

    raygui_paged_list_t *gui_list = get_current_gui_list(list);
    raygui_paged_list_draw(gui_list);

    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, save_bg_color);

    return list->active;
}

int draw_gui_browser_big_button(gui_list_vars_t *list, const char *button_text, bool edit)
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

    if (edit) {
        btn_rect = browse_preview_level
            ? list->edit_btn_rect_preview
            : list->edit_btn_rect;

    }

    if (GuiButton(*btn_rect, button_text)) {
        if (list->active >= 0 && list->active < list->count) {
            rv = list->active;
        }
    }

    if (disable_button) {
        GuiEnable();
    }

    set_default_font();
    return rv;
}

void draw_gui_browser_classics(void)
{
    draw_gui_browser_list(&classics);

    int selected = draw_gui_browser_big_button(&classics, browser_open_button_text, false);

    if (selected > -1) {
        open_classics_game_pack(selected + 1);
    }
}

#if defined(PLATFORM_DESKTOP)
static inline void draw_gui_browser_local_level_file_new_button(gui_list_fspath_entry_t *entry)
{
    Rectangle new_btn_rect = browse_preview_level
        ? local_files_new_button_with_preview_rect
        : local_files_new_button_rect;

    if (GuiButton(new_btn_rect, local_files_new_button_text)) {
        gui_browser_new(entry);
    }
}

static inline void draw_gui_browser_local_level_file_rename_button(gui_list_fspath_entry_t *entry)
{
    Rectangle rename_btn_rect = browse_preview_level
        ? local_files_rename_button_with_preview_rect
        : local_files_rename_button_rect;

    bool disable_rename_btn = local_files.active == -1;
    if (disable_rename_btn) {
        GuiDisable();
    }

    if (GuiButton(rename_btn_rect, local_files_rename_button_text)) {
        gui_browser_rename(entry);
    }

    if (disable_rename_btn) {
        GuiEnable();
    }
}

static void draw_gui_browser_entry_buttons(gui_list_vars_t *list, gui_list_entry_t *entry)
{
    int selected = -1;
    int edit_selected = -1;

    char *button_text =  browser_play_button_text;

    if (entry) {
        switch (entry->status) {
        case ENTRY_STATUS_NULL:
            fallthrough;
        case ENTRY_STATUS_NOT_LOADABLE:
            fallthrough;
        case ENTRY_STATUS_LOAD_ERROR:
            return;
        default:
            break;
        }
    }

    if (list->active >= 0) {
        switch (entry->type) {
        case ENTRY_TYPE_DIR:
            button_text = browser_open_button_text;
            break;

        case ENTRY_TYPE_LEVEL_FILE:
            fallthrough;
        case ENTRY_TYPE_COLLECTION_FILE:
            fallthrough;
        case ENTRY_TYPE_COLLECTION_DIR:
            break;

        default:
            break;
        }

        selected = draw_gui_browser_big_button(list, button_text, false);

        if  (options->allow_edit_mode) {
            edit_selected = draw_gui_browser_big_button(list, browser_edit_button_text, true);
        }
    }

    if (browse_preview_level) {
        if (draw_level_preview(browse_preview_level, browser_preview_rect)) {
            open_entry(entry, false);
            return;
        }
    }

    if (edit_selected > -1) {
        if (entry) {
            open_entry(entry, true);
        }
    } else if (selected > -1) {
        if (entry) {
            open_entry(entry, false);
        }
    }
}

void draw_gui_browser_local_level_file(void)
{
    if (defer_setup_browse_dir) {
        defer_setup_browse_dir = false;
        setup_browse_dir();
    }

    GuiLabel(local_files_dir_label_rect, local_files_dir_label_text);
    GuiStatusBar(local_files_dir_rect, browse_path);

    if (GuiButton(local_files_refresh_button_rect, local_files_refresh_button_text)) {
        setup_browse_dir();
    }

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

    gui_list_fspath_entry_t *entry = get_gui_list_fspath_entry(&local_files, local_files.active);

    level_t *old_browse_preview_level = browse_preview_level;

    if (local_files.active >= 0) {
        preview_entry((gui_list_entry_t *)entry);
    } else {
        disable_preview();
    }

    if ((old_browse_preview_level != browse_preview_level) &&
        local_files.active > -1) {
        raygui_paged_list_t *gui_list = get_current_gui_list(&local_files);
        raygui_paged_list_select_active_page(gui_list);
    }

    draw_gui_browser_local_level_file_new_button(entry);
    draw_gui_browser_local_level_file_rename_button(entry);

    draw_gui_browser_entry_buttons(&local_files, (gui_list_entry_t *)entry);
}

void draw_gui_browser_finished_levels_history(void)
{
    if (defer_setup_browse_history || rebuild_history_browser) {
        defer_setup_browse_history = false;
        rebuild_history_browser = false;
        setup_browse_history();
    }

    draw_gui_browser_list(&history);

    gui_list_history_entry_t *entry = NULL;

    level_t *old_browse_preview_level = browse_preview_level;

    if (history.active >= 0) {
        entry = get_gui_list_history_entry(&history, history.active);
        preview_entry((gui_list_entry_t *)entry);
    } else {
        disable_preview();
    }

    if ((old_browse_preview_level != browse_preview_level) &&
        history.active > -1) {
        raygui_paged_list_t *gui_list = get_current_gui_list(&history);
        raygui_paged_list_select_active_page(gui_list);
    }

    draw_gui_browser_entry_buttons(&history, (gui_list_entry_t *)entry);
}
#endif

void draw_gui_browser(void)
{
    GuiPanel(browser_panel_rect, browser_panel_text);

    GuiSimpleTabBar(browser_tabbar_rect, browser_tabbar_text, NUM_TABS, &browser_active_tab);

    switch (browser_active_tab) {
    case 0:
        draw_gui_browser_classics();
        break;

#if defined(PLATFORM_DESKTOP)
    case 1:
        draw_gui_browser_local_level_file();
        break;

    case 2:
        draw_gui_browser_finished_levels_history();
        break;
#endif
    }
}

void gui_browsee_reload(void)
{
    setup_browse_dir();
}
