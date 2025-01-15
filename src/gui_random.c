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
#include "raylib_gui_numeric.h"

#include "pcg/pcg_basic.h"

Rectangle gui_random_panel_rect;
Rectangle gui_random_area_rect;
Rectangle gui_random_play_button_rect;
Rectangle gui_random_radius_label_rect;
Rectangle gui_random_radius_left_button_rect;
Rectangle gui_random_radius_display_rect;
Rectangle gui_random_radius_right_button_rect;
Rectangle gui_random_save_button_rect;
Rectangle gui_random_color_label_rect;
Rectangle gui_random_symmetry_label_rect;
Rectangle gui_random_symmetry_button_none_rect;
Rectangle gui_random_symmetry_button_reflect_rect;
Rectangle gui_random_symmetry_button_rotate_rect;
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
char gui_random_fixed_label_text[] = "Fixed Tiles";
char gui_random_hidden_label_text[] = "Hidden Tiles";
char gui_random_minimum_tile_density_label_text[] = "Path Density";
char gui_random_symmetry_label_text[] = "Symmetry";
char gui_random_symmetry_button_none_text[] = "#79#None";
char gui_random_symmetry_button_reflect_text[] = "#40#Reflect";
char gui_random_symmetry_button_rotate_text[] = "#77#Rotate";
char gui_random_seed_text[] = "RNG Seed";
char gui_random_rng_seed_text[] = "Randomize";
char gui_random_enter_seed_text_str[] = "Enter";
char *gui_random_enter_seed_text = NULL;

char gui_random_save_button_text_str[] = "Save";
#define GUI_RANDOM_SAVE_BUTTON_TEXT_LENGTH (6 + sizeof(gui_random_save_button_text_str))
char gui_random_save_button_text[GUI_RANDOM_SAVE_BUTTON_TEXT_LENGTH];

bool gui_random_color[PATH_TYPE_COUNT];
int gui_random_color_count = PATH_TYPE_COUNT - 1;

gui_int_range_t *gui_range_fixed  = NULL;
gui_int_range_t *gui_range_hidden = NULL;
raylib_gui_numeric_t *gui_random_density = NULL;

bool played_level = false;
level_t *gui_random_level = NULL;
level_t *gui_random_level_preview = NULL;
uint64_t gui_random_seed;
char *gui_random_seed_str = NULL;

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

#if 0
static void enable_color(path_type_t type)
{
    gui_random_color[type] = true;
    update_rng_color_count();
}

static void disable_color(path_type_t type)
{
    gui_random_color[type] = false;
    if (update_rng_color_count() < 1) {
        // cannot allow zero colors
        gui_random_color[type] = true;;
        update_rng_color_count();
    }
}
#endif

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

