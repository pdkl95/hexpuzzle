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

#include "raygui/raygui.h"

#include "options.h"
#include "tile.h"
#include "grid.h"
#include "level.h"

#define LEVEL_DEFAULT_NAME "Untitled"
#define LEVEL_MIN_RADIUS 1
#define LEVEL_MAX_RADIUS 5
#define LEVEL_DEFAULT_RADIUS LEVEL_MIN_RADIUS

static level_t *alloc_level()
{
    level_t *level = calloc(1, sizeof(level_t));

    level->name[0] = '\0';

    level->tile_width  = 0;
    level->tile_height = 0;

    level->id = NULL;
    level->filename = NULL;
    level->tiles = NULL;
    level->old_tiles = NULL;
    level->changed = false;

    level->next = NULL;

    return level;
}

static void level_fill_radius_with_tiles(level_t *level)
{
    int width  = (2 * level->radius) + 1;
    int height = (2 * level->radius) + 1;

    level->tile_width  = width;
    level->tile_height = height;

    hex_axial_t center = {
        .q = level->radius,
        .r = level->radius
    };

    int tile_count = width * height;
    level->tiles = calloc(tile_count, sizeof(tile_t));

    for (int q=0; q < width; q++) {
        for (int r=0; r < height; r++) {
            hex_axial_t pos = {
                .q = q,
                .r = r
            };

            if (hex_axial_distance(pos, center) <= level->radius) {
                int idx = (r * width) + q;
                tile_t *tile = &(level->tiles[idx]);
                init_tile(tile, pos);
                tile->enabled = true;
            }
        }
    }
}

level_t *create_level(void)
{
    level_t *level = alloc_level();

    static int seq = 0;
    seq++;
    snprintf(level->name, NAME_MAXLEN, "%s-%d", LEVEL_DEFAULT_NAME, seq);

    level->radius = LEVEL_DEFAULT_RADIUS;

    level_fill_radius_with_tiles(level);

    return level;
}

void level_clear_tiles(level_t *level)
{
    assert_not_null(level);

    if (level->tiles) {
        destroy_tile(level->tiles);
        level->tiles = NULL;
    }
}

