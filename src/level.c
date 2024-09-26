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

#include <libgen.h>

#include "raygui/raygui.h"

#include "options.h"
#include "tile.h"
#include "tile_pos.h"
#include "tile_draw.h"
#include "level.h"
#include "collection.h"
#include "shader.h"
#include "win_anim.h"

//#define DEBUG_DRAG_AND_DROP 1

bool feature_single_sector_editing = false;
bool feature_adjacency_editing     = true;

void print_tiles(level_t *level)
{
    level_sort_tiles(level);

    printf("Level \"%s\" tiles:\n", level->name);
    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = level->sorted_tiles[i];
        printf("%02d: ", i);
        print_tile(tile);
    }
}

void print_tile_solved_positionss(level_t *level)
{
    printf("Level \"%s\" solved tile positionss:\n", level->name);

    for (int i = 0; i < LEVEL_MAXTILES; i++) {
        tile_pos_t *pos = &level->solved_positions[i];
        printf("idx=%02d [%d, %d] ", i, pos->position.q, pos->position.r);
        print_tile(pos->tile);
    }
}

void print_tile_unsolved_positionss(level_t *level)
{
    printf("Level \"%s\" unsolved tile positionss:\n", level->name);

    for (int i = 0; i < LEVEL_MAXTILES; i++) {
        tile_pos_t *pos = &level->unsolved_positions[i];
        printf("idx=%02d [%d, %d] ", i, pos->position.q, pos->position.r);
        print_tile(pos->tile);
    }
}

static void level_enable_solved_tile_callback(hex_axial_t axial, void *data)
{
    level_t *level = (level_t *)data;
    tile_pos_t *pos = level_get_solved_tile_pos(level, axial);
    if (pos) {
        if (pos->tile) {
            pos->tile->enabled = true;
        } else {
            assert(false && "pos is missing tike");
        }
    }
}

static void level_enable_unsolved_tile_callback(hex_axial_t axial, void *data)
{
    level_t *level = (level_t *)data;
    tile_pos_t *pos = level_get_solved_tile_pos(level, axial);
    if (pos) {
        pos->tile->enabled = true;
    }
}

static void level_enable_current_tile_callback(hex_axial_t axial, void *data)
{
    level_t *level = (level_t *)data;
    switch (level->currently_used_tiles) {
    case USED_TILES_NULL:
        if (options->verbose) {
            errmsg("level_toggle_currently_used_tiles() No tile set in use!");
        }
        assert(false && "null tile set");
        break;

    case USED_TILES_SOLVED:
        level_enable_solved_tile_callback(axial, data);
        break;

    case USED_TILES_UNSOLVED:
        level_enable_unsolved_tile_callback(axial, data);
        break;
    }
}

static void level_disable_solved_tile_callback(hex_axial_t axial, void *data)
{
    level_t *level = (level_t *)data;
    tile_pos_t *pos = level_get_solved_tile_pos(level, axial);
    if (pos) {
        pos->tile->enabled = false;
    }
}

static void level_disable_unsolved_tile_callback(hex_axial_t axial, void *data)
{
    level_t *level = (level_t *)data;
    tile_pos_t *pos = level_get_solved_tile_pos(level, axial);
    if (pos) {
        pos->tile->enabled = false;
    }
}

static void level_disable_current_tile_callback(hex_axial_t axial, void *data)
{
    level_t *level = (level_t *)data;
    switch (level->currently_used_tiles) {
    case USED_TILES_NULL:
        /* do nothing */
        if (options->verbose) {
            errmsg("level_toggle_currently_used_tiles() No tile set in use!");
        }
        break;

    case USED_TILES_SOLVED:
        level_disable_solved_tile_callback(axial, data);
        break;

    case USED_TILES_UNSOLVED:
        level_disable_unsolved_tile_callback(axial, data);
        break;
    }
}

void level_use_solved_tile_pos(level_t *level)
{
    level->currently_used_tiles = USED_TILES_SOLVED;
}

void level_use_unsolved_tile_pos(level_t *level)
{
    level->currently_used_tiles = USED_TILES_UNSOLVED;
}

void level_use_null_tile_pos(level_t *level)
{
    level->currently_used_tiles = USED_TILES_NULL;
}

