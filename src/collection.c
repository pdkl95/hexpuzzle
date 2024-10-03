/****************************************************************************
 *                                                                          *
 * collection.c                                                             *
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

#include "raygui/raygui.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "sglib/sglib.h"

#include "options.h"
#include "raylib_helper.h"
#include "collection.h"

#include "zip-kuba/zip.h"

//#define INITIAL_LEVEL_NAME_COUNT 64
#define INITIAL_LEVEL_NAME_COUNT 4

void create_new_level(void);

static void collection_alloc_level_names(collection_t *collection)
{
    collection->level_names = calloc(collection->level_name_count, sizeof(char *));
}

static collection_t *alloc_collection(void)
{
    collection_t *collection = calloc(1, sizeof(collection_t));

    collection->level_name_count = INITIAL_LEVEL_NAME_COUNT;
    collection_alloc_level_names(collection);

    //collection->name[0] = '\0';

    collection->dirpath = NULL;
    collection->filename = NULL;
    collection->levels = NULL;
    collection->prev = NULL;
    collection->next = NULL;

    collection->filename_seq = 1;

    collection->level_count = 0;

    collection->gui_list_scroll_index = 0;
    collection->gui_list_active = -1;
    collection->gui_list_focus = -1;

    collection->changed = false;
    collection->is_zip = false;

    return collection;
}

collection_t *create_collection(void)
{
    collection_t *collection = alloc_collection();

    return collection;
}

void collection_clear_levels(collection_t *collection)
{
    assert_not_null(collection);

    if (collection->levels) {
        destroy_level(collection->levels);
        collection->levels = NULL;
    }
}

static int level_file_filter(UNUSED const struct dirent *file)
{
    const char *ext = filename_ext(file->d_name);
    return strcmp(ext, LEVEL_FILENAME_EXT) == 0;
}

void collection_scan_dir(collection_t *collection)
{
    assert_not_null(collection);
    assert_not_null(collection->dirpath);

    collection_clear_levels(collection);

    struct dirent **namelist;
    int n = scandir(collection->dirpath, &namelist, level_file_filter, versionsort);
    if (n == -1) {
        errmsg("cannot scan directory \"%s\": %s",
               collection->dirpath, strerror(errno));
        return;
    }

    while (n--) {
        char *filepath;
        asprintf(&filepath, "%s/%s",
                 collection->dirpath, namelist[n]->d_name);

        collection_add_level_file(collection, filepath);

        free(filepath);
        free(namelist[n]);
    }

    free(namelist);
}

collection_t *load_collection_dir(const char *dirpath)
{
    assert_not_null(dirpath);

    collection_t *collection = create_collection();
    collection->dirpath = strdup(dirpath);

    int last_idx = strlen(collection->dirpath);
    if (last_idx > 0) {
        last_idx--;
        if (collection->dirpath[last_idx] == '/') {
            collection->dirpath[last_idx] = '\0';
        }

        collection_scan_dir(collection);
    } else {
        errmsg("Directory name is zero-length!");
    }


    return collection;
}

collection_t *load_collection_level_file(const char *filename)
{
    collection_t *collection = create_collection();

    if (collection_add_level_file(collection, filename)) {
        return collection;
    } else {
        destroy_collection(collection);
        return NULL;
    }
}

collection_t *load_collection_zip_file(const char *filename)
{
    int errnum;

    collection_t *collection = create_collection();
    collection->filename = strdup(filename);

    struct zip_t *zip = zip_openwitherror(filename, 0, 'r', &errnum);
    if (NULL == zip) {
        errmsg("Cannot open level collection \"%s\" - %s", filename, zip_strerror(errnum));

      fail_collection:
        destroy_collection(collection);
        return NULL;
    } else {
        if (options->verbose) {
            infomsg("Decompressing and extracting level files from: \"%s\"", filename);
            infomsg("Reading filenames from \"%s\"", COLLECTION_ZIP_INDEX_FILENAME);
        }

        char *indexbuf = NULL;

        errnum = zip_entry_open(zip, COLLECTION_ZIP_INDEX_FILENAME);
        if (errnum < 0) {
            errmsg("Cannot open level collection index file \"%s\" - %s",
                   COLLECTION_ZIP_INDEX_FILENAME, zip_strerror(errnum));
            goto fail_collection;
        } else {
            size_t bufsize = zip_entry_size(zip);
            indexbuf = calloc(bufsize + 11, sizeof(char));
            errnum = zip_entry_noallocread(zip, (void *)indexbuf, bufsize);
            if (errnum < 0) {
                errmsg("Error reading the collection index file \"%s\" - %s",
                   COLLECTION_ZIP_INDEX_FILENAME, zip_strerror(errnum));

              fail_indexbuf_and_collection:
                free(indexbuf);
                goto fail_collection;
            }
            indexbuf[bufsize] = '\0';
        }
        errnum = zip_entry_close(zip);
        if (errnum < 0) {
            errmsg("Error closing the collection index file \"%s\" in collection \"%s\" - %s",
                   COLLECTION_ZIP_INDEX_FILENAME, collection->filename, zip_strerror(errnum));
            goto fail_indexbuf_and_collection;
        }

        char *delim = "\n";
        int n=0;
        char *filename = indexbuf, *end = indexbuf;
        while (filename != NULL) {
            strsep(&end, delim);

            if (filename && strlen(filename) > 0) {
                if (options->verbose) {
                    infomsg("Unpacking level file \"%s\"", filename);
                }

                char *levelbuf = NULL;

                errnum = zip_entry_open(zip, filename);
                if (errnum < 0) {
                    errmsg("Cannot open the level file \"%s\" in collection \"%s\" - %s",
                           filename, collection->filename, zip_strerror(errnum));
                    free(levelbuf);
                    continue;

                } else {
                    size_t bufsize = zip_entry_size(zip);
                    levelbuf = calloc(bufsize + 11, sizeof(char));
                    errnum = zip_entry_noallocread(zip, (void *)levelbuf,  bufsize);
                    if (errnum < 0) {
                        errmsg("Error reading the level file \"%s\" in collection \"%s\" - %s",
                               filename, collection->filename, zip_strerror(errnum));
                        free(levelbuf);
                        continue;
                    }
                    levelbuf[bufsize] = '\0';
                }
                errnum = zip_entry_close(zip);
                if (errnum < 0) {
                    errmsg("Error closing the level file \"%s\" in collection \"%s\" - %s",
                           filename, collection->filename, zip_strerror(errnum));
                    free(levelbuf);
                    continue;
                }

                level_t *level = load_level_string(filename, levelbuf, true);
                collection_add_level(collection, level);

                if (levelbuf) {
                    free(levelbuf);
                }

                n++;
            }

            filename = end;
        }

        free(indexbuf);
    }
    zip_close(zip);

    collection->is_zip = true;

    return collection;
}

collection_t *load_collection_file(const char *filename)
{
    assert_not_null(filename);

    const char *ext = filename_ext(filename);

    if (strcmp(ext, LEVEL_FILENAME_EXT) == 0) {
        return load_collection_level_file(filename);
    } else if (strcmp(ext, COLLECTION_FILENAME_EXT) == 0) {
        return load_collection_zip_file(filename);
    } else {
        errmsg("cannot open \"%s\" - don't understand .%s files",
               filename, ext);
        return NULL;
    }
}

collection_t *load_collection_path(const char *path)
{
    assert_not_null(path);

    struct stat sb;
    if (-1 == stat(path, &sb)) {
        errmsg("couldn't stst() the path \"%s\": %s",
               path, strerror(errno));
    }

    if (S_ISREG(sb.st_mode)) {
        return load_collection_file(path);
    } else if (S_ISDIR(sb.st_mode)) {
        return load_collection_dir(path);
    } else {
        errmsg("cannot openn \"%s\" - not a regular file or directory", path);
        return NULL;
    }
}

void destroy_collection(collection_t *collection)
{
    if (collection) {
        if (collection->next) {
            destroy_collection(collection->next);
        }

        if (collection->levels) {
            destroy_level(collection->levels);
        }

        SAFEFREE(collection->dirpath);
        SAFEFREE(collection->filename);
    }
}

static const char *collection_path(collection_t *collection)
{
    if (!collection->dirpath && !collection->filename) {
        return NULL;
    }
    if (collection->dirpath && !collection->filename) {
        return collection->dirpath;
    }
    if (!collection->dirpath && collection->filename) {
        return collection->filename;
    }

    return concat_dir_and_filename(collection->dirpath, collection->filename);
}

static const char *collection_name(collection_t *collection)
{
    const char *path = collection_path(collection);
    if (path) {
        return path;
    } else {
        return "(untitled)";
    }
}

static bool collection_level_filename_exists_in_collection(collection_t *collection, const char *name)
{
    assert_not_null(collection);
    assert_not_null(name);
    SGLIB_DL_LIST_MAP_ON_ELEMENTS(level_t, collection->levels, level, prev, next, {
            if (level->filename) {
                if (0 == strcmp(level->filename, name)) {
                    return true;
                }
            }
        });

    return false;
}

static bool collection_level_filename_exists_as_existing_file(collection_t *collection, const char *name)
{
    if (collection->dirpath) {
        const char *path = concat_dir_and_filename(collection->dirpath, name);
        return file_exists(path);
    } else {
        return false;
    }
}

static bool collection_level_filename_exists(collection_t *collection, const char *name)
{
    return collection_level_filename_exists_in_collection(collection, name)
        || collection_level_filename_exists_as_existing_file(collection, name);
}

static void collection_generate_level_filename(collection_t *collection, level_t *level)
{
    assert_not_null(collection);
    assert_not_null(level);

    if (level->filename) {
        return;
    }

    char *prefix = COLLECTION_DEFAULT_FILENAME_PREFIX;
    char *suffix = COLLECTION_DEFAULT_FILENAME_SUFFIX;

    for (int n=collection->filename_seq; n<1000; n++) {
        char *tmp;
        asprintf(&tmp, "%s%03d%s", prefix, n, suffix);
        if (!collection_level_filename_exists(collection, tmp)) {
            collection->filename_seq = n + 1;
            level_set_file_path(level, tmp);
            if (options->verbose) {
                infomsg("Generated filename: \"%s\"", level->filename);
            }
            return;
        }
    }

    DIE("Only 999 level filenames are supported; sorry!");
}

static void collection_show_level_names(collection_t *collection)
{
    assert_not_null(collection);

    printf("<collection");
    if (collection->filename) {
        printf(" file=\"%s\"", collection->filename);
    }
    if (collection->dirpath) {
        printf(" dir=\"%s\"", collection->dirpath);
    }
    printf(">\n");

    level_t *level = collection->levels;
    for (int i=0; i < collection->level_count; i++) {
//#define DEBUG_SHOW_UI_NAME 1
#ifdef DEBUG_SHOW_UI_NAME
        printf("  %d:\tfilename=\"%s\"\tname=\"%s\"\tui_name=\"%s\"\n",
               i,
               level->filename,
               level->name,
               collection->level_names[i]);
#else
        printf("  %d:\tfilename=\"%s\"\tname=\"%s\"\n",
               i,
               level->filename,
               level->name);
#endif
        level = level->next;
    }
    printf("</collection>\n");
}

void collection_update_level_names(collection_t *collection)
{
    level_t *lp = collection->levels;
    int n=0;
    while (lp) {
        assert(n < collection->level_name_count);

        level_update_ui_name(lp);
        collection->level_names[n] = lp->ui_name;

        lp = lp->next;
        n++;
    }

    assert(n == collection->level_count);

    if (options->verbose) {
        collection_show_level_names(collection);
    }
}

bool collection_level_name_exists(collection_t *collection, const char *name)
{
    assert_not_null(collection);
    assert_not_null(name);

    SGLIB_DL_LIST_MAP_ON_ELEMENTS(level_t, collection->levels, level, prev, next, {
            if (0 == strcmp(level->name, name)) {
                return true;
            }
        });

    return false;
}

void collection_add_level(collection_t *collection, level_t *level)
{
    assert_not_null(collection);
    assert_not_null(level);

    if (!level->filename) {
        collection_generate_level_filename(collection, level);
    }

    if (collection->levels) {
        level_t *last;
        SGLIB_DL_LIST_GET_LAST(level_t, collection->levels, prev, next, last);
        SGLIB_DL_LIST_ADD_AFTER(level_t, last, level, prev, next);
    } else {
        collection->levels = level;
    }

    collection->level_count++;

    if (collection->level_count >= collection->level_name_count) {
        const char **old_ptr = collection->level_names;
        int old_count  = collection->level_name_count;

        collection->level_name_count *= 2;
        collection_alloc_level_names(collection);

        memcpy(collection->level_names, old_ptr, old_count);
        free(old_ptr);
    }

    collection_update_level_names(collection);

    if (collection->gui_list_active == -1) {
        collection->gui_list_active = 0;
    }

    level->collection = collection;
}

bool collection_add_level_file(collection_t *collection, const char *filename)
{
    assert_not_null(collection);
    assert_not_null(filename);

    level_t *level = load_level_file(filename);
    if (level) {
        collection_add_level(collection, level);
        return true;
    } else {
        errmsg("cannot load level file \"%s\" into collection \"%s\"",
               filename, collection_name(collection));
        return false;
    }
}

level_t *collection_find_level_by_filename(collection_t *collection, const char *filename)
{
    level_t *level = collection->levels;
    while (level) {
        if (level->filename) {
            if (0 == strcmp(filename, level->filename)) {
                return level;
            }
        }

        level = level->next;
    }

    return NULL;
}

void collection_save_dir(collection_t *collection)
{
    assert_not_null(collection);
    assert_not_null(collection->dirpath);

    SGLIB_DL_LIST_MAP_ON_ELEMENTS(level_t, collection->levels, level, prev, next, {
            printf("trying to save level \"%s\"\n", level->name);
            level_save_to_file_if_changed(level, collection->dirpath);
        });

    const char *path = concat_dir_and_filename(collection->dirpath, COLLECTION_ZIP_INDEX_FILENAME);
    FILE *f = fopen(path, "w");
    if (NULL == f) {
        errmsg("Could not open \"%s\" for writing: %s", path, strerror(errno));
        return;
    }

    level_t *level = collection->levels;
    bool first = true;
    while (level) {
        assert_not_null(level->filename);

        if (first) {
            first = false;
        } else {
            if (EOF == fputs("\n", f)) {
                errmsg("Error writing index file \"%s\" in collection \"%s\" - fputs() returned EOF",
                       COLLECTION_ZIP_INDEX_FILENAME, collection->filename);
                goto close_file;
            }
        }
        if (EOF == fputs(level->filename, f)) {
            errmsg("Error writing index file \"%s\" in collection \"%s\" - fputs() returned EOF",
                   COLLECTION_ZIP_INDEX_FILENAME, collection->filename);
            goto close_file;
        }

        level = level->next;
    }

  close_file:
    fclose(f);
}

void collection_save_zip(collection_t *collection)
{
    assert_not_null(collection);

    char *tmpname;
    asprintf(&tmpname, "%s.tmp", collection->filename);

    if (options->verbose) {
        infomsg("Writing level collection to \"%s\"", tmpname);
    }

    int errnum;
    struct zip_t *zip = zip_openwitherror(tmpname, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w', &errnum);
    if (NULL == zip) {
        errmsg("Cannot open \"%s\" for writing - %s", tmpname, zip_strerror(errnum));
        goto cleanup;
    }

    errnum = zip_entry_open(zip, COLLECTION_ZIP_INDEX_FILENAME);
    if (errnum < 0) {
        errmsg("Cannot open level collection index file \"%s\" for writing - %s",
               COLLECTION_ZIP_INDEX_FILENAME, zip_strerror(errnum));
        goto close_zip;
    } else {
        level_t *level = collection->levels;
        bool first = true;
        while (level) {
            assert_not_null(level->filename);

            if (first) {
                first = false;
            } else {
                errnum = zip_entry_write(zip, "\n", 1);
                if (errnum < 0) {
                    errmsg("Error writing index file \"%s\" in collection \"%s\" - %s",
                           COLLECTION_ZIP_INDEX_FILENAME, collection->filename, zip_strerror(errnum));
                    goto close_zip;
                }
            }
            errnum = zip_entry_write(zip, level->filename, strlen(level->filename));
            if (errnum < 0) {
                errmsg("Error writing index file \"%s\" in collection \"%s\" - %s",
                       COLLECTION_ZIP_INDEX_FILENAME, collection->filename, zip_strerror(errnum));
                goto close_zip;
            }

            level = level->next;
        }
    }
    errnum = zip_entry_close(zip);
    if (errnum < 0) {
        errmsg("Error closing the collection index file \"%s\" in collection \"%s\" - %s",
               COLLECTION_ZIP_INDEX_FILENAME, collection->filename, zip_strerror(errnum));
        goto close_zip;
    }

    level_t *level = collection->levels;
    while (level) {
        errnum = zip_entry_open(zip, level->filename);
        if (errnum < 0) {
            errmsg("Cannot open level collection level file \"%s\" for writing - %s",
                   level->filename, zip_strerror(errnum));
            goto close_zip;
        } else {

            char *str = level_serialize_memory(level);
            assert_not_null(str);

            errnum = zip_entry_write(zip, str, strlen(str));
            if (errnum < 0) {
                errmsg("Error writing level file \"%s\" in collection \"%s\" - %s",
                       level->filename, collection->filename, zip_strerror(errnum));
                goto close_zip;
            }

            errnum = zip_entry_close(zip);
            if (errnum < 0) {
                errmsg("Error closing level file \"%s\" in collection \"%s\" - %s",
                       level->filename, collection->filename, zip_strerror(errnum));
                goto close_zip;
            }
        }

        level = level->next;
    }

    zip_close(zip);

    if (options->verbose) {
        infomsg("Renaming \"%s\" to \"%s\"", tmpname, collection->filename);
    }

    if (-1 == rename(tmpname, collection->filename)) {
        errmsg("Error trying to rename \"%s\" to \"%s\" - ",
               tmpname, collection->filename, strerror(errno));
    }

  cleanup:
    free(tmpname);
    return;

  close_zip:
    zip_close(zip);
    goto cleanup;
}

void collection_save(collection_t *collection)
{
    assert_not_null(collection);

    collection->changed = false;

    if (collection->dirpath) {
        collection_save_dir(collection);
    } else if (collection->filename) {
        collection_save_zip(collection);
    } else {
        errmsg("Cannot save collection; a filename or directory path is required");
    }
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

static void collection_draw_buttons(collection_t *collection, Rectangle collection_list_rect)
{
    float margin = (float)RAYGUI_ICON_SIZE;

    Rectangle collection_play_button_rect = {
        .x = collection_list_rect.x,
        .y = collection_list_rect.y + collection_list_rect.height + margin,
        .width  = collection_list_rect.width,
        .height = window_size.y * 0.12
    };

    char *collection_play_button_text = "Play";

    Rectangle collection_edit_button_rect = {
        .x = collection_play_button_rect.x + collection_play_button_rect.width + margin,
        .y = collection_play_button_rect.y,
        .width  = collection_play_button_rect.height,
        .height = collection_play_button_rect.height
    };

    char *collection_edit_button_text = "Edit";

    Rectangle collection_new_button_rect = {
        .x = collection_play_button_rect.x - margin - collection_play_button_rect.height,
        .y = collection_play_button_rect.y,
        .width  = collection_play_button_rect.height,
        .height = collection_play_button_rect.height
    };

    char *collection_new_button_text = "New Level";

    if (collection->gui_list_active == -1) {
        GuiDisable();
    }

    GuiSetFont(font20);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 2);

    if (GuiButton(collection_play_button_rect, collection_play_button_text)) {
        level_t *level = collection->levels;
        int n = collection->gui_list_active;
        if (n >= 0) {
            while (n--) {
                level = level->next;
            }
            level_play(level);
        }
    }

    if (game_mode == GAME_MODE_EDIT_COLLECTION) {
        if (GuiButton(collection_edit_button_rect, collection_edit_button_text)) {
            level_t *level = collection->levels;
            int n = collection->gui_list_active;
            if (n >= 0) {
                while (n--) {
                    level = level->next;
                }
                level_edit(level);
            }
        }
    }

    GuiEnable();

    if (game_mode == GAME_MODE_EDIT_COLLECTION) {
        if (GuiButton(collection_new_button_rect, collection_new_button_text)) {
            create_new_level();
        }
    }

    GuiSetFont(font16);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 2);
}

void collection_draw_move_buttons(collection_t *collection, Rectangle bounds)
{
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
            if (GuiButton(up_btn_rect, GuiIconText(ICON_ARROW_UP, NULL))) {
                collection_move_level_earlier(collection, i);
            }
        }

        if (i < visibleItems - 1) {
            if (GuiButton(down_btn_rect, GuiIconText(ICON_ARROW_DOWN, NULL))) {
                collection_move_level_later(collection, i);
            }
        }

        itemBounds.y += (GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING));
    }
}

void collection_draw(collection_t *collection)
{
    assert_not_null(collection);

    float width =  window_size.x * 0.4;
    float height = window_size.y * 0.64;

    Rectangle collection_list_rect = {
        .x = (window_size.x - width)  / 2.0f,
        .y = (window_size.y - height) / 2.0f,
        .width  = width,
        .height = height
    };

    float theight = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT + RAYGUI_ICON_SIZE;

    Rectangle title_rect = {
        .x = collection_list_rect.x,
        .y = collection_list_rect.y - theight - RAYGUI_ICON_SIZE,
        .width  = collection_list_rect.width,
        .height = theight
    };

    float name_height = RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT * 1.2;

    Rectangle title_name_rect = {
        .x = title_rect.x,
        .y = title_rect.y + name_height,
        .width  = title_rect.width,
        .height = title_rect.height - name_height
    };

    GuiPanel(title_rect, "Collection");

    int tempTextAlign = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiLabel(title_name_rect, collection_name(collection));
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, tempTextAlign);


    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    GuiListViewEx(collection_list_rect,
                  collection->level_names,
                  collection->level_count,
                  &collection->gui_list_scroll_index,
                  &collection->gui_list_active,
                  &collection->gui_list_focus);

    if (game_mode == GAME_MODE_EDIT_COLLECTION) {
        collection_draw_move_buttons(collection, collection_list_rect);
    }

    collection_draw_buttons(collection, collection_list_rect);
}
