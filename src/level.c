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
#include "raylib_helper.h"

#include <libgen.h>

#include "cJSON/cJSON.h"
#include "raygui/raygui.h"

#include "raylib_helper.h"
#include "options.h"
#include "tile.h"
#include "tile_pos.h"
#include "level.h"
#include "level_undo.h"
#include "collection.h"
#include "nvdata.h"
#include "nvdata_finished.h"
#include "gui_random.h"
#include "win_anim.h"
#include "solver.h"

//#define DEBUG_DRAG_AND_DROP 1
//#define DEBUG_LEVEL_FADE

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
            assert(false && "pos is missing tile");
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

int hex_axial_to_idx(hex_axial_t axial)
{
    return (axial.q * TILE_LEVEL_WIDTH) + axial.r;
}

#define RETURN_NULL_IF_OUT_OF_BOUNDS                       \
    if ((axial.r < 0) || (axial.r >= TILE_LEVEL_HEIGHT) || \
        (axial.q < 0) || (axial.q >= TILE_LEVEL_WIDTH)) {  \
        return NULL;                                       \
    }

tile_pos_t *level_get_solved_tile_pos(level_t *level,  hex_axial_t axial)
{
    assert_not_null(level);
    RETURN_NULL_IF_OUT_OF_BOUNDS;
    return &level->solved_positions[hex_axial_to_idx(axial)];
}

tile_pos_t *level_get_unsolved_tile_pos(level_t *level,  hex_axial_t axial)
{
    assert_not_null(level);
    RETURN_NULL_IF_OUT_OF_BOUNDS;
    return &level->unsolved_positions[hex_axial_to_idx(axial)];
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
        /* if (options->verbose) { */
        /*     infomsg("level_toggle_currently_used_tiles() NULL"); */
        /* } */
        level_use_null_tile_pos(level);
        break;

    case USED_TILES_SOLVED:
        /* if (options->verbose) { */
        /*     infomsg("level_toggle_currently_used_tiles() Using UNSOLVED tiles"); */
        /* } */
        level_use_unsolved_tile_pos(level);
        break;

    case USED_TILES_UNSOLVED:
        /* if (options->verbose) { */
        /*     infomsg("level_toggle_currently_used_tiles() Using SOLVED tiles"); */
        /* } */
        level_use_solved_tile_pos(level);
        break;
    }
}

static void update_neighbor_groups(tile_pos_t *pos)
{
    assert_not_null(pos);

    pos->outer_neighbors_count = 0;
    pos->ring_neighbors_count  = 0;
    pos->inner_neighbors_count = 0;

    for (hex_direction_t dir = 0; dir < 6; dir++) {
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (!neighbor) {
            continue;
        }

        if        (neighbor->center_distance < pos->center_distance) {
            pos->inner_neighbors[pos->inner_neighbors_count] = neighbor;
            pos->inner_neighbors_count++;
        } else if (neighbor->center_distance > pos->center_distance) {
            pos->outer_neighbors[pos->outer_neighbors_count] = neighbor;
            pos->outer_neighbors_count++;
        } else {
            pos->ring_neighbors[pos->ring_neighbors_count] = neighbor;
            pos->ring_neighbors_count++;
        }
    }
}

static level_t *init_level(level_t *level)
{
    assert_not_null(level);

    memset(level, 0, sizeof(level_t));

    level->center = LEVEL_CENTER_POSITION;

    int i=0;
    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            hex_axial_t addr = {
                .q = q,
                .r = r
            };

            init_tile(&level->tiles[i]);
            level->tiles[i].id = i;

            init_tile_pos(&level->solved_positions[i], &level->tiles[i], addr);
            init_tile_pos(&level->unsolved_positions[i], &level->tiles[i], addr);

            level->sorted_tiles[i] = &(level->tiles[i]);

            level->tiles[i].solved_pos   = &level->solved_positions[i];
            level->tiles[i].unsolved_pos = &level->unsolved_positions[i];

            level->solved_positions[i].solved = true;
            level->unsolved_positions[i].solved = false;

            level->solved_positions[i].center_distance =
                hex_axial_distance(level->solved_positions[i].position, level->center);
            level->unsolved_positions[i].center_distance =
                hex_axial_distance(level->unsolved_positions[i].position, level->center);

            i++;
        }
    }

    level->win_anim = create_win_anim(level);

    level->name[0] = '\0';

    level->have_id = false;

    level->id = NULL;
    level->filename = NULL;
    level->savepath = NULL;
    level->changed = false;
    level->solver = NULL;

    level->undo = NULL;
    level->next = NULL;

    level->seed = 0;

    level->enabled_tile_count = 0;

    level->finished_hue = 0.0f;

    level->fade.active = false;
    level->fade.do_rotate = true;
    level->fade.value        = 0.0f;
    level->fade.value_eased  = 0.0f;
    level->fade.delta        = 0.0f;
    level->fade.target       = 0.0f;

    level->fade.rotate_level  = 0.0f;
    level->extra_rotate_level = 0.0f;
    level->extra_rotate_level_speed = 0.0f;

    if (rand() & 0x00000001) {
        level->fade.spin_direction = -1.0;
    } else {
        level->fade.spin_direction = 1.0;
    }

    level->radius = LEVEL_MIN_RADIUS;

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            hex_axial_t addr = {
                .q = q,
                .r = r
            };

            tile_pos_t *solved_pos   = level_get_solved_tile_pos(level, addr);
            tile_pos_t *unsolved_pos = level_get_unsolved_tile_pos(level, addr);

            for (hex_direction_t dir = 0; dir < 6; dir++) {
                solved_pos->neighbors[dir]   = level_find_solved_neighbor_tile_pos(  level,   solved_pos, dir);
                unsolved_pos->neighbors[dir] = level_find_unsolved_neighbor_tile_pos(level, unsolved_pos, dir);

#if 0
                hex_axial_t sn = hex_axial_neighbor(  solved_pos->position, dir);
                hex_axial_t un = hex_axial_neighbor(unsolved_pos->position, dir);

                printf("  solved_pos->neighbors[%d] = %9p  <% 2d,% 2d> -> <% 2d,% 2d>\n",
                       dir,   solved_pos->neighbors[dir],
                       solved_pos->position.q, solved_pos->position.r,
                       sn.q, sn.r);
                printf("unsolved_pos->neighbors[%d] = %9p  <% 2d,% 2d> -> <% 2d,% 2d>\n",
                       dir, unsolved_pos->neighbors[dir],
                       unsolved_pos->position.q, unsolved_pos->position.r,
                       un.q, un.r);
#endif
            }

            update_neighbor_groups(solved_pos);
            update_neighbor_groups(unsolved_pos);
        }
    }

