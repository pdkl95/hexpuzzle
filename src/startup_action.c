/****************************************************************************
 *                                                                          *
 * startup_action.c                                                         *
 *                                                                          *
 * This file is part of hexpuzzle.                                              *
 *                                                                          *
 * hexpuzzle is free software: you can redistribute it and/or                   *
 * modify it under the terms of the GNU General Public License as published *
 * by the Free Software Foundation, either version 3 of the License,        *
 * or (at your option) any later version.                                   *
 *                                                                          *
 * hexpuzzle is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General *
 * Public License for more details.                                         *
 *                                                                          *
 * You should have received a copy of the GNU General Public License along  *
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#include "common.h"
#include "options.h"
#include "level.h"
#include "collection.h"
#include "gui_random.h"
#include "startup_action.h"
#include "generate_level.h"

bool startup_action_ok = false;

#if 0
create_level_mode_t parse_create_level_mode(const char *str)
{
    if (str) {
        if (0 == strcmp(str, "blank")) {
            return CREATE_LEVEL_MODE_BLANK;
        }
        if (0 == strcmp(str, "dfs")) {
            return CREATE_LEVEL_MODE_DFS;
        }
        if (0 == strcmp(str, "scatter")) {
            return CREATE_LEVEL_MODE_SCATTER;
        }
    }

    return CREATE_LEVEL_MODE_NULL;
}
#endif

void action_create_level(void)
{
    infomsg("ACTION: create random level");

    for (int arg=0; arg < options->extra_argc; arg++) {
        char *name = options->extra_argv[arg];
        char *filename = NULL;

        const char *ext = GetFileExtension(name);
        if (!ext) {
            ext = ".";
        }
        const char *level_ext = "." LEVEL_FILENAME_EXT;

        if (0 == strcmp(ext, level_ext)) {
            filename = strdup(name);
        } else {
            safe_asprintf(&filename, "%s.%s", name, LEVEL_FILENAME_EXT);
        }

        infomsg("CREATE LEVEL: \"%s\"", filename);

        if (!options->force && FileExists(filename)) {
            errmsg("File already exists: \"%s\"", filename);
            return;
        }

        level_t *level = NULL;
        switch (options->create_level_mode) {
        case CREATE_LEVEL_MODE_BLANK:
            level = generate_blank_level();
            break;

        case CREATE_LEVEL_MODE_RANDOM:
            level = generate_random_level_simple("--create-r5anom-level");
            break;

        default:
            /* do nothing */
            break;
        }

        if (level) {
            level_save_to_filename(level, filename);
            destroy_level(level);
        } else {
            errmsg("Failed to create level \"%s\"", filename);
            return;
        }

        free(filename);
    }

    startup_action_ok = true;
}

void action_pack_collection(void)
{
    infomsg("ACTION: pack");

    for (int arg=0; arg < options->extra_argc; arg++) {
        char *path = options->extra_argv[arg];
        if (!DirectoryExists(path)) {
            errmsg("Directory does not exist: \"%s\"\n", path);
            return;
        }

        const char *dir = directory_without_end_separator(path);
        char *filename = NULL;
        safe_asprintf(&filename, "%s.%s", dir, COLLECTION_FILENAME_EXT);

        infomsg("PACK: \"%s\" -> \"%s\"", path, filename);

        if (!options->force && FileExists(filename)) {
            errmsg("File already exists: \"%s\"", filename);
            return;
        }

        collection_t *collection = load_collection_dir(path);
        if (!collection) {
            errmsg("Couldn't parse dir \"%s\" as a collection", path);
            return;
        }

        collection_save_pack(collection, filename);
        destroy_collection(collection);

        free(filename);
    }

    startup_action_ok = true;
}

void action_unpack_collection(void)
{
    infomsg("ACTION: unpack");

    for (int arg=0; arg < options->extra_argc; arg++) {
        char *path = options->extra_argv[arg];
        if (!FileExists(path)) {
            errmsg("File does not exist: \"%s\"\n", path);
            return;
        }

        const char *dir = GetFileNameWithoutExt(path);

        infomsg("UNPACK: \"%s\" -> \"%s\"", path, dir);

        if (DirectoryExists(dir)) {
            if (options->force) {
                warnmsg("Unpacking into existing directory: \"%s\"", dir);
            } else {
                errmsg("Directory already exists: \"%s\"", dir);
                return;
            }
        } else if (FileExists(dir)) {
            errmsg("Existing file is blocking destination directory \"%s\"", dir);
            return;
        }

        collection_t *collection = load_collection_file(path);
        if (!collection) {
            errmsg("Couldn't parse collection pack file \"%s\"", path);
            return;
        }

        collection_save_dir(collection, dir, false);
        destroy_collection(collection);
    }

    startup_action_ok = true;
}

bool run_startup_action(void)
{
    if (options->startup_action != STARTUP_ACTION_NONE) {
        // options->verbose = true;
    }

    switch (options->startup_action) {
    case STARTUP_ACTION_DEMO_SOLVE:
        /* fall through */
    case STARTUP_ACTION_DEMO_WIN_ANIM:
        /* fall through */
    case STARTUP_ACTION_PLAY:
        /* fall through */
    case STARTUP_ACTION_RANDOM:
        /* fall through */
    case STARTUP_ACTION_EDIT:
        return false;

    case STARTUP_ACTION_CREATE_LEVEL:
        action_create_level();
        return true;

    case STARTUP_ACTION_PACK_COLLECTION:
        action_pack_collection();
        return true;

    case STARTUP_ACTION_UNPACK_COLLECTION:
        action_unpack_collection();
        return true;

    case STARTUP_ACTION_NONE:
        /* fall through */
    default:
        return false;
    }
}
