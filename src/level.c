/****************************************************************************
 *                                                                          *
 * level.c                                                                  *
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
#include "tile.h"
#include "grid.h"
#include "level.h"

static level_t *alloc_level()
{
    level_t *level = calloc(1, sizeof(level_t));
    return level;
}

void destroy_level(level_t *level)
{
    if (level) {
        SAFEFREE(level->name);
        SAFEFREE(level->filename);
        SAFEFREE(level);
    }
}

struct token_list {
    int token_count;
    char **tokens;
};
typedef struct token_list token_list_t;

static token_list_t level_tokenize_string(char *str)
{
    assert_not_null(str);

    char *delim = " \n";

    int token_count = 0;
    char *scan = str;

    while (true) {
        scan = strpbrk(scan, delim);
        if (scan) {
            scan++;
            token_count++;
        } else {
            break;
        }
    }

    char **tokens = calloc(token_count + 1, sizeof(char *));

    int n=0;
    char *combine = NULL;
    char *free_after_use = NULL;
    char *tok = str, *end = str;
    while (tok != NULL) {
        strsep(&end, delim);

        if (combine) {
            char *tmp;
            asprintf(&tmp, "%s %s", combine, tok);
            free(combine);
            combine = tmp;
            tok = NULL;

            if (combine[strlen(combine) - 1] == '"') {
                tok = combine;
                free_after_use = combine;
                combine = NULL;
            }
        } else if (tok[0] == '"') {
            combine = strdup(tok);
            tok = NULL;
        }

        if (tok) {
            if (strlen(tok) > 0) {
                tokens[n] = strdup(tok);
                n++;
            }

            if (free_after_use) {
                free(free_after_use);
                free_after_use = NULL;
            }
        }

        tok = end;
    }

    tokens[n] = NULL;

    token_list_t list = {
        .tokens = tokens,
        .token_count = n
    };

    return list;
}

static void level_free_tokens(token_list_t list)
{
    for (int i=0; i<list.token_count; i++) {
        SAFEFREE(list.tokens[i]);
    }
    SAFEFREE(list.tokens);
}

bool level_parse_string(level_t *level, char *str)
{
    assert_not_null(level);
    assert_not_null(str);

    token_list_t list = level_tokenize_string(str);

#define CMPSTR(test, expected) do {                                  \
        if (0 != strncmp(test, expected, strlen(expected))) {        \
            fprintf(stderr,                                          \
                    "Parse failed: expected \"%s\", found \"%s\"\n", \
                    expected, test);                                 \
            goto fail;                                               \
        }                                                            \
    } while(0)
#define CMP(idx, expected) \
    CMPSTR(list.tokens[idx], expected)

    CMP(0, "hexlevel");
    CMP(1, "version");
    CMP(3, "name");
    CMP(5, "radius");
    CMP(7, "begin_tiles");

    level->name   = strdup(list.tokens[4]);
    level->radius     = (int)strtol(list.tokens[6], NULL, 10);
    level->tile_count = (int)strtol(list.tokens[8], NULL, 10);

    printf("c=%d, r=%d, n=\"%s\"\n", level->tile_count, level->radius, level->name);

    for(int i = 9; i<(list.token_count - 2); i += 4) {
        CMP(i, "tile");
        char *addr  = list.tokens[i+1];
        char *path  = list.tokens[i+2];
        char *flags = list.tokens[i+3];

        tile_t *tile = create_tile_from_serialized_strings(addr, path, flags);
        tile->next = level->tiles;
        level->tiles = tile;
    }


    level_free_tokens(list);
    return true;

  fail:
    level_free_tokens(list);
    return false;
}

static char *read_file_into_string(char *filename)
{
    assert_not_null(filename);

    FILE *f = fopen(filename, "rb");

    if (!f) {
        fprintf(stderr, "Could not open level file \"%s\": %s",
                filename, strerror(errno));
        return NULL;
    }

    fseek(f, 0L, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char *str = calloc(1, size + 1);

    if (1 != fread(str, size, 1, f)) {
        fclose(f);
        free(str);
        fprintf(stderr, "Reading from file \"%s\" failed.", filename);
        exit(1);
    }

    fclose(f);

    //fprintf(stderr, "Successfully read %zd characters from file \"%s\"\n", size, filename);

    return str;
}

level_t *load_level_file(char *filename)
{
    assert_not_null(filename);

    char *str = read_file_into_string(filename);
    if (NULL == str) {
        return NULL;
    }

    level_t *level = alloc_level();

    if (level_parse_string(level, str)) {
        free(str);
        level->filename = strdup(filename);
        return level;
    } else {
        free(str);
        return NULL;
    }
}

grid_t *level_create_grid(level_t *level)
{
    grid_t *grid = create_grid(level->radius);

    tile_t *tile = level->tiles;
    while (tile) {
        tile_t *grid_tile = grid_get_tile(grid, tile->position);\
        tile_copy_attributes(grid_tile, tile);
        tile = tile->next;
    }

    return grid;
}
