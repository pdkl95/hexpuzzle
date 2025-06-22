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
#include "nvdata_finished.h"

char *nvdata_state_finished_levels_file_path = NULL;
char *nvdata_state_finished_levels_backup_file_path = NULL;

#define TREE_INDENT_MAXLEN 60
char indentbuf[TREE_INDENT_MAXLEN];

SGLIB_DEFINE_RBTREE_FUNCTIONS(finished_level, left, right, rb_color, compare_finished_level);

finished_levels_t finished_levels;

void print_finished_levels(void)
{
    struct finished_level *e = NULL;
    struct sglib_finished_level_iterator it;
    int count = 0;

    printf("<finished_levels>\n");

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

    int rv = strcmp(a->id, b->id);
    return rv;
}

void init_nvdata_finished(void)
{
    finished_levels.tree = NULL;
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
}

void cleanup_nvdata_finished(void)
{
    destroy_finished_levels_tree();

    SAFEFREE(nvdata_state_finished_levels_backup_file_path);
    SAFEFREE(nvdata_state_finished_levels_file_path);
}

void nvdata_mark_id_finished(char *id, time_t win_time)
{
    assert_not_null(id);

    struct finished_level *e, *member;
    e = calloc(1, sizeof(struct finished_level));
    snprintf(&e->id, ID_MAXLEN, "%s", id);
    e->win_time = win_time;

    if (options->verbose) {
        infomsg("MARK finished: \"%s\"", e->id);
    }

    sglib_finished_level_add_if_not_member(&(finished_levels.tree), e, &member);
}

void nvdata_mark_finished(struct level *level)
{
    assert_not_null(level);

    if (!level->finished || demo_mode) {
        return;
    }

    nvdata_mark_id_finished(level->unique_id, level->win_time);
}

void nvdata_unmark_finished(struct level *level)
{
    assert_not_null(level);

    if (!level->have_id) {
        return;
    }

    struct finished_level e, *result;
    e.left = e.right = NULL;
    snprintf(&e.id, ID_MAXLEN, "%s", level->id);

    if (options->verbose) {
        infomsg("UNMARK finished: \"%s\"", e.id);
    }

    sglib_finished_level_delete_if_member(&(finished_levels.tree), &e, &result);
}

bool nvdata_is_finished(struct level *level)
{
    assert_not_null(level);

    if (!level->have_id || demo_mode) {
        return false;
    }

    struct finished_level e, *result;
    e.left = e.right = NULL;
    snprintf(&e.id, ID_MAXLEN, "%s", level->id);
    return NULL != sglib_finished_level_find_member(finished_levels.tree, &e);
}

void nvdata_finished_write(FILE *f)
{
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
        fprintf(f, "%s\t%s\n", e->id, win_time_str);
        count++;
        //printf("WRITE \"%s\t%s\"\n", e->id, win_time_str);
    }

    if (options->verbose) {
        infomsg("Wrote %d level IDs", count);
    }
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

    FILE *f = fopen(nvdata_state_finished_levels_file_path, "r");
    if (NULL == f) {
        errmsg("Couldn't open finished level file \"%s\": %s",
               nvdata_state_finished_levels_file_path, strerror(errno));
        return;
    }

    char * line = NULL;
    size_t len = 0;
    int count = 0;

    while ((getline(&line, &len, f)) != -1) {
        assert_not_null(line);
        line[strcspn(line, "\n")] = '\0';

        char *win_time_str = line;
        char *id_str = strsep(&win_time_str, "\t");

#if 0
        pstr(win_time_str);
        pstr(id_str);
#endif

        char *errsrc = NULL;
        if (!win_time_str) {
            errsrc = "win_time";
        }

        if (!id_str) {
            errsrc = "unique_id";
        }

        if (errsrc) {
            warnmsg("While reading \"%s\", line %d: error parsing %s",
                    nvdata_state_finished_levels_file_path,
                    count + 1,
                    errsrc);
            goto load_finished_levels_cleanup;
        }

        struct tm win_tm = {0};
        strptime(win_time_str, "%F %T", &win_tm);
        time_t win_time = mktime(&win_tm);

        if (strlen(line) >= 3) {
            if (current_collection) {
                level_t *level = collection_find_level_by_unique_id(current_collection, id_str);
                if (level) {
                    level->finished = true;
                    level->win_time = win_time;
                }
            }

            nvdata_mark_id_finished(line, win_time);

            count++;
        }
    }

  load_finished_levels_cleanup:
    if (line) {
        free(line);
    }

    fclose(f);

    if (options->verbose) {
        infomsg("Read %d level IDs", count);
    }

    if (current_collection) {
        collection_update_level_names(current_collection);
    }

    //print_finished_levels();
}

void save_nvdata_finished_levels(void)
{
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

    FILE *f = fopen(nvdata_state_finished_levels_file_path, "w");
    if (NULL == f) {
        errmsg("Couldn't open finished level file \"%s\": %s",
               nvdata_state_finished_levels_file_path, strerror(errno));
        return;
    }

    nvdata_finished_write(f);

    fclose(f);

    if (options->verbose) {
        infomsg("Finished level data saved in \"%s\"",
                nvdata_state_finished_levels_file_path);
    }
    //print_finished_levels();
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
