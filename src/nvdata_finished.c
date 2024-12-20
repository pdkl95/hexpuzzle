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
#include "options.h"
#include "level.h"
#include "collection.h"
#include "nvdata_finished.h"

char *nvdata_state_finished_levels_file_path = NULL;

#define TREE_INDENT_MAXLEN 60
char indentbuf[TREE_INDENT_MAXLEN];

void _print_finished_level(struct finished_level *node, int indent, char type)
{
    if (indent >= TREE_INDENT_MAXLEN) {
        return;
    }

    int nextindent = indent + 1;
    indentbuf[indent] = '\0';

    printf("%s<%c> %s\n", indentbuf, type, node->id);

    int i=indent;
    indentbuf[i] = ' ';
    i++;

    if (node->left && node->right) {
        indentbuf[i] = '|';
        _print_finished_level(node->left, i+1, 'L');
        indentbuf[i] = ' ';
        _print_finished_level(node->right, i+1, 'R');
    } else if (node->left) {
        indentbuf[i] = ' ';
        _print_finished_level(node->left, i+1, 'L');
    } else if (node->right) {
        indentbuf[i] = ' ';
        _print_finished_level(node->right, i+1, 'R');
    } else {
    }
}

void print_finished_level(struct finished_level *node)
{
    _print_finished_level(node, 0, 'T');
}

int compare_finished_level(struct finished_level *a, struct finished_level *b)
{
    assert_not_null(a);
    assert_not_null(b);

    int rv = strcmp(a->id, b->id);
    return rv;
}

SGLIB_DEFINE_RBTREE_FUNCTIONS(finished_level, left, right, rb_color, compare_finished_level);

finished_levels_t finished_levels;

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

void cleanup_nvdata_finished(void)
{
    if (finished_levels.tree) {
        destroy_finished_level(finished_levels.tree);
    }

    SAFEFREE(nvdata_state_finished_levels_file_path);
}

void nvdata_mark_id_finished(char *id)
{
    assert_not_null(id);

    struct finished_level *e, *member;
    e = calloc(1, sizeof(struct finished_level));
    snprintf(&e->id, ID_MAXLEN, "%s", id);

    if (options->verbose) {
        printf("MARK finished: \"%s\"\n", e->id);
    }

    sglib_finished_level_add_if_not_member(&(finished_levels.tree), e, &member);
}

void nvdata_mark_finished(struct level *level)
{
    assert_not_null(level);

    if (!level->have_id) {
        return;
    }

    nvdata_mark_id_finished(level->id);
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
        printf("UNMARK finished: \"%s\"\n", e.id);
    }

    sglib_finished_level_delete_if_member(&(finished_levels.tree), &e, &result);
}

bool nvdata_is_finished(struct level *level)
{
    assert_not_null(level);

    if (!level->have_id) {
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
        fprintf(f, "%s\n", e->id);
        count++;
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
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) >= 3) {
            if (current_collection) {
                level_t *level = collection_find_level_by_id(current_collection, line);
                if (level) {
                    level->finished = true;
                }
            }

            nvdata_mark_id_finished(line);

            count++;
        }
    }

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

    //print_finished_level(finished_levels.tree);
}

void save_nvdata_finished_levels(void)
{
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
}

