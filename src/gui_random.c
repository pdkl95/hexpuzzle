/****************************************************************************
 *                                                                          *
 * gui_random.c                                                             *
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

#include <limits.h>

#include "options.h"
#include "gui_random.h"
#include "color.h"
#include "tile.h"
#include "tile_pos.h"
#include "tile_draw.h"
#include "level.h"
#include "gui_dialog.h"

#include "pcg/pcg_basic.h"

generate_features_t default_generate_feature_options[] = {
    {
        .name = "Common (symmetric: reflect)",
        .fixed  = { .min = 3, .max = 5 },
        .hidden = { .min = 3, .max = 6 },
        .symmetry_mode = SYMMETRY_MODE_REFLECT
    },
    {
        .name = "Common (symmetric: rotate)",
        .fixed  = { .min = 3, .max = 5 },
        .hidden = { .min = 3, .max = 6 },
        .symmetry_mode = SYMMETRY_MODE_ROTATE
    },
    {
        .name = "Common (random)",
        .fixed  = { .min = 4, .max = 6 },
        .hidden = { .min = 4, .max = 6 },
        .symmetry_mode = SYMMETRY_MODE_NONE
    },
    {
        .name = "Uncommon (symmetric: reflect)",
        .fixed  = { .min = 1, .max = 3 },
        .hidden = { .min = 1, .max = 4 },
        .symmetry_mode = SYMMETRY_MODE_REFLECT
    },
    {
        .name = "Uncommon (symmetric: rotate)",
        .fixed  = { .min = 1, .max = 3 },
        .hidden = { .min = 1, .max = 4 },
        .symmetry_mode = SYMMETRY_MODE_ROTATE
    },
    {
        .name = "Uncommon (random)",
        .fixed  = { .min = 2, .max = 3 },
        .hidden = { .min = 2, .max = 4 },
        .symmetry_mode = SYMMETRY_MODE_NONE
    },
    {
        .name = "Rare (symmetric: reflect)",
        .fixed  = { .min = 0, .max = 2 },
        .hidden = { .min = 0, .max = 2 },
        .symmetry_mode = SYMMETRY_MODE_REFLECT
    },
    {
        .name = "Rare (symmetric: rotate)",
        .fixed  = { .min = 0, .max = 2 },
        .hidden = { .min = 0, .max = 2 },
        .symmetry_mode = SYMMETRY_MODE_ROTATE
    },
    {
        .name = "Rare (random)",
        .fixed  = { .min = 0, .max = 2 },
        .hidden = { .min = 0, .max = 2 },
        .symmetry_mode = SYMMETRY_MODE_NONE
    },
    {
        .name = "None",
        .fixed  = { .min = 0, .max = 0 },
        .hidden = { .min = 0, .max = 0 },
        .symmetry_mode = SYMMETRY_MODE_NONE
    }
};
#define NUM_DEFAULT_GENERATE_FEATURE_OPTIONS NUM_ELEMENTS(generate_features_t,default_generate_feature_options)

generate_features_t *generate_feature_options_loaded = NULL;
generate_features_t *generate_feature_options = &(default_generate_feature_options[0]);
int num_generate_feature_options = NUM_DEFAULT_GENERATE_FEATURE_OPTIONS;

Rectangle gui_random_panel_rect;
Rectangle gui_random_area_rect;
Rectangle gui_random_play_button_rect;
Rectangle gui_random_radius_label_rect;
Rectangle gui_random_radius_left_button_rect;
Rectangle gui_random_radius_display_rect;
Rectangle gui_random_radius_right_button_rect;
Rectangle gui_random_save_button_rect;
Rectangle gui_random_color_label_rect;
Rectangle gui_random_gen_style_label_rect;
Rectangle gui_random_gen_style_rect;
Rectangle gui_random_difficulty_label_rect;
Rectangle gui_random_difficulty_rect;
Rectangle gui_random_fixed_hidden_assist_label_rect;
Rectangle gui_random_fixed_hidden_assist_rect;
Rectangle gui_random_seed_rect;
Rectangle gui_random_seed_bg_rect;
Rectangle gui_random_enter_seed_rect;
Rectangle gui_random_rng_seed_rect;
Rectangle gui_random_preview_rect;

Vector2 radius_display_text_location;
Vector2 radius_display_text_shadow_location;
Vector2 seed_text_location;

char gui_random_panel_text[] = "Random Level";
char gui_random_play_button_text[] = "Play";
char gui_random_continue_button_text[] = "Continue";
char gui_random_radius_label_text[] = "Tile Radius";
char gui_random_radius_left_button_text[6];
char gui_random_radius_right_button_text[6];
char gui_random_color_label_text[] = "Colors";
char gui_random_gen_style_label_text[] = "Gen. Method";
char gui_random_difficulty_label_text[] = "Difficulty";
char gui_random_fixed_hidden_assist_label_text[] = "Fixed/Removed";
char gui_random_seed_text[] = "RNG Seed";
char gui_random_rng_seed_text[] = "Randomize";
char gui_random_enter_seed_text_str[] = "Enter";
char *gui_random_enter_seed_text = NULL;

char gui_random_save_button_text_str[] = "Save";
#define GUI_RANDOM_SAVE_BUTTON_TEXT_LENGTH (6 + sizeof(gui_random_save_button_text_str))
char gui_random_save_button_text[GUI_RANDOM_SAVE_BUTTON_TEXT_LENGTH];

const char *gui_random_gen_styles[] = {
    "Density / Scatter",
    "Hamiltonian Path - DFS",
    "Connect To Point"
};
char *gui_random_gen_style_text = NULL;
#define NUM_GEN_STYLES (sizeof(gui_random_gen_styles)/sizeof(char *))
bool gui_random_gen_style_edit_mode = false;

int gen_style = 2;

const char *gui_random_difficulties[] = {
    "Easy    (~2 paths/tile)",
    "Medium  (~3 paths/tile)",
    "Hard   (4~6 paths/tile)"
};
char *gui_random_difficulty_text = NULL;
#define NUM_DIFFICULTIES (sizeof(gui_random_difficulties)/sizeof(char *))
bool gui_random_difficulty_edit_mode = false;

int difficulty = 1;

const char **gui_random_fixed_hidden_assist = NULL;
char *gui_random_fixed_hidden_assist_text = NULL;
bool gui_random_fixed_hidden_assist_edit_mode = false;

int fixed_hidden_assist = 0;

bool any_drop_down_active = false;

bool gui_random_color[PATH_TYPE_COUNT];
int gui_random_color_count = PATH_TYPE_COUNT - 1;

bool played_level = false;
level_t *gui_random_level = NULL;
level_t *gui_random_level_preview = NULL;
uint64_t gui_random_seed;
char *gui_random_seed_str = NULL;

Color seed_bg_color;

pcg32_random_t rng;

static void set_random_seed(int value)
{
    gui_random_seed = (uint64_t)value;
    if (gui_random_seed_str) {
        free(gui_random_seed_str);
    }
    safe_asprintf(&gui_random_seed_str, "%d", value);
}

static void new_random_seed(void)
{
    set_random_seed(rand());
}

static void rng_seed(void)
{
    pcg32_srandom_r(&rng, gui_random_seed, (uint64_t)options->create_level_radius);
}

static int rng_get(int bound)
{
    if (bound <= 1) {
        return 0;
    } else {
        return (int)pcg32_boundedrand_r(&rng, (uint32_t)bound);
    }
}

void shuffle_int(int *list, int len)
{
    int i, j, tmp;
    for (i = len - 1; i > 0; i--) {
        j = rng_get(i + 1);

        tmp = list[j];

        list[j] = list[i];
        list[i] = tmp;
    }
}

hex_direction_order_t get_random_direction_order(int len)
{
    assert(len >  0);
    assert(len <= 6);

    hex_direction_order_t order;
    for(int i=0; i < len; i++) {
        order.dir[i] = i;
    }

    shuffle_int((int *)(&(order.dir[0])), len);

#if 0
    printf("shuffe<len=%d> = [%d", len, order.dir[0]);
    for(int i=1; i < len; i++) {
        printf(", %d", order.dir[i]);
    }
    printf("]\n");
#endif

    return order;
}

#if 0
static bool rng_bool(int true_chances, int false_chances)
{
    int total_chances = true_chances + false_chances;
    return rng_get(total_chances) <= true_chances;
}

static int rng_sign(int pos_chances, int neg_chances)
{
    if (rng_bool(pos_chances, neg_chances)) {
        return 1;
    } else {
        return -1;
    }
}
#endif

static int rng_color_count(void)
{
    int count = 0;

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        if (gui_random_color[type]) {
            count++;
        }
    }

    return count;
}

static int update_rng_color_count(void)
{
    gui_random_color_count = rng_color_count();
    return gui_random_color_count;
}

static void toggle_color(path_type_t type)
{
    gui_random_color[type] = !gui_random_color[type];
    if (update_rng_color_count() < 1) {
        // cannot allow zero colors - undo the toggle
        gui_random_color[type] = !gui_random_color[type];
        update_rng_color_count();
    }
}

static path_type_t rng_color(void)
{
    int skip = rng_get(gui_random_color_count);

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        if (gui_random_color[type]) {
            if (skip) {
                skip--;
            } else {
                return type;
            }
        }
    }

    __builtin_unreachable();

    assert(false && "shouldn't ever reach this!");

    return PATH_TYPE_NONE;
}

static void generate_random_paths(level_t *level, int num_tiles)
{
    for (int i=0; i<num_tiles; i++) {
        int path_delta = options->create_level_max_path - options->create_level_min_path;
        int path_rand  = rng_get(path_delta);
        int num_paths  = path_rand + options->create_level_min_path;

        tile_t *tile = level->enabled_tiles[i];
        tile_pos_t *pos = tile->solved_pos;

        each_direction {
            if (tile->path[dir]) {
                num_paths--;
            } else {
                tile_pos_t *neighbor = pos->neighbors[dir];
                if (neighbor && neighbor->tile && !neighbor->tile->enabled) {
                    num_paths--;
                }
            }
        }

        //print_tile_pos(pos);
        //pint(num_paths);

        if (num_paths > 0) {
            hex_direction_order_t order = get_random_direction_order(num_paths);
            for (int i=0; i<num_paths; i++) {
                hex_direction_t idx = order.dir[i];

                if (tile->path[idx]) {
                    continue;
                }

                tile_pos_t *neighbor = pos->neighbors[idx];
                if (neighbor && neighbor->tile && neighbor->tile->enabled) {
                    hex_direction_t opp_idx = hex_opposite_direction(idx);
                    tile->path[idx] = rng_color();
                    neighbor->tile->path[opp_idx] = tile->path[idx];
                    num_paths--;
                    break;
                }
            }
        }
    }
}

static void set_all_hover(level_t *level, bool value)
{
    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(level->tiles[i]);
        tile->solved_pos->hover = value;
        tile->unsolved_pos->hover = value;
    }
}

//#define DEBUG_DFS
#ifdef DEBUG_DFS
int dfs_depth;
#endif

static void dfs_tile_pos(tile_pos_t *pos)
{
    bool todo[6] = {0};
    int todo_count = 0;

#ifdef DEBUG_DFS
    printf("DFS: depth = %d ", dfs_depth);
    print_tile_pos(pos);
    dfs_depth++;
#endif

    pos->hover = true;

    each_direction {
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (neighbor && neighbor->tile && neighbor->tile->enabled && !neighbor->hover) {
            todo[dir] = true;
            todo_count++;
        }
    }

    if (todo_count > 0) {
        hex_direction_order_t order = get_random_direction_order(todo_count);
        for (int i=0; i<todo_count; i++) {
            hex_direction_t idx = order.dir[i];
            if (todo[idx]) {
                todo[idx] = false;

                tile_pos_t *neighbor = pos->neighbors[idx];

                if (neighbor && neighbor->tile && neighbor->tile->enabled && !neighbor->hover) {
                    hex_direction_t opp_idx = hex_opposite_direction(idx);
                    pos->tile->path[idx] = rng_color();
                    neighbor->tile->path[opp_idx] = pos->tile->path[idx];

                    dfs_tile_pos(neighbor);
                }
                break;
            }

            todo_count--;
        }
    }

#ifdef DEBUG_DFS
    dfs_depth--;
#endif
}

static tile_pos_t *rng_get_start_pos(level_t *level, int num_tiles)
{
    while (true) {
        int start_idx = rng_get(num_tiles);
        tile_t *start = level->enabled_tiles[start_idx];
        tile_pos_t *start_pos = start->solved_pos;

        if (start->start_for_path_type == PATH_TYPE_NONE) {
            return start_pos;
        }
    }
}

static void generate_dfs_path(level_t *level, int num_tiles)
{
    tile_pos_t *start_pos = rng_get_start_pos(level, num_tiles);

    set_all_hover(level, false);
#ifdef DEBUG_DFS
    dfs_depth = 0;
#endif
    dfs_tile_pos(start_pos);
    set_all_hover(level, false);
}

static tile_pos_t *find_random_empty_tile(level_t *level, tile_t *not_this_tile, bool blank_only)
{
    assert_not_null(level);

    //printf("Finding blank tile\n");
    for (int i=0; i<LEVEL_MAXTILES; i++) {
        int idx = (i + rng_get(LEVEL_MAXTILES)) % LEVEL_MAXTILES;
        tile_t *tile = &(level->tiles[idx]);

        //printf("i=%d idx=%d tile=%p ", i, idx, tile);
        //print_tile(tile);

        assert_not_null(tile);

        if (!tile->enabled || tile->hidden) {
            //printf("continue - not enabled\n");
            continue;
        }

        if (blank_only) {
            if (tile_is_blank(tile)) {
                tile_pos_t *pos = tile->unsolved_pos;

                if (not_this_tile && pos->tile == not_this_tile) {
                    //printf("continue - not_this_tile\n");
                    continue;
                }

                //printf("returning: ");
                //print_tile_pos(pos);

                return pos;
            }
        } else {
            tile_pos_t *pos = tile->unsolved_pos;

            if (not_this_tile && pos->tile == not_this_tile) {
                //printf("continue - not_this_tile\n");
                continue;
            }

            //printf("returning: ");
            //print_tile_pos(pos);

            return pos;
        }
    }

    //printf("returning NULL\n");

    return NULL;
}

static tile_pos_t *find_random_tile_empty_first(level_t *level, tile_t *not_this_tile)
{
    tile_pos_t *pos = find_random_empty_tile(level, not_this_tile, true);
    if (pos) {
        return pos;
    } else {
        return find_random_empty_tile(level, not_this_tile, false);
    }
}

static tile_pos_t *find_nearest_matching_color_tile(level_t *level, tile_pos_t *pos, path_type_t type)
{
    assert_not_null(level);
    assert_not_null(pos);

    tile_pos_t *closest = NULL;
    int closest_distance = INT_MAX;

    for (int i=0; i<LEVEL_MAXTILES; i++) {
        int idx = (i + rng_get(LEVEL_MAXTILES)) % LEVEL_MAXTILES;
        tile_t *test_tile = &(level->tiles[idx]);

        if (!test_tile->enabled || test_tile->hidden) {
            continue;
        }

        if (tile_has_path_type(test_tile, type)) {
            tile_pos_t *test_pos = test_tile->unsolved_pos;
            if (test_tile == pos->tile) {
                //printf("SKIP\n");
                continue;
            }

            int dist = hex_axial_distance(pos->position, test_pos->position);
            if ((dist < closest_distance) || (closest == NULL)) {
                closest_distance = dist;
                closest = test_pos;
            }
        }
    }

    if (closest) {
        //printf("closest: ");
        //print_tile_pos(closest);
        return closest;
    } else {
        tile_pos_t *rand_tile = find_random_tile_empty_first(level, pos->tile);
        //printf("rand_tile: ");
        //print_tile_pos(rand_tile);
        return rand_tile;
    }
}

static void draw_path_between_neighbor_tiles(tile_pos_t *a, tile_pos_t *b, path_type_t type)
{
    assert_not_null(a);
    assert_not_null(b);
    assert(a != b);

#if 0
    printf("\t <%d, %d> amd <%d, %d>\n",
           a->position.q, a->position.r,
           b->position.q, b->position.r);
#endif

    each_direction {
        if (a->neighbors[dir] == b) {
            hex_direction_t opp_dir = hex_opposite_direction(dir);
            a->tile->path[    dir] = type;
            b->tile->path[opp_dir] = type;
            return;
        }
    }

    assert(false && "not actually neighbors");
}

static void draw_path_between_tiles(level_t *level, tile_pos_t *a, tile_pos_t *b, path_type_t type)
{
    assert_not_null(level);
    assert_not_null(a);
    assert_not_null(b);
    assert(a != b);

    int dist = hex_axial_distance(a->position, b->position);

#if 0
    printf(">>> draw path between <%d, %d> amd <%d, %d> (dist == %d)\n",
           a->position.q, a->position.r,
           b->position.q, b->position.r,
           dist);
#endif

    Vector2 a_px = hex_axial_to_pixel(a->position, a->size);
    Vector2 b_px = hex_axial_to_pixel(b->position, b->size);

    Vector2 prev_px = Vector2Lerp(a_px, b_px, 0.0f);
    hex_axial_t prev_position = pixel_to_hex_axial(prev_px, a->size);
    tile_pos_t *prev_pos = level_get_unsolved_tile_pos(level, prev_position);

    int connection_count = 0;
    for (int i=1; i<=dist; i++) {
        Vector2 px = Vector2Lerp(a_px, b_px, ((float)i / ((float)dist)));
        hex_axial_t position = pixel_to_hex_axial(px, a->size);
        tile_pos_t *mid_pos = level_get_unsolved_tile_pos(level, position);
        draw_path_between_neighbor_tiles(prev_pos, mid_pos, type);
        prev_pos = mid_pos;
        connection_count++;
    }

    assert(connection_count > 0);
}

static bool generate_connect_to_point_once(level_t *level)
{
    assert_not_null(level);

    tile_pos_t *blank_pos = find_random_tile_empty_first(level, NULL);
    if (!blank_pos) {
        //printf("out of blank tiles?!\n");
        return true;;
    }

    path_type_t type = rng_color();

    tile_pos_t *nearest_pos = find_nearest_matching_color_tile(level, blank_pos, type);
    if (!nearest_pos) {
        return true;
    }
    //assert_not_null(nearest_pos);
    assert(nearest_pos != blank_pos);

    draw_path_between_tiles(level, blank_pos, nearest_pos, type);

    return false;
}

static void generate_connect_to_point(level_t *level)
{
    assert_not_null(level);
    while (level_has_empty_tiles(level)) {
        if (generate_connect_to_point_once(level)) {
            break;
        }
    }

    int extra = options->create_level_expoints * level->radius;

    for (int i=0; i<extra; i++) {
        if (generate_connect_to_point_once(level)) {
            //break;
        }
    }
}

static void shuffle_tiles(level_t *level)
{
    level_use_unsolved_tile_pos(level);
    int num_positions = level_get_movable_positions(level);

    for (int i=num_positions-1; i>0; i--) {
        int j = i;
        while (i == j) {
            j = rng_get(i + 1);
        }

        tile_pos_t *pos_i = level->enabled_positions[i];
        tile_pos_t *pos_j = level->enabled_positions[j];
        assert(!pos_i->tile->hidden);
        assert(!pos_j->tile->hidden);
        level_swap_tile_pos(level, pos_i, pos_j, false);
        level->enabled_positions[i] = pos_j;
        level->enabled_positions[j] = pos_i;
    }
}

static void mark_tile_hidden(tile_t *tile)
{
    tile_pos_t *pos = tile->solved_pos;
    pos->tile->hidden = true;
    each_direction {
        hex_direction_t opposite_dir = hex_opposite_direction(dir);
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (!neighbor) {
            continue;
        }

        pos->tile->path[dir] = PATH_TYPE_NONE;
        neighbor->tile->path[opposite_dir] = PATH_TYPE_NONE;
    }
}

static void mark_symmetric_fixed_and_hidden(level_t *level, int num_tiles, generate_features_t *features)
{
    level_use_solved_tile_pos(level);
    int num_fixed = features->fixed.min
        + rng_get(features->fixed.max -
                  features->fixed.min);
    int num_hidden = features->hidden.min
        + rng_get(features->hidden.max -
                  features->hidden.min);

    hex_axial_t center_pos = level_get_center_tile_pos(level)->position;
    bool reflect = features->symmetry_mode == SYMMETRY_MODE_REFLECT;

    for (int i=0; i<num_fixed; i++) {
        int idx = rng_get(num_tiles);
        tile_t *tile = level->enabled_tiles[idx];
        tile->fixed = true;
        hex_axial_t rpos;
        if (reflect) {
            rpos = hex_axial_reflect_horiz(tile->solved_pos->position, center_pos);
        } else {
            rpos = hex_axial_rotate(tile->solved_pos->position, center_pos);
        }
        tile_t *refl = level_get_solved_tile(level, rpos);
        refl->fixed = true;
    }

    for (int i=0; i<num_hidden; i++) {
        int idx = rng_get(num_tiles);
        tile_t *tile = level->enabled_tiles[idx];
        mark_tile_hidden(tile);

        hex_axial_t rpos;
        if (reflect) {
            rpos = hex_axial_reflect_horiz(tile->solved_pos->position, center_pos);
        } else {
            rpos = hex_axial_rotate(tile->solved_pos->position, center_pos);
        }
        tile_t *refl = level_get_solved_tile(level, rpos);
        mark_tile_hidden(refl);
    }
}

static void mark_random_fixed_and_hidden(level_t *level, int num_tiles, generate_features_t *features)
{
    int num_fixed = features->fixed.min
        + rng_get(features->fixed.max-
                  features->fixed.min);
    int num_hidden = features->hidden.min
        + rng_get(features->hidden.max -
                  features->hidden.min);

    for (int i=0; i<num_fixed; i++) {
        int idx = rng_get(num_tiles);
        level->enabled_tiles[idx]->fixed = true;
    }

    for (int i=0; i<num_hidden; i++) {
        int idx = rng_get(num_tiles);
        mark_tile_hidden(level->enabled_tiles[idx]);
    }
}

static void mark_features(level_t *level, int num_tiles, generate_features_t *features)
{
    switch (features->symmetry_mode) {
    case SYMMETRY_MODE_NONE:
        mark_random_fixed_and_hidden(level, num_tiles, features);
        break;
    case SYMMETRY_MODE_REFLECT:
        /* fall through */
    case SYMMETRY_MODE_ROTATE:
        mark_symmetric_fixed_and_hidden(level, num_tiles, features);
        break;
    default:
        __builtin_unreachable();
    }
}