void level_toggle_currently_used_tiles(level_t *level)
{
    switch (level->currently_used_tiles) {
    case USED_TILES_NULL:
        /* do nothing */
        if (options->verbose) {
            infomsg("level_toggle_currently_used_tiles() NULL");
        }
        level_use_null_tile_pos(level);
        break;

    case USED_TILES_SOLVED:
        if (options->verbose) {
            infomsg("level_toggle_currently_used_tiles() Using UNSOLVED tiles");
        }
        level_use_unsolved_tile_pos(level);
        break;

    case USED_TILES_UNSOLVED:
        if (options->verbose) {
            infomsg("level_toggle_currently_used_tiles() Using SOLVED tiles");
        }
        level_use_solved_tile_pos(level);
        break;
    }
}

static level_t *alloc_level()
{
    level_t *level = calloc(1, sizeof(level_t));

    int i=0;
    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            hex_axial_t addr = {
                .q = q,
                .r = r
            };

            init_tile(&level->tiles[i]);
            init_tile_pos(&level->solved_positions[i], &level->tiles[i], addr);
            init_tile_pos(&level->unsolved_positions[i], &level->tiles[i], addr);

            level->sorted_tiles[i] = &(level->tiles[i]);

            level->tiles[i].solved_pos   = &level->solved_positions[i];
            level->tiles[i].unsolved_pos = &level->unsolved_positions[i];

            i++;
        }
    }

    level->win_anim = create_win_anim(level);

    level->name[0] = '\0';

    level->id = NULL;
    level->filename = NULL;
    level->changed = false;

    level->next = NULL;

    level->finished_hue = 0.0f;

    level->fade_value        = 0.0f;
    level->fade_value_eased  = 0.0f;
    level->fade_delta        = 0.0f;
    level->fade_target       = 0.0f;

    level->radius = LEVEL_MIN_RADIUS;

    for (int i = 0; i < LEVEL_MAXTILES; i++) {
        for (hex_direction_t section = 0; section < 6; section++) {
            tile_pos_t *solved_pos   = &level->solved_positions[i];
            tile_pos_t *unsolved_pos = &level->unsolved_positions[i];
            solved_pos->neighbors[section]   = level_find_solved_neighbor_tile_pos(  level,   solved_pos, section);
            unsolved_pos->neighbors[section] = level_find_unsolved_neighbor_tile_pos(level, unsolved_pos, section);
        }
    }

    level_reset(level);

    return level;
}

void level_reset(level_t *level)
{
    assert_not_null(level);

    level->req_tile_size = 60.0f;

    level->drag_reset_total_frames = 12;
    level->drag_reset_frames = 0;

    level->hover = NULL;
    level->hover_section_adjacency_radius = 16.0;
    level->drag_target = NULL;

    level->center = LEVEL_CENTER_POSITION;

    level_resize(level);

    level_use_solved_tile_pos(level);
    level_enable_spiral(level, level->radius);

    level_use_solved_tile_pos(level);
    level_enable_spiral(level, level->radius);

    level_use_null_tile_pos(level);
}

level_t *create_level(void *collection)
{
    collection_t *c = (collection_t *)collection;
    level_t *level = alloc_level();

    static int seq = 0;
    do {
        seq++;
        snprintf(level->name, NAME_MAXLEN, "%s-%d", LEVEL_DEFAULT_NAME, seq);
    } while (c && collection_level_name_exists(c, level->name));

    level->radius = LEVEL_DEFAULT_RADIUS;

    level->changed = true;

    return level;
}

void destroy_level(level_t *level)
{
    if (level) {
        SAFEFREE(level->id);
        SAFEFREE(level->filename);
        SAFEFREE(level);
    }
}

bool level_eq_tiles(level_t *level, level_t *other)
{
    level_sort_tiles(level);
    level_sort_tiles(other);

    for (int i=0; i < LEVEL_MAXTILES; i++) {
        tile_t *level_tile = level->sorted_tiles[i];
        tile_t *other_tile = other->sorted_tiles[i];

        if (!tile_eq(level_tile, other_tile)) {
            return false;
        }
    }

    return true;
}

void level_sort_tiles(level_t *level)
{
    qsort(level->sorted_tiles, LEVEL_MAXTILES, sizeof(level_t *), compare_tiles);
}

tile_pos_t *level_get_center_tile_pos(level_t *level)
{
    return level_get_current_tile_pos(level, LEVEL_CENTER_POSITION);
}

