/****************************************************************************
 *                                                                          *
 * level_undo.h                                                             *
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

#ifndef LEVEL_UNDO_H
#define LEVEL_UNDO_H

#include "hex.h"
#include "tile.h"
#include "level.h"

/* basic event containers */

struct undo_swap_event {
    hex_axial_t a;
    hex_axial_t b;
};
typedef struct undo_swap_event undo_swap_event_t;

struct undo_use_tiles_event {
    used_tiles_t from;
    used_tiles_t to;
};
typedef struct undo_use_tiles_event undo_use_tiles_event_t;

struct undo_set_radius_event {
    int from;
    int to;
};
typedef struct undo_set_radius_event undo_set_radius_event_t;

struct undo_set_flags_event {
    tile_t      *tile;
    tile_flags_t from;
    tile_flags_t to;
};
typedef struct undo_set_flags_event undo_set_flags_event_t;

struct undo_set_flags_and_paths_event {
    tile_t               *tile;
    tile_flags_t          flags_from;
    tile_flags_t          flags_to;
    tile_neighbor_paths_t paths_from;
    tile_neighbor_paths_t paths_to;
};
typedef struct undo_set_flags_and_paths_event undo_set_flags_and_paths_event_t;

struct undo_change_path_event {
    tile_t         *tile1;
    hex_direction_t tile1_section;
    path_type_t     tile1_path_from;
    path_type_t     tile1_path_to;

    tile_t         *tile2;
    hex_direction_t tile2_section;
    path_type_t     tile2_path_from;
    path_type_t     tile2_path_to;
};
typedef struct undo_change_path_event undo_change_path_event_t;


/*** play events */

enum undo_play_event_type {
    UNDO_PLAY_TYPE_NONE = 0,
    UNDO_PLAY_TYPE_SWAP
};
typedef enum undo_play_event_type undo_play_event_type_t;
struct undo_play_event {
    undo_play_event_type_t type;
    union {
        undo_swap_event_t swap;
    };
};
typedef struct undo_play_event undo_play_event_t;

/*** edit events ***/

enum undo_edit_event_type {
    UNDO_EDIT_TYPE_NONE = 0,
    UNDO_EDIT_TYPE_SWAP,
    UNDO_EDIT_TYPE_USE_TILES,
    UNDO_EDIT_TYPE_SET_RADIUS,
    UNDO_EDIT_TYPE_SET_FLAGS,
    UNDO_EDIT_TYPE_SET_FLAGS_AND_PATHS,
    UNDO_EDIT_TYPE_CHANGE_PATH
};
typedef enum undo_edit_event_type undo_edit_event_type_t;
struct undo_edit_event {
    undo_edit_event_type_t type;
    union {
        undo_swap_event_t                swap;
        undo_use_tiles_event_t           use_tiles;
        undo_set_radius_event_t          set_radius;
        undo_set_flags_event_t           set_flags;
        undo_set_flags_and_paths_event_t set_flags_and_paths;
        undo_change_path_event_t         change_path;
    };
};
typedef struct undo_edit_event undo_edit_event_t;

/*** yndo (generic) events ***/

enum undo_event_type {
    UNDO_EVENT_TYPE_NULL = 0,
    UNDO_EVENT_TYPE_PLAY,
    UNDO_EVENT_TYPE_EDIT
};
typedef enum undo_event_type undo_event_type_t;
struct undo_event {
    undo_event_type_t type;
    union {
        undo_play_event_t play;
        undo_edit_event_t edit;
    };
};
typedef struct undo_event undo_event_t;


/*** undo management wrapper */

struct undo {
    struct level *level;
    int play_count;
    int edit_count;
    struct undo_list *play_event_list;
    struct undo_list *edit_event_list;
};
typedef struct undo undo_t;


undo_t *create_level_undo(struct level *level);
void destroy_level_undo(undo_t *undo);

void level_undo_add_event(level_t *level, undo_event_t event);
void level_undo_add_play_event(level_t *level, undo_play_event_t event);
void level_undo_add_edit_event(level_t *level, undo_edit_event_t event);

void level_undo_add_swap_event(level_t *level, hex_axial_t a, hex_axial_t b);
void level_undo_add_use_tiles_event(level_t *level, used_tiles_t from, used_tiles_t to);
void level_undo_add_set_radius_event(level_t *level, int from, int to);

void level_undo_add_set_flags_event(
    level_t *level,
    tile_t *tile,
    tile_flags_t from,
    tile_flags_t to);

void level_undo_add_set_flags_event_with_neighbor_paths(
    level_t *level,
    tile_t *tile,
    tile_flags_t flags_from,
    tile_neighbor_paths_t neighbor_paths_from,
    tile_flags_t flags_to,
    tile_neighbor_paths_t neighbor_paths_to);

void level_undo_add_change_path_event(
    level_t *level,

    tile_t *tile,
    hex_direction_t section,
    path_type_t from,
    path_type_t to);

void level_undo_add_change_paths_event(
    level_t *level,

    tile_t *tile1,
    hex_direction_t tile1_section,
    path_type_t tile1_path_from,
    path_type_t tile1_path_to,

    tile_t *tile2,
    hex_direction_t tile2_section,
    path_type_t tile2_path_from,
    path_type_t tile2_path_to);

void level_undo_play(level_t *level);
void level_redo_play(level_t *level);
void level_undo_edit(level_t *level);
void level_redo_edit(level_t *level);

#endif /*LEVEL_UNDO_H*/

