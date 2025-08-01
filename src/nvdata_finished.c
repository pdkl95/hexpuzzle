/****************************************************************************
 *                                                                          *
 * nvdata_finished.c                                                        *
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

#include <time.h>

#include "options.h"
#include "level.h"
#include "collection.h"
#include "solve_timer.h"
#include "nvdata_finished.h"

uint16_t history_log_file_version = 1;

char *nvdata_state_finished_levels_file_path = NULL;
char *nvdata_state_finished_levels_backup_file_path = NULL;

#define TREE_INDENT_MAXLEN 60
char indentbuf[TREE_INDENT_MAXLEN];

SGLIB_DEFINE_RBTREE_FUNCTIONS(finished_level, left, right, rb_color, compare_finished_level);

finished_levels_t finished_levels;
bool finished_levels_changed = false;

char *finished_level_flag_str(flags16_t flags)
{
    static char buf[6] = {0};

    buf[0] = (flags & FINISHED_LEVEL_FLAG_NAME)         ? 'N' : '-';
    buf[1] = (flags & FINISHED_LEVEL_FLAG_WIN_TIME)     ? 'W' : '-';
    buf[2] = (flags & FINISHED_LEVEL_FLAG_ELAPSED_TIME) ? 'E' : '-';
    buf[3] = (flags & FINISHED_LEVEL_FLAG_BLUEPRINT)    ? 'B' : '-';
    buf[4] = (flags & FINISHED_LEVEL_FLAG_SOLVER)       ? 'S' : '-';
    buf[5] = '\0';

    return buf;
}

void print_finished_levels(void)
{
    struct finished_level *e = NULL;
    struct sglib_finished_level_iterator it;
    int count = 0;

    printf("<finished_levels count=%d>\n", finished_levels.count);

    for(e = sglib_finished_level_it_init_inorder(&it, finished_levels.tree);
        e != NULL;
        e = sglib_finished_level_it_next(&it)
    ) {
        struct tm win_tm;
        gmtime_r(&e->win_time, &win_tm);

        char win_time_str[256];
        strftime(win_time_str, sizeof(win_time_str), "%F %T", &win_tm);
        printf("\t\"%s\",\t\"%s\"\n", e->id, win_time_str);
    }

    printf("</finished_levels>\n");
}

int compare_finished_level(struct finished_level *a, struct finished_level *b)
{
    assert_not_null(a);
    assert_not_null(b);

    int rv = 0;

    if (finished_level_has_win_time(a) &&
        finished_level_has_win_time(b)) {
        rv = ((int)(a->win_time - b->win_time));
        if (rv != 0) {
            return rv;
        }
    }

    rv = strcmp(a->id, b->id);
    return rv;
}

void init_nvdata_finished(void)
{
    finished_levels.tree = NULL;
    finished_levels.count = 0;
}

void destroy_finished_level(struct finished_level *node)
{
    if (node) {
        if (node->left) {
            destroy_finished_level(node->left);
        }

        if (node->right) {
            destroy_finished_level(node->right);
        }

        FREE(node);
    }
}

static void destroy_finished_levels_tree(void)
{
    if (finished_levels.tree) {
        destroy_finished_level(finished_levels.tree);
        finished_levels.tree = NULL;
    }

    finished_levels.count = 0;
}

void cleanup_nvdata_finished(void)
{
    destroy_finished_levels_tree();

    SAFEFREE(nvdata_state_finished_levels_backup_file_path);
    SAFEFREE(nvdata_state_finished_levels_file_path);
}

void nvdata_mark_id_finished(struct finished_level *entry)
{
    assert_not_null(entry);

    struct finished_level *e, *member;
    e = calloc(1, sizeof(struct finished_level));

    snprintf(&e->id, ID_MAXLEN, "%s", entry->id);

    if (finished_level_has_name(entry)) {
        finished_level_set_name(e, entry->name);
    }

    if (finished_level_has_solver(entry)) {
        finished_level_set_solver(e);
    }

    if (finished_level_has_blueprint(entry)) {
        finished_level_set_blueprint(e, entry->blueprint);
    }

    if (finished_level_has_classic(entry)) {
        finished_level_set_classic(e,
                                   entry->classic_nameref.collection_id,
                                   entry->classic_nameref.level_unique_id);
    }

    if (finished_level_has_fileref(entry)) {
        finished_level_set_fileref(e,
                                   entry->fileref.filename,
                                   entry->fileref.level_unique_id);
    }

    if (finished_level_has_win_time(entry)) {
        finished_level_set_win_time(e, entry->win_time);
    }

    if (finished_level_has_elapsed_time(entry)) {
        finished_level_set_elapsed_time(e, &entry->elapsed_time);
    }

    if (options->verbose) {
        infomsg("MARK finished: id=\"%s\" flags=%s", e->id, finished_level_flag_str(e->flags));
    }

    if (sglib_finished_level_add_if_not_member(&(finished_levels.tree), e, &member)) {
        finished_levels.count++;
    }

    assert(finished_levels.count > 0);

    finished_levels_changed = true;
    rebuild_history_browser = true;
}

void nvdata_mark_finished(struct level *level)
{
    assert_not_null(level);

    if (!level->finished || demo_mode || !options->log_finished_levels) {
        return;
    }

    struct finished_level entry = {0};
    finished_level_set_name(&entry, level->name);
    finished_level_set_win_time(&entry, level->win_time);
    finished_level_set_elapsed_time(&entry, &level->elapsed_time);

    if (level->blueprint) {
        finished_level_set_blueprint(&entry, level->blueprint);
    } else if (level->classic_collection) {
        finished_level_set_classic(&entry, level->classic_collection->id, level->unique_id);
    } else if (level->loaded_from_file) {
        if (level->loadpath) {
            finished_level_set_fileref(&entry,
                                       (char *)GetFileName(level->loadpath),
                                       level->unique_id);
        } else {
            warnmsg("cannot save a level fileref for level \"%s\" - missing 'loadpath'", level->id);
        }
    } else if (level->stored_in_collection) {
        assert_not_null(level->collection);
        if (level->collection->loadpath) {
            finished_level_set_fileref(&entry,
                                       (char *)GetFileName(level->collection->loadpath),
                                       level->unique_id);
            finished_level_set_collection(&entry);
        } else {
            warnmsg("cannot save a collection fileref for level \"%s\" - missing 'loadpath'", level->collection->id);
        }
    } else {
        errmsg("don't know how to encode level into a finished_level entry");
    }

    if (level->solver) {
        finished_level_set_solver(&entry);
    }

    snprintf(&entry.id, ID_MAXLEN, "%s", level->unique_id);

    nvdata_mark_id_finished(&entry);
}

void nvdata_unmark_finished(struct level *level)
{
    assert_not_null(level);

    if (!level->have_id || !options->log_finished_levels) {
        return;
    }

    struct finished_level e, *result;
    e.left = e.right = NULL;
    snprintf(&e.id, ID_MAXLEN, "%s", level->unique_id);

    if (options->verbose) {
        infomsg("UNMARK finished: \"%s\"", e.id);
    }

    if (sglib_finished_level_delete_if_member(&(finished_levels.tree), &e, &result)) {
        finished_levels.count--;
    }

    assert(finished_levels.count >= 0);

    finished_levels_changed = true;
    rebuild_history_browser = true;
}

bool nvdata_is_finished(struct level *level)
{
    assert_not_null(level);

    if (!level->have_id || demo_mode) {
        printf("is_finished[%s]: have_id is false\n", level->name);
        return false;
    }

    struct finished_level e, *result;
    e.left = e.right = NULL;
    snprintf(&e.id, ID_MAXLEN, "%s", level->unique_id);
    return NULL != sglib_finished_level_find_member(finished_levels.tree, &e);
}

bool nvdata_finished_from_json(cJSON *root_json)
{
    bool new_changed_value = false;

    if (!cJSON_IsObject(root_json)) {
        errmsg("Error parsing nvdata finished level JSON: not an Object");
        return false;
    }

    cJSON *json = cJSON_GetObjectItem(root_json, PROJECT_FINISHED_LEVEL_LOG_JSON_NAMESPACE);
    if (!json) {
        errmsg("Error parsing nvdata finished level JSON: root object \"" PROJECT_FINISHED_LEVEL_LOG_JSON_NAMESPACE "\" is missing");
        return false;
    }
    if (!cJSON_IsObject(json)) {
        errmsg("Error parsing nvdata finished level JSON: root object \"" PROJECT_FINISHED_LEVEL_LOG_JSON_NAMESPACE "\" is not an Object");
        return false;
    }

    cJSON *version_json = cJSON_GetObjectItem(json, "history_log_version");
    if (version_json) {
        if (!cJSON_IsNumber(version_json)) {
            errmsg("Error parsing program state: JSON['history_log_version'] is not a Number");
            return false;
        }

        if (version_json->valueint != history_log_file_version) {
            errmsg("Program state JSON is version %d, expected %d",
                   version_json->valueint, history_log_file_version);
        }
    } else {
        warnmsg("Program state JSON is missing \"history_log_version\"");
    }

    cJSON *finished_levels_json = cJSON_GetObjectItem(json, "finished_levels");
    if (!cJSON_IsArray(finished_levels_json)) {
        errmsg("Error parsing nvdata finished level JSON: 'finished_levels' is not an Array");
        return false;
    }

    struct finished_level e = {0};
    cJSON *level_json;
    int count = 0;
    cJSON_ArrayForEach(level_json, finished_levels_json) {
        cJSON *id_json = cJSON_GetObjectItem(level_json, "id");
        if (!cJSON_IsString(id_json)) {
            errmsg("Error parsing nvdata finished level JSON [%d]: 'id' is not a String", count);
            return false;
        }
        snprintf(&e.id, ID_MAXLEN, "%s", id_json->valuestring);

        cJSON *flags_json = cJSON_GetObjectItem(level_json, "flags");
        if (!cJSON_IsNumber(flags_json)) {
            errmsg("Error parsing nvdata finished level JSON [%d]: 'flags' is not a Number", count);
            return false;
        }
        e.flags = flags_json->valueint;

        if (finished_level_has_name(&e)) {
            cJSON *name_json = cJSON_GetObjectItem(level_json, "name");
            if (name_json) {
                if (!cJSON_IsString(name_json)) {
                    errmsg("Error parsing nvdata finished level JSON [%d]: 'name' is not a String", count);
                    return false;
                }
                finished_level_set_name(&e, name_json->valuestring);
            } else {
                warnmsg("Error parsing nvdata finished level JSON [%d]: 'name' is missing - using the 'id' field as substitute", count);
                int n = snprintf(&e.name, NAME_MAXLEN, "%s", e.id);
                if (n < 0) {
                    assert(false && "snprintf failed?!");
                    return false;
                }
                if ((size_t)n < strlen(e.id)) {
                    warnmsg("'id' field was truncated; %d characters lost", strlen(e.id) - n);
                }

                finished_level_set_name(&e, e.id);
                new_changed_value = true;
            }
        } else {
            finished_level_clear_name(&e);
        }

        if (finished_level_has_elapsed_time(&e)) {
            cJSON *elapsed_time_json = cJSON_GetObjectItem(level_json, "elapsed_time");
            if (elapsed_time_json) {
                if (!cJSON_IsString(elapsed_time_json)) {
                    errmsg("Error parsing nvdata finished level JSON [%d]: 'elapsed_time' is not a String", count);
                    return false;
                }
                elapsed_time_parts_t elapsed_time;
                str_to_elapsed_time_parts(elapsed_time_json->valuestring, &elapsed_time);
                finished_level_set_elapsed_time(&e, &elapsed_time);
            } else {
                warnmsg("While parsing nvdata finished level JSON [%d]: 'elapsed_time' flag is set but the String data field is missing. Clearing the misleading flag.", count);
                finished_level_clear_elapsed_time(&e);
                new_changed_value = true;
            }
        } else {
            finished_level_clear_elapsed_time(&e);
        }

        if (finished_level_has_win_time(&e)) {
            cJSON *win_time_json = cJSON_GetObjectItem(level_json, "win_time");
            if (win_time_json) {
                if (!cJSON_IsString(win_time_json)) {
                    errmsg("Error parsing nvdata finished level JSON [%d]: 'win_time' is not a String", count);
                    return false;
                }

                struct tm win_tm = {0};
                strptime(win_time_json->valuestring, "%F %T", &win_tm);
                finished_level_set_win_time(&e, mktime(&win_tm));
            } else {
                warnmsg("While parsing nvdata finished level JSON [%d]: 'win_time' flag is set but the String data field is missing. Clearing the misleading flag.", count);
                finished_level_clear_win_time(&e);
                new_changed_value = true;
            }
        } else {
            finished_level_clear_win_time(&e);
        }

        if (finished_level_has_blueprint(&e)) {
            cJSON *blueprint_json = cJSON_GetObjectItem(level_json, "blueprint");
            if (blueprint_json) {
                if (!cJSON_IsString(blueprint_json)) {
                    errmsg("Error parsing nvdata finished level JSON [%d]: 'blueprint' is not a String", count);
                    return false;
                }
                finished_level_set_blueprint(&e, blueprint_json->valuestring);
            } else {
                warnmsg("While parsing nvdata finished level JSON [%d]: 'blueprint' flag is set but the String data field is missing. Clearing the misleading flag.", count);
                finished_level_clear_blueprint(&e);
                new_changed_value = true;
            }
        } else {
            finished_level_clear_blueprint(&e);
        }

        if (finished_level_has_classic(&e)) {
            cJSON *classic_collection_json = cJSON_GetObjectItem(level_json, "classic_collection");
            cJSON *classic_level_json = cJSON_GetObjectItem(level_json, "classic_level");
            if (classic_collection_json && classic_level_json) {
                if (!cJSON_IsString(classic_collection_json)) {
                    errmsg("Error parsing nvdata finished level JSON [%d]: 'classic_collection' is not a String", count);
                    return false;
                }
                if (!cJSON_IsString(classic_level_json)) {
                    errmsg("Error parsing nvdata finished level JSON [%d]: 'classic_level' is not a String", count);
                    return false;
                }
                finished_level_set_classic(&e,
                                           classic_collection_json->valuestring,
                                           classic_level_json->valuestring);
            } else {
                if (!classic_collection_json) {
                    warnmsg("While parsing nvdata finished level JSON [%d]: 'classic' flag is set but the String data field 'classic_collection' is missing.", count);
                }
                if (!classic_level_json) {
                    warnmsg("While parsing nvdata finished level JSON [%d]: 'classic' flag is set but the String data field 'classic_level' is missing.", count);
                }
                finished_level_clear_classic(&e);
                new_changed_value = true;
            }
        } else {
            finished_level_clear_classic(&e);
        }

        if (finished_level_has_fileref(&e)) {
            cJSON *filename_json = cJSON_GetObjectItem(level_json, "filename");
            cJSON *level_unique_id_json = cJSON_GetObjectItem(level_json, "level_unique_id");
            if (filename_json && level_unique_id_json) {
                if (!cJSON_IsString(filename_json)) {
                    errmsg("Error parsing nvdata finished level JSON [%d]: 'filename' is not a String", count);
                    return false;
                }
                if (!cJSON_IsString(level_unique_id_json)) {
                    errmsg("Error parsing nvdata finished level JSON [%d]: 'level_unique_id_json' is not a String", count);
                    return false;
                }
                finished_level_set_fileref(&e,
                                           filename_json->valuestring,
                                           level_unique_id_json->valuestring);
            } else {
                if (!filename_json) {
                    warnmsg("While parsing nvdata finished level JSON [%d]: 'fileref' flag is set but the String data field 'filename' is missing.", count);
                }
                if (!level_unique_id_json) {
                    warnmsg("While parsing nvdata finished level JSON [%d]: 'fileref' flag is set but the String data field 'level_unique_id' is missing.", count);
                }
                finished_level_clear_fileref(&e);
                new_changed_value = true;
            }
        } else {
            finished_level_clear_fileref(&e);
        }

        if (current_collection) {
            level_t *level = collection_find_level_by_unique_id(current_collection, e.id);
            if (level) {
                level->finished = true;
                level->win_time     = e.win_time;
                level->elapsed_time = e.elapsed_time;
            }
        }

        nvdata_mark_id_finished(&e);

        count++;
    }

    if (count != finished_levels.count) {
        warnmsg("Parsed %d finished level entries, but finished_levels.count = %d", count, finished_levels.count);
    }

    finished_levels_changed = new_changed_value;

    if (options->verbose) {
        infomsg("Read %d level finished level IDs", count);
    }

    if (current_collection) {
        collection_update_level_names(current_collection);
    }

    //print_finished_levels();

    return true;
}

cJSON *nvdata_finished_to_json(void)
{
    cJSON *root_json = cJSON_CreateObject();

    cJSON *json = cJSON_AddObjectToObject(root_json, PROJECT_FINISHED_LEVEL_LOG_JSON_NAMESPACE);
    if (json == NULL) {
        goto json_err;
    }

    if (cJSON_AddNumberToObject(json, "history_log_version", history_log_file_version) == NULL) {
        errmsg("Error adding \"history_loh_version\" to JSON");
        goto json_err;
    }

    cJSON *finished_levels_json = cJSON_AddArrayToObject(json, "finished_levels");
    if (finished_levels_json == NULL) {
        goto json_err;
    }

    //printf(">> JSON encoding %d entries\n", finished_levels.count);

    struct finished_level *e = NULL;
    struct sglib_finished_level_iterator it;

    int count = 0;

    for(e = sglib_finished_level_it_init_inorder(&it, finished_levels.tree);
        e != NULL;
        e = sglib_finished_level_it_next(&it)
    ) {
        struct tm win_tm = {0};
        gmtime_r(&e->win_time, &win_tm);

        char win_time_str[256];
        strftime(win_time_str, sizeof(win_time_str), "%F %T", &win_tm);

        char *elapsed_time_str = elapsed_time_parts_to_str(&e->elapsed_time);

        //printf("  [%d] id=\"%s\" flags=%s (%u)\n", count, e->id, finished_level_flag_str(e->flags), e->flags);

        cJSON *level_json = cJSON_CreateObject();

        if (cJSON_AddStringToObject(level_json, "id", e->id) == NULL) {
            goto json_err;
        }

        if (finished_level_has_name(e)) {
            if (cJSON_AddStringToObject(level_json, "name", e->name) == NULL) {
                goto json_err;
            }
        }

        if (finished_level_has_win_time(e)) {
            if (cJSON_AddStringToObject(level_json, "win_time", win_time_str) == NULL) {
                goto json_err;
            }
        }

        if (finished_level_has_elapsed_time(e)) {
            if (cJSON_AddStringToObject(level_json, "elapsed_time", elapsed_time_str) == NULL) {
                goto json_err;
            }
        }

        if (finished_level_has_blueprint(e)) {
            if (cJSON_AddStringToObject(level_json, "blueprint", e->blueprint) == NULL) {
                goto json_err;
            }
        }

        if (finished_level_has_classic(e)) {
            if (cJSON_AddStringToObject(level_json, "classic_collection", e->classic_nameref.collection_id) == NULL) {
                goto json_err;
            }
            if (cJSON_AddStringToObject(level_json, "classic_level", e->classic_nameref.level_unique_id) == NULL) {
                goto json_err;
            }
        }

        if (finished_level_has_fileref(e)) {
            if (cJSON_AddStringToObject(level_json, "filename", e->fileref.filename) == NULL) {
                goto json_err;
            }
            if (cJSON_AddStringToObject(level_json, "level_unique_id", e->fileref.level_unique_id) == NULL) {
                goto json_err;
            }
        }

        if (cJSON_AddNumberToObject(level_json, "flags", (double)e->flags) == NULL) {
            goto json_err;
        }

        cJSON_AddItemToArray(finished_levels_json, level_json);

        count++;
    }

    return root_json;

  json_err:
    cJSON_Delete(root_json);
    return NULL;

}

bool nvdata_finished_load_from_file(const char *filepath)
{
    cJSON *json = ReadPossiblyCompressedJSONFile(filepath);
    if (NULL == json) {
        return false;
    }

    bool ret = nvdata_finished_from_json(json);

    cJSON_Delete(json);

    return ret;
}

bool nvdata_finished_save_to_file(const char *filepath)
{
    cJSON *json = nvdata_finished_to_json();
    if (NULL == json) {
        return false;
    }

    //print_cjson(json);

    bool ret = false;
    if (options->compress_finished_levels_dat) {
        ret = WriteCompressedJSONFile(filepath, json);
    } else {
        ret = WriteUncompressedJSONFile(filepath, json);
    }

    cJSON_Delete(json);

    return ret;
}

void load_nvdata_finished_levels(void)
{
    if (!FileExists(nvdata_state_finished_levels_file_path)) {
        if (options->verbose) {
            infomsg("Skipping loading finished levels file (\"%s\" doesn't exist)",
                    nvdata_state_finished_levels_file_path);
        }
        return;
    }

    nvdata_finished_load_from_file(nvdata_state_finished_levels_file_path);
}

void force_save_nvdata_finished_levels(void)
{
    if (nvdata_finished_save_to_file(nvdata_state_finished_levels_file_path)) {
        if (options->verbose) {
            infomsg("Successfully saved finished level data \"%s\"",
                    nvdata_state_finished_levels_file_path);
        }
        //print_finished_levels();
        finished_levels_changed = false;
    } else {
        errmsg("Loading saved finished level data from \"%s\" failed!",
               nvdata_state_finished_levels_file_path);
    }
}

void save_nvdata_finished_levels(void)
{
    if (!options->log_finished_levels) {
        infomsg("Skipping saving finished level data (option \"log-finished-levels\" is disabled))");
        return;
    }

    if (demo_mode) {
        infomsg("Skipping saving finished level data (demo mode is enabled)");
        return;
    }

    if (!have_nvdata_finished_levels_data()) {
        if (options->verbose) {
            infomsg("Skipping saving finished level data (nothing to save)");
        }
        return;
    }

    if (!finished_levels_changed) {
        if (options->verbose) {
            infomsg("Skipping saving finished level data (nothing changed)");
        }
        return;
    }

    force_save_nvdata_finished_levels();
}

bool reset_nvdata_finished_levels(void)
{
    destroy_finished_levels_tree();

    assert_not_null(nvdata_state_finished_levels_file_path);
    assert_not_null(nvdata_state_finished_levels_backup_file_path);

    if (!FileExists(nvdata_state_finished_levels_file_path)) {
        if (options->verbose) {
            infomsg("Reset of Finished level data unnecessary; data file \"%s\" does not exist.\n", nvdata_state_finished_levels_file_path);
        }
        return false;
    }

    infomsg("Reseting finished level data...\n");
    infomsg("Saving a backup of the old finished level data in \"%s\"",
            nvdata_state_finished_levels_backup_file_path);
    infomsg("Removing finished level data storge file \"%s\"",
            nvdata_state_finished_levels_file_path);

    if (-1 == rename(nvdata_state_finished_levels_file_path,
                     nvdata_state_finished_levels_backup_file_path)) {
        int err = errno;
        errmsg("Error renaming \"%s\"", nvdata_state_finished_levels_file_path);
        errmsg("            to \"%s\"", nvdata_state_finished_levels_backup_file_path);
        errmsg("  %s", strerror(err));
    }

    return true;
}

bool have_nvdata_finished_levels_data(void)
{
    return finished_levels.tree != NULL;
}
