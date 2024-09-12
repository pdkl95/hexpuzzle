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

#include "const.h"

#define IS_LEVEL_FILENAME(filename)                             \
    (0 == strcmp(filename_ext(filename), LEVEL_FILENAME_EXT))

#include "const.h"
#include "hex.h"
#include "tile.h"

struct level {
    char *id;
    char name[NAME_MAXLEN];
    char name_backup[NAME_MAXLEN];

    int radius;
    tile_t tiles[TILE_LEVEL_WIDTH][TILE_LEVEL_HEIGHT];
    int tile_count;

    hex_axial_t center;
    tile_t *center_tile;

    float req_tile_size;
    float tile_size;

    Vector2 px_offset;
    Vector2 px_grid_size;
    Vector2 px_min;
    Vector2 px_max;
    Rectangle px_bounding_box;

    tile_t *hover;
    hex_direction_t hover_section;
    tile_t *hover_adjacent;
    hex_direction_t hover_adjacent_section;
    float hover_section_adjacency_radius;

    Vector2 mouse_pos;

    tile_t *drag_target;
    Vector2 drag_start;
    Vector2 drag_offset;
    int drag_reset_frames;
    int drag_reset_total_frames;
    Vector2 drag_reset_vector;

    char *filename;
    bool changed;

    bool finished;
    char ui_name[UI_NAME_MAXLEN];

    struct level *prev, *next;
};
typedef struct level level_t;

level_t *create_level(void);
void destroy_level(level_t *level);
void level_reset(level_t *level);

bool level_parse_string(level_t *level, char *str);
level_t *load_level_file(char *filename);

tile_t *level_get_tile(level_t *level,  hex_axial_t axial);

void level_update_ui_name(level_t *level);

bool level_check(level_t *level);
void level_play(level_t *level);

void level_save_to_file(level_t *level, char *dirpath);
void level_save_to_file_if_changed(level_t *level, char *dirpath);

bool level_replace_from_memory(level_t *level, char *str);
void level_serialize(level_t *level, FILE *f);

void level_resize(level_t *level);
tile_t *level_find_neighbor_tile(level_t *level, tile_t *tile, hex_direction_t section);
void level_set_hover(level_t *level, IVector2 mouse_position);
void level_drag_start(level_t *level);
void level_drag_stop(level_t *level);
void level_modify_hovered_feature(level_t *level);
void level_serialize(level_t *level, FILE *f);
void level_enable_spiral(level_t *level, int radius);
void level_disable_spiral(level_t *level, int radius);
void level_enable_ring(level_t *level, int radius);
void level_disable_ring(level_t *level, int radius);
void level_set_radius(level_t *level, int new_radius);
void level_draw(level_t *level, bool finished);

extern level_t *current_level;

#endif /*LEVEL_H*/