static bool set_tile_and_neighbor_path(tile_pos_t *pos, hex_direction_t dir, path_type_t type)
{
    tile_pos_t *neighbor = pos->neighbors[dir];
    if (neighbor && neighbor->tile && neighbor->tile->enabled) {
        hex_direction_t opp_dir = hex_opposite_direction(dir);
        pos->tile->path[dir] = type;
        neighbor->tile->path[opp_dir] = pos->tile->path[dir];
        return true;
    } else {
        return false;
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

static void add_random_path(level_t *level)
{
    tile_pos_t *pos = find_random_empty_tile(level, NULL, false);
    assert_not_null(pos);
    tile_t *tile = pos->tile;
    assert_not_null(tile);

    int offset = rng_get(6);
    path_type_t color = rng_color();

    each_direction {
        int d = (dir + offset) % 6;
        if (tile->path[d] == PATH_TYPE_NONE) {
            set_tile_and_neighbor_path(pos, d, color);
            return;
        }
    }
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

    for (int i=0; i<MAX_PATH_DENSITY_ITER; i++) {
        float path_density = level_average_paths_per_tile(level);
        if (path_density >= options->create_level_minimum_path_density) {
            break;
        }

        add_random_path(level);
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

static void mark_symmetric_fixed_and_hidden(level_t *level, int num_tiles)
{
    level_use_solved_tile_pos(level);

    int num_fixed = options->create_level_fixed.min
        + rng_get(options->create_level_fixed.max -
                  options->create_level_fixed.min);
    int num_hidden = options->create_level_hidden.min
        + rng_get(options->create_level_hidden.max -
                  options->create_level_hidden.min);

    hex_axial_t center_pos = level_get_center_tile_pos(level)->position;
    bool reflect = options->create_level_symmetry_mode == SYMMETRY_MODE_REFLECT;

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

static void mark_random_fixed_and_hidden(level_t *level, int num_tiles)
{
    int num_fixed = options->create_level_fixed.min
        + rng_get(options->create_level_fixed.max -
                  options->create_level_fixed.min);
    int num_hidden = options->create_level_hidden.min
        + rng_get(options->create_level_hidden.max -
                  options->create_level_hidden.min);

    for (int i=0; i<num_fixed; i++) {
        int idx = rng_get(num_tiles);
        level->enabled_tiles[idx]->fixed = true;
    }

    for (int i=0; i<num_hidden; i++) {
        int idx = rng_get(num_tiles);
        mark_tile_hidden(level->enabled_tiles[idx]);
    }
}

static void mark_features(level_t *level, int num_tiles)
{
    switch (options->create_level_symmetry_mode) {
    case SYMMETRY_MODE_NONE:
        mark_random_fixed_and_hidden(level, num_tiles);
        break;
    case SYMMETRY_MODE_REFLECT:
        /* fall through */
    case SYMMETRY_MODE_ROTATE:
        mark_symmetric_fixed_and_hidden(level, num_tiles);
        break;
    default:
        __builtin_unreachable();
    }
}

struct level *generate_random_level(void)
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

#if 0
    int num_colors = update_rng_color_count();
    printf(">>> colors enabled: %d\n", num_colors);
    for (path_type_t type=0; type<PATH_TYPE_COUNT; type++) {
        printf("\t- path_%d = %s\n", type, gui_random_color[type] ? "true" : "false");
    }
#else
    update_rng_color_count();
#endif

    generate_connect_to_point(level);
    mark_features(level, n);

    level_update_path_counts(level);

#ifndef RANDOM_GEN_DEBUG
    shuffle_tiles(level);
#endif

    level_use_unsolved_tile_pos(level);
    level_backup_unsolved_tiles(level);

    return level;
}

struct level *generate_random_title_level(void)
{
    long save_radius = options->create_level_radius;
    options->create_level_radius = LEVEL_MAX_RADIUS;

    bool save_color[PATH_TYPE_COUNT];
    memcpy(save_color, gui_random_color, sizeof(save_color));
    for (path_type_t i=1; i<PATH_TYPE_COUNT; i++) {
        gui_random_color[i] = true;
    }
    update_rng_color_count();

    uint64_t save_random_seed = gui_random_seed;
    rng_seed();

    level_t *level = generate_random_level();

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

    SAFEFREE(gui_random_enter_seed_text);

    gui_random_enter_seed_text = strdup(GuiIconText(ICON_PENCIL, NULL));

    memcpy(gui_random_save_button_text,
           GuiIconText(ICON_FILE_SAVE_CLASSIC, gui_random_save_button_text_str),
           GUI_RANDOM_SAVE_BUTTON_TEXT_LENGTH);

    gui_range_fixed     = create_gui_int_range(&options->create_level_fixed,
                                               gui_random_fixed_label_text,
                                               LEVEL_MIN_FIXED,
                                               LEVEL_MAX_FIXED);

    gui_range_hidden    = create_gui_int_range(&options->create_level_hidden,
                                               gui_random_hidden_label_text,
                                               LEVEL_MIN_HIDDEN,
                                               LEVEL_MAX_HIDDEN);

    gui_random_density = create_gui_numeric_float(gui_random_minimum_tile_density_label_text,
                                                  &options->create_level_minimum_path_density,
                                                  LEVEL_MIN_MINIMUM_PATH_DENSITY,
                                                  LEVEL_MAX_MINIMUM_PATH_DENSITY,
                                                  0.25f);
}

void cleanup_gui_random(void)
{
    if (gui_random_density) {
        destroy_gui_numeric(gui_random_density);
        gui_random_density = NULL;
    }

    if (gui_random_level) {
        destroy_level(gui_random_level);
        gui_random_level = NULL;
    }

    if (gui_range_hidden) {
        destroy_gui_int_range(gui_range_hidden);
        gui_range_hidden = NULL;
    }

    if (gui_range_fixed) {
        destroy_gui_int_range(gui_range_fixed);
        gui_range_fixed = NULL;
    }

    SAFEFREE(gui_random_seed_str);
    SAFEFREE(gui_random_enter_seed_text);
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

    gui_numeric_resize(gui_random_density, &gui_random_area_rect);

    Vector2 gui_random_symmetry_label_text_size = measure_gui_text(gui_random_symmetry_label_text);
    Vector2 gui_random_symmetry_button_none_text_size = measure_gui_text(gui_random_symmetry_button_none_text);
    Vector2 gui_random_symmetry_button_reflect_text_size = measure_gui_text(gui_random_symmetry_button_reflect_text);
    Vector2 gui_random_symmetry_button_rotate_text_size = measure_gui_text(gui_random_symmetry_button_rotate_text);

    gui_random_symmetry_label_rect.x      = gui_random_area_rect.x;
    gui_random_symmetry_label_rect.y      = gui_random_area_rect.y;
    gui_random_symmetry_label_rect.width  = gui_random_symmetry_label_text_size.x + (4 * BUTTON_MARGIN);
    gui_random_symmetry_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_symmetry_button_none_rect.x      = gui_random_symmetry_label_rect.x + gui_random_symmetry_label_rect.width + RAYGUI_ICON_SIZE;
    gui_random_symmetry_button_none_rect.y      = gui_random_symmetry_label_rect.y;
    gui_random_symmetry_button_none_rect.width  = gui_random_symmetry_button_none_text_size.x;
    gui_random_symmetry_button_none_rect.height = gui_random_symmetry_label_rect.height;

    gui_random_symmetry_button_reflect_rect.x      = gui_random_symmetry_button_none_rect.x + gui_random_symmetry_button_none_rect.width + RAYGUI_ICON_SIZE;
    gui_random_symmetry_button_reflect_rect.y      = gui_random_symmetry_label_rect.y;
    gui_random_symmetry_button_reflect_rect.width  = gui_random_symmetry_button_reflect_text_size.x;
    gui_random_symmetry_button_reflect_rect.height = gui_random_symmetry_label_rect.height;

    gui_random_symmetry_button_rotate_rect.x      = gui_random_symmetry_button_reflect_rect.x + gui_random_symmetry_button_reflect_rect.width + RAYGUI_ICON_SIZE;
    gui_random_symmetry_button_rotate_rect.y      = gui_random_symmetry_label_rect.y;
    gui_random_symmetry_button_rotate_rect.width  = gui_random_symmetry_button_rotate_text_size.x;
    gui_random_symmetry_button_rotate_rect.height = gui_random_symmetry_label_rect.height;

    gui_random_area_rect.y      += gui_random_symmetry_label_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_symmetry_label_rect.height + RAYGUI_ICON_SIZE;

    resize_gui_int_range(gui_range_fixed,  &gui_random_area_rect);
    resize_gui_int_range(gui_range_hidden, &gui_random_area_rect);

    float range_opt_label_width = MAX(gui_range_fixed->label_rect.width,
                                      gui_range_hidden->label_rect.width);

    gui_int_range_set_label_width(gui_range_fixed,  range_opt_label_width);
    gui_int_range_set_label_width(gui_range_hidden, range_opt_label_width);

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

static void draw_gui_randon_symmetry_modes(void)
{
    GuiLabel(gui_random_symmetry_label_rect, gui_random_symmetry_label_text);

    if (options->create_level_symmetry_mode == SYMMETRY_MODE_NONE) {
        GuiSetState(STATE_PRESSED);
    } else {
        GuiSetState(STATE_NORMAL);
    }

    if (GuiButton(gui_random_symmetry_button_none_rect, gui_random_symmetry_button_none_text)) {
        options->create_level_symmetry_mode = SYMMETRY_MODE_NONE;
        regen_level();
    }

    if (options->create_level_symmetry_mode == SYMMETRY_MODE_REFLECT) {
        GuiSetState(STATE_PRESSED);
    } else {
        GuiSetState(STATE_NORMAL);
    }

    if (GuiButton(gui_random_symmetry_button_reflect_rect, gui_random_symmetry_button_reflect_text)) {
        options->create_level_symmetry_mode = SYMMETRY_MODE_REFLECT;
        regen_level();
    }

    if (options->create_level_symmetry_mode == SYMMETRY_MODE_ROTATE) {
        GuiSetState(STATE_PRESSED);
    } else {
        GuiSetState(STATE_NORMAL);
    }

    if (GuiButton(gui_random_symmetry_button_rotate_rect, gui_random_symmetry_button_rotate_text)) {
        options->create_level_symmetry_mode = SYMMETRY_MODE_ROTATE;
        regen_level();
    }

    GuiSetState(STATE_NORMAL);
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
        bool hover = CheckCollisionPointRec(mouse_positionf, rect);

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
    bool hover = CheckCollisionPointRec(mouse_positionf, gui_random_preview_rect);

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

    if (draw_gui_numeric(gui_random_density)) {
        regen_level();
    }

    draw_gui_randon_symmetry_modes();

    if (draw_gui_int_range(gui_range_fixed)) {
        regen_level();
    }
    if (draw_gui_int_range(gui_range_hidden)) {
        regen_level();
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

bool create_level_from_json(cJSON *json)
{
    if (!cJSON_IsObject(json)) {
        errmsg("JSON['create_level'] should be an Object");
        return false;
    }

    cJSON *radius_json = cJSON_GetObjectItem(json, "radius");
    if (radius_json) {
        if (cJSON_IsNumber(radius_json)) {
            options->create_level_radius = radius_json->valueint;
        } else {
            errmsg("JSON['create_level']['radius'] not a Number");
            return false;
        }
        CLAMPVAR(options->create_level_radius,
                 LEVEL_MIN_RADIUS,
                 LEVEL_MAX_RADIUS);
    } else {
        warnmsg("JSON['create_level']['radius'] is missing");
    }

    cJSON *density_json = cJSON_GetObjectItem(json, "minimum_path_density");
    if (density_json) {
        if (cJSON_IsNumber(density_json)) {
            options->create_level_minimum_path_density = density_json->valuedouble;
        } else {
            errmsg("JSON['create_level']['minimum_path_density'] not a Number");
            return false;
        }
        CLAMPVAR(options->create_level_minimum_path_density,
                 LEVEL_MIN_MINIMUM_PATH_DENSITY,
                 LEVEL_MAX_MINIMUM_PATH_DENSITY);
    } else {
        warnmsg("JSON['create_level']['minimum_path_density'] is missing");
    }

    cJSON *symmetry_mode_json = cJSON_GetObjectItem(json, "symmetry_mode");
    if (symmetry_mode_json) {
        if (cJSON_IsString(symmetry_mode_json)) {
            options->create_level_symmetry_mode = parse_symmetry_mode_string(cJSON_GetStringValue(symmetry_mode_json));
        } else {
            errmsg("JSON['create_level']['symmetry_mode'] not a Number");
            return false;
        }
    } else {
        warnmsg("JSON['create_level']['symmetry_mode'] is missing");
    }

    cJSON *fixed_json = cJSON_GetObjectItem(json, "fixed");
    if (fixed_json) {
        if (!int_range_from_json(fixed_json, &options->create_level_fixed)) {
            errmsg("Error parsing program state: JSON['create_level']['fixed']");
            return false;
        }
        int_range_clamp(&options->create_level_fixed,
                        LEVEL_MIN_FIXED,
                        LEVEL_MAX_FIXED);
    } else {
        warnmsg("Program state JSON['create_level'] is missing \"fixed\"");
    }

    cJSON *hidden_json = cJSON_GetObjectItem(json, "hidden");
    if (hidden_json) {
        if (!int_range_from_json(hidden_json, &options->create_level_hidden)) {
            errmsg("Error parsing program state: JSON['create_level']['hidden']");
            return false;
        }
        int_range_clamp(&options->create_level_hidden,
                        LEVEL_MIN_HIDDEN,
                        LEVEL_MAX_HIDDEN);
    } else {
        warnmsg("Program state JSON['create_level'] is missing \"hidden\"");
    }

    cJSON *color_json = cJSON_GetObjectItem(json, "color_enabled");
    if (color_json) {
        for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
            const char *color_name = TextFormat("path_%d", type);

            cJSON *color_type_json = cJSON_GetObjectItem(color_json, color_name);
            if (color_type_json) {
                if (cJSON_IsBool(color_type_json)) {
                    if (cJSON_IsTrue(color_type_json)) {
                        gui_random_color[type] = true;
                    } else {
                        gui_random_color[type] = false;
                    }
                } else {
                    errmsg("Error parsing program state: JSON['create_level']['color_enabled']['%s']", color_name);
                    return false;
                }
            } else {
                warnmsg("Program state JSON['create_level']['color_enabled'] is missing \"%s\"", color_name);
            }
        }

        update_rng_color_count();
    } else {
        warnmsg("Program state JSON['create_level'] is missing \"color_enabled\"");
    }

    return true;
}

cJSON *create_level_to_json(void)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(json, "radius", options->create_level_radius) == NULL) {
        goto create_level_to_json_error;
    }

    if (cJSON_AddNumberToObject(json, "minimum_path_density", options->create_level_minimum_path_density) == NULL) {
        goto create_level_to_json_error;
    }

    if (cJSON_AddStringToObject(json, "symmetry_mode", symmetry_mode_string(options->create_level_symmetry_mode)) == NULL) {
        goto create_level_to_json_error;
    }

    cJSON *fixed_json = int_range_to_json(&options->create_level_fixed);
    if (fixed_json) {
        if (!cJSON_AddItemToObject(json, "fixed", fixed_json)) {
            goto create_level_to_json_error;
        }
    } else {
        goto create_level_to_json_error;
    }

    cJSON *hidden_json = int_range_to_json(&options->create_level_hidden);
    if (hidden_json) {
        if (!cJSON_AddItemToObject(json, "hidden", hidden_json)) {
            goto create_level_to_json_error;
        }
    } else {
        goto create_level_to_json_error;
    }

    cJSON *color_json = cJSON_AddObjectToObject(json, "color_enabled");
    if (color_json) {
        for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
            const char *color_name = TextFormat("path_%d", type);
            if (gui_random_color[type]) {
                if (cJSON_AddTrueToObject(color_json, color_name) == NULL) {
                    goto create_level_to_json_error;
                }
            } else {
                if (cJSON_AddFalseToObject(color_json, color_name) == NULL) {
                    goto create_level_to_json_error;
                }
            }
        }
    } else {
        goto create_level_to_json_error;
    }

    return json;

  create_level_to_json_error:
    cJSON_Delete(json);
    return NULL;
}
