/****************************************************************************
 *                                                                          *
 * level.h                                                                  *
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

#ifndef LEVEL_H
#define LEVEL_H

#include "cJSON/cJSON.h"

#include "const.h"

#define IS_LEVEL_FILENAME(filename)                             \
    (0 == strcmp(filename_ext(filename), LEVEL_FILENAME_EXT))

#include "const.h"
#include "hex.h"
#include "tile.h"
#include "tile_pos.h"

enum used_tiles {
    USED_TILES_NULL = 0,
    USED_TILES_SOLVED,
    USED_TILES_UNSOLVED
};
typedef enum used_tiles used_tiles_t;

struct collection;
struct level;

typedef void (*level_fade_finished_cb_t)(struct level *level, void *data);

struct level {
    char *id;
    bool have_id;

    char name[NAME_MAXLEN];
    char name_backup[NAME_MAXLEN];

    int radius;

    used_tiles_t currently_used_tiles;

    int tile_count;

    int current_tile_write_idx;
    tile_t tiles[LEVEL_MAXTILES];
    tile_t *sorted_tiles[LEVEL_MAXTILES];
    tile_t *enabled_tiles[LEVEL_MAXTILES];

    tile_pos_t solved_positions[LEVEL_MAXTILES];
    tile_pos_t unsolved_positions[LEVEL_MAXTILES];
    tile_pos_t *enabled_positions[LEVEL_MAXTILES];

    hex_axial_t center;

    float req_tile_size;
    float tile_size;

    Vector2 px_offset;
    Vector2 px_grid_size;
    Vector2 px_min;
    Vector2 px_max;
    Rectangle px_bounding_box;

    tile_pos_t *hover;
    hex_direction_t hover_section;
    tile_pos_t *hover_adjacent;
    hex_direction_t hover_adjacent_section;
    float hover_section_adjacency_radius;

    Vector2 mouse_pos;

    char ui_name[UI_NAME_MAXLEN];

    tile_pos_t *drag_target;
    Vector2 drag_start;
    Vector2 drag_offset;
    int drag_reset_frames;
    int drag_reset_total_frames;
    Vector2 drag_reset_vector;

    char *savepath;
    char *filename;
    char *dirpath;
    bool changed;

    bool finished;
    float finished_hue;

    bool  fade_active;
    float fade_value;
    float fade_value_eased;
    float fade_delta;
    float fade_target;
    level_fade_finished_cb_t fade_finished_callback;
    void *fade_finished_data;
    float fade_rotate_speed;
    float spin_direction;

    bool have_physics_body;
    struct PhysicsBodyData *physics_floor;
    Rectangle floor_rect;
    Vector2 physics_rotate_center;

    struct win_anim *win_anim;

    struct collection *collection;

    struct level *prev, *next;
};
typedef struct level level_t;

#define edit_mode_solved   ((edit_mode) && current_level && \
                            (current_level->currently_used_tiles == USED_TILES_SOLVED))
#define edit_mode_unsolved ((edit_mode) && current_level && \
                            (current_level->currently_used_tiles == USED_TILES_UNSOLVED))

#define dragable_mode (play_mode || edit_mode_unsolved)

int hex_axial_to_idx(hex_axial_t axial);
tile_pos_t *level_get_solved_tile_pos(level_t *level,  hex_axial_t axial);
tile_pos_t *level_get_unsolved_tile_pos(level_t *level,  hex_axial_t axial);

level_t *create_level(struct collection *collection);
void destroy_level(level_t *level);
void level_reset(level_t *level);
void level_backup_unsolved_tiles(level_t *level);
void level_reset_tile_positions(level_t *level);

void level_update_id(level_t *level);

bool level_eq_tiles(level_t *level, level_t *other);
void level_sort_tiles(level_t *level);
int level_get_enabled_tiles(level_t *level);
int level_get_enabled_positions(level_t *level);

inline static bool level_using_solved_tiles(level_t *level)
{
    return level->currently_used_tiles == USED_TILES_SOLVED;
}

inline static bool level_using_unsolved_tiles(level_t *level)
{
    return level->currently_used_tiles == USED_TILES_UNSOLVED;
}

void level_use_solved_tile_pos(level_t *level);
void level_use_unsolved_tile_pos(level_t *level);
void level_use_null_tile_pos(level_t *level);

void level_toggle_currently_used_tiles(level_t *level);

tile_t *level_get_tile(level_t *level,  hex_axial_t axial);
tile_pos_t *level_get_current_tile_pos(level_t *level,  hex_axial_t axial);

bool level_parse_string(level_t *level, const char *str);
level_t *load_level_string(const char *filename, const char *str, bool is_pack);
level_t *load_level_json(const char *filename, cJSON *json, bool is_pack);
level_t *load_level_file(const char *filename);

void level_update_ui_name(level_t *level, int idx);

bool level_has_empty_tiles(level_t *level);
bool level_check(level_t *level);
void level_unload(void);
void level_load(level_t *level);
void level_play(level_t *level);
void level_edit(level_t *level);

void level_set_file_path(level_t *level, const char *path);
void level_save_to_filename(level_t *level, const char *filepath);
void level_save_to_file(level_t *level, const char *dirpath);
void level_save_to_file_if_changed(level_t *level, const char *dirpath);
void level_save(level_t *level);
bool level_from_json(level_t *level, cJSON *json);
cJSON *level_to_json(level_t *level);

tile_pos_t *level_find_solved_neighbor_tile_pos(level_t *level, tile_pos_t *tile, hex_direction_t section);
tile_pos_t *level_find_unsolved_neighbor_tile_pos(level_t *level, tile_pos_t *tile, hex_direction_t section);
tile_pos_t *level_find_current_neighbor_tile_pos(level_t *level, tile_pos_t *pos, hex_direction_t section);
tile_pos_t *level_get_center_tile_pos(level_t *leve);

void level_resize(level_t *level);
void level_set_hover(level_t *level, IVector2 mouse_position);
void level_swap_tile_pos(level_t *level, tile_pos_t *a, tile_pos_t *b);
void level_drag_start(level_t *level);
void level_drag_stop(level_t *level);
void level_modify_hovered_feature(level_t *level);
void level_set_hovered_feature(level_t *level, path_type_t type);
void level_clear_hovered_tile(level_t *level);
void level_serialize(level_t *level, FILE *f);
char *level_serialize_memory(level_t *level);
void level_enable_spiral(level_t *level, int radius);
void level_disable_spiral(level_t *level, int radius);
void level_enable_ring(level_t *level, int radius);
void level_disable_ring(level_t *level, int radius);
void level_set_radius(level_t *level, int new_radius);
void level_draw(level_t *level, bool finished);
void level_preview(level_t *level, Rectangle rect);
void level_win(level_t *level);
void level_unwin(level_t *level);
bool level_update_fade(level_t *level);
void level_fade_in(level_t *level, level_fade_finished_cb_t callback, void *data);
void level_fade_out(level_t *level, level_fade_finished_cb_t callback, void *data);

void level_create_physics_body(level_t *level);
void level_reset_physics_body_positions(level_t *level);
void level_destroy_physics_body(level_t *level);
void level_update_physics_forces(level_t *level);

extern level_t *current_level;

#endif /*LEVEL_H*/