#if 0
    printf("<level name=\"%s\">\n", level->name);
    printf("  <neighbors tiles=SOLVED>\n");

    for (int i = 0; i < LEVEL_MAXTILES; i++) {
        tile_pos_t *solved_pos   = &level->solved_positions[i];
        printf("    ");
        print_tile_pos(solved_pos);
        //if (solved_pos->tile->enabled) {
            for (hex_direction_t dir = 0; dir < 6; dir++) {
                printf("      [%d,%s] ", dir, hex_direction_name(dir));
                print_tile_pos(solved_pos->neighbors[dir]);
            }
            //}
    }
    printf("  </neighbors>\n");
    printf("  <neighbors tiles=UNSOLVED>\n");
    for (int i = 0; i < LEVEL_MAXTILES; i++) {
        tile_pos_t *unsolved_pos = &level->unsolved_positions[i];
        printf("    ");
        print_tile_pos(unsolved_pos);
        //if (unsolved_pos->tile->enabled) {
            for (hex_direction_t dir = 0; dir < 6; dir++) {
                printf("      [%d,%s] ", dir, hex_direction_name(dir));
                print_tile_pos(unsolved_pos->neighbors[dir]);
            }
            //}
    }
    printf("  </neighbors>\n");
    printf("</level>\n");
#endif

    level_reset(level);

    return level;
}

static level_t *alloc_level(void)
{
    level_t *level = calloc(1, sizeof(level_t));
    init_level(level);
    return level;
}

void level_backup_unsolved_tiles(level_t *level)
{
    for (int i = 0; i < LEVEL_MAXTILES; i++) {
        tile_pos_t *pos = &level->unsolved_positions[i];
        pos->orig_tile = pos->tile;
    }
}

static void level_reset_unsolved_tiles(level_t *level)
{
    assert_not_null(level);

    for (int i = 0; i < LEVEL_MAXTILES; i++) {
        tile_pos_t *pos = &level->unsolved_positions[i];
        pos->tile = pos->orig_tile;
        pos->tile->unsolved_pos = pos;
   }
}

void level_reset(level_t *level)
{
    assert_not_null(level);

    level->req_tile_size = 60.0f;

    level->drag_reset_total_frames = options->max_fps * TILE_RESET_TIME;
    level->drag_reset_frames = 0;

    level->hover = NULL;
    level->hover_section_adjacency_radius = 16.0;
    level->drag_target = NULL;

    level->center = LEVEL_CENTER_POSITION;

    level_resize(level);

    level->finished_fract = 0.0f;

    level_use_solved_tile_pos(level);
    level_enable_spiral(level, level->radius);

    level_use_unsolved_tile_pos(level);
    level_enable_spiral(level, level->radius);

    level_use_null_tile_pos(level);

    if (level->undo) {
        destroy_level_undo(level->undo);
    }
    level->undo = create_level_undo(level);
}

void level_reset_tile_positions(level_t *level)
{
    used_tiles_t save_currently_used_tiles = level->currently_used_tiles;
    level_reset_unsolved_tiles(level);
    level->currently_used_tiles = save_currently_used_tiles;
}

void level_update_id(level_t *level)
{
    SAFEFREE(level->id);

    if (level->collection && level->collection->have_id) {
        level->have_id = true;
        safe_asprintf(&level->id, "%s:%s", level->collection->id, level->name);

        //printf("level_update_id() -> \"%s\"\n", level->id);
    } else {
        level->have_id = false;
    }
}

void level_update_path_counts(level_t *level)
{
    for (int i = 0; i < LEVEL_MAXTILES; i++) {
        tile_t *tile = &(level->tiles[i]);
        tile_update_path_count(tile);
    }
}

level_t *create_level(struct collection *collection)
{
    level_t *level = alloc_level();

    level->collection = collection;

    if (collection) {
        static int seq = 0;
        do {
            seq++;
            snprintf(level->name, NAME_MAXLEN, "%s-%d", LEVEL_DEFAULT_NAME, seq);
        } while (collection && collection_level_name_exists(collection, level->name));
    } else {
        snprintf(level->name, NAME_MAXLEN, "%s-%d", LEVEL_DEFAULT_NAME, 0);
    }

    level->radius = LEVEL_DEFAULT_RADIUS;

    level->changed = true;

    level_update_id(level);

    return level;
}