struct level *_generate_random_level(generate_features_t *given_features)
{
    assert(rng_color_count() > 0);

    if (options->rng_seed_str) {
        if (!parse_random_seed_str(options->rng_seed_str)) {
            errmsg("RNG seed \"%s\" is empty or unusable");
            new_random_seed();
            warnmsg("Using random RNG seed %d instead!", gui_random_seed_str);
        }
        SAFEFREE(options->rng_seed_str);
        options->rng_seed_str = NULL;
    }

    assert(gui_random_seed != 0);
    rng_seed();

    level_t *level = create_level(NULL);
    level_reset(level);

    level->seed = gui_random_seed;
    snprintf(level->name, NAME_MAXLEN, "%s", TextFormat("%d", gui_random_seed));
    if (options->verbose) {
        infomsg("Generating random level \"%s\"", level->name);
    }

    level_set_radius(level, options->create_level_radius);
    int n = level_get_enabled_tiles(level);

    /*** Difficulty ***/
    switch (difficulty) {
    case 0: /* easy */
        options->create_level_min_path = OPTIONS_DEFAULT_CREATE_LEVEL_EASY_MIN_PATH;
        options->create_level_max_path = OPTIONS_DEFAULT_CREATE_LEVEL_EASY_MAX_PATH;
        options->create_level_expoints = OPTIONS_DEFAULT_CREATE_LEVEL_EASY_EXPOINTS;
        break;

    case 1: /* medium */
        options->create_level_min_path = OPTIONS_DEFAULT_CREATE_LEVEL_MEDIUM_MIN_PATH;
        options->create_level_max_path = OPTIONS_DEFAULT_CREATE_LEVEL_MEDIUM_MAX_PATH;
        options->create_level_expoints = OPTIONS_DEFAULT_CREATE_LEVEL_MEDIUM_EXPOINTS;
        break;

    case 2: /* hard */
        options->create_level_min_path = OPTIONS_DEFAULT_CREATE_LEVEL_HARD_MIN_PATH;
        options->create_level_max_path = OPTIONS_DEFAULT_CREATE_LEVEL_HARD_MAX_PATH;
        options->create_level_expoints = OPTIONS_DEFAULT_CREATE_LEVEL_HARD_EXPOINTS;
       break;

    default:
        __builtin_unreachable();
        assert(false && "should never reach here");
    }