tile_pos_t *level_get_current_tile_pos(level_t *level,  hex_axial_t axial)
{
    assert_not_null(level);
    RETURN_NULL_IF_OUT_OF_BOUNDS;
    switch (level->currently_used_tiles) {
    case USED_TILES_NULL:
        assert(false && "not using any tile set");
        __builtin_unreachable();
        return NULL;

    case USED_TILES_SOLVED:
        return level_get_solved_tile_pos(level, axial);

    case USED_TILES_UNSOLVED:
        return level_get_unsolved_tile_pos(level, axial);
    }

    __builtin_unreachable();
    return NULL;
}

tile_t *level_get_tile(level_t *level,  hex_axial_t axial)
{
    tile_pos_t *pos = level_get_current_tile_pos(level, axial);
    return pos->tile;
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

void level_setup_tiles_from_serialized_strings(level_t *level, char *solved_addr_str, char *unsolved_addr_str, char *path, char *flags)
{
    assert_not_null(level);
    assert_not_null(solved_addr_str);
    assert_not_null(unsolved_addr_str);
    assert_not_null(path);
    assert_not_null(flags);

    assert(strlen(solved_addr_str)   >= 3);
    assert(strlen(unsolved_addr_str) >= 3);
    assert(strlen(path)  == 6);
    assert(strlen(flags) == 3);

#if 0
    printf("Creating tile from: solved_addr=\"%s\" unsolved_addr=\"%s\" path=\"%s\" flags=\"%s\"\n",
           solved_addr, unsolved_addr, path, flags);
#endif

    hex_axial_t solved_addr = {0}, unsolved_addr = {0};

    char *p = solved_addr_str;
    solved_addr.q = (int)strtol(solved_addr_str, &p, 10);
    p++;
    solved_addr.r = (int)strtol(p, NULL, 10);

    p = unsolved_addr_str;
    unsolved_addr.q = (int)strtol(unsolved_addr_str, &p, 10);
    p++;
    unsolved_addr.r = (int)strtol(p, NULL, 10);

    tile_pos_t   *solved_pos = level_get_solved_tile_pos(  level,   solved_addr);
    tile_pos_t *unsolved_pos = level_get_unsolved_tile_pos(level, unsolved_addr);

    if (!solved_pos) {
        errmsg("Cannot find tile with solved_address (%d, %d)\n", solved_addr.q, solved_addr.r);
        return;
    }
    if (!unsolved_pos) {
        errmsg("Cannot find tile with unsolved_address (%d, %d)\n", unsolved_addr.q, unsolved_addr.r);
        return;
    }

    tile_t *tile = &(level->tiles[level->current_tile_write_idx]);
    tile->solved_pos = solved_pos;
    tile->unsolved_pos = unsolved_pos;
    solved_pos->tile = tile;
    unsolved_pos->tile = tile;

    for (int i=0; i<6; i++) {
        char digit[2];
        digit[0] = path[i];
        digit[1] = '\0';

        path_type_t ptype = (int)strtol(digit, NULL, 10);
        tile->path[i] = ptype;
    }

    for (int i=0; i<3; i++) {
        tile_set_flag_from_char(tile, flags[i]);
    }

    level->current_tile_write_idx++;
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

    level->current_tile_write_idx = 0;

    for(int i = 9; i<(list.token_count - 2); i += 5) {
        CMP(i, "tile");
        char *solved_addr   = list.tokens[i+1];
        char *unsolved_addr = list.tokens[i+2];
        char *path          = list.tokens[i+3];
        char *flags         = list.tokens[i+4];

        level_setup_tiles_from_serialized_strings(level, solved_addr, unsolved_addr, path, flags);
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

void level_set_file_path(level_t *level, char *path)
{
    level->filename = strdup(basename(path));
    level->dirpath  = strdup(dirname(path));
}

level_t *load_level_string(char *filename, char *str)
{
    level_t *level = alloc_level();

#if 0
    if (options->verbose) {
        printf("Parsing level file data:\n");
        printf("----- BEGIN LEVEL FILE DATA -----\n");
        puts(str);
        if (last_char(str) != '\n') {
            puts("\n");
        }
        printf("----- END LEVEL FILE DATA -----\n");
    }
#endif

    if (level_parse_string(level, str)) {
        level_set_file_path(level, filename);
        if (options->verbose) {
            infomsg("Successfully loaded level file \"%s/%s\"",
                    level->dirpath, level->filename);
        }
        return level;
    } else {
        errmsg("Error parsing level file \"%s\"",
               filename);
        free(str);
        return NULL;
    }
}

level_t *load_level_file(char *filename)
{
    assert_not_null(filename);

    char *str = read_file_into_string(filename);
    if (NULL == str) {
        errmsg("Error reading level file \"%s\"", filename);
        return NULL;
    }

    level_t *level = load_level_string(filename, str);
    free(str);
    return level;
}

void level_update_ui_name(level_t *level, UNUSED int idx)
{
    assert_not_null(level);

    int icon = level->finished
        ? ICON_OK_TICK
        : ICON_CROSS_SMALL;

    snprintf(level->ui_name, UI_NAME_MAXLEN, "%s", GuiIconText(icon, level->name));
}

void level_unload(void)
{
    if (current_level) {
        // unload level?
        current_level = NULL;

        switch (game_mode) {
        case GAME_MODE_PLAY_LEVEL:
            game_mode = GAME_MODE_PLAY_COLLECTION;
            break;

        case GAME_MODE_EDIT_LEVEL:
            game_mode = GAME_MODE_EDIT_COLLECTION;
            break;

        default:
            assert(false && "shouldn't be called in this game mode");
        }
    }
}

void level_load(level_t *level)
{
    level_unload();
    current_level = level;
    level_reset(current_level);
}

void level_play(level_t *level)
{
    assert_not_null(level);

    level_load(level);
    level_use_unsolved_tile_pos(level);
    level_fade_in(level);
    game_mode = GAME_MODE_PLAY_LEVEL;
}

void level_edit(level_t *level)
{
    assert_not_null(level);

    level_load(level);
    level_use_solved_tile_pos(level);
    game_mode = GAME_MODE_EDIT_LEVEL;
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

static int level_count_enabled_tiles(level_t *level)
{
    int count = 0;
    for (int i=0; i < TILE_LEVEL_WIDTH; i++) {
        tile_t *tile = &(level->tiles[i]);
        if (tile->enabled) {
            count++;
        }
    }
    return count;
}

void level_serialize(level_t *level, FILE *f)
{
    level_sort_tiles(level);

    fprintf(f, "hexlevel version 1\n");
    fprintf(f, "name \"%s\"\n", level->name);
    fprintf(f, "radius %d\n", level->radius);
    int total_tiles = level_count_enabled_tiles(level);
    fprintf(f, "begin_tiles %d\n", total_tiles);
    for (int i=0; i < LEVEL_MAXTILES; i++) {
        tile_t *tile = level->sorted_tiles[i];
        tile_serialize(tile, f);
    }
    fprintf(f, "end_tiles\n");
}

char *level_serialize_memory(level_t *level)
{
    static char buf[LEVEL_SERIALIZE_BUFSIZE];

    FILE *f = fmemopen(buf, LEVEL_SERIALIZE_BUFSIZE, "w");
    level_serialize(level, f);
    fclose(f);

    return buf;
}

static void level_add_to_bounding_box(level_t *level, tile_pos_t *pos)
{
    assert_not_null(level);
    assert_not_null(pos);

    Vector2 *corners = pos->corners;
    for (int i=0; i<6; i++) {
        level->px_min.x = MIN(level->px_min.x, corners[i].x);
        level->px_min.y = MIN(level->px_min.y, corners[i].y);

        level->px_max.x = MAX(level->px_max.x, corners[i].x);
        level->px_max.y = MAX(level->px_max.y, corners[i].y);
   }
}

void level_resize(level_t *level)
{
    assert_not_null(level);

    //tile_pos_t *center_tile = level_get_center_tile_pos(level);

    Vector2 window_level_margin = { 0.8, 0.8 };
    Vector2 window = Vector2Scale(ivector2_to_vector2(window_size), 1.0);
    Vector2 max_level_size_px = Vector2Multiply(window, window_level_margin);
    int level_width_in_hex_radii = 2 + (3 * level->radius);
    int current_level_height =  ((2 * level->radius) + 1);
    Vector2 max_tile_size = {
        .x =  max_level_size_px.x / ((float)level_width_in_hex_radii),
        .y = (max_level_size_px.y / current_level_height) * INV_SQRT_3
    };

    level->tile_size = MIN(level->req_tile_size,
                          MIN(max_tile_size.x,
                              max_tile_size.y));

#if 0
    printf(">>>=-- ~ --=<<<\n");
    pfloat(level->tile_size);
    pvec2(window_level_margin);
    pvec2(max_level_size_px);
    pint(level_width_in_hex_radii);
    pvec2(max_tile_size);
    pfloat(level->tile_size);
#endif

    level->px_min.x = (float)window_size.x * 10.0;
    level->px_min.y = (float)window_size.y * 10.0;

    level->px_max.x = 0.0f;
    level->px_max.y = 0.0f;

    level_sort_tiles(level);

    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(level->tiles[i]);
        assert_not_null(tile);

        tile_pos_t *solved_pos   = tile->solved_pos;
        tile_pos_t *unsolved_pos = tile->unsolved_pos;
        tile_pos_set_size(  solved_pos, level->tile_size);
        tile_pos_set_size(unsolved_pos, level->tile_size);
        if (tile->enabled) {
            level_add_to_bounding_box(level, solved_pos);
        }
    }

    level->px_bounding_box.x = level->px_min.x;
    level->px_bounding_box.y = level->px_min.y;
    level->px_bounding_box.width  = level->px_max.x - level->px_min.x;
    level->px_bounding_box.height = level->px_max.y - level->px_min.y;

    level->px_offset.x = ((float)window_size.x - level->px_bounding_box.width)  / 2;
    level->px_offset.y = ((float)window_size.y - level->px_bounding_box.height) / 2;

    level->px_offset.x -= level->px_bounding_box.x;
    level->px_offset.y -= level->px_bounding_box.y;

#if 0
    printf("-- px --\n");
    printf("window_size = (%d x %d)\n", window_size.x, window_size.y);
    printf("px_min = [ %f, %f ]\n", level->px_min.x, level->px_min.y);
    printf("px_max = [ %f, %f ]\n", level->px_max.x, level->px_max.y);
    printf("px_bounding_box:\n");
    printrect(level->px_bounding_box);
    printf("px_offset = [ %f, %f ]\n", level->px_offset.x, level->px_offset.y);
#endif
}

tile_pos_t *level_find_solved_neighbor_tile_pos(level_t *level, tile_pos_t *pos, hex_direction_t section)
{
    section = (section + 1) % 6;
    hex_axial_t neighbor_pos = hex_axial_neighbor(pos->position, section);
    tile_pos_t *neighbor = level_get_solved_tile_pos(level, neighbor_pos);
    return neighbor;
}

tile_pos_t *level_find_unsolved_neighbor_tile_pos(level_t *level, tile_pos_t *pos, hex_direction_t section)
{
    section = (section + 1) % 6;
    hex_axial_t neighbor_pos = hex_axial_neighbor(pos->position, section);
    tile_pos_t *neighbor = level_get_unsolved_tile_pos(level, neighbor_pos);
    return neighbor;
}

tile_pos_t *level_find_current_neighbor_tile_pos(level_t *level, tile_pos_t *pos, hex_direction_t section)
{
    section = (section + 1) % 6;
    hex_axial_t neighbor_pos = hex_axial_neighbor(pos->position, section);
    tile_pos_t *neighbor = level_get_current_tile_pos(level, neighbor_pos);
    return neighbor;
}

bool level_check(level_t *level)
{
    assert_not_null(level);

    path_int_t total = {0};

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            hex_axial_t axial = {
                .q = q,
                .r = r
            };
            tile_pos_t *pos = level_get_current_tile_pos(level, axial);
            if (pos->tile->enabled) {
                if (!tile_pos_check(pos)) {
                    return false;
                }

                path_int_t counts = tile_count_path_types(pos->tile);
                for (int i=0; i<PATH_TYPE_COUNT; i++) {
                    total.path[i] += counts.path[i];
                }
            }
        }
    }

    for (int i=0; i<PATH_TYPE_COUNT; i++) {
        if (i == PATH_TYPE_NONE) {
            continue;
        }
        if (total.path[i] > 0) {
            return true;
        }
    }

    // level is blank
    return false;
}

void level_set_hover(level_t *level, IVector2 mouse_position)
{
    if (level) {
        if (level->hover) {
            if (level->hover_adjacent) {
                tile_pos_unset_hover_adjacent(level->hover);
                tile_pos_unset_hover_adjacent(level->hover_adjacent);
                level->hover_adjacent = NULL;
            }

            tile_pos_unset_hover(level->hover);
            level->hover->hover = false;
        }

        level->mouse_pos.x = (float)mouse_position.x;
        level->mouse_pos.y = (float)mouse_position.y;

        if (level->drag_target) {
            if (level->drag_reset_frames > 0) {
                float reset_fract = ((float)level->drag_reset_frames) / ((float)level->drag_reset_total_frames);
                reset_fract = ease_exponential_in(reset_fract);
                level->drag_offset = Vector2Scale(level->drag_reset_vector, reset_fract);
                level->drag_reset_frames--;
                if (0 == level->drag_reset_frames) {
                    level->drag_target = NULL;
                    disable_automatic_events();
                }
            } else {
                level->drag_offset = Vector2Subtract(level->mouse_pos, level->drag_start);
            }
        } else {
            level->drag_offset.x = 0.0f;
            level->drag_offset.y = 0.0f;
        }

        Vector2 mouse_tile_pos = Vector2Subtract(level->mouse_pos, level->px_offset);
        hex_axial_t mouse_hex = pixel_to_hex_axial(mouse_tile_pos, level->tile_size);

        level->hover = level_get_current_tile_pos(level, mouse_hex);

        if (level->hover) {
            if (level->hover->tile->enabled) {
                if (edit_mode && feature_adjacency_editing) {
                    tile_pos_t *pos = level->hover;
                    Vector2 midpoint = pos->midpoints[pos->hover_section];
                    midpoint = Vector2Add(midpoint, level->px_offset);
                    if (Vector2Distance(midpoint, level->mouse_pos) < level->hover_section_adjacency_radius) {
                        level->hover_adjacent = level_find_current_neighbor_tile_pos(level, pos, pos->hover_section);
                        if (level->hover_adjacent) {
                            level->hover_section = pos->hover_section;
                            level->hover_adjacent_section = hex_opposite_direction(level->hover_section);
                            tile_pos_set_hover_adjacent(level->hover,          level->hover_section,          level->hover_adjacent);
                            tile_pos_set_hover_adjacent(level->hover_adjacent, level->hover_adjacent_section, level->hover);
                        }
                    }
                }

                tile_pos_set_hover(level->hover, Vector2Subtract(level->mouse_pos, level->px_offset));
            }
        }
    }
}

void level_drag_start(level_t *level)
{
    assert_not_null(level);

    if (level->drag_target) {
#ifdef DEBUG_DRAG_AND_DROP
        printf("drag_stop(): hover = %p\n", level->hover);
#endif
        level->drag_target = NULL;
    }

    if (level->hover && level_using_unsolved_tiles(level)) {
        if (level->hover->tile->enabled && !level->hover->tile->fixed) {
            level->drag_target = level->hover;
            level->drag_start  = level->mouse_pos;
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_start(): drag_target = %p\n", level->drag_target);
#endif
        }
    }
}

void level_swap_tile_pos(level_t *level, tile_pos_t *a, tile_pos_t *b)
{
    assert_not_null(level);
    assert_not_null(a);
    assert_not_null(b);

    assert(a != b);
    assert(a->tile != b->tile);
    assert(!((a->position.q == b->position.q) && (a->position.r == b->position.r)));

    tile_t *old_a_tile = a->tile;
    tile_t *old_b_tile = b->tile;

#ifdef DEBUG_DRAG_AND_DROP
    printf("swap_tile_pos(): a=(%d, %d)\n", a->position.q, a->position.r);
    printf("                 b=(%d, %d)\n", b->position.q, b->position.r);
#endif

    a->tile = old_b_tile;
    b->tile = old_a_tile;

    switch (level->currently_used_tiles) {
    case USED_TILES_NULL:
        assert(false && "trying to swap tiles but not using any tile set");
        break;

    case USED_TILES_SOLVED:
        old_a_tile->solved_pos = b;
        old_b_tile->solved_pos = a;
        break;

    case USED_TILES_UNSOLVED:
        old_a_tile->unsolved_pos = b;
        old_b_tile->unsolved_pos = a;
        break;
    }

    level->changed = true;
}



void level_drop_tile(level_t *level, tile_pos_t *drag_target, tile_pos_t *drop_target)
{
    assert_not_null(level);
    assert_not_null(drag_target);
    assert_not_null(drop_target);

    assert(drag_target->tile->enabled);
    assert(drop_target->tile->enabled);

    if (!drop_target->tile->fixed && level_using_unsolved_tiles(level) && (drag_target != drop_target)) {
        level_swap_tile_pos(level, level->drag_target, drop_target);
    }
}

void level_drag_stop(level_t *level)
{
    assert_not_null(level);

    if (level->drag_target) {
        tile_pos_t *drop_target = level->hover;

        if (drop_target && !drop_target->tile->enabled) {
            drop_target = NULL;
        }

        if (drop_target && !drop_target->tile->fixed && !drop_target->tile->hidden) {
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_stop(): drop target\n");
#endif
            level_drop_tile(level, level->drag_target, drop_target);
            level->drag_target = NULL;
        } else {
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_stop(): reset\n");
#endif
            level->drag_reset_frames = level->drag_reset_total_frames;;
            level->drag_reset_vector = level->drag_offset;
            enable_automatic_events();
        }
    } else {
#ifdef DEBUG_DRAG_AND_DROP
        printf("drag_stop(): missing drag target\n");
#endif
    }
}

void level_modify_hovered_feature(level_t *level)
{
    assert_not_null(level);
    if (level->hover) {
        tile_pos_t *pos = level->hover;
        if (pos->tile->enabled) {
            tile_pos_modify_hovered_feature(pos);
            level->changed = true;
        }
    }
}

static void level_set_fade_transition(level_t *level, tile_pos_t *pos)
{
    tile_pos_t *center_pos = level_get_center_tile_pos(level);
    if (center_pos == pos) {
        return;
    }

    Vector2 radial = Vector2Subtract(pos->center, center_pos->center);
    Vector2 modded = Vector2Scale(radial, 3.0);
    Vector2 faded  = Vector2Lerp(modded, radial, level->fade_value_eased);

    Vector2 translate = Vector2Subtract(faded, radial);

    rlTranslatef(translate.x,
                 translate.y,
                 0.0);
}

void level_draw(level_t *level, bool finished)
{
    assert_not_null(level);

    bool do_fade = level_update_fade(level);

    level->finished_hue += FINISHED_HUE_STEP;
    while (level->finished_hue > 360.0f) {
        level->finished_hue -= 360.0f;
    }
    win_anim_update(level->win_anim);

    Color finished_color = ColorFromHSV(level->finished_hue, 0.7, 1.0);

    rlPushMatrix();

    if (do_fade) {
        float rot = (1.0 - ease_circular_out(level->fade_value)) * (TAU/2.0);
        Vector2 hwin = {
            .x = window_size.x / 2.0,
            .y = window_size.y / 2.0
        };

        rlTranslatef(hwin.x,
                     hwin.y,
                     0.0);

        rlRotatef(rot * (300.0/TAU), 0.0, 0.0, 1.0);

        rlTranslatef(-hwin.x,
                     -hwin.y,
                     0.0);
    }

    rlTranslatef(level->px_offset.x,
                 level->px_offset.y,
                 0.0);

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            hex_axial_t addr = {
                .q = q,
                .r = r
            };
            tile_pos_t *pos = level_get_current_tile_pos(level, addr);
            assert_not_null(pos);

            if (pos->tile->enabled) {
                if (level->drag_target && pos->tile == level->drag_target->tile) {
                    // defer until after bg tiles are drawn
                } else {
                    if (do_fade) {
                        rlPushMatrix();
                        level_set_fade_transition(level, pos);
                        tile_draw(pos, level->drag_target, finished, finished_color);
                        rlPopMatrix();
                    } else {
                        tile_draw(pos, level->drag_target, finished, finished_color);
                    }
                }
            }
        }
    }

    if (finished) {
        BeginShaderMode(win_border_shader);
        {
            for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
                for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
                    hex_axial_t addr = {
                        .q = q,
                        .r = r
                    };
                    tile_pos_t *pos = level_get_current_tile_pos(level, addr);

                    if (do_fade) {
                        rlPushMatrix();
                        level_set_fade_transition(level, pos);
                        tile_draw_win_anim(pos, level);
                        rlPopMatrix();
                    } else {
                        tile_draw_win_anim(pos, level);
                    }
                }
            }
        }
        EndShaderMode();
    }

    //win_anim_draw(level->win_anim);

    if (level->drag_target) {
        rlPushMatrix();

        rlTranslatef(level->drag_offset.x,
                     level->drag_offset.y,
                     0.0);

        tile_draw(level->drag_target, level->drag_target, finished, finished_color);

        /* if (finished) { */
        /*     BeginShaderMode(win_border_shader); */
        /*     { */
        /*         tile_draw_win_anim(pos, level); */
        /*     } */
        /*     EndShaderMode(); */
        /* } */

        rlPopMatrix();
    }

    //DrawRectangleLinesEx(level->px_bounding_box, 5.0, LIME);

    rlPopMatrix();

