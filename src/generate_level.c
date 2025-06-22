/****************************************************************************
 *                                                                          *
 * generate_level.c                                                         *
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
#include "generate_level.h"
#include "blueprint_string.h"

#include "pcg/pcg_basic.h"

#include <limits.h>

pcg32_random_t rng;

generate_level_param_t gen_param;

int tile_count = 0;

int total_used_paths = 0;
int total_possible_paths = 0;

static const char *color_flag_string(generate_level_param_t *param)
{
    static char buf[PATH_TYPE_COUNT + 1];
    for (int i=0; i<PATH_TYPE_COUNT; i++) {
        if (param->color[i]) {
            buf[i] = 'T';
        } else {
            buf[i] = 'F';
        }
    }
    buf[PATH_TYPE_COUNT] = '\0';
    return buf;
}

void print_generate_level_param(generate_level_param_t *param)
{
    printf("<generate_level_param %p>\n", param);
    printf("            mode = %d\n", param->mode);
    printf("            seed = 0x%lX\n", param->seed);
    printf("          series = 0x%lX\n", param->series);
    printf("     tile_radius = %d\n", param->tile_radius);
    printf("           fixed = %d (min=%d, max=%d)\n", param->fixed_count, param->fixed.min, param->fixed.max);
    printf("          hidden = %d (min=%d, max=%d)\n", param->hidden_count, param->hidden.min, param->hidden.max);
    printf("    path_density = %d\n", (int)param->path_density);
    printf("           color = %s\n", color_flag_string(param));
    printf("     color_count = %d\n", param->color_count);
    printf("   symmetry_mode = %s\n", symmetry_mode_string(param->symmetry_mode));
    printf("  fill_all_tiles = %s\n", param->fill_all_tiles ? "true" : "false");
    printf("</generate_level_param>\n");
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

bool parse_random_seed_str(char *seedstr, uint64_t *dst)
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

        *dst = value;
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

        *dst = value;
    }

    return true;
}


static void rng_seed(uint64_t seed, uint64_t series)
{
    pcg32_srandom_r(&rng, seed, series);
}

static int rng_get(int bound)
{
    if (bound <= 1) {
        return 0;
    } else {
        return (int)pcg32_boundedrand_r(&rng, (uint32_t)bound);
    }
}

static void shuffle_int(int *list, int len)
{
    int i, j, tmp;
    for (i = len - 1; i > 0; i--) {
        j = rng_get(i + 1);

        tmp = list[j];

        list[j] = list[i];
        list[i] = tmp;
    }
}

static hex_direction_order_t get_random_direction_order(int len)
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

static int rng_range(int_range_t range)
{
    return range.min + rng_get(range.max - range.min);
}

static path_type_t rng_color(void)
{
    int skip = rng_get(gen_param.color_count);

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        assert(type >= PATH_TYPE_MIN);
        assert(type <= PATH_TYPE_MAX);

        if (gen_param.color[type]) {
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

static tile_t *rng_get_tile(level_t *level)
{
    assert_not_null(level);
    int max = level_get_enabled_tiles(level);
    int idx = rng_get(max);
    return level->enabled_tiles[idx];
}

static bool set_tile_and_neighbor_path(tile_pos_t *pos, hex_direction_t dir, path_type_t type)
{
    assert_not_null(pos);

    assert(type >= PATH_TYPE_MIN);
    assert(type <= PATH_TYPE_MAX);
    assert(dir >= HEX_DIRECTION_MIN);
    assert(dir <= HEX_DIRECTION_MAX);

    tile_pos_t *neighbor = pos->neighbors[dir];
    if (neighbor && neighbor->tile && neighbor->tile->enabled) {
        hex_direction_t opp_dir = hex_opposite_direction(dir);
        path_type_t old_type = pos->tile->path[dir] = type;

        pos->tile->path[dir] = type;
        neighbor->tile->path[opp_dir] = pos->tile->path[dir];

        if (type != old_type) {
            if (type == PATH_TYPE_NONE) {
                total_used_paths -= 1;
                assert( total_used_paths >= 0 );
            } else {
                total_used_paths += 1;
                assert( total_used_paths <= total_possible_paths );
            }
        }

        return true;
    } else {
        return false;
    }
}

static bool add_path(tile_pos_t *pos, hex_direction_t dir, path_type_t type)
{
    assert_not_null(pos);
    assert(type != PATH_TYPE_NONE);
    return set_tile_and_neighbor_path(pos, dir, type);
}

static bool remove_path(tile_pos_t *pos, hex_direction_t dir)
{
    assert_not_null(pos);
    return set_tile_and_neighbor_path(pos, dir, PATH_TYPE_NONE);
}


path_type_t find_random_path_type_on_tile(tile_pos_t *pos)
{
    tile_t *tile = pos->tile;

    hex_direction_order_t order = get_random_direction_order(6);

    for(int i=0; i < 6; i++) {
        hex_direction_t dir = order.dir[i];
        path_type_t type = tile->path[dir];
        if (type != PATH_TYPE_NONE) {
            return type;
        }
    }

    return PATH_TYPE_NONE;
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
            add_path(a, dir, type);
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
            add_path(pos, d, color);
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
        long path_density = level_average_paths_per_tile(level);
        if (path_density >= gen_param.path_density) {
            break;
        }

        add_random_path(level);
    }
}

static void fill_remaining_single_tile(tile_pos_t *pos)
{
    assert_not_null(pos);

    hex_direction_order_t order = get_random_direction_order(6);

    for(int i=0; i < 6; i++) {
        hex_direction_t dir = order.dir[i];
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (!neighbor) {
            continue;
        }

        path_type_t type = find_random_path_type_on_tile(neighbor);

        if (type != PATH_TYPE_NONE) {
            add_path(pos, dir, type);
            return;
        }
    }

    /* neighbors didn't have existing pths; so just
       pick a neighbor and path_type at random */
    for(int i=0; i < 6; i++) {
        hex_direction_t dir = order.dir[i];
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (neighbor) {
            add_path(pos, dir, rng_color());
            return;
        }
    }

    __builtin_unreachable();
}

