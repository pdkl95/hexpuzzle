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

#include "cJSON/cJSON.h"

#include "sglib/sglib.h"

#include "options.h"
#include "raylib_helper.h"
#include "collection.h"
#include "fonts.h"

#define COLLECTION_JSON_VERSION 1

#define INITIAL_LEVEL_NAME_COUNT 64
//#define INITIAL_LEVEL_NAME_COUNT 4

void create_new_level(void);

static void collection_alloc_level_names(collection_t *collection)
{
    collection->level_names = calloc(collection->level_name_count, sizeof(char *));
}

void collection_set_id(collection_t *collection, const char *new_id)
{
    memcpy(collection->id, new_id, COLLECTION_ID_LENGTH);
    collection->have_id = true;

    SGLIB_DL_LIST_MAP_ON_ELEMENTS(level_t, collection->levels, level, prev, next, {
            level_update_id(level);
        });

    collection_update_level_names(collection);
}

static collection_t *alloc_collection(void)
{
    collection_t *collection = calloc(1, sizeof(collection_t));

    collection->level_name_count = INITIAL_LEVEL_NAME_COUNT;
    collection_alloc_level_names(collection);

    gen_unique_id(collection->unique_id);
    collection_set_id(collection, collection->unique_id);

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
    collection->is_pack = false;

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

static bool collection_scan_index_file(collection_t *collection, const char *index_file)
{
    bool ret = true;

    char *index_str = LoadFileText(index_file);

    char *p = index_str;
    char *filename = p;
    while (*p) {
        if (*p == '\n') {
            *p = '\0';
            const char *filepath = concat_dir_and_filename(collection->dirpath, filename);
            if (!collection_add_level_file(collection, filepath)) {
                ret = false;
                goto scan_index_file_cleanup;
            }
            p++;
            filename = p;
        } else {
            p++;
        }
    }
    if ((*p != '\0') && (*p != '\n')) {
        const char *filepath = concat_dir_and_filename(collection->dirpath, filename);
        if (!collection_add_level_file(collection, filepath)) {
            ret = false;
            goto scan_index_file_cleanup;
        }
    }

  scan_index_file_cleanup:
    free(index_str);
    return ret;
}

static bool collection_scan_dir_by_file_ext(collection_t *collection)
{
    FilePathList list = LoadDirectoryFilesEx(collection->dirpath, "." LEVEL_FILENAME_EXT, false);

    for (int i=0; i < (int)list.count; i++) {
        char *filename = list.paths[i];
        if (FileExists(filename)) {
            if (!collection_add_level_file(collection, filename)) {
                return false;
            }
        } else {
            return false;
        }
    }

    UnloadDirectoryFiles(list);

    return true;
}

bool collection_scan_dir(collection_t *collection)
{
    assert_not_null(collection);
    assert_not_null(collection->dirpath);

    collection_clear_levels(collection);

    const char *index_file = concat_dir_and_filename(collection->dirpath, COLLECTION_ZIP_INDEX_FILENAME);
    if (FileExists(index_file)) {
        return collection_scan_index_file(collection, index_file);
    } else {
        return collection_scan_dir_by_file_ext(collection);
    }
}

collection_t *load_collection_dir(const char *dirpath)
{
    assert_not_null(dirpath);

    collection_t *collection = create_collection();
    collection->dirpath = strdup(dirpath);

    int last_idx = strlen(collection->dirpath);
    if (last_idx > 0) {
        last_idx--;
        if (is_dir_separator(collection->dirpath[last_idx])) {
            collection->dirpath[last_idx] = '\0';
        }

        if (!collection_scan_dir(collection)) {
            destroy_collection(collection);
            return NULL;
        }

        collection_update_level_names(collection);
    } else {
        errmsg("Directory name is zero-length!");
    }


    return collection;
}

collection_t *load_collection_level_file(const char *filename)
{
    collection_t *collection = create_collection();

    if (collection_add_level_file(collection, filename)) {
        collection_update_level_names(collection);
        return collection;
    } else {
        destroy_collection(collection);
        return NULL;
    }
}

bool collection_from_json(collection_t *collection, cJSON *json)
{
    assert_not_null(collection);
    assert_not_null(json);

    if (!cJSON_IsObject(json)) {
        errmsg("Error parsing pack JSON: not an Object");
        return false;
    }

    cJSON *version_json = cJSON_GetObjectItem(json, "version");
    if (!cJSON_IsNumber(version_json)) {
        errmsg("Error parsing pack JSON: 'version' is not a Number");
        return false;
    }

    if (version_json->valueint != COLLECTION_JSON_VERSION) {
        errmsg("Error parsing pack JSON: 'version' is %d, expected %d",
               version_json->valueint, COLLECTION_JSON_VERSION);
        return false;
    }

    cJSON *unique_id_json = cJSON_GetObjectItem(json, "unique_id");
    if (!cJSON_IsString(unique_id_json)) {
        errmsg("Error parsing pack JSON: 'unique_id' is not a String");
        return false;
    }
    snprintf(collection->unique_id, UNIQUE_ID_LENGTH, "%s", unique_id_json->valuestring);

    cJSON *id_json = cJSON_GetObjectItem(json, "id");
    if (!cJSON_IsString(id_json)) {
        errmsg("Error parsing pack JSON: 'id' is not a String");
        return false;
    }
    snprintf(collection->id, COLLECTION_ID_LENGTH, "%s", id_json->valuestring);

    cJSON *levels_json = cJSON_GetObjectItem(json, "levels");
    if (!cJSON_IsObject(levels_json)) {
        errmsg("Error parsing pack JSON: 'levels' is not an Object");
        return false;
    }

    cJSON *level_json;
    cJSON_ArrayForEach(level_json, levels_json) {
        if (!cJSON_IsObject(level_json)) {
            errmsg("Error parsing pack JSON: child of 'levels' named \"%s\" is not an Object",
                   level_json->string);
            return false;
        }

        level_t *level = load_level_json(level_json->string, level_json, true);
        collection_add_level(collection, level);
    }

    return true;
}

collection_t *load_collection_pack_compressed_data(const char *filename, unsigned char *compressed, int compsize)
{
    assert_not_null(filename);
    assert_not_null(compressed);
    assert(compsize > 0);

    int pack_str_size = 0;
    unsigned char *pack_str_data = DecompressData(compressed, compsize, &pack_str_size);
    char *pack_str = (char *)pack_str_data;

    collection_t *rv = NULL;
    cJSON *json = NULL;

    if ((NULL == pack_str) || (pack_str_size != ((int)strlen(pack_str) + 1))) {
        errmsg("Error loading " COLLECTION_FILENAME_EXT "\"%s\"", filename);
        goto cleanup_pack_str;
    }

    json = cJSON_Parse(pack_str);
    if (NULL == json) {
        errmsg("Error parsing " COLLECTION_FILENAME_EXT " \"%s\" as JSON", filename);
        goto cleanup_pack_str;
    }

    collection_t *collection = create_collection();
    collection->filename = strdup(filename);
    collection->is_pack = true;

    if (collection_from_json(collection, json)) {
        rv = collection;
    }

    collection_update_level_names(collection);

  cleanup_pack_str:
    if (json) {
        cJSON_Delete(json);
    }
    SAFEFREE(pack_str_data);

    return rv;
}

collection_t *load_collection_pack_file(const char *filename)
{
    assert_not_null(filename);

    int compsize = 0;
    unsigned char *compressed = LoadFileData(filename, &compsize);

    collection_t *collection = load_collection_pack_compressed_data(filename, compressed, compsize);

    if (compressed) {
        UnloadFileData(compressed);
    }

    return collection;
}

collection_t *load_collection_file(const char *filename)
{
    assert_not_null(filename);

    const char *ext = filename_ext(filename);

    if (strcmp(ext, LEVEL_FILENAME_EXT) == 0) {
        return load_collection_level_file(filename);
    } else if (strcmp(ext, COLLECTION_FILENAME_EXT) == 0) {
        return load_collection_pack_file(filename);
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

const char *collection_path(collection_t *collection)
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

const char *collection_name(collection_t *collection)
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

#if defined(PLATFORM_DESKTOP)
static bool collection_level_filename_exists_as_existing_file(collection_t *collection, const char *name)
{
    if (collection->dirpath) {
        const char *path = concat_dir_and_filename(collection->dirpath, name);
        return file_exists(path);
    } else {
        return false;
    }
}
#else
static bool collection_level_filename_exists_as_existing_file(UNUSED collection_t *collection, UNUSED const char *name)
{
    return false;
}
#endif

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
        safe_asprintf(&tmp, "%s%03d%s", prefix, n, suffix);
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

void collection_show_level_names(collection_t *collection)
{
    assert_not_null(collection);

    printf("<collection");
    if (collection->have_id) {
        printf(" id=\"%s\"", collection->id);
    }
    if (collection->filename) {
        printf(" file=\"%s\"", collection->filename);
    }
    if (collection->dirpath) {
        printf(" dir=\"%s\"", collection->dirpath);
    }
    printf(">\n");

    level_t *level = collection->levels;
    for (int i=0; i < collection->level_count; i++) {
#define DEBUG_SHOW_UI_NAME 1
#ifdef DEBUG_SHOW_UI_NAME
        printf("  %d:\tfilename=\"%s\"\tname=\"%s\"\tui_name=\"%s\"\n",
               i,
               level->filename,
               level->name,
               collection->level_names[i]);
#else
        printf("  %d:\tfilename=\"%s\"\tname=\"%s\"\tid-\"%s\"\n",
               i,
               level->filename,
               level->name,
               level->id);
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

        level_update_ui_name(lp, n + 1);
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

    if (collection->gui_list_active == -1) {
        collection->gui_list_active = 0;
    }

    level->collection = collection;
    level_update_id(level);
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

level_t *collection_get_level_after(UNUSED collection_t *collection, level_t *level)
{
    assert_not_null(collection);
    assert_not_null(level);

    return level->next;
}

level_t *collection_find_level_by_id(collection_t *collection, const char *id)
{
    assert_not_null(collection);

    level_t *level = collection->levels;
    while (level) {
        if (level->id) {
            if (0 == strcmp(id, level->id)) {
                return level;
            }
        }

        level = level->next;
    }

    return NULL;
}

level_t *collection_find_level_by_unique_id(collection_t *collection, const char *unique_id)
{
    assert_not_null(collection);

    level_t *level = collection->levels;
    while (level) {
        if (level->id) {
            if (0 == strcmp(unique_id, level->unique_id)) {
                return level;
            }
        }

        level = level->next;
    }

    return NULL;
}

level_t *collection_find_level_by_filename(collection_t *collection, const char *filepath)
{
    assert_not_null(collection);
    assert_not_null(filepath);

    const char *filename = GetFileName(filepath);

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

#if defined(PLATFORM_DESKTOP)
void collection_save_dir(collection_t *collection, const char *dirpath, bool changed_only)
{
    assert_not_null(collection);
    assert_not_null(dirpath);

    mkdir_p(dirpath, CREATE_DIR_MODE);

    SGLIB_DL_LIST_MAP_ON_ELEMENTS(level_t, collection->levels, level, prev, next, {
            if (changed_only) {
                level_save_to_file_if_changed(level, dirpath);
            } else {
                level_save_to_file(level, dirpath);
            }
        });

    const char *path = concat_dir_and_filename(dirpath, COLLECTION_ZIP_INDEX_FILENAME);
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
#endif

cJSON *collection_to_json(collection_t *collection)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(json, "version", COLLECTION_JSON_VERSION) == NULL) {
        goto json_err;
    }

    if (cJSON_AddStringToObject(json, "unique_id", collection->unique_id) == NULL) {
        goto json_err;
    }

    if (cJSON_AddStringToObject(json, "id", collection->id) == NULL) {
        goto json_err;
    }

    cJSON *levels_json = cJSON_AddObjectToObject(json, "levels");
    if (levels_json == NULL) {
        goto json_err;
    }

    level_t *level = collection->levels;
    while (level) {
        assert_not_null(level->filename);

        cJSON *level_json = level_to_json(level);
        if (level_json == NULL) {
            goto json_err;
        }

        cJSON_AddItemToObject(levels_json, level->filename, level_json);

        level = level->next;
    }

    return json;

  json_err:
    cJSON_Delete(json);
    return NULL;
}

#if defined(PLATFORM_DESKTOP)
void collection_save_pack(collection_t *collection, const char *filename)
{
    assert_not_null(collection);
    assert_not_null(filename);

    char *tmpname;
    safe_asprintf(&tmpname, "%s.tmp", filename);

    if (options->verbose) {
        infomsg("Writing level collection to \"%s\"", tmpname);
    }

    cJSON *json = collection_to_json(collection);
    char *json_str = cJSON_PrintUnformatted(json);

    int newsize = 0;
    unsigned char *compressed = CompressData((const unsigned char *)json_str, strlen(json_str) + 1, &newsize);
    SaveFileData(tmpname, compressed, newsize);
    MemFree(compressed);

    free(json_str);
    cJSON_Delete(json);

    if (-1 == rename(tmpname, filename)) {
        errmsg("Error trying to rename \"%s\" to \"%s\" - ",
               tmpname, filename);
    }

    free(tmpname);
}
#endif

void collection_save(collection_t *collection)
{
    assert_not_null(collection);

    collection->changed = false;

#if defined(PLATFORM_DESKTOP)
    if (collection->dirpath) {
        collection_save_dir(collection, collection->dirpath, true);
    } else if (collection->filename) {
        collection_save_pack(collection, collection->filename);
    } else {
        errmsg("Cannot save collection; a filename or directory path is required");
    }
#else
    errmsg("Saving only supported on PLATFORM_DESKTOP; sorry");
#endif
}