    generate_features_t *features = &(generate_feature_options[fixed_hidden_assist]);
    if (given_features) {
        features = given_features;
    }

#define override(feat_field, opt_field)           \
    if (options->opt_field >= 0) {                \
        features->feat_field = options->opt_field; \
    }

    override( fixed.min, create_level_min_fixed);
    override( fixed.max, create_level_max_fixed);
    override(hidden.min, create_level_min_hidden);
    override(hidden.max, create_level_max_hidden);

#undef override

    switch (level->radius) {
    case 1:
        if (features->fixed.min  > 2) { features->fixed.min  = 2; }
        if (features->fixed.max  > 2) { features->fixed.max  = 2; }
        if (features->hidden.min > 2) { features->hidden.min = 2; }
        if (features->hidden.max > 2) { features->hidden.max = 2; }
       break;
    }

    /*** Feneration Style ***/
    switch (gen_style) {
    case 0:
        generate_random_paths(level, n);
        break;

    case 1:
        generate_dfs_path(level, n);
        generate_random_paths(level, n);
        break;

    case 2:
        generate_connect_to_point(level);
        break;

    default:
        __builtin_unreachable();
        assert(false && "should never reach here");
    }

    mark_features(level, n, features);

    level_update_path_counts(level);

#ifndef RANDOM_GEN_DEBUG
    shuffle_tiles(level);
#endif