#if 0
    if (level->drag_target) {
        DrawText(TextFormat("drag_target<%d,%d> drag_offset = (%f, %f)",
                            level->drag_target->position.q, level->drag_target->position.r,
                            level->drag_offset.x, level->drag_offset.y),
                 10, 10, 20, GREEN);
    }
#endif
}

void level_enable_spiral(level_t *level, int radius)
{
    tile_pos_t *pos = level_get_center_tile_pos(level);
    hex_axial_foreach_in_spiral(pos->position,
                                radius,
                                level_enable_current_tile_callback,
                                level);
    level_resize(level);
}

void level_disable_spiral(level_t *level, int radius)
{
    tile_pos_t *pos = level_get_center_tile_pos(level);
    hex_axial_foreach_in_spiral(pos->position,
                                radius,
                                level_disable_current_tile_callback,
                                level);
    level_resize(level);
}

void level_enable_ring(level_t *level, int radius)
{
    tile_pos_t *pos = level_get_center_tile_pos(level);
    hex_axial_foreach_in_ring(pos->position,
                              radius,
                              level_enable_current_tile_callback,
                              level);
    level_resize(level);
}

void level_disable_ring(level_t *level, int radius)
{
    tile_pos_t *pos = level_get_center_tile_pos(level);
    hex_axial_foreach_in_ring(pos->position,
                              radius,
                              level_disable_current_tile_callback,
                              level);
    level_resize(level);
}

