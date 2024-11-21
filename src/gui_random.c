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

#include "pcg/pcg_basic.h"

Rectangle gui_random_panel_rect;
Rectangle gui_random_area_rect;
Rectangle gui_random_play_button_rect;
Rectangle gui_random_radius_rect;
Rectangle gui_random_color_label_rect;
Rectangle gui_random_gen_style_label_rect;
Rectangle gui_random_gen_style_rect;
Rectangle gui_random_difficulty_label_rect;
Rectangle gui_random_difficulty_rect;
Rectangle gui_random_seed_rect;
Rectangle gui_random_seed_bg_rect;
Rectangle gui_random_enter_seed_rect;
Rectangle gui_random_rng_seed_rect;
Rectangle gui_random_preview_rect;

Vector2 seed_text_location;

char gui_random_panel_text[] = "Random Level";
char gui_random_play_button_text[] = "Play";
char gui_random_radius_text[] = "Tile Radius  ";
char gui_random_color_label_text[] = "Colors";
char gui_random_gen_style_label_text[] = "Gen. Method";
char gui_random_difficulty_label_text[] = "Difficulty";
char gui_random_seed_text[] = "RNG Seed";
char gui_random_rng_seed_text[] = "Randomize";
char gui_random_enter_seed_text_str[] = "Enter";
char *gui_random_enter_seed_text = NULL;

const char *gui_random_gen_styles[] = {
    "Density / Scatter",
    "Hamiltonian Path - DFS",
    "Separate DFS Per-Color",
    "Connect To Point"
};
char *gui_random_gen_style_text = NULL;
#define NUM_GEN_STYLES (sizeof(gui_random_gen_styles)/sizeof(char *))
bool gui_random_gen_style_edit_mode = false;

int gen_style = 3;

const char *gui_random_difficulties[] = {
    "Easy    (~2 paths/tile)",
    "Medium  (~3 paths/tile)",
    "Hard   (4~6 paths/tile)"
};
char *gui_random_difficulty_text = NULL;
#define NUM_DIFFICULTIES (sizeof(gui_random_difficulties)/sizeof(char *))
bool gui_random_difficulty_edit_mode = false;

int difficulty = 1;

bool gui_random_color[PATH_TYPE_COUNT];
int gui_random_color_count = PATH_TYPE_COUNT - 1;

level_t *gui_random_level = NULL;
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