    level_use_unsolved_tile_pos(level);
    level_backup_unsolved_tiles(level);

    return level;
}

struct level *generate_random_level(void)
{
    return _generate_random_level(NULL);
}

struct level *generate_random_title_level(void)
{
    long save_radius = options->create_level_radius;
    options->create_level_radius = LEVEL_MAX_RADIUS;

    bool save_color[PATH_TYPE_COUNT];
    memcpy(save_color, gui_random_color, sizeof(save_color));
    for (int i=0; i<PATH_TYPE_COUNT; i++) {
        gui_random_color[i] = true;
    }
    update_rng_color_count();

    uint64_t save_random_seed = gui_random_seed;
    rng_seed();

    int save_difficulty = difficulty;
    difficulty = NUM_DIFFICULTIES - 1;

    int save_gen_style = gen_style;
    gen_style = 2;

    generate_features_t features = {
        .name = "Title",
        .fixed  = { .min = 0, .max = 0 },
        .hidden = { .min = 0, .max = 0 },
        .symmetry_mode = SYMMETRY_MODE_NONE
    };
    level_t *level = _generate_random_level(&features);

    gen_style = save_gen_style;
    difficulty = save_difficulty;
    gui_random_seed = save_random_seed;
    memcpy(gui_random_color, save_color, sizeof(save_color));
    options->create_level_radius = save_radius;

    return level;
}

static void regen_level(void)
{
    if (gui_random_level) {
        destroy_level(gui_random_level);
    }

    gui_random_level = generate_random_level();
    played_level = false;
}

void regen_level_preview(void)
{
    assert_not_null(gui_random_level);

    if (gui_random_level_preview) {
        destroy_level(gui_random_level_preview);
    }

    gui_random_seed = gui_random_level->seed + 1;
    gui_random_level_preview = generate_random_level();
}

void promote_preview_to_level(void)
{
    if (gui_random_level) {
        destroy_level(gui_random_level);
    }

    gui_random_level = gui_random_level_preview;
    gui_random_level_preview = NULL;
}