void destroy_level(level_t *level)
{
    if (level) {
        if (level->undo) {
            destroy_level_undo(level->undo);
            level->undo = NULL;
        }

        if (level->solver) {
            destroy_solver(level->solver);
            level->solver = NULL;
        }

        if (level->win_anim) {
            destroy_win_anim(level->win_anim);
            level->win_anim = NULL;
        }

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

int level_get_enabled_tiles(level_t *level)
{
    memset(level->enabled_tiles, 0, sizeof(level->enabled_tiles));

    int num_enabled = 0;

    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(level->tiles[i]);
        if (tile->enabled) {
            level->enabled_tiles[num_enabled] = tile;
            num_enabled++;
        }
    }

    return num_enabled;
}

int level_get_enabled_positions(level_t *level)
{
    memset(level->enabled_positions, 0, sizeof(level->enabled_positions));

    int num_enabled = 0;

    switch (level->currently_used_tiles) {
    case USED_TILES_NULL:
        if (options->verbose) {
            errmsg("level_toggle_currently_used_tiles() No tile set in use!");
        }
        assert(false && "null tile set");
        break;

    case USED_TILES_SOLVED:
        for (int i=0; i<LEVEL_MAXTILES; i++) {
            tile_t *tile = &(level->tiles[i]);
            if (tile->enabled) {
                level->enabled_positions[num_enabled] = tile->solved_pos;
                num_enabled++;
            }
        }
        break;

    case USED_TILES_UNSOLVED:
        for (int i=0; i<LEVEL_MAXTILES; i++) {
            tile_t *tile = &(level->tiles[i]);
            if (tile->enabled) {
                level->enabled_positions[num_enabled] = tile->unsolved_pos;
                num_enabled++;
            }
        }
        break;
    }

    return num_enabled;
}

int level_get_movable_positions(level_t *level)
{
    memset(level->enabled_positions, 0, sizeof(level->enabled_positions));

    int num_movable = 0;

    switch (level->currently_used_tiles) {
    case USED_TILES_NULL:
        assert(false && "null tile set");
        break;

    case USED_TILES_SOLVED:
        for (int i=0; i<LEVEL_MAXTILES; i++) {
            tile_t *tile = &(level->tiles[i]);
            if (tile->enabled) {
                level->enabled_positions[num_movable] = tile->solved_pos;
                num_movable++;
            }
        }
        break;

    case USED_TILES_UNSOLVED:
        for (int i=0; i<LEVEL_MAXTILES; i++) {
            tile_t *tile = &(level->tiles[i]);
            if (tile->enabled && !tile->fixed && !tile->hidden) {
                level->enabled_positions[num_movable] = tile->unsolved_pos;
                num_movable++;
            }
        }
        break;
    }

    return num_movable;
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

tile_t *level_get_solved_tile(level_t *level,  hex_axial_t axial)
{
    tile_pos_t *pos = level_get_solved_tile_pos(level, axial);
    return pos->tile;
}

tile_t *level_get_unsolved_tile(level_t *level,  hex_axial_t axial)
{
    tile_pos_t *pos = level_get_unsolved_tile_pos(level, axial);
    return pos->tile;
}

bool level_parse_string(level_t *level, const char *str)
{
    assert_not_null(level);
    assert_not_null(str);

    cJSON *json = cJSON_Parse(str);
    bool rv = level_from_json(level, json);
    cJSON_Delete(json);
    return rv;
}

void level_set_file_path(level_t *level, const char *path)
{
    char *dirc  = strdup(path);
    char *basec = strdup(path);
    level->filename = strdup(basename(basec));
    level->dirpath  = strdup(dirname(dirc));
    free(basec);
    free(dirc);
}

level_t *load_level_string(const char *filename, const char *str, bool is_pack)
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
        if (is_pack) {
            level->dirpath = "{" COLLECTION_FILENAME_EXT "}";
            level->filename = strdup(filename);
        } else {
            level_set_file_path(level, filename);
        }
        if (options->verbose) {
            infomsg("Successfully loaded level file \"%s/%s\"",
                    level->dirpath, level->filename);
        }
        return level;
    } else {
        errmsg("Error parsing level file \"%s\"",
               filename);
        return NULL;
    }
}

level_t *load_level_json(const char *filename, cJSON *json, bool is_pack)
{
    level_t *level = alloc_level();

    if (level_from_json(level, json)) {
        if (is_pack) {
            level->dirpath = "{" COLLECTION_FILENAME_EXT "}";
            level->filename = strdup(filename);
        } else {
            level_set_file_path(level, filename);
        }
        if (options->verbose) {
            infomsg("Successfully loaded level file \"%s/%s\"",
                    level->dirpath, level->filename);
        }
        return level;
    } else {
        errmsg("Error parsing level JSON \"%s\"",
               filename);
        return NULL;
    }
}

level_t *load_level_file(const char *filename)
{
    assert_not_null(filename);

    char *str = LoadFileText(filename);
    if (NULL == str) {
        errmsg("Error reading level file \"%s\"", filename);
        return NULL;
    }

    level_t *level = load_level_string(filename, str, false);
    level->savepath = strdup(filename);
    free(str);
    return level;
}

