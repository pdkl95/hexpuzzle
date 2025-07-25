/****************************************************************************
 *                                                                          *
 * tile_pos.h                                                               *
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

#ifndef TILE_POS_H
#define TILE_POS_H

#include "tile.h"

struct level;

struct tile_section {
    Vector2 corners[3];
};
typedef struct tile_section tile_section_t;

struct tile_coord {
    Vector2 center;
    Vector2 corners[7];
    Vector2 midpoints[7];
    Vector2 midpoint_path_cw[7];
    Vector2 midpoint_path_ccw[7];
    Vector2 radial_unit[7];
    tile_section_t sections[6];
};
typedef struct tile_coord tile_coord_t;

struct tile_pos {
    tile_t *tile;
    tile_t *orig_tile;

    hex_axial_t position;
    hex_axial_t orig_position;

    int ring_radius;

    bool solved;

    /*
     * drawing
     */
    float size;
    float line_width;
    float center_circle_draw_radius;

    tile_coord_t win; // window coordinates
    tile_coord_t rel; // tile-center relative coordinates

#ifdef USE_PHYSICS
    Vector2 physics_position;
    Vector2 physics_velocity;
    float   physics_rotation;
#endif

    float   extra_rotate;
    float   extra_rotate_magnitude;
    Vector2 extra_translate;
    float   extra_magnitude;

    float   pop_out_phase;
    float   pop_in_phase;
    float   pop_magnitude;
    Vector2 pop_translate;
    float   prev_ring_phase;

    int     center_distance;

    Vector2 radial_vector;
    Vector2 radial_vector_norm;
    float   radial_angle;

    /*
     * ui
     */
    bool hover;
    bool hover_center;
    hex_direction_t hover_section;
    float center_circle_hover_radius;

    struct tile_pos *neighbors[6];
    struct tile_pos *outer_neighbors[6];
    struct tile_pos  *ring_neighbors[6];
    struct tile_pos *inner_neighbors[6];
    int outer_neighbors_count;
    int  ring_neighbors_count;
    int inner_neighbors_count;

    struct tile_pos *swap_target;

    struct tile_pos *hover_adjacent;
};
typedef struct tile_pos tile_pos_t;

void print_tile_pos(tile_pos_t *pos);
tile_pos_t *init_tile_pos(tile_pos_t *pos, tile_t *tile, hex_axial_t addr);
tile_pos_t *create_tile_pos(hex_axial_t addr, tile_t *tile);
void destroy_tile_pos(tile_pos_t *tile_pos);

void tile_pos_reset(tile_pos_t *tile_pos);
bool tile_pos_check(tile_pos_t *tile_pos, int *path_count, int *finished_path_count);
void tile_pos_rebuild(tile_pos_t *pos);
void tile_pos_set_size(tile_pos_t *tile_pos, float tile_size);
void tile_pos_draw(tile_pos_t *tile_pos, tile_pos_t *drag_target, bool finished, Color finished_color);

void tile_pos_set_hover(tile_pos_t *tile_pos, Vector2 mouse_pos);
void tile_pos_unset_hover(tile_pos_t *tile_pos);

void tile_pos_cycle_path_section(tile_pos_t *tile_pos, hex_direction_t section);
void tile_pos_modify_hovered_feature(tile_pos_t *pos);
void tile_pos_set_hovered_feature(tile_pos_t *pos, path_type_t type);

void tile_pos_clear(tile_pos_t *pos, struct level *level);
void tile_pos_toggle_hidden(tile_pos_t *pos, bool add_undo);

static inline bool tile_pos_dragable(tile_pos_t *pos)
{
    return pos && pos->tile && tile_dragable(pos->tile);
}

void tile_pos_reset_win_anim(tile_pos_t *pos);

#endif /*TILE_POS_H*/