static void rebuild_dropdown_box_string(void)
{
    SAFEFREE(gui_random_fixed_hidden_assist);
    gui_random_fixed_hidden_assist = calloc(num_generate_feature_options, sizeof(char *));

    for (int i=0; i<num_generate_feature_options; i++) {
        gui_random_fixed_hidden_assist[i] = generate_feature_options[i].name;
    }

    const char *join_str = TextJoin(gui_random_fixed_hidden_assist, num_generate_feature_options, ";");
    gui_random_fixed_hidden_assist_text = strdup(join_str);
}

const char *symmetry_mode_string(symmetry_mode_t mode)
{
    switch (mode) {
    case SYMMETRY_MODE_NONE:
        return "none";
    case SYMMETRY_MODE_REFLECT:
        return "reflect";
    case SYMMETRY_MODE_ROTATE:
        return "rotate";
    default:
        __builtin_unreachable();
    }
}

symmetry_mode_t parse_symmetry_mode_string(const char *string)
{
    if (string) {
#define mode(name, id)                                  \
    if (0 == strncasecmp(string, name, strlen(name))) { \
        return id;                                      \
    }
        mode("none",    SYMMETRY_MODE_NONE);
        mode("reflect", SYMMETRY_MODE_REFLECT);
        mode("rotate",  SYMMETRY_MODE_ROTATE);
#undef mode

        errmsg("Invalid symmetry_mode: \"%s\"", string);
    }

    return SYMMETRY_MODE_NONE;
}

cJSON *generate_features_to_json(generate_features_t features)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddStringToObject(json, "name", features.name) == NULL) {
        goto features_json_error;
    }

    if (cJSON_AddStringToObject(json, "symmetry_mode", symmetry_mode_string(features.symmetry_mode)) == NULL) {
        goto features_json_error;
    }

    cJSON *fixed_json = int_range_to_json(&features.fixed);
    if (fixed_json) {
        if (!cJSON_AddItemToObject(json, "fixed", fixed_json)) {
            goto features_json_error;
        }
    } else {
        goto features_json_error;
    }

    cJSON *hidden_json = int_range_to_json(&features.hidden);
    if (hidden_json) {
        if (!cJSON_AddItemToObject(json, "hidden", hidden_json)) {
            goto features_json_error;
        }
    } else {
        goto features_json_error;
    }

    return json;

  features_json_error:
    cJSON_Delete(json);
    return NULL;

}

bool generate_features_from_json(cJSON *json, generate_features_t *features)
{
    if (!cJSON_IsObject(json)) {
        errmsg("JSON['generate_feature_options'][n] should be an Object");
        return false;
    }

    cJSON *name_json = cJSON_GetObjectItem(json, "name");
    if (name_json) {
        if (!cJSON_IsString(name_json)) {
            errmsg("Error parsing program state: JSON['generate_feature_options']['name'] is not a String");
            return false;
        }

        char *name = cJSON_GetStringValue(name_json);
        if (name) {
            features->name = strdup(name);
        } else {
            errmsg("Error parsing program state: JSON['generate_feature_options']['name'] is NULL");
            return false;
        }
    } else {
        warnmsg("Program state JSON['generate_feature_options'] is missing \"name\"");
    }

    cJSON *mode_json = cJSON_GetObjectItem(json, "symmetry_mode");
    if (mode_json) {
        if (!cJSON_IsString(mode_json)) {
            errmsg("Error parsing program state: JSON['generate_feature_options']['symmetry_mode'] is not a String");
            return false;
        }

        char *mode = cJSON_GetStringValue(mode_json);
        if (mode) {
            features->symmetry_mode = parse_symmetry_mode_string(mode);
        } else {
            errmsg("Error parsing program state: JSON['generate_feature_options']['mode'] is NULL");
            return false;
        }
    } else {
        warnmsg("Program state JSON['generate_feature_options'] is missing \"symmetry_mode\"");
    }

    cJSON *fixed_json = cJSON_GetObjectItem(json, "fixed");
    if (fixed_json) {
        if (!int_range_from_json(fixed_json, &features->fixed)) {
            errmsg("Error parsing program state: JSON['generate_feature_options']['fixed']");
            return false;
        }
    } else {
        warnmsg("Program state JSON['generate_feature_options'] is missing \"fixed\"");
    }

    cJSON *hidden_json = cJSON_GetObjectItem(json, "hidden");
    if (hidden_json) {
        if (!int_range_from_json(hidden_json, &features->hidden)) {
            errmsg("Error parsing program state: JSON['generate_feature_options']['hidden']");            return false;
        }
    } else {
        warnmsg("Program state JSON['generate_feature_options'] is missing \"hidden\"");
    }

    return true;
}

cJSON *generate_feature_options_to_json(void)
{
    cJSON *json = cJSON_CreateArray();

    for (int i=0; i<num_generate_feature_options; i++) {
        generate_features_t *opt = &(generate_feature_options[i]);
        cJSON *json_opt = generate_features_to_json(*opt);
        if (json_opt) {
            cJSON_AddItemToArray(json, json_opt);
        } else {
            goto feature_opts_json_error;
        }
    }

    return json;

  feature_opts_json_error:
    cJSON_Delete(json);
    return NULL;
}

bool generate_feature_options_from_json(cJSON *json)
{
    if (!cJSON_IsArray(json)) {
        errmsg("JSON['generate_feature_options'] should be an Array");
        return false;
    }

    int size = cJSON_GetArraySize(json);

    generate_features_t *loaded = calloc(size, sizeof(generate_features_t));

    for (int i=0; i<size; i++) {
        cJSON *item_json = cJSON_GetArrayItem(json, i);
        generate_features_t *features = &(loaded[i]);
        if (!generate_features_from_json(item_json, features)) {
            errmsg("Could not parse JSON['generate_feature_options'][%d]", i);
            return false;
        }
    }

    if (generate_feature_options_loaded) {
        for (int i=0; i<num_generate_feature_options; i++) {
            generate_features_t *features = &(generate_feature_options_loaded[i]);
            SAFEFREE(features->name);
        }

        FREE(generate_feature_options_loaded);
    }

    generate_feature_options_loaded = loaded;
    generate_feature_options = generate_feature_options_loaded;
    num_generate_feature_options = size;

    rebuild_dropdown_box_string();

    return true;
}

void init_gui_random_minimal(void)
{
    gui_random_level = NULL;

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        gui_random_color[type] = true; 
    }

    new_random_seed();
}

void init_gui_random(void)
{
    init_gui_random_minimal();

    seed_bg_color = ColorBrightness(tile_bg_color, -0.25);

    SAFEFREE(gui_random_enter_seed_text);
    SAFEFREE(gui_random_gen_style_text);
    SAFEFREE(gui_random_difficulty_text);
    SAFEFREE(gui_random_fixed_hidden_assist_text);

    //gui_random_enter_seed_text = strdup(GuiIconText(ICON_PENCIL, gui_random_enter_seed_text_str));
    gui_random_enter_seed_text = strdup(GuiIconText(ICON_PENCIL, NULL));

    const char *join_str = TextJoin(gui_random_gen_styles,   NUM_GEN_STYLES,   ";");
    gui_random_gen_style_text  = strdup(join_str);

    join_str = TextJoin(gui_random_difficulties, NUM_DIFFICULTIES, ";");
    gui_random_difficulty_text = strdup(join_str);

    rebuild_dropdown_box_string();

    memcpy(gui_random_save_button_text,
           GuiIconText(ICON_FILE_SAVE_CLASSIC, gui_random_save_button_text_str),
           GUI_RANDOM_SAVE_BUTTON_TEXT_LENGTH);
}