void level_update_ui_name(level_t *level, int idx)
{
    assert_not_null(level);

    bool is_finished = nvdata_is_finished(level);
    int icon = is_finished
        ? ICON_OK_TICK
        : ICON_CROSS_SMALL;

    snprintf(level->ui_name, UI_NAME_MAXLEN, "%s% 2d. %s", GuiIconText(icon, NULL), idx, level->name);

#if 0
    printf("update_ui id=\"%s\" %s \"%s\"\n",
           level->id,
           is_finished ? "true" : "false",
           level->ui_name);
#endif
}

void level_unload(void)
{
    if (current_level) {
        if (current_level->win_anim) {
            win_anim_stop(current_level->win_anim);
        }

        if (current_level->solver) {
            destroy_solver(current_level->solver);
            current_level->solver = NULL;
        }
    }

    current_level = NULL;
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

    if (level->finished) {
        level_reset_tile_positions(level);
        level_unwin(level);
    }

    level_fade_in(level, NULL, NULL);
    set_game_mode(GAME_MODE_PLAY_LEVEL);

    if (options->cheat_autowin) {
        level_solve(level);
    }
}

void level_edit(level_t *level)
{
    assert_not_null(level);

    level_load(level);
    level_use_solved_tile_pos(level);
    level_fade_in(level, NULL, NULL);
    set_game_mode(GAME_MODE_EDIT_LEVEL);
}

void level_save_to_filename(level_t *level, const char *filepath)
{
    assert_not_null(level);
    assert_not_null(filepath);

    char *tmpname;
    safe_asprintf(&tmpname, "%s.tmp", filepath);

    if (options->verbose) {
        infomsg("saving level \"%s\" to: \"%s\"", level->name, tmpname);
    }

    cJSON *json = level_to_json(level);
    char *json_str = cJSON_PrintUnformatted(json);

    SaveFileText(tmpname, json_str);

    free(json_str);
    cJSON_Delete(json);

    if (tmpname) {
        if (-1 == rename(tmpname, filepath)) {
            errmsg("Error trying to rename \"%s\" to \"%s\" - ",
                   tmpname, filepath);
        }

        free(tmpname);
    }
}

void level_save_to_file(level_t *level, const char *dirpath)
{
    assert_not_null(level);

    if (!level->filename) {
        errmsg("Asked to save level to a file, but missing filename");
    }

    char *filepath = level->filename;
    char *pathbuf = NULL;
    if (level->savepath) {
        filepath = level->savepath;
    } else if (dirpath) {
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

    level_save_to_filename(level, filepath);

    if (pathbuf) {
        free(pathbuf);
    }

    level->changed = false;
}

void level_save_to_local_levels(level_t *level, const char *prefix, const char *name)
{
    if (!level->filename) {
        safe_asprintf(&level->filename, "%s-%s.%s", prefix, name, LEVEL_FILENAME_EXT);
    }

    level_save_to_file(level, nvdata_default_browse_path);
}

void level_save_to_file_if_changed(level_t *level, const char *dirpath)
{
    assert_not_null(level);

    if (level->changed) {
        level_save_to_file(level, dirpath);
    } else {
        printf("level file \"%s\" hasn't changed - skipping save\n", level->name);
    }
}

void level_save(level_t *level)
{
    assert_not_null(level);
    if (level->collection && level->collection->is_pack) {
        collection_save(level->collection);
    } else if (level->savepath) {
        level_save_to_file_if_changed(level, NULL);
    } else if (level->filename && level->collection && level->collection->dirpath) {
        level_save_to_file_if_changed(level, level->collection->dirpath);
    } else {
        errmsg("Not enough file metadata to save level!");
        pstr(level->filename);
        if (level->collection) {
            pstr(level->collection->dirpath);
        } else {
            printf("level->collection == NULL\n");
        }
    }
}

#define LEVEL_JSON_VERSION 1

bool level_from_json(level_t *level, cJSON *json)
{
    if (!cJSON_IsObject(json)) {
        errmsg("Error parsing level JSON: not an Object");
        return false;
    }

    cJSON *version_json = cJSON_GetObjectItemCaseSensitive(json, "version");
    if (!cJSON_IsNumber(version_json)) {
        errmsg("Error parsing level JSON: 'version' is not a Number");
        return false;
    }

    if (version_json->valueint != LEVEL_JSON_VERSION) {
        errmsg("Error parsing level JSON: 'version' is %d, expected %d",
               version_json->valueint, LEVEL_JSON_VERSION);
        return false;
    }

    cJSON *name_json = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (!cJSON_IsString(name_json)) {
        errmsg("Error parsing level JSON: 'name' is not a String");
        return false;
    }
    snprintf(level->name, NAME_MAXLEN, "%s", name_json->valuestring);
    level_update_id(level);

    cJSON *radius_json = cJSON_GetObjectItemCaseSensitive(json, "radius");
    if (!cJSON_IsNumber(radius_json)) {
        errmsg("Error parsing level JSON: 'radius' is not a Number");
        return false;
    }
    level->radius = radius_json->valueint;

    cJSON *tiles_json = cJSON_GetObjectItemCaseSensitive(json, "tiles");
    if (!cJSON_IsArray(tiles_json)) {
        errmsg("Error parsing level JSON: 'tiles' is not an Array");
        return false;
    }

    level->current_tile_write_idx = 0;
    cJSON *tile_json;
    cJSON_ArrayForEach(tile_json, tiles_json) {
        tile_t *tile = &(level->tiles[level->current_tile_write_idx]);
        if (!tile_from_json(tile, level, tile_json)) {
            errmsg("Error parsing level JSON: parsing tike %d failed", level->current_tile_write_idx);
            return false;
        }
        level->current_tile_write_idx++;
    }

    level_backup_unsolved_tiles(level);

    return true;
}

cJSON *level_to_json(level_t *level)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(json, "version", LEVEL_JSON_VERSION) == NULL) {
        goto json_err;
    }

    if (cJSON_AddStringToObject(json, "name", level->name) == NULL) {
        goto json_err;
    }

    if (cJSON_AddNumberToObject(json, "radius", level->radius) == NULL) {
        goto json_err;
    }

    cJSON *tiles = cJSON_AddArrayToObject(json, "tiles");
    if (tiles == NULL) {
        goto json_err;
    }

    level_sort_tiles(level);

    for (int i=0; i < LEVEL_MAXTILES; i++) {
        tile_t *tile = level->sorted_tiles[i];
        cJSON *tjson = tile_to_json(tile);
        if (tjson == NULL) {
            goto json_err;
        }
        cJSON_AddItemToArray(tiles, tjson);
    }

    return json;

  json_err:
    cJSON_Delete(json);
    return NULL;
}