void destroy_level(level_t *level)
{
    if (level) {
        SAFEFREE(level->id);
        SAFEFREE(level->tiles);
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

            int lastidx = strlen(combine) - 1;
            if (combine[lastidx] == '"') {
                combine[lastidx] = '\0';
                tok = combine;
                free_after_use = combine;
                combine = NULL;
            }
        } else if (tok[0] == '"') {
            tok++;
            int lastidx = strlen(tok) - 1;
            if (tok[lastidx] == '"') {
                tok[lastidx] = '\0';
                tokens[n] = strdup(tok);
                n++;
                tok = NULL;
            } else {
                combine = strdup(tok);
                tok = NULL;
            }
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

#if 0
    for (int i=0; i<n; i++) {
        printf("token[%d] = \"%s\"\n", i, tokens[i]);
    }
#endif

    return list;
}

static void level_free_tokens(token_list_t list)
{
    for (int i=0; i<list.token_count; i++) {
        SAFEFREE(list.tokens[i]);
    }
    SAFEFREE(list.tokens);
}

tile_t *level_setup_tile_from_serialized_strings(level_t *level, char *addr, char *path, char *flags)
{
    assert_not_null(level);
    assert_not_null(addr);
    assert_not_null(path);
    assert_not_null(flags);
    assert(strlen(addr)  >= 3);
    assert(strlen(path)  == 6);
    assert(strlen(flags) == 3);

#if 0
    printf("Creating tile from: addr=\"%s\" path=\"%s\" flags=\"%s\"\n",
           addr, path, flags);
#endif

    tile_t *tile = create_tile();

    hex_axial_t pos = {0};
    char *p = addr;
    pos.q = (int)strtol(addr, &p, 10);
    p++;
    pos.r = (int)strtol(p, NULL, 10);

    tile->position = pos;

    for (int i=0; i<6; i++) {
        char digit[2];
        digit[0] = path[i];
        digit[1] = '\0';

        tile->path[i] = (int)strtol(digit, NULL, 10);
    }

    tile_set_flag_from_char(tile, flags[0]);
    tile_set_flag_from_char(tile, flags[1]);
    tile_set_flag_from_char(tile, flags[2]);

    return tile;
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

    snprintf(level->name, NAME_MAXLEN, "%s", list.tokens[4]);
    level->radius     = (int)strtol(list.tokens[6], NULL, 10);
    level->tile_count = (int)strtol(list.tokens[8], NULL, 10);

    //printf("c=%d, r=%d, n=\"%s\"\n", level->tile_count, level->radius, level->name);

    level_fill_radius_with_tiles(level);

    for(int i = 9; i<(list.token_count - 2); i += 4) {
        CMP(i, "tile");
        char *addr  = list.tokens[i+1];
        char *path  = list.tokens[i+2];
        char *flags = list.tokens[i+3];

        level_setup_tile_from_serialized_strings(level, addr, path, flags);
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

bool level_replace_from_memory(level_t *level, char *str)
{
    assert_not_null(level);

    level_clear_tiles(level);

    return level_parse_string(level, str);
}

grid_t *level_create_grid(level_t *level)
{
    assert_not_null(level);

    grid_t *grid = create_grid(level->radius);

    for (int q=0; q < level->tile_width; q++) {
        for (int r=0; r < level->tile_height; r++) {
            int idx = (r * level->tile_width) + q;
            tile_t *tile = &(level->tiles[idx]);
            tile_t *grid_tile = grid_get_tile(grid, tile->position);
            tile_copy_attributes(grid_tile, tile);
        }
    }

    return grid;
}

void level_update_ui_name(level_t *level)
{
    assert_not_null(level);

    int icon = level->finished
        ? ICON_OK_TICK
        : ICON_CROSS_SMALL;

    snprintf(level->ui_name, UI_NAME_MAXLEN, "%s", GuiIconText(icon, level->name));
}

void level_play(level_t *level)
{
    assert_not_null(level);

    if (current_grid) {
        destroy_grid(current_grid);
        current_grid = NULL;
    }

    current_grid = level_create_grid(level);;
    current_level = level;
    game_mode = GAME_MODE_PLAY_LEVEL;
}

void level_save_to_file(level_t *level, char *dirpath)
{
    assert_not_null(level);

    if (!level->filename) {
        errmsg("Asked to save level to a file, but missing filename");
    }

    char *filepath = level->filename;
    char *pathbuf = NULL;
    if (dirpath) {
        int len = strlen(level->filename)
            + strlen(dirpath)
            + 1   // "/"
            + 1;  // \0
        pathbuf = calloc(len, sizeof(char));

        pathbuf[0] = '\0';
        strcat(pathbuf, dirpath);
        if ('/' != dirpath[strlen(dirpath) - 1]) {
            strcat(pathbuf, "/");
        }
        strcat(pathbuf, level->filename);
        filepath = &(pathbuf[0]);
    }

    if (options->verbose) {
        infomsg("saving level \"%s\" to: \"%s\"", level->name, filepath);
    }

    FILE *f = fopen(pathbuf, "w");
    if (!f) {
        errmsg("cannot save level \"%s\" to \"%s\": $a",
               level->name, pathbuf, strerror(errno));
        return;
    }

    level_serialize(level, f);

    fclose(f);
    if (options->verbose) {
        infomsg("save to \"%s\" finished", pathbuf);
    }

    if (pathbuf) {
        free(pathbuf);
    }

    level->changed = false;
}

void level_save_to_file_if_changed(level_t *level, char *dirpath)
{
    assert_not_null(level);

    if (level->changed) {
        level_save_to_file(level, dirpath);
    } else {
        printf("level file \"%s\" hasn't changed - skipping save\n", level->name);
    }
}

void level_extract_from_grid(level_t *level, grid_t *grid)
{
#if 1
    int n=0;
    for (int q=0; q < level->tile_width; q++) {
        for (int r=0; r < level->tile_height; r++) {
            int idx = (r * level->tile_width) + q;
            tile_t *tile = &(level->tiles[idx]);
            tile_t *grid_tile = grid_get_tile(grid, tile->position);
            if (!tile_eq(tile, grid_tile)) {
                n++;
                level->changed = true;
                tile_copy_attributes(tile, grid_tile);
            }
        }
    }
    if (level->changed) {
        printf("extraction finished - %d tiles changed", n);
    } else {
        printf("extraction finished - zero changes!");
    }
#else
    assert_not_null(level);
    assert_not_null(current_level);
    assert_not_null(current_grid);
    assert(level == current_level);

    int size = 40 * current_grid->maxtiles
        + 64 //50    // header
        + 32 //24    // footer
        + NAME_MAXLEN;

    char *buf = calloc(size, sizeof(char));

    FILE *f = fmemopen(buf, size, "w");
    grid_serialize(current_grid, f);
    long filesize = ftell(f);
    fclose(f);
    buf[filesize + 1] = '\0';

#if 0
    printf("\nBEG>>>\n");
    fwrite(buf, 1, filesize, stdout);
    printf("\n<<<END\n");
#endif

    level_replace_from_memory(level, buf);

    free(buf);
#endif
}

void level_serialize(level_t *level, FILE *f)
{
    fprintf(f, "hexlevel version 1\n");
    fprintf(f, "name \"%s\"\n", level->name);
    fprintf(f, "radius %d\n", level->radius);
    int total_tiles = level->tile_width * level->tile_height;
    fprintf(f, "begin_tiles %d\n", total_tiles);
    for (int i=0; i<total_tiles; i++) {
        tile_serialize(&level->tiles[i], f);
    }
    fprintf(f, "end_tiles\n");
}