void cleanup_gui_random(void)
{
    if (gui_random_level) {
        destroy_level(gui_random_level);
        gui_random_level = NULL;
    }

    SAFEFREE(gui_random_seed_str);

    SAFEFREE(gui_random_enter_seed_text);
    SAFEFREE(gui_random_gen_style_text);
    SAFEFREE(gui_random_difficulty_text);
    SAFEFREE(gui_random_fixed_hidden_assist_text);
}

void resize_gui_random(void)
{
    gui_random_panel_rect.width  = window_size.x * 0.4;
    gui_random_panel_rect.height = window_size.y * 0.45;

    MINVAR(gui_random_panel_rect.width,  480);
    MINVAR(gui_random_panel_rect.height, 550);

    gui_random_panel_rect.x = (window_size.x / 2) - (gui_random_panel_rect.width  / 2);
    gui_random_panel_rect.y = (window_size.y / 2) - (gui_random_panel_rect.height / 2);

    float panel_bottom = gui_random_panel_rect.y + gui_random_panel_rect.height;

    gui_random_area_rect.x      = gui_random_panel_rect.x + PANEL_INNER_MARGIN;
    gui_random_area_rect.y      = gui_random_panel_rect.y + PANEL_INNER_MARGIN + RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
    gui_random_area_rect.width  = gui_random_panel_rect.width - (2 * PANEL_INNER_MARGIN);
    gui_random_area_rect.height = panel_bottom - PANEL_INNER_MARGIN - gui_random_area_rect.y;

    memcpy(gui_random_radius_left_button_text,  GuiIconText(ICON_ARROW_LEFT_FILL, NULL), 6);
    memcpy(gui_random_radius_right_button_text, GuiIconText(ICON_ARROW_RIGHT_FILL, NULL), 6);

    Vector2 gui_random_radius_label_text_size = measure_gui_text(gui_random_radius_label_text);

    gui_random_radius_label_rect.x      = gui_random_area_rect.x;
    gui_random_radius_label_rect.y      = gui_random_area_rect.y;
    gui_random_radius_label_rect.width  = gui_random_radius_label_text_size.x + (4 * BUTTON_MARGIN);
    gui_random_radius_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_radius_left_button_rect.x      = gui_random_radius_label_rect.x + gui_random_radius_label_rect.width + RAYGUI_ICON_SIZE;;
    gui_random_radius_left_button_rect.y      = gui_random_radius_label_rect.y;
    gui_random_radius_left_button_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_radius_left_button_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_radius_display_rect.x      = gui_random_radius_left_button_rect.x + gui_random_radius_left_button_rect.width + BUTTON_MARGIN;
    gui_random_radius_display_rect.y      = gui_random_radius_left_button_rect.y;
    gui_random_radius_display_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_radius_display_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_radius_right_button_rect.x      = gui_random_radius_display_rect.x + gui_random_radius_display_rect.width + BUTTON_MARGIN;
    gui_random_radius_right_button_rect.y      = gui_random_radius_display_rect.y;
    gui_random_radius_right_button_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_radius_right_button_rect.height = TOOL_BUTTON_HEIGHT;

    radius_display_text_location.x = gui_random_radius_display_rect.x + 8;
    radius_display_text_location.y = gui_random_radius_display_rect.y + 3;
    gui_random_radius_display_rect.x      += 1;
    gui_random_radius_display_rect.width  -= 2;
    gui_random_radius_display_rect.height += 1;

    radius_display_text_shadow_location = radius_display_text_location;
    radius_display_text_shadow_location.x += 1.0f;
    radius_display_text_shadow_location.y += 1.0f;

    Vector2 gui_random_save_button_text_size = measure_gui_text(gui_random_save_button_text);
    fflush(stdout);
    gui_random_save_button_rect.x      = gui_random_area_rect.x + gui_random_area_rect.width - gui_random_save_button_text_size.x - PANEL_INNER_MARGIN;
    gui_random_save_button_rect.y      = gui_random_radius_right_button_rect.y;
    gui_random_save_button_rect.width  = gui_random_save_button_text_size.x;
    gui_random_save_button_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_area_rect.y      += gui_random_radius_label_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_radius_label_rect.height + RAYGUI_ICON_SIZE;

    Vector2 color_label_text_size = measure_gui_text(gui_random_color_label_text);
 
    gui_random_color_label_rect.x      = gui_random_area_rect.x;
    gui_random_color_label_rect.y      = gui_random_area_rect.y;
    gui_random_color_label_rect.width  = color_label_text_size.x;
    gui_random_color_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_area_rect.y      += gui_random_color_label_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_color_label_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_gen_style_label_text_size = measure_gui_text(gui_random_gen_style_label_text);

    gui_random_gen_style_label_rect.x      = gui_random_area_rect.x;
    gui_random_gen_style_label_rect.y      = gui_random_area_rect.y;
    gui_random_gen_style_label_rect.width  = gui_random_gen_style_label_text_size.x + (2 * BUTTON_MARGIN);
    gui_random_gen_style_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_gen_style_rect.x      = gui_random_gen_style_label_rect.x + gui_random_gen_style_label_rect.width + RAYGUI_ICON_SIZE;
    gui_random_gen_style_rect.y      = gui_random_gen_style_label_rect.y;
    gui_random_gen_style_rect.width  = gui_random_area_rect.width - (gui_random_gen_style_rect.x - gui_random_gen_style_label_rect.x);
    gui_random_gen_style_rect.height = gui_random_gen_style_label_rect.height;

    gui_random_area_rect.y      += gui_random_gen_style_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_gen_style_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_difficulty_label_text_size = measure_gui_text(gui_random_difficulty_label_text);

    gui_random_difficulty_label_rect.x      = gui_random_area_rect.x;
    gui_random_difficulty_label_rect.y      = gui_random_area_rect.y;
    gui_random_difficulty_label_rect.width  = gui_random_difficulty_label_text_size.x + (2 * BUTTON_MARGIN);
    gui_random_difficulty_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_difficulty_rect.x      = gui_random_difficulty_label_rect.x + gui_random_difficulty_label_rect.width + RAYGUI_ICON_SIZE;
    gui_random_difficulty_rect.y      = gui_random_difficulty_label_rect.y;
    gui_random_difficulty_rect.width  = gui_random_area_rect.width - (gui_random_difficulty_rect.x - gui_random_difficulty_label_rect.x);
    gui_random_difficulty_rect.height = gui_random_difficulty_label_rect.height;
\
    gui_random_area_rect.y      += gui_random_difficulty_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_difficulty_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_fixed_hidden_assist_label_text_size = measure_gui_text(gui_random_fixed_hidden_assist_label_text);

    gui_random_fixed_hidden_assist_label_rect.x      = gui_random_area_rect.x;
    gui_random_fixed_hidden_assist_label_rect.y      = gui_random_area_rect.y;
    gui_random_fixed_hidden_assist_label_rect.width  = gui_random_fixed_hidden_assist_label_text_size.x + (2 * BUTTON_MARGIN);
    gui_random_fixed_hidden_assist_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_fixed_hidden_assist_rect.x      = gui_random_fixed_hidden_assist_label_rect.x + gui_random_fixed_hidden_assist_label_rect.width + RAYGUI_ICON_SIZE;
    gui_random_fixed_hidden_assist_rect.y      = gui_random_fixed_hidden_assist_label_rect.y;
    gui_random_fixed_hidden_assist_rect.width  = gui_random_area_rect.width - (gui_random_fixed_hidden_assist_rect.x - gui_random_fixed_hidden_assist_label_rect.x);
    gui_random_fixed_hidden_assist_rect.height = gui_random_fixed_hidden_assist_label_rect.height;

    gui_random_area_rect.y      += gui_random_fixed_hidden_assist_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_fixed_hidden_assist_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_seed_text_size = measure_gui_text(gui_random_seed_text);

    gui_random_seed_rect.x      = gui_random_area_rect.x;
    gui_random_seed_rect.y      = gui_random_area_rect.y;
    gui_random_seed_rect.width  = gui_random_seed_text_size.x + (2 * BUTTON_MARGIN);
    gui_random_seed_rect.height = TOOL_BUTTON_HEIGHT;

    seed_text_location.x = gui_random_seed_rect.x + gui_random_seed_rect.width + RAYGUI_ICON_SIZE;
    seed_text_location.y = gui_random_seed_rect.y + 1;

    Vector2 gui_random_rng_seed_text_size   = measure_gui_text(gui_random_rng_seed_text);

    gui_random_rng_seed_rect.y      = gui_random_seed_rect.y;
    gui_random_rng_seed_rect.width  = gui_random_rng_seed_text_size.x + (4 * BUTTON_MARGIN);
    gui_random_rng_seed_rect.height = TOOL_BUTTON_HEIGHT;
    gui_random_rng_seed_rect.x      = gui_random_area_rect.x + gui_random_area_rect.width - gui_random_rng_seed_rect.width;

    gui_random_enter_seed_rect.y      = seed_text_location.y;
    gui_random_enter_seed_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_enter_seed_rect.height = TOOL_BUTTON_HEIGHT;
    gui_random_enter_seed_rect.x      = gui_random_rng_seed_rect.x - RAYGUI_ICON_SIZE - gui_random_enter_seed_rect.width;

    gui_random_seed_bg_rect.x      = seed_text_location.x;
    gui_random_seed_bg_rect.y      = seed_text_location.y;
    gui_random_seed_bg_rect.width  = 2 + gui_random_enter_seed_rect.x - gui_random_seed_bg_rect.x - RAYGUI_ICON_SIZE;
    gui_random_seed_bg_rect.height = 3 + gui_random_enter_seed_rect.height;

    gui_random_area_rect.y      += gui_random_rng_seed_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_rng_seed_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_play_button_text_size = measure_big_button_text(gui_random_play_button_text);
    Vector2 gui_random_continue_button_text_size = measure_big_button_text(gui_random_continue_button_text);
    Vector2 play_or_continue_text_size = {
        .x = MAX(gui_random_play_button_text_size.x, gui_random_continue_button_text_size.x),
        .y = MAX(gui_random_play_button_text_size.y, gui_random_continue_button_text_size.y)
    };
    gui_random_play_button_rect.height = play_or_continue_text_size.y + (3 * BUTTON_MARGIN);;
    gui_random_play_button_rect.width  = gui_random_area_rect.width;
    gui_random_play_button_rect.x      = gui_random_area_rect.x;
    gui_random_play_button_rect.y      = gui_random_area_rect.y + gui_random_area_rect.height - gui_random_play_button_rect.height;

    gui_random_area_rect.height -= gui_random_play_button_rect.height + RAYGUI_ICON_SIZE;

    gui_random_preview_rect.height = gui_random_area_rect.height;
    gui_random_preview_rect.width  = MIN(gui_random_preview_rect.height, gui_random_area_rect.width);
    gui_random_preview_rect.y      = gui_random_area_rect.y;
    gui_random_preview_rect.x      = gui_random_area_rect.x + (gui_random_area_rect.width / 2) - (gui_random_preview_rect.width / 2);

    seed_text_location.x += 3;
    seed_text_location.y += 5;
    gui_random_seed_rect.y += BUTTON_MARGIN;
}