static void level_add_to_bounding_box(level_t *level, tile_pos_t *pos)
{
    assert_not_null(level);
    assert_not_null(pos);

    Vector2 *corners = pos->win.corners;
    each_direction {
        level->px_min.x = MIN(level->px_min.x, corners[dir].x);
        level->px_min.y = MIN(level->px_min.y, corners[dir].y);

        level->px_max.x = MAX(level->px_max.x, corners[dir].x);
        level->px_max.y = MAX(level->px_max.y, corners[dir].y);
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

    level->enabled_tile_count = 0;

    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(level->tiles[i]);
        assert_not_null(tile);

        tile_pos_t *solved_pos   = tile->solved_pos;
        tile_pos_t *unsolved_pos = tile->unsolved_pos;
        tile_pos_set_size(  solved_pos, level->tile_size);
        tile_pos_set_size(unsolved_pos, level->tile_size);
        if (tile->enabled) {
            level->enabled_tile_count++;
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

    level_use_unsolved_tile_pos(level);

    tile_pos_t *center_pos = level_get_center_tile_pos(level);

    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(level->tiles[i]);

        if (tile->enabled) {
            tile_pos_t *solved_pos   = tile->solved_pos;
            tile_pos_t *unsolved_pos = tile->unsolved_pos;

            solved_pos->radial_vector   = Vector2Subtract(  solved_pos->win.center, center_pos->win.center);
            unsolved_pos->radial_vector = Vector2Subtract(unsolved_pos->win.center, center_pos->win.center);

            solved_pos->radial_vector_norm   = Vector2Normalize(  solved_pos->radial_vector);
            unsolved_pos->radial_vector_norm = Vector2Normalize(unsolved_pos->radial_vector);

            float solved_theta   = atan2f(  -solved_pos->radial_vector.y,   -solved_pos->radial_vector.x);
            float unsolved_theta = atan2f(-unsolved_pos->radial_vector.y, -unsolved_pos->radial_vector.x);

            solved_theta   += TAU/2.0;
            unsolved_theta += TAU/2.0;

            float ring_phase = TAU / 32.0f;
            solved_theta   +=   solved_pos->ring_radius * (TAU/ring_phase);
            unsolved_theta += unsolved_pos->ring_radius * (TAU/ring_phase);

            solved_theta   = fmodf(  solved_theta, TAU);
            unsolved_theta = fmodf(unsolved_theta, TAU);

            solved_pos->radial_angle   = TAU -   solved_theta;
            unsolved_pos->radial_angle = TAU - unsolved_theta;
        }
    }


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
    hex_direction_t section_1 = (section + 1) % 6;
    hex_axial_t neighbor_pos = hex_axial_neighbor(pos->position, section_1);
    paxial(neighbor_pos);
    tile_pos_t *neighbor = level_get_current_tile_pos(level, neighbor_pos);
    return neighbor;
}

bool level_has_empty_tiles(level_t *level)
{
    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(level->tiles[i]);

        if (!tile->enabled || tile->hidden) {
            continue;
        }

        if (tile_is_blank(tile)) {
            return true;
        }
    }

    return false;
}

bool level_check(level_t *level)
{
    assert_not_null(level);

    switch (game_mode) {
    case GAME_MODE_PLAY_LEVEL:
        break;
    case GAME_MODE_WIN_LEVEL:
        level->finished_fract = 1.0;
        break;
    default:
        level->finished_fract = 0.0;
        return false;
    }

    if (options->cheat_autowin) {
        level->finished_fract = 1.0;
        return true;
    }

    bool rv = true;

    path_int_t total = {0};
    level->path_count = 0;
    level->finished_path_count = 0;

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            hex_axial_t axial = {
                .q = q,
                .r = r
            };
            tile_pos_t *pos = level_get_current_tile_pos(level, axial);
            if (pos->tile->enabled) {
                if (!tile_pos_check(pos, &(level->path_count), &(level->finished_path_count))) {
                    rv = false;
                }

                path_int_t counts = tile_count_path_types(pos->tile);
                for (int i=0; i<PATH_TYPE_COUNT; i++) {
                    total.path[i] += counts.path[i];
                }
            }
        }
    }

    bool blank = true;
    for (int i=0; i<PATH_TYPE_COUNT; i++) {
        if (i == PATH_TYPE_NONE) {
            continue;
        }
        if (total.path[i] > 0) {
            blank = false;
            break;
        }
    }

    if (blank || (level->path_count == 0)) {
        rv = false;
    }

    level->finished_fract = ((float)level->finished_path_count) / ((float)level->path_count);

    return rv;
}