static void toggle_color(path_type_t type)
{
    gui_random_color[type] = !gui_random_color[type];
    gui_random_color_count = rng_color_count();
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

        while (num_paths > 0) {
            int offset = rng_get(6);
            each_direction {
                hex_direction_t idx = (dir + offset) % 6;

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

static void dfs_tile_pos(tile_pos_t *pos)
{
    bool todo[6] = {0};
    int todo_count = 0;

    pos->hover = true;

    each_direction {
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (neighbor && neighbor->tile && neighbor->tile->enabled && !neighbor->hover) {
            todo[dir] = true;
            todo_count++;
        }
    }

    while (todo_count > 0) {
        int offset = rng_get(6);
        each_direction {
            hex_direction_t idx = (dir + offset) % 6;
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
        }

        todo_count--;
    }
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
    dfs_tile_pos(start_pos);
    set_all_hover(level, false);
}

static int limited_dfs_neighbor_score(tile_pos_t *neighbor, path_type_t type)
{
    assert_not_null(neighbor);
    assert_not_null(neighbor->tile);
    assert(type != PATH_TYPE_NONE);

    int score = 0;

    tile_t *tile = neighbor->tile;
    if (!tile->enabled) {
        return INT_MAX;
    }

    each_direction {
        if (tile->path[dir] == type) {
            score += 10;
        } else if (tile->path[dir] != PATH_TYPE_NONE) {
            score += 2;
        }
    }

    score += rng_get(6);
    score += rng_get(3);

    return score;
}

static void limited_dfs_tile_pos(tile_pos_t *pos, path_type_t type, int color_count, int min_path, int max_path)
{
    assert_not_null(pos);

    int todo_count = 0;
    tile_pos_t *todo[6];
    hex_direction_t todo_dir[6];

    memset(todo, 0, sizeof(todo));
    if (min_path < 1) {
        //min_path = 1;
        return;
    }
    if (max_path < 1) {
        //max_path = 1;
        //printf("STOP max_path == %d", max_path);
        return;
    }

    pos->hover = true;

    int pathcount = rng_get(max_path - min_path) + min_path;

    for_each_direction(_dir) {
        hex_direction_t dir = _dir;
        tile_pos_t *neighbor = pos->neighbors[dir];

        if (neighbor && neighbor->tile && neighbor->tile->enabled) {
            if (neighbor->hover) {
                //printf("SKIP marked tile\n");
            } else {
                for (int i=0; i<todo_count; i++) {
                    int cur_score = limited_dfs_neighbor_score( todo[i], type);
                    int new_score = limited_dfs_neighbor_score(neighbor, type);
                    if (new_score > cur_score) {
                        tile_pos_t *tmp = neighbor;
                        neighbor = todo[i];
                        todo[i] = tmp;

                        hex_direction_t dirtmp = dir;
                        dir = todo_dir[i];
                        todo_dir[i] = dirtmp;
                    }
                }

                todo[todo_count] = neighbor;
                todo_dir[todo_count] = dir;
                todo_count++;
            }
#ifdef RANDOM_GEN_DEBUG
        } else {
            if (neighbor && neighbor->tile) {
                //printf("neighbor->tile is NOT ENABLED\n");
            } else if (neighbor) {
                printf("neighbor->tile is NULL\n");
            } else {
                printf("neighbor is NULL\n");
            }
#endif
        }
    }

#ifdef RANDOM_GEN_DEBUG
    if (todo_count < 1) {
        printf("STOP todo_count = %d\n", todo_count);
    }

    if (pathcount < 1) {
        printf("STOP pathcount = %d\n", pathcount);
    }
#endif

    while ((todo_count > 0) && (pathcount > 0)) {
        //printf("\t\ttodo_count = %d, pathcount = %d\n", todo_count, pathcount);

        todo_count--;

        tile_pos_t *neighbor = todo[todo_count];
        hex_direction_t dir = todo_dir[todo_count];

        hex_direction_t opp_dir = hex_opposite_direction(dir);

#ifdef RANDOM_GEN_DEBUG
        printf("\ttile<%d, %d>[%d, %s] = %s\n",
               pos->position.q, pos->position.r,
               dir, hex_direction_name(dir),
               path_type_name(type));
#endif

        if ((pos->tile->path[dir] == PATH_TYPE_NONE) || (rng_bool(color_count + 1, 7))) {
            pos->tile->path[dir] = type;
            neighbor->tile->path[opp_dir] = pos->tile->path[dir];

            int dmin = rng_sign(1, 5);
            int dmax = rng_sign(1, 7);
//            dmin = -1;
//            dmax = -1;
            limited_dfs_tile_pos(neighbor, type, color_count, min_path + dmin, max_path + dmax);
        }

        pathcount--;
    }
}

static void generate_limited_single_color_path(level_t *level, int num_tiles, path_type_t type, int color_count, int min_path, int max_path)
{
    tile_pos_t *start_pos = rng_get_start_pos(level, num_tiles);
    start_pos->tile->start_for_path_type = type;
    //printf(">>> Generating %s from <%d, %d>\n", path_type_name(type), start_pos->position.q, start_pos->position.r);

    set_all_hover(level, false);
    limited_dfs_tile_pos(start_pos, type, color_count, min_path, max_path);
}

static void generate_separate_limited_paths_per_color(level_t *level, int num_tiles, int min_path, int max_path)
{
    assert_not_null(level);

    int color_count = 0;
    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        if (gui_random_color[type]) {
            generate_limited_single_color_path(level, num_tiles, type, color_count, min_path, max_path);
            color_count++;
        }
    }
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
    int num_positions = level_get_enabled_positions(level);

    for (int i=num_positions-1; i>0; i--) {
        int j = i;
        while (i == j) {
            j = rng_get(i + 1);
        }

        tile_pos_t *pos_i = level->enabled_positions[i];
        tile_pos_t *pos_j = level->enabled_positions[j];
        level_swap_tile_pos(level, pos_i, pos_j);
        level->enabled_positions[i] = pos_j;
        level->enabled_positions[j] = pos_i;
    }
}

struct level *generate_random_level(void)
{
    assert(rng_color_count() > 0);

    rng_seed();

    level_t *level = create_level(NULL);
    level_reset(level);

    snprintf(level->name, NAME_MAXLEN, "%s", TextFormat("%d", gui_random_seed));
    level_set_radius(level, options->create_level_radius);
    int n = level_get_enabled_tiles(level);

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

    switch (gen_style) {
    case 0:
        generate_random_paths(level, n);
        break;

    case 1:
        generate_dfs_path(level, n);
        generate_random_paths(level, n);
        break;

    case 2:
        generate_separate_limited_paths_per_color(level, n, options->create_level_min_path, options->create_level_max_path);
        break;

    case 3:
        generate_connect_to_point(level);
        break;

    default:
        __builtin_unreachable();
        assert(false && "should never reach here");
    }

    level_update_path_counts(level);

#ifndef RANDOM_GEN_DEBUG
    shuffle_tiles(level);
#endif

    level_use_unsolved_tile_pos(level);
    level_backup_unsolved_tiles(level);

    return level;
}

static void regen_level(void)
{
    if (gui_random_level) {
        destroy_level(gui_random_level);
    }

    gui_random_level = generate_random_level();
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

    seed_bg_color = ColorBrightness(tile_bg_color, -0.15);

    SAFEFREE(gui_random_enter_seed_text);
    SAFEFREE(gui_random_gen_style_text);
    SAFEFREE(gui_random_difficulty_text);

    //gui_random_enter_seed_text = strdup(GuiIconText(ICON_PENCIL, gui_random_enter_seed_text_str));
    gui_random_enter_seed_text = strdup(GuiIconText(ICON_PENCIL, NULL));

    const char *join_str = TextJoin(gui_random_gen_styles,   NUM_GEN_STYLES,   ";");
    gui_random_gen_style_text  = strdup(join_str);

    join_str = TextJoin(gui_random_difficulties, NUM_DIFFICULTIES, ";");
    gui_random_difficulty_text = strdup(join_str);
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
}

void resize_gui_random(void)
{
    gui_random_panel_rect.width  = window_size.x * 0.4;
    gui_random_panel_rect.height = window_size.y * 0.45;

    MINVAR(gui_random_panel_rect.width,  350);
    MINVAR(gui_random_panel_rect.height, 550);

    gui_random_panel_rect.x = (window_size.x / 2) - (gui_random_panel_rect.width  / 2);
    gui_random_panel_rect.y = (window_size.y / 2) - (gui_random_panel_rect.height / 2);

    float panel_bottom = gui_random_panel_rect.y + gui_random_panel_rect.height;

    gui_random_area_rect.x      = gui_random_panel_rect.x + PANEL_INNER_MARGIN;
    gui_random_area_rect.y      = gui_random_panel_rect.y + PANEL_INNER_MARGIN + RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
    gui_random_area_rect.width  = gui_random_panel_rect.width - (2 * PANEL_INNER_MARGIN);
    gui_random_area_rect.height = panel_bottom - PANEL_INNER_MARGIN - gui_random_area_rect.y;

    Vector2 gui_random_radius_text_size = MeasureGuiText(gui_random_radius_text);

    gui_random_radius_rect.x      = gui_random_area_rect.x + gui_random_radius_text_size.x;
    gui_random_radius_rect.y      = gui_random_area_rect.y;
    gui_random_radius_rect.width  = 90; //gui_random_area_rect.width - gui_random_radius_text_size.x;
    gui_random_radius_rect.height = 30;

    gui_random_area_rect.y      += gui_random_radius_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_radius_rect.height + RAYGUI_ICON_SIZE;

    Vector2 color_label_text_size = MeasureGuiText(gui_random_color_label_text);
 
    gui_random_color_label_rect.x      = gui_random_area_rect.x;
    gui_random_color_label_rect.y      = gui_random_area_rect.y;
    gui_random_color_label_rect.width  = color_label_text_size.x;
    gui_random_color_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_area_rect.y      += gui_random_color_label_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_color_label_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_gen_style_label_text_size = MeasureGuiText(gui_random_gen_style_label_text);

    gui_random_gen_style_label_rect.x      = gui_random_area_rect.x;
    gui_random_gen_style_label_rect.y      = gui_random_area_rect.y;
    gui_random_gen_style_label_rect.width  = gui_random_gen_style_label_text_size.x;
    gui_random_gen_style_label_rect.height = TOOL_BUTTON_HEIGHT;;

    gui_random_gen_style_rect.x      = gui_random_gen_style_label_rect.x + gui_random_gen_style_label_rect.width + RAYGUI_ICON_SIZE;
    gui_random_gen_style_rect.y      = gui_random_gen_style_label_rect.y;
    gui_random_gen_style_rect.width  = gui_random_area_rect.width - (gui_random_gen_style_rect.x - gui_random_gen_style_label_rect.x);
    gui_random_gen_style_rect.height = gui_random_gen_style_label_rect.height;

    gui_random_area_rect.y      += gui_random_gen_style_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_gen_style_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_difficulty_label_text_size = MeasureGuiText(gui_random_difficulty_label_text);

    gui_random_difficulty_label_rect.x      = gui_random_area_rect.x;
    gui_random_difficulty_label_rect.y      = gui_random_area_rect.y;
    gui_random_difficulty_label_rect.width  = gui_random_difficulty_label_text_size.x;
    gui_random_difficulty_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_difficulty_rect.x      = gui_random_difficulty_label_rect.x + gui_random_difficulty_label_rect.width + RAYGUI_ICON_SIZE;
    gui_random_difficulty_rect.y      = gui_random_difficulty_label_rect.y;
    gui_random_difficulty_rect.width  = gui_random_area_rect.width - (gui_random_difficulty_rect.x - gui_random_difficulty_label_rect.x);
    gui_random_difficulty_rect.height = gui_random_difficulty_label_rect.height;

    gui_random_area_rect.y      += gui_random_difficulty_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_difficulty_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_seed_text_size = MeasureGuiText(gui_random_seed_text);

    gui_random_seed_rect.x      = gui_random_area_rect.x;
    gui_random_seed_rect.y      = gui_random_area_rect.y;
    gui_random_seed_rect.width  = gui_random_seed_text_size.x;
    gui_random_seed_rect.height = TOOL_BUTTON_HEIGHT;

    seed_text_location.x = gui_random_seed_rect.x + gui_random_seed_rect.width + RAYGUI_ICON_SIZE;
    seed_text_location.y = gui_random_seed_rect.y;

    Vector2 gui_random_rng_seed_text_size   = MeasureGuiText(gui_random_rng_seed_text);

    gui_random_rng_seed_rect.y      = seed_text_location.y;
    gui_random_rng_seed_rect.width  = gui_random_rng_seed_text_size.x;
    gui_random_rng_seed_rect.height = TOOL_BUTTON_HEIGHT;
    gui_random_rng_seed_rect.x      = gui_random_area_rect.x + gui_random_area_rect.width - gui_random_rng_seed_rect.width;

    gui_random_enter_seed_rect.y      = seed_text_location.y;
    gui_random_enter_seed_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_enter_seed_rect.height = TOOL_BUTTON_HEIGHT;
    gui_random_enter_seed_rect.x      = gui_random_rng_seed_rect.x - RAYGUI_ICON_SIZE - gui_random_enter_seed_rect.width;

    gui_random_seed_bg_rect.x      = seed_text_location.x - 6;
    gui_random_seed_bg_rect.y      = seed_text_location.y - 2;
    gui_random_seed_bg_rect.width  = 2 + gui_random_enter_seed_rect.x - gui_random_seed_bg_rect.x - RAYGUI_ICON_SIZE;
    gui_random_seed_bg_rect.height = 3 + gui_random_enter_seed_rect.height;

    gui_random_area_rect.y      += gui_random_rng_seed_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_rng_seed_rect.height + RAYGUI_ICON_SIZE;

    gui_random_play_button_rect.height = 3 * RAYGUI_ICON_SIZE;
    gui_random_play_button_rect.width  = gui_random_area_rect.width;
    gui_random_play_button_rect.x      = gui_random_area_rect.x;
    gui_random_play_button_rect.y      = gui_random_area_rect.y + gui_random_area_rect.height - gui_random_play_button_rect.height;

    gui_random_area_rect.height -= gui_random_play_button_rect.height + RAYGUI_ICON_SIZE;

    gui_random_preview_rect.height = gui_random_area_rect.height;
    gui_random_preview_rect.width  = MIN(gui_random_preview_rect.height, gui_random_area_rect.width);
    gui_random_preview_rect.y      = gui_random_area_rect.y;
    gui_random_preview_rect.x      = gui_random_area_rect.x + (gui_random_area_rect.width / 2) - (gui_random_preview_rect.width / 2);
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
    float color_button_roundness = 0.5;
    float color_button_line_thickness = 1.0;
    float cross_thickness = 2.0;
    Color cross_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    cross_color = ColorAlpha(cross_color, 0.7);

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        bool hover = CheckCollisionPointRec(mouse_positionf, rect);

        DrawRectangleRounded(rect, color_button_roundness, color_button_segments,
                             hover ? path_type_highlight_color(type) : path_type_color(type));
        if (gui_random_color[type]) {
            DrawRectangleRoundedLines(rect, color_button_roundness, color_button_segments, color_button_line_thickness, WHITE); 
        } else {
            Vector2 s1 = { rect.x,              rect.y               };
            Vector2 e1 = { rect.x + rect.width, rect.y + rect.height };
            Vector2 s2 = { rect.x + rect.width, rect.y               };
            Vector2 e2 = { rect.x,              rect.y + rect.height };
            DrawLineEx(s1, e1, cross_thickness, cross_color);
            DrawLineEx(s2, e2, cross_thickness, cross_color);
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

void draw_gui_random(void)
{
    if (!gui_random_level) {
        regen_level();
    }

    GuiPanel(gui_random_panel_rect, gui_random_panel_text);

    int old_radius = options->create_level_radius;

    int radius = (int)options->create_level_radius;
    GuiSpinner(gui_random_radius_rect, gui_random_radius_text, &radius, LEVEL_MIN_RADIUS, LEVEL_MAX_RADIUS, false);
    options->create_level_radius = (long)radius;

    if (old_radius != options->create_level_radius) {
        regen_level();
    }

    draw_gui_random_colors();

    bool colors_ok = false;
    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        colors_ok = colors_ok || gui_random_color[type];
    }

    GuiLabel(gui_random_seed_rect, gui_random_seed_text);

    DrawRectangleRec(gui_random_seed_bg_rect, seed_bg_color);
    DrawTextEx(PANEL_LABEL_FONT, gui_random_seed_str, seed_text_location,
               PANEL_LABEL_FONT_SIZE, PANEL_LABEL_FONT_SPACING, panel_header_text_color);


    if (GuiButton(gui_random_enter_seed_rect, gui_random_enter_seed_text)) {
        regen_level();
    }

    if (GuiButton(gui_random_rng_seed_rect, gui_random_rng_seed_text)) {
        new_random_seed();
        regen_level();
    }

    if (!colors_ok) {
        GuiDisable();
    }

    if (GuiButton(gui_random_play_button_rect, gui_random_play_button_text)) {
        play_gui_random_level();
    }

    if (!colors_ok) {
        GuiEnable();
    }

    if (gui_random_level) {
        DrawRectangleRec(gui_random_preview_rect, BLACK);
        level_preview(gui_random_level, gui_random_preview_rect);
    }

    GuiLabel(gui_random_difficulty_label_rect, gui_random_difficulty_label_text);
    if (GuiDropdownBox(gui_random_difficulty_rect, gui_random_difficulty_text, &difficulty, gui_random_difficulty_edit_mode)) {
        gui_random_difficulty_edit_mode = !gui_random_difficulty_edit_mode;
        regen_level();
    }

    GuiLabel(gui_random_gen_style_label_rect, gui_random_gen_style_label_text);
    if (GuiDropdownBox(gui_random_gen_style_rect, gui_random_gen_style_text, &gen_style, gui_random_gen_style_edit_mode)) {
        gui_random_gen_style_edit_mode = !gui_random_gen_style_edit_mode;
        regen_level();
    }
}

void play_gui_random_level(void)
{
    level_play(gui_random_level);
}