static void draw_gui_random_colors(void)
{
    GuiLabel(gui_random_color_label_rect, gui_random_color_label_text);

    Rectangle rect = {
        .x = gui_random_color_label_rect.x + gui_random_color_label_rect.width + RAYGUI_ICON_SIZE,
        .y = gui_random_color_label_rect.y,
        .width  = TOOL_BUTTON_WIDTH,
        .height = TOOL_BUTTON_HEIGHT
    };

    int color_button_segments = 12;
    float color_button_roundness = 0.5f;
    float color_button_line_thickness = 1.0f;
    float cross_thickness = 2.0f;
    Color cross_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    cross_color = ColorAlpha(cross_color, 0.7);

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        bool hover = false;
        if (!any_drop_down_active) {
            hover = CheckCollisionPointRec(mouse_positionf, rect);
        }

        if (gui_random_color[type]) {
            /* button for color ON */
            if (gui_random_color_count < 2) {
                hover = false;
            }

            DrawRectangleRounded(rect,
                                 color_button_roundness,
                                 color_button_segments,
                                 hover
                                 ? path_type_hover_color(type)
                                 : path_type_color(type));

            DrawRectangleRoundedLines(rect,
                                      color_button_roundness,
                                      color_button_segments,
                                      hover
                                      ? color_button_line_thickness + 1.0f
                                      : color_button_line_thickness,
                                      hover
                                      ? WHITE
                                      : LIGHTGRAY);

        } else {
            /* button for color OFF */
            DrawRectangleRounded(rect,
                                 color_button_roundness,
                                 color_button_segments,
                                 hover
                                 ? path_type_disabled_hover_color(type)
                                 : path_type_disabled_color(type));

            Vector2 s1 = { rect.x,              rect.y               };
            Vector2 e1 = { rect.x + rect.width, rect.y + rect.height };
            Vector2 s2 = { rect.x + rect.width, rect.y               };
            Vector2 e2 = { rect.x,              rect.y + rect.height };
            DrawLineEx(s1, e1, cross_thickness, cross_color);
            DrawLineEx(s2, e2, cross_thickness, cross_color);

            if (hover) {
                DrawRectangleRoundedLines(rect,
                                          color_button_roundness,
                                          color_button_segments,
                                          color_button_line_thickness + 1.0f,
                                          RAYWHITE);
            }
        }

        if (hover) {
            if (mouse_left_click) {
                toggle_color(type);
                regen_level();
            } else {
                set_mouse_cursor(MOUSE_CURSOR_POINTING_HAND);
            }
        }

        rect.x += TOOL_BUTTON_WIDTH + ICON_BUTTON_SIZE;
    }
}