void level_set_hover(level_t *level, IVector2 mouse_position)
{
    if (!level) {
        return;
    }

    if (level->hover) {
        tile_pos_unset_hover(level->hover);
        level->hover = false;
    }

    if (level->hover_adjacent) {
        tile_pos_unset_hover(level->hover_adjacent);
        level->hover_adjacent = false;
    }

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            hex_axial_t axial = {
                .q = q,
                .r = r
            };
            tile_pos_t *pos = level_get_current_tile_pos(level, axial);
            pos->hover = false;
            pos->hover_adjacent = NULL;
        }
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

    if (game_mode == GAME_MODE_WIN_LEVEL) {
        return;
    }

    Vector2 mouse_tile_pos = Vector2Subtract(level->mouse_pos, level->px_offset);
    hex_axial_t mouse_hex = pixel_to_hex_axial(mouse_tile_pos, level->tile_size);

    level->hover = level_get_current_tile_pos(level, mouse_hex);

#if 0
    printf("set_hover() m=(%3f,%3f) mh-[%d,%d] hover=%p\n",
           mouse_tile_pos.x, mouse_tile_pos.y,
           mouse_hex.q, mouse_hex.r,
           level->hover);
#endif

    if (level->hover && level->hover->tile && level->hover->tile->enabled) {
        level->hover->hover = true;

        if (level->drag_target && (level->drag_target != level->hover)) {
            if ((level->drag_target->tile->fixed) ||
                (level->drag_target->tile->hidden) ||
                (level->hover->tile->fixed) ||
                (level->hover->tile->hidden)) {
                level->hover->swap_target = NULL;
                level->drag_target->swap_target = NULL;
            } else {
                level->hover->swap_target = level->drag_target;
                level->drag_target->swap_target = level->hover;
            }
        } else {
            level->hover->swap_target = NULL;
        }

        if (edit_mode_solved) {
            tile_pos_set_hover(level->hover, Vector2Subtract(level->mouse_pos, level->px_offset));
            level->hover_section  = level->hover->hover_section;
            level->hover_adjacent = level->hover->neighbors[level->hover_section];

#if 0
            paxial(mouse_hex);
            tile_pos_t *p = level->hover;
            print_tile_pos(p);
            printf("  neighbors[] = {\n");
            for (hex_direction_t dir=0; dir < 6; dir++) {
                printf("    [%d, %s] %p ", dir, hex_direction_name(dir), p->neighbors[dir]);
                print_tile_pos(p->neighbors[dir]);
            }
            printf("}\n");
#endif

            if (level->hover_adjacent && level->hover_adjacent->tile &&
                (!level->hover_adjacent->tile->enabled || level->hover_adjacent->tile->hidden)) {
#if 0
                static int count = 0;
                printf("<rollback count=%d>\n", count++);
                printf("  hover[%s] ", hex_direction_name(level->hover_section));
                print_tile_pos(level->hover);
                printf("    adj[%s] ", hex_direction_name(level->hover_adjacent_section));
                print_tile_pos(level->hover_adjacent);
                printf("</rollback>\n");
#endif

                tile_pos_unset_hover(level->hover);
                tile_pos_unset_hover(level->hover_adjacent);
                level->hover = NULL;
                level->hover_adjacent = NULL;
            }

            if (level->hover_adjacent) {
#if 0
                static int count = 0;
                printf("<hover_adjacent count=%d>\n", count++);
                printf("  hover[%s] ", hex_direction_name(level->hover_section));
                print_tile_pos(level->hover);
                printf("    adj[%s] ", hex_direction_name(level->hover_adjacent_section));
                print_tile_pos(level->hover_adjacent);
                printf("</hover_adjacent>\n");
#endif

                level->hover_section = level->hover->hover_section;
                level->hover->hover_adjacent = level->hover_adjacent;
                level->hover->hover_section = level->hover_section;

                level->hover_adjacent_section = hex_opposite_direction(level->hover_section);
                level->hover_adjacent->hover_adjacent = level->hover;
                level->hover_adjacent->hover_section = level->hover_adjacent_section;

                if (level->hover->hover_center) {
                    tile_pos_unset_hover(level->hover_adjacent); 
                }
            }
        }
    }

    if (tile_pos_dragable(level->hover) && dragable_mode) {
        set_mouse_cursor(MOUSE_CURSOR_POINTING_HAND);
    }
}


void level_drag_start(level_t *level)
{
    assert_not_null(level);

    if (level->drag_target) { 
#ifdef DEBUG_DRAG_AND_DROP
        printf("drag_start(): forgetting old drag_target %p\n", level->hover);
#endif
       level->drag_target = NULL;
    }

    if (level->hover && level_using_unsolved_tiles(level)) {
        if (!level->hover->tile->enabled) {
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_start(): reject drag start (NOT enabled)\n");
#endif
            return;
        }
        if (level->hover->tile->fixed) {
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_start(): reject drag start (IS fixed))\n");
#endif
            return;
        }
        if (level->hover->tile->hidden) {
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_start(): reject drag start (IS hidden)\n");
#endif
            return;
        }

        level->drag_target = level->hover;
        level->drag_start  = level->mouse_pos;
#ifdef DEBUG_DRAG_AND_DROP
        printf("drag_start(): drag_target = %p\n", level->drag_target);
#endif
    }
}