static void fill_remaining_tiles(level_t *level)
{
    assert_not_null(level);

    while (level_has_empty_tiles(level)) {
        tile_pos_t *pos = find_random_empty_tile(level, NULL, true);
        fill_remaining_single_tile(pos);
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
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (!neighbor) {
            continue;
        }

        remove_path(pos, dir);
    }
}

static void mark_symmetric_fixed_and_hidden(level_t *level)
{
    level_use_solved_tile_pos(level);

    hex_axial_t center_pos = level_get_center_tile_pos(level)->position;
    bool reflect = options->create_level_symmetry_mode == SYMMETRY_MODE_REFLECT;

    for (int i=0; i<gen_param.fixed_count; i++) {
        tile_t *tile = rng_get_tile(level);
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

    for (int i=0; i<gen_param.hidden_count; i++) {
        tile_t *tile = rng_get_tile(level);
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

static void mark_random_fixed_and_hidden(level_t *level)
{
    for (int i=0; i<gen_param.fixed_count; i++) {
        tile_t *tile = rng_get_tile(level);
        tile->fixed = true;
    }

    for (int i=0; i<gen_param.hidden_count; i++) {
        mark_tile_hidden(rng_get_tile(level));
    }
}

static void mark_features(level_t *level)
{
    switch (options->create_level_symmetry_mode) {
    case SYMMETRY_MODE_NONE:
        mark_random_fixed_and_hidden(level);
        break;
    case SYMMETRY_MODE_REFLECT:
        fallthrough;
    case SYMMETRY_MODE_ROTATE:
        mark_symmetric_fixed_and_hidden(level);
        break;
    default:
        __builtin_unreachable();
    }
}

struct level *generate_random_level(generate_level_param_t *param, const char *purpose)
{
    assert(param->color_count > 0);
    assert(param->color_count <= PATH_TYPE_COUNT);

    gen_param = *param;

    level_t *level = create_level(NULL);
    level_reset(level);

    level->seed = gen_param.seed;
    snprintf(level->name, NAME_MAXLEN, "%s", TextFormat("%d", gen_param.seed));
    if (options->verbose) {
        infomsg("Generating random level \"%s\" for %s\n<blueprint>%s</blueprint>",
                level->name,
                purpose ? purpose : "(unknown)",
                serialize_generate_level_params(gen_param));
    }

    level_set_radius(level, options->create_level_radius);
    tile_count = level_get_enabled_tiles(level);

    if (gen_param.have_series) {
        assert(gen_param.have_fixed_count);
        assert(gen_param.have_hidden_count);

        if (!gen_param.have_fixed_count ||
            !gen_param.have_hidden_count) {
            errmsg("param.have_series requires both have_fixed_count and have_hidden_count");
            destroy_level(level);
            return NULL;
        }
    } else {
        gen_param.series = gen_param.tile_radius;
        rng_seed(gen_param.seed, gen_param.series);

        int random_fixed_count  = rng_range(param->fixed);
        int random_hidden_count = rng_range(param->hidden);

        if (!gen_param.have_fixed_count) {
            gen_param.fixed_count  = random_fixed_count;
            gen_param.have_fixed_count = true;
        }
        if (!gen_param.have_hidden_count) {
            gen_param.hidden_count = random_hidden_count;
            gen_param.have_hidden_count = true;
        }

        gen_param.series += 10 * gen_param.fixed_count;
        gen_param.series += 108 * gen_param.hidden_count;
    }

    rng_seed(gen_param.seed, gen_param.series);

    generate_connect_to_point(level);
    if (gen_param.fill_all_tiles) {
        fill_remaining_tiles(level);
    }
    mark_features(level);

    level_update_path_counts(level);

#ifndef RANDOM_GEN_DEBUG
    shuffle_tiles(level);
#endif

    level_use_unsolved_tile_pos(level);
    level_backup_unsolved_tiles(level);

    level->gen_param = calloc(1, sizeof(generate_level_param_t));
    memcpy(level->gen_param, &gen_param, sizeof(generate_level_param_t));

    const char *blueprint = serialize_generate_level_params(*level->gen_param);
    if (blueprint) {
        level->blueprint = strdup(blueprint);
    } else {
        warnmsg("failed to generate a blueprint string");
    }

    return level;
}

struct level *generate_blank_level(void)
{
    level_t *level = create_level(NULL);
    level_reset(level);

    level_set_radius(level, options->create_level_radius);

    return level;
}

struct level *generate_random_title_level(void)
{
    generate_level_param_t param = {
        .mode = GENERATE_LEVEL_RANDOM,
        .seed = rand(),
        .tile_radius = LEVEL_MAX_RADIUS,
        .color = { 0, true, true, true, true },
        .color_count = 4,
        .fixed  = { 0, 0 },
        .hidden = { 0, 0 },
        .symmetry_mode = SYMMETRY_MODE_NONE,
        .path_density = TITLE_MINIMUM_PATH_DENSITY,
        .fill_all_tiles = true
    };

    return generate_random_level(&param, "title");
}

struct level *generate_random_level_simple(const char *purpose)
{
    uint64_t seed = rand();

    if (options->rng_seed_str) {
        if (!parse_random_seed_str(options->rng_seed_str, &seed)) {
            errmsg("RNG seed \"%s\" is empty or unusable", options->rng_seed_str);
            seed = rand();
            warnmsg("Using random RNG seed %d instead!", seed);
        }
    }

    generate_level_param_t param = {
        .mode = GENERATE_LEVEL_RANDOM,
        .seed = seed,
        .tile_radius = options->create_level_radius,
        .color = { 0, true, true, true, true },
        .color_count = 4,
        .fixed  = {
            options->create_level_fixed.min,
            options->create_level_fixed.max
        },
        .hidden = {
            options->create_level_hidden.min,
            options->create_level_hidden.max
        },
        .symmetry_mode = options->create_level_symmetry_mode,
        .path_density = options->create_level_minimum_path_density
    };

    return generate_random_level(&param, purpose);
}