void level_set_radius(level_t *level, int new_radius)
{
    assert_not_null(level);
    assert(new_radius >= LEVEL_MIN_RADIUS);
    assert(new_radius <= LEVEL_MAX_RADIUS);

    //printf("change level radius from %d to %d\n", level->radius, new_radius);

    while (new_radius > level->radius) {
        level->radius++;
        level_enable_ring(level, level->radius );
    }

    while (new_radius < level->radius) {
        level->radius--;
        level_disable_ring(level, level->radius + 1);
    }
}

void level_win(level_t *level)
{
    if (win_anim_running(level->win_anim)) {
        return;
    }

    win_anim_start(level->win_anim);
}

void level_unwin(level_t *level)
{
    if (!win_anim_running(level->win_anim)) {
        return;
    }

    win_anim_stop(level->win_anim);
}

bool level_update_fade(level_t *level)
{
    if (level->fade_value != level->fade_target) {
        if (fabs(level->fade_value - level->fade_target) <= LEVEL_FADE_DELTA) {
            level->fade_value       = level->fade_target;
            level->fade_value_eased = level->fade_target;
#ifdef DEBUG_LEVEL_FADE
            printf("level_update_fade(): stopped at fade_target = %f\n", level->fade_target);
#endif
        } else {
#ifdef DEBUG_LEVEL_FADE
            float old_fade_value = level->fade_value;
#endif

            level->fade_value += level->fade_delta;
            level->fade_value_eased = ease_back_in(level->fade_value);

#ifdef DEBUG_LEVEL_FADE
            printf("level_update_fade(): fade_value %f -> %f\n", old_fade_value, level->fade_value);
#endif
        }
    }

    return level->fade_value != 1.0f;
}

static void level_fade_transition(level_t *level)
{
    if (level->fade_value < level->fade_target) {
        level->fade_delta = LEVEL_FADE_DELTA;
    } else if (level->fade_value > level->fade_target) {
        level->fade_delta = -LEVEL_FADE_DELTA;
    } else {
        /* equal */
    }

#ifdef DEBUG_LEVEL_FADE
    printf("level_fade_transition(): fade_value  = %f\n", level->fade_value);
    printf("level_fade_transition(): fade_target = %f\n", level->fade_target);
    printf("level_fade_transition(): fade_delta  = %f\n", level->fade_delta);
#endif
}

void level_fade_in(level_t *level)
{
#ifdef DEBUG_LEVEL_FADE
    printf("level_fade_in()\n");
#endif
    level->fade_target = 1.0f;
    level_fade_transition(level);
}

void level_fade_out(level_t *level)
{
#ifdef DEBUG_LEVEL_FADE
    printf("level_fade_out()\n");
#endif
    level->fade_target = 0.0f;
    level_fade_transition(level);
}