void level_swap_tile_pos(level_t *level, tile_pos_t *a, tile_pos_t *b, bool save_to_undo)
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
    printf("swap_tile_pos(): a=(%d, %d) %p %s\n", a->position.q, a->position.r, a, a->solved ? "Solved" : "UNsolved");
    printf("                 b=(%d, %d) %p %s\n", b->position.q, b->position.r, b, b->solved ? "Solved" : "UNsolved");
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

    if (save_to_undo) {
        level_undo_add_swap_event(level, a->position, b->position);
    }
}

void level_swap_tile_pos_by_position(level_t *level, hex_axial_t a, hex_axial_t b, bool save_to_undo)
{
    tile_t *atile = level_get_tile(level, a);
    tile_t *btile = level_get_tile(level, b);
    assert_not_null(atile);
    assert_not_null(btile);

    tile_pos_t *apos = atile->unsolved_pos;
    tile_pos_t *bpos = btile->unsolved_pos;
    assert_not_null(apos);
    assert_not_null(bpos);

    level_swap_tile_pos(level, apos, bpos, save_to_undo);
}

void level_solve_tile(level_t *level, hex_axial_t position, bool save_to_undo)
{
    tile_t *tile = level_get_tile(level, position);
    assert_not_null(tile);

    hex_axial_t solved_p   = tile->solved_pos->position;
    hex_axial_t unsolved_p = tile->unsolved_pos->position;

    if (!hex_axial_eq(solved_p, unsolved_p)) {
        level_swap_tile_pos_by_position(level, solved_p, unsolved_p, save_to_undo);
    }
}

void level_solve(level_t *level)
{
    assert_not_null(level);

    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(level->tiles[i]);

        if ((!tile->enabled) ||
            (tile->hidden) ||
            (!tile->solved_pos) ||
            (!tile->unsolved_pos)) {
            continue;
        }

        hex_axial_t solved_p   = tile->solved_pos->position;
        hex_axial_t unsolved_p = tile->unsolved_pos->position;

        if (!hex_axial_eq(solved_p, unsolved_p)) {
            level_swap_tile_pos_by_position(level, solved_p, unsolved_p, false);
        }
    }

    /* create_or_use_solver(current_level); */
    /* solver_start_fast(current_level->solver); */
}