bool parse_random_seed_str(char *seedstr)
{
    if (is_number(seedstr)) {
        if (options->verbose) {
            infomsg("Parsing RNG seed \"%s\" as an INTEGER", seedstr);
        }

        const char *src = seedstr;
        char *endptr;

        errno = 0;
        long value = strtol(src, &endptr, 10);

        if (  ( errno == ERANGE &&
                (  value == LONG_MAX ||
                   value == LONG_MIN )  ) ||
              ( errno != 0 && value == 0)
        ) {
            perror("strtol");
            errmsg("cannot parse numewric seed");
            return false;
        }

        if (endptr == src) {
            errmsg("No digits were found");
            return false;
        }

        set_random_seed(value);
    } else {
        int value = 0;
        char *p = seedstr;

        while (*p) {
            value <<= 7;
            value ^= *p;
            p++;
        }

        if (options->verbose) {
            infomsg("Hashed RNG seed \"%s\" into %d", seedstr, value);
        }
        set_random_seed(value);
    }

    return true;
}

void ask_for_random_seed_callback(gui_dialog_t *dialog)
{
    if (dialog->status) {
        if (strlen(dialog->string) > 0) {
            parse_random_seed_str(dialog->string);
        }
    }
}

bool ask_for_random_seed(void)
{
    gui_dialog_ask_for_string("", NULL, ask_for_random_seed_callback);
    return true;
}

void draw_preview(void)
{
    bool hover = false;;
    if (!any_drop_down_active) {
        hover = CheckCollisionPointRec(mouse_positionf, gui_random_preview_rect);
    }

    DrawRectangleRec(gui_random_preview_rect, BLACK);
    level_preview(gui_random_level, gui_random_preview_rect);

    if (hover) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Rectangle shift_rect = gui_random_preview_rect;
            shift_rect.x -= 1;
            shift_rect.y += 1;

            DrawRectangleLinesEx(gui_random_preview_rect, 2.0, text_shadow_color);
            DrawRectangleLinesEx(shift_rect, 1.0, tile_edge_drag_color);
        } else {
            Rectangle shift_rect = gui_random_preview_rect;
            shift_rect.x += 1;
            shift_rect.y -= 1;

            DrawRectangleLinesEx(gui_random_preview_rect, 2.0, text_shadow_color);
            DrawRectangleLinesEx(shift_rect, 1.0, tile_edge_hover_color);
        }

        if (mouse_left_click) {
            play_gui_random_level();
        }
    }
}

static void draw_tile_radius_gui(void)
{
    GuiLabel(gui_random_radius_label_rect, gui_random_radius_label_text);
    if (GuiButton(gui_random_radius_left_button_rect, gui_random_radius_left_button_text)) {
        if (options->create_level_radius > LEVEL_MIN_RADIUS) {
            options->create_level_radius--;
            regen_level();
        }
    }
    DrawRectangleRec(gui_random_radius_display_rect, seed_bg_color);

    draw_panel_text(TextFormat("%d", options->create_level_radius),
                    radius_display_text_shadow_location,
                    text_shadow_color);

    draw_panel_text(TextFormat("%d", options->create_level_radius),
                    radius_display_text_location,
                    RAYWHITE);

    if (GuiButton(gui_random_radius_right_button_rect, gui_random_radius_right_button_text)) {
        if (options->create_level_radius < LEVEL_MAX_RADIUS) {
            options->create_level_radius++;
            regen_level();
        }
    }
}

static char *get_play_or_continue_text(void)
{
    if (played_level) {
        return gui_random_continue_button_text;
    } else {
        return gui_random_play_button_text;
    }
}

void draw_gui_random(void)
{
    if (!gui_random_level) {
        regen_level();
    }

    if (any_drop_down_active) {
        GuiLock();
    }

    GuiPanel(gui_random_panel_rect, gui_random_panel_text);

    draw_tile_radius_gui();

#if defined(PLATFORM_DESKTOP)
    bool save_ok = false;
    if (current_level && (current_level->seed > 0)) {
        save_ok = true;
    }

    if (!save_ok) {
        //     GuiDisable();
    }

    if (GuiButton(gui_random_save_button_rect, gui_random_save_button_text)) {
        save_gui_random_level();
    }

    if (!save_ok) {
        GuiEnable();
    }
#endif

    draw_gui_random_colors();

    bool colors_ok = false;
    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        colors_ok = colors_ok || gui_random_color[type];
    }

    GuiLabel(gui_random_seed_rect, gui_random_seed_text);

    DrawRectangleRec(gui_random_seed_bg_rect, seed_bg_color);
    draw_panel_text(gui_random_seed_str, seed_text_location, panel_header_text_color);

    if (GuiButton(gui_random_enter_seed_rect, gui_random_enter_seed_text)) {
        if (ask_for_random_seed()) {
            regen_level();
        }
    }

    if (GuiButton(gui_random_rng_seed_rect, gui_random_rng_seed_text)) {
        new_random_seed();
        regen_level();
    }

    if (!colors_ok) {
        GuiDisable();
    }

    set_big_button_font();

    if (GuiButton(gui_random_play_button_rect, get_play_or_continue_text())) {
        play_gui_random_level();
    }

    set_default_font();

    if (!colors_ok) {
        GuiEnable();
    }

    if (gui_random_level) {
        draw_preview();
    }

    GuiUnlock();

    if (gui_random_fixed_hidden_assist_edit_mode) {
        GuiLock();
    }

    GuiLabel(gui_random_fixed_hidden_assist_label_rect, gui_random_fixed_hidden_assist_label_text);
    if (GuiDropdownBox(gui_random_fixed_hidden_assist_rect, gui_random_fixed_hidden_assist_text, &fixed_hidden_assist, gui_random_fixed_hidden_assist_edit_mode)) {
        gui_random_fixed_hidden_assist_edit_mode = !gui_random_fixed_hidden_assist_edit_mode;
        regen_level();
    }

    GuiUnlock();

    if (gui_random_gen_style_edit_mode) {
        GuiLock();
    }

    GuiLabel(gui_random_difficulty_label_rect, gui_random_difficulty_label_text);
    if (GuiDropdownBox(gui_random_difficulty_rect, gui_random_difficulty_text, &difficulty, gui_random_difficulty_edit_mode)) {
        gui_random_difficulty_edit_mode = !gui_random_difficulty_edit_mode;
        regen_level();
    }

    GuiUnlock();

    if (gui_random_gen_style_edit_mode) {
        GuiLock();
    }

    GuiLabel(gui_random_gen_style_label_rect, gui_random_gen_style_label_text);
    if (GuiDropdownBox(gui_random_gen_style_rect, gui_random_gen_style_text, &gen_style, gui_random_gen_style_edit_mode)) {
        gui_random_gen_style_edit_mode = !gui_random_gen_style_edit_mode;
        regen_level();
    }

    any_drop_down_active = gui_random_difficulty_edit_mode || gui_random_gen_style_edit_mode || gui_random_fixed_hidden_assist_edit_mode;
}

void play_gui_random_level(void)
{
    if (!gui_random_level) {
        regen_level();
    }

    level_play(gui_random_level);
    played_level = true;
}

void play_gui_random_level_preview(void)
{
    promote_preview_to_level();
    play_gui_random_level();
}

#if defined(PLATFORM_DESKTOP)
void save_gui_random_level(void)
{
    if (gui_random_level) {
        level_save_to_local_levels(gui_random_level,
                                   GUI_RAMDOM_SAVE_PREFIX,
                                   gui_random_level->name);
    }
}
#endif
