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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "sglib/sglib.h"

#include "collection.h"

//#define INITIAL_LEVEL_NAME_COUNT 64
#define INITIAL_LEVEL_NAME_COUNT 4

static void collection_alloc_level_names(collection_t *collection)
{
    collection->level_names = calloc(collection->level_name_count, sizeof(char *));
}

static collection_t *alloc_collection(void)
{
    collection_t *collection = calloc(1, sizeof(collection_t));

    collection->level_name_count = INITIAL_LEVEL_NAME_COUNT;
    collection_alloc_level_names(collection);

    collection->name = NULL;
    collection->dirpath = NULL;
    collection->filename = NULL;
    collection->levels = NULL;
    collection->prev = NULL;
    collection->next = NULL;

    collection->level_count = 0;

    return collection;
}

collection_t *create_collection(char *name)
{
    collection_t *collection = alloc_collection();

    if (name) {
        collection->name = strdup(name);
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
        printf("%s\n", namelist[n]->d_name);
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

        SAFEFREE(collection->name);
        SAFEFREE(collection->dirpath);
        SAFEFREE(collection->filename);
    }
}

void collection_add_level(collection_t *collection, level_t *level)
{
    assert_not_null(collection);
    assert_not_null(level);

    if (collection->levels) {
        level_t *last;
        SGLIB_DL_LIST_GET_LAST(level_t, collection->levels, prev, next, last);
        SGLIB_DL_LIST_ADD_AFTER(level_t, last, level, prev, next);
    } else {
        collection->levels = level;
    }

    collection->level_count++;

    if (collection->level_count >= collection->level_name_count) {
        char **old_ptr = collection->level_names;
        int old_count  = collection->level_name_count;

        collection->level_name_count *= 2;
        collection_alloc_level_names(collection);

        memcpy(collection->levels, old_ptr, old_count);
        free(old_ptr);
    }

    level_update_ui_name(level);

    collection->level_names[collection->level_count] = level->ui_name;
}

bool collection_add_level_file(collection_t *collection, char *filename)
{
    level_t *level = load_level_file(filename);
    if (level) {
        collection_add_level(collection, level);
        return true;
    } else {
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