void level_drop_tile(level_t *level, tile_pos_t *drag_target, tile_pos_t *drop_target)
{
    assert_not_null(level);
    assert_not_null(drag_target);
    assert_not_null(drop_target);

    assert(drag_target->tile->enabled);
    assert(drop_target->tile->enabled);

    if (!drop_target->tile->fixed && level_using_unsolved_tiles(level) && (drag_target != drop_target)) {
        level_swap_tile_pos(level, level->drag_target, drop_target, true);
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

void level_set_hovered_feature(level_t *level, path_type_t type)
{
    assert_not_null(level);
    if (level->hover) {
        tile_pos_t *pos = level->hover;
        if (pos->tile->enabled) {
            tile_pos_set_hovered_feature(pos, type);
            level->changed = true;
        }
    }
}

void level_clear_hovered_tile(level_t *level)
{
    assert_not_null(level);
    if (level->hover) {
        tile_pos_t *pos = level->hover;
        if (pos->tile->enabled) {
            tile_pos_clear(pos, level);
            level->changed = true;
        }
    }
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

    used_tiles_t save_used_tiles = level->currently_used_tiles;
    level_use_solved_tile_pos(level);

    while (new_radius > level->radius) {
        level->radius++;
        level_enable_ring(level, level->radius );
    }

    while (new_radius < level->radius) {
        level->radius--;
        level_disable_ring(level, level->radius + 1);
    }
    level->currently_used_tiles = save_used_tiles;
}

void level_win(level_t *level)
{
    assert_not_null(level);

    if (win_anim_running(level->win_anim)) {
        return;
    }

    level->finished = true;
    if (game_mode == GAME_MODE_PLAY_LEVEL) {
        set_game_mode(GAME_MODE_WIN_LEVEL);
    }

    win_anim_start(level->win_anim);

    if (level->radius > options->max_win_radius) {
        options->max_win_radius = level->radius;
    }

    if (level->seed) {
        regen_level_preview();
    }

    nvdata_mark_finished(level);
}

void level_unwin(level_t *level)
{
    assert_not_null(level);

    if (!win_anim_running(level->win_anim)) {
        return;
    }

    win_anim_stop(level->win_anim);

    level->finished = false;
    if (game_mode == GAME_MODE_WIN_LEVEL) {
        set_game_mode(GAME_MODE_PLAY_LEVEL);
    }
}

bool level_is_fading(level_t *level)
{
    return level->fade.active;
}

bool level_update_fade(level_t *level)
{
    assert_not_null(level);

    if (level->fade.active && (level->fade.value != level->fade.target)) {
        if (fabs(level->fade.value - level->fade.target) <= LEVEL_FADE_DELTA) {
            level->fade.value       = level->fade.target;
            level->fade.value_eased = level->fade.target;
#ifdef DEBUG_LEVEL_FADE
            printf("level_update_fade(): stopped at fade_target = %f\n", level->fade.target);
#endif

            if (level->fade.finished_callback) {
                level->fade.finished_callback(level, level->fade.finished_data);
            }
            level->fade.active = false;

        } else {
#ifdef DEBUG_LEVEL_FADE
            float old_fade_value = level->fade.value;
#endif

            level->fade.value += level->fade.delta;
            level->fade.value_eased = ease_exponential_out(level->fade.value);

#ifdef DEBUG_LEVEL_FADE
            printf("level_update_fade(): fade_value %f -> %f\n", old_fade_value, level->fade.value);
#endif
        }

        if (level->fade.do_rotate) {
            level->fade.rotate_level = (1.0 - ease_circular_out(level->fade.value)) * (TAU/2.0);
            level->fade.rotate_level *= level->fade.rotate_speed;
        } else {
            level->fade.rotate_level = 0.0f;
        }
    }

    return level->fade.value != 1.0f;
}

static void level_fade_transition(level_t *level, level_fade_finished_cb_t callback, void *data)
{
    assert_not_null(level);

    level->fade.finished_callback = callback;
    level->fade.finished_data = data;

    level->fade.rotate_speed = 0.8333;
    if (rand() & 0x00000001) {
        level->fade.rotate_speed *= -1.0;
    }

    if (level->fade.value < level->fade.target) {
        level->fade.delta = LEVEL_FADE_DELTA;
        level->fade.active = true;
    } else if (level->fade.value > level->fade.target) {
        level->fade.delta = -LEVEL_FADE_DELTA;
        level->fade.active = true;
    } else {
        /* equal */
    }

#ifdef DEBUG_LEVEL_FADE
    printf("level_fade_transition(): fade_value  = %f\n", level->fade.value);
    printf("level_fade_transition(): fade_target = %f\n", level->fade.target);
    printf("level_fade_transition(): fade_delta  = %f\n", level->fade.delta);
#endif
}

void level_fade_in(level_t *level, level_fade_finished_cb_t callback, void *data)
{
    assert_not_null(level);

    if (level_is_fading(level)) {
        warnmsg("level_fade_in() called while fade is already running");
        return;
    }

#ifdef DEBUG_LEVEL_FADE
    printf("level_fade_in()\n");
#endif
    level->fade.target = 1.0f;
    level_fade_transition(level, callback, data);
}

void level_fade_out(level_t *level, level_fade_finished_cb_t callback, void *data)
{
    assert_not_null(level);

    if (level_is_fading(level)) {
        warnmsg("level_fade_out() called while fade is already running");
        return;
    }

#ifdef DEBUG_LEVEL_FADE
    printf("level_fade_out()\n");
#endif
    level->fade.target = 0.0f;
    level_fade_transition(level, callback, data);
}

static void level_update_tile_pops_callback(hex_axial_t axial, void *data)
{
    level_t *level = (level_t *)data;
    tile_pos_t *pos = level_get_unsolved_tile_pos(level, axial);
    if (!pos) {
        return;
    }

    assert(pos->inner_neighbors_count >= 0);

    switch (pos->inner_neighbors_count) {
    case 0:
        break;

    case 1:
        pos->extra_magnitude = MAX(pos->extra_magnitude,
                                   pos->inner_neighbors[0]->extra_magnitude);
        break;

    default:
        for (int i=0; i<pos->inner_neighbors_count; i++) {
            pos->extra_magnitude = MAX(pos->extra_magnitude,
                                       pos->inner_neighbors[i]->extra_magnitude);
        }
        break;
    }

    pos->extra_translate = Vector2Scale(pos->radial_vector_norm, pos->extra_magnitude);
}

void level_update_tile_pops(level_t *level)
{
    tile_pos_t *center = level_get_center_tile_pos(level);
    for (int ring = 1; ring < level->radius; ring++) {
        hex_axial_foreach_in_ring(center->position,
                                  ring + 1,
                                  level_update_tile_pops_callback,
                                  level);
    }
}

void level_shuffle_tiles(level_t *level)
{
    used_tiles_t save_currently_used_tiles = level->currently_used_tiles;
    level->currently_used_tiles = USED_TILES_UNSOLVED;

    tile_pos_t *backup_enabled_positions[LEVEL_MAXTILES];
    memcpy(backup_enabled_positions, level->enabled_positions, sizeof(backup_enabled_positions));

    int num_positions = level_get_enabled_positions(level);

    for (int i=num_positions-1; i>0; i--) {
        int j = i;
        while (i == j) {
            j = global_rng_get(i + 1);
        }

        tile_pos_t *pos_i = level->enabled_positions[i];
        tile_pos_t *pos_j = level->enabled_positions[j];
        level_swap_tile_pos(level, pos_i, pos_j, false);
        level->enabled_positions[i] = pos_j;
        level->enabled_positions[j] = pos_i;
    }

    memcpy(level->enabled_positions, backup_enabled_positions, sizeof(backup_enabled_positions));
    level->currently_used_tiles = save_currently_used_tiles;
}

void level_reset_win_anim(level_t *level)
{
    level->extra_rotate_level          = 0.0f;
    level->extra_rotate_level_speed    = 0.0f;
    level->extra_rotate_level_velocity = 0.0f;

    for (int i = 0; i < LEVEL_MAXTILES; i++) {
        tile_pos_t *solved_pos = &level->solved_positions[i];
        tile_pos_t *unsolved_pos = &level->unsolved_positions[i];

        tile_pos_reset_win_anim(solved_pos);
        tile_pos_reset_win_anim(unsolved_pos);
    }

    //printf("level_reset_win_anim()\n");
}
