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
#include "level.h"

//#define DEBUG_DRAG_AND_DROP 1

void print_tiles(level_t *level)
{
    printf("Level \"%s\" tiles:\n", level->name);

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            tile_t *tile = &(level->tiles[q][r]);
            print_tile(tile);
        }
    }
}

static void level_enable_tile_callback(hex_axial_t axial, void *data)
{
    level_t *level = (level_t *)data;
    tile_t *tile = level_get_tile(level, axial);
    if (tile) {
        tile->enabled = true;
    }
}

static void level_disable_tile_callback(hex_axial_t axial, void *data)
{
    level_t *level = (level_t *)data;
    tile_t *tile = level_get_tile(level, axial);
    if (tile) {
        tile->enabled = false;
    }
}

static level_t *alloc_level()
{
    level_t *level = calloc(1, sizeof(level_t));

    level->name[0] = '\0';

    level->id = NULL;
    level->filename = NULL;
    level->changed = false;

    level->next = NULL;

    level->finished_hue = 0.0f;

    level->radius = LEVEL_MIN_RADIUS;

    level_reset(level);

    return level;
}

static void level_prepare_tiles(level_t *level);

void level_reset(level_t *level)
{
    assert_not_null(level);
    level->req_tile_size = 60.0f;

    level->drag_reset_total_frames = 12;
    level->drag_reset_frames = 0;

    level->hover = NULL;
    level->hover_section_adjacency_radius = 12.0;
    level->drag_target = NULL;

    level->center = LEVEL_CENTER_POSITION;
    level->center_tile = level_get_tile(level, level->center);

    level_resize(level);

    level_enable_spiral(level, level->radius);
}

static void level_prepare_tiles(level_t *level)
{
    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            tile_t *tile = &(level->tiles[q][r]);
            hex_axial_t pos = {
                .q = q,
                .r = r
            };
            init_tile(tile, pos);

            for (hex_direction_t section = 0; section < 6; section++) {
                tile->neighbors[section] = level_find_neighbor_tile(level, tile, section);
            }
        }
    }

    level_resize(level);
}

level_t *create_level(void)
{
    level_t *level = alloc_level();

    static int seq = 0;
    seq++;
    snprintf(level->name, NAME_MAXLEN, "%s-%d", LEVEL_DEFAULT_NAME, seq);

    level->radius = LEVEL_DEFAULT_RADIUS;

    level_prepare_tiles(level);

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

tile_t *level_get_tile(level_t *level,  hex_axial_t axial)
{
    assert_not_null(level);

    if ((axial.r < 0) || (axial.r >= TILE_LEVEL_HEIGHT) ||
        (axial.q < 0) || (axial.q >= TILE_LEVEL_WIDTH)) {
        return NULL;
    }

    return &(level->tiles[axial.q][axial.r]);
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

    hex_axial_t pos = {0};
    char *p = addr;
    pos.q = (int)strtol(addr, &p, 10);
    p++;
    pos.r = (int)strtol(p, NULL, 10);

    tile_t *tile = level_get_tile(level, pos);
    if (!tile) {
        errmsg("Cannot find tile with address (%d, %d)\n", pos.q, pos.r);
        return NULL;
    }

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

    level_prepare_tiles(level);

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
        free(str);
        level->filename = strdup(filename);
        if (options->verbose) {
            infomsg("Successfully loaded level file \"%s\"", filename);
        }
        return level;
    } else {
        errmsg("Error parsing level file \"%s\"", filename);
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

    return load_level_string(filename, str);
}

void level_update_ui_name(level_t *level)
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
    }

    game_mode = GAME_MODE_COLLECTION;
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
    game_mode = GAME_MODE_PLAY_LEVEL;
}

void level_edit(level_t *level)
{
    assert_not_null(level);

    level_load(level);
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
    for (int q=0; q < TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r < TILE_LEVEL_HEIGHT; r++) {
            tile_t *tile = &(level->tiles[q][r]);
            if (tile->enabled) {
                count++;
            }
        }
    }
    return count;
}

void level_serialize(level_t *level, FILE *f)
{
    fprintf(f, "hexlevel version 1\n");
    fprintf(f, "name \"%s\"\n", level->name);
    fprintf(f, "radius %d\n", level->radius);
    int total_tiles = level_count_enabled_tiles(level);
    fprintf(f, "begin_tiles %d\n", total_tiles);
    for (int q=0; q < TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r < TILE_LEVEL_HEIGHT; r++) {
            tile_t *tile = &(level->tiles[q][r]);
            tile_serialize(tile, f);
        }
    }
    fprintf(f, "end_tiles\n");
}

