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

//#define INITIAL_LEVEL_NAME_COUNT 64
#define INITIAL_LEVEL_NAME_COUNT 4

#define COLLECTION_DEFAULT_FILENAME_PREFIX "level-"
#define COLLECTION_DEFAULT_FILENAME_SUFFIX ".hexlevel"

static void collection_alloc_level_names(collection_t *collection)
{
    collection->level_names = calloc(collection->level_name_count, sizeof(char *));
}

static collection_t *alloc_collection(void)
{
    collection_t *collection = calloc(1, sizeof(collection_t));

    collection->level_name_count = INITIAL_LEVEL_NAME_COUNT;
    collection_alloc_level_names(collection);

    collection->name[0] = '\0';

    collection->dirpath = NULL;
    collection->filename = NULL;
    collection->levels = NULL;
    collection->prev = NULL;
    collection->next = NULL;

    collection->filename_seq = 1;

    collection->level_count = 0;

    collection->gui_list_scroll_index = 0;
    collection->gui_list_active = 0;
    collection->gui_list_focus = 0;

    return collection;
}

collection_t *create_collection(char *name)
{
    collection_t *collection = alloc_collection();

    if (name) {
        snprintf(collection->name, NAME_MAXLEN, "%s", name);
    }

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

collection_t *load_collection_dir(char *dirpath)
{
    assert_not_null(dirpath);

    collection_t *collection = create_collection(dirpath);
    collection->dirpath = strdup(dirpath);

    collection_scan_dir(collection);

    return collection;
}

collection_t *load_collection_level_file(char *filename)
{
    collection_t *collection = create_collection(NULL);

    if (collection_add_level_file(collection, filename)) {
        return collection;
    } else {
        destroy_collection(collection);
        return NULL;
    }
}

collection_t *load_collection_zip_file(char *filename)
{
    collection_t *collection = create_collection(filename);
    collection->filename = strdup(filename);

    return collection;
}

collection_t *load_collection_file(char *filename)
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

collection_t *load_collection_path(char *path)
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
    int len = strlen(prefix)
        + strlen(suffix)
        + 3   // number 001-999
        + 2   // \0
        + 10;
    char *tmp = calloc(len, sizeof(char));
    len--;
    for (int n=collection->filename_seq; n<1000; n++) {
        snprintf(tmp, len, "%s%03d%s", prefix, n, suffix);
        if (!collection_level_filename_exists(collection, tmp)) {
            collection->filename_seq = n + 1;
            level->filename = tmp;
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

    printf("<level_names>\n");
    level_t *level = collection->levels;
    for (int i=0; i < collection->level_count; i++) {
        printf("\t\"%s\" - \"%s\"\n", collection->level_names[i], level->name);
        level = level->next;
    }
    printf("</level_names>\n");
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

        memcpy(collection->levels, old_ptr, old_count);
        free(old_ptr);
    }

    level_t *lp = collection->levels;
    int n=0;
    while (lp) {
        level_update_ui_name(level);
        collection->level_names[n] = level->ui_name;

        lp = lp->next;
        n++;
    }

    assert(n == collection->level_count);

    if (options->verbose) {
        collection_show_level_names(collection);
    }
}

bool collection_add_level_file(collection_t *collection, char *filename)
{
    assert_not_null(collection);
    assert_not_null(filename);

    level_t *level = load_level_file(filename);
    if (level) {
        collection_add_level(collection, level);
        return true;
    } else {
        errmsg("cannot load level file \"%s\" into collection \"%s\"",
               filename, collection->name);
        return false;
    }
}

level_t *collection_find_level_by_filename(collection_t *collection, char *filename)
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
}

void collection_save_zip(collection_t *collection)
{
    assert_not_null(collection);

    assert(0);
}

void collection_save(collection_t *collection)
{
    assert_not_null(collection);

    if (collection->dirpath) {
        collection_save_dir(collection);
    } else if (collection->filename) {
        collection_save_zip(collection);
    } else {
        errmsg("Cannot save collection; a filename or directory path is required");
    }
}

void collection_draw(collection_t *collection)
{
    assert_not_null(collection);

    float width =  window_size.x * 0.4;
    float height = window_size.y * 0.64;

    DrawTextWindowCenter("Collection", window_size.y * 0.08, 22, LIME);

    Rectangle collection_list_rect = {
        .x = (window_size.x - width)  / 2.0f,
        .y = (window_size.y - height) / 2.0f,
        .width  = width,
        .height = height
    };

    GuiListViewEx(collection_list_rect,
                  collection->level_names,
                  collection->level_count,
                  &collection->gui_list_scroll_index,
                  &collection->gui_list_active,
                  &collection->gui_list_focus);

    Rectangle collection_button_rect = {
        .x = collection_list_rect.x,
        .y = collection_list_rect.y + collection_list_rect.height + 16,
        .width  = collection_list_rect.width,
        .height = window_size.y * 0.12
    };

    char *collection_button_text = "Play";

    if (GuiButton(collection_button_rect, collection_button_text)) {
        level_t *level = collection->levels;
        int n = collection->gui_list_active;
        if (n >= 0) {
            while (n--) {
                level = level->next;
            }
            level_play(level);
        }
    }
}