static void level_add_to_bounding_box(level_t *level, tile_t *tile)
{
    assert_not_null(level);
    assert_not_null(tile);

    Vector2 *corners = tile->corners;
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

    level->center_tile = level_get_tile(level, level->center);

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

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            tile_t *tile = &(level->tiles[q][r]);
            tile_set_size(tile, level->tile_size);
            if (tile->enabled) {
              level_add_to_bounding_box(level, tile);
            }
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

tile_t *level_find_neighbor_tile(level_t *level, tile_t *tile, hex_direction_t section)
{
    section = (section + 1) % 6;
    hex_axial_t neighbor_pos = hex_axial_neighbor(tile->position, section);
    tile_t *neighbor = level_get_tile(level, neighbor_pos);
    return neighbor;
}

bool level_check(level_t *level)
{
    assert_not_null(level);

    path_int_t total = {0};

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            tile_t *tile = &(level->tiles[q][r]);
            if (tile->enabled) {
                if (!tile_check(tile)) {
                    return false;
                }

                path_int_t counts = tile_count_path_types(tile);
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
                tile_unset_hover_adjacent(level->hover);
                tile_unset_hover_adjacent(level->hover_adjacent);
                level->hover_adjacent = NULL;
            }

            tile_unset_hover(level->hover);
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

        level->hover = level_get_tile(level, mouse_hex);

        if (level->hover) {
            if (level->hover->enabled) {
                tile_t *tile = level->hover;
                Vector2 midpoint = tile->midpoints[tile->hover_section];
                midpoint = Vector2Add(midpoint, level->px_offset);
                if (Vector2Distance(midpoint, level->mouse_pos) < level->hover_section_adjacency_radius) {
                    level->hover_adjacent = level_find_neighbor_tile(level, tile, tile->hover_section);
                    if (level->hover_adjacent) {
                        level->hover_section = tile->hover_section;
                        level->hover_adjacent_section = hex_opposite_direction(level->hover_section);
                        tile_set_hover_adjacent(level->hover,          level->hover_section,          level->hover_adjacent);
                        tile_set_hover_adjacent(level->hover_adjacent, level->hover_adjacent_section, level->hover);
                    }
                }

                tile_set_hover(level->hover, Vector2Subtract(level->mouse_pos, level->px_offset));
            }
        }
    }
}

void level_drag_start(level_t *level)
{
    assert_not_null(level);

    if (level->drag_target) {
        level->drag_target = NULL;
    }

    if (level->hover) {
#ifdef DEBUG_DRAG_AND_DROP
        printf("drag_stop(): hover = %p\n", level->hover);
#endif

        if (level->hover->enabled && !level->hover->fixed) {
            level->drag_target = level->hover;
            level->drag_start  = level->mouse_pos;
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_start(): drag_target = %p\n", level->drag_target);
#endif
        }
    }
}

void level_drop_tile(level_t *level, tile_t *drag_target, tile_t *drop_target)
{
    assert_not_null(level);
    assert_not_null(drag_target);
    assert_not_null(drop_target);

    assert(drag_target->enabled);
    assert(drop_target->enabled);

    if (!drop_target->fixed) {
        tile_swap_attributes(drag_target, drop_target);
    }
}

void level_drag_stop(level_t *level)
{
    assert_not_null(level);

    if (level->drag_target) {
        tile_t *drop_target = level->hover;

        if (drop_target && !drop_target->enabled) {
            drop_target = NULL;
        }

        if (drop_target && !drop_target->fixed && !drop_target->hidden) {
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
        tile_modify_hovered_feature(level->hover);
    }
}

void level_draw(level_t *level, bool finished)
{
    assert_not_null(level);

    level->finished_hue += FINISHED_HUE_STEP;
    while (level->finished_hue > 360.0f) {
        level->finished_hue -= 360.0f;
    }
    Color finished_color = ColorFromHSV(level->finished_hue, 0.7, 1.0);

    rlPushMatrix();

    rlTranslatef(level->px_offset.x,
                 level->px_offset.y,
                 0.0);

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            tile_t *tile = &(level->tiles[q][r]);
            assert_not_null(tile);

            if (tile->enabled) {
                if (tile == level->drag_target) {
                    // defer until after bg tiles are drawn
                } else {
                    tile_draw(tile, level->drag_target, finished, finished_color);
                }
            }
        }
    }

    if (level->drag_target) {
        rlPushMatrix();

        rlTranslatef(level->drag_offset.x,
                     level->drag_offset.y,
                     0.0);

        tile_draw(level->drag_target, level->drag_target, finished, finished_color);

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
    hex_axial_foreach_in_spiral(level->center_tile->position,
                                radius,
                                level_enable_tile_callback,
                                level);
    level_resize(level);
}

void level_disable_spiral(level_t *level, int radius)
{
    hex_axial_foreach_in_spiral(level->center_tile->position,
                                radius,
                                level_disable_tile_callback,
                                level);
    level_resize(level);
}

void level_enable_ring(level_t *level, int radius)
{
    hex_axial_foreach_in_ring(level->center_tile->position,
                              radius,
                              level_enable_tile_callback,
                              level);
    level_resize(level);
}

void level_disable_ring(level_t *level, int radius)
{
    hex_axial_foreach_in_ring(level->center_tile->position,
                              radius,
                              level_disable_tile_callback,
                              level);
    level_resize(level);
}

void level_set_radius(level_t *level, int new_radius)
{
    assert_not_null(level);
    assert(new_radius >= LEVEL_MIN_RADIUS);
    assert(new_radius <= LEVEL_MAX_RADIUS);

    printf("change level radius from %d to %d\n", level->radius, new_radius);

    while (new_radius > level->radius) {
        level->radius++;
        level_enable_ring(level, level->radius );
    }

    while (new_radius < level->radius) {
        level->radius--;
        level_disable_ring(level, level->radius + 1);
    }
}
