/****************************************************************************
 *                                                                          *
 * tile_pos.c                                                               *
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
#include "tile.h"
#include "tile_pos.h"
#include "level.h"
#include "level_undo.h"

void print_tile_pos(tile_pos_t *pos)
{
    if (pos) {
        printf("tile_pos<pos=%s flags=%s path=%s h=%s ha=%s>\n",
               tile_our_pos_string(pos),
               tile_flag_string(pos->tile),
               tile_path_string(pos->tile),
               pos->hover ? "T" : "F",
               pos->hover_adjacent ? "T" : "F");
    } else {
        printf("tile_pos<NULL>\n");
    }
}

tile_pos_t *init_tile_pos(tile_pos_t *pos, tile_t *tile, hex_axial_t addr)
{
    assert_not_null(pos);
    assert_not_null(tile);

    pos->tile = tile;

    pos->position = addr;
    pos->orig_position = addr;

    pos->swap_target = NULL;
    pos->hover_adjacent = NULL;
    pos->hover = false;
    pos->hover_center = false;

    pos->physics_position = VEC2_ZERO;
    pos->physics_rotation = 0.0f;

    return pos;
}

tile_pos_t *create_tile_pos(hex_axial_t addr, tile_t *tile)
{
    tile_pos_t *pos = calloc(1, sizeof(tile_pos_t));
    init_tile_pos(pos, tile, addr);
    return pos;
}

void destroy_tile_pos(tile_pos_t *pos)
{
    SAFEFREE(pos);
}

bool tile_pos_check(tile_pos_t *pos, int *path_count, int *finished_path_count)
{
    bool rv = true;

    each_direction {
        if (pos->tile->path[dir] != PATH_TYPE_NONE) {
            if (path_count) {
                *path_count += + 1;
            }

            hex_direction_t opp_i = hex_opposite_direction(dir);
            tile_pos_t *neighbor = pos->neighbors[dir];
            if (neighbor) {
                if (pos->tile->path[dir] == neighbor->tile->path[opp_i]) {
                    if (finished_path_count) {
                        *finished_path_count += 1;
                    }
                } else {
                    rv = false;
                }
            }
        }
    }

    return rv;
}

void tile_pos_set_hover(tile_pos_t *pos, Vector2 mouse_pos)
{
    assert_not_null(pos);

    Vector2 relvec = Vector2Subtract(mouse_pos, pos->win.center);
    float theta = atan2f(-relvec.y, -relvec.x);
    theta += TAU/2.0;
    theta = TAU - theta;
    pos->hover = true;
    pos->hover_section = (int)(theta/TO_RADIANS(60.0));
    //printf("tile_pos->hover_section = %d (theta = %f)\n", pos->hover_section, theta);
    pos->hover_center =
        (Vector2DistanceSqr(mouse_pos, pos->win.center)
         < (pos->center_circle_hover_radius *
            pos->center_circle_hover_radius));
}

void tile_pos_unset_hover(tile_pos_t *pos)
{
    assert_not_null(pos);

    pos->hover = false;
    pos->hover_center = false;

    pos->hover_adjacent = NULL;

    if (pos->swap_target) {
        pos->swap_target->swap_target = NULL;
        tile_pos_unset_hover(pos->swap_target);
        pos->swap_target = NULL;
    }
}

void tile_pos_toggle_fixed(tile_pos_t *pos)
{
    assert_not_null(pos);

    bool include_prev_swap = false;
    if (!hex_axial_eq(pos->position, pos->tile->unsolved_pos->position)) {
        level_solve_tile(current_level, pos->position, true);
        include_prev_swap = true;
    }

    tile_flags_t old_flags = tile_get_flags(pos->tile);

    pos->tile->fixed = !pos->tile->fixed;

    tile_flags_t new_flags = tile_get_flags(pos->tile);
    level_undo_add_set_flags_event(current_level, include_prev_swap, pos->tile, old_flags, new_flags);
}

void tile_pos_toggle_hidden(tile_pos_t *pos, bool add_undo)
{
    assert_not_null(pos);

    bool include_prev_swap = false;
    if (!hex_axial_eq(pos->position, pos->tile->unsolved_pos->position)) {
        level_solve_tile(current_level, pos->position, true);
        include_prev_swap = true;
    }

    tile_flags_t old_flags;
    tile_neighbor_paths_t old_neighbor_paths;
    if (add_undo) {
        old_flags = tile_get_flags(pos->tile);
        old_neighbor_paths = tile_get_neighbor_paths(pos->tile);
    }

    pos->tile->hidden = !pos->tile->hidden;

    each_direction {
        hex_direction_t opp_dir =
            hex_opposite_direction(dir);
        tile_pos_t *neighbor = pos->neighbors[dir];

        if (neighbor) {
            if (pos->tile->hidden) {
                //printf("hide neighbor path at (%d, %d) dir %d to (%d, %d)\n", pos->position.q, pos->position.r, dir, neighbor->position.q, neighbor->position.r);
                pos->tile->saved_path[dir] =  pos->tile->path[dir];
                pos->tile->path[dir] = PATH_TYPE_NONE;;
                neighbor->tile->path[opp_dir] = PATH_TYPE_NONE;
            } else {
                pos->tile->path[dir] =  pos->tile->saved_path[dir];
                pos->tile->saved_path[dir] = PATH_TYPE_NONE;;
                neighbor->tile->path[opp_dir] = pos->tile->path[dir];;
                //printf("show neighbor path at (%d, %d) dir %d to (%d, %d)\n", pos->position.q, pos->position.r, dir, neighbor->position.q, neighbor->position.r);
            }
            //} else {
            //printf("no neighbor at (%d, %d) dir %d\n", pos->position.q, pos->position.r, dir);
        }
    }

    tile_update_path_count(pos->tile);

    if (add_undo) {
        tile_flags_t new_flags = tile_get_flags(pos->tile);
        tile_neighbor_paths_t new_neighbor_paths = tile_get_neighbor_paths(pos->tile);
        level_undo_add_set_flags_event_with_neighbor_paths(current_level,
                                                           include_prev_swap,
                                                           pos->tile,
                                                           old_flags, old_neighbor_paths,
                                                           new_flags, new_neighbor_paths);
    }
}

void tile_pos_set_path_section(tile_pos_t *pos, hex_direction_t section, path_type_t type)
{
    assert_not_null(current_level);
    assert_not_null(pos);
    assert(type >= 0);
    assert(type <= PATH_TYPE_COUNT);

    path_type_t old_path = pos->tile->path[section];
    pos->tile->path[section] = type;

    tile_update_path_count(pos->tile);

    if (pos->hover_adjacent) {
        hex_direction_t opposite_section =
            hex_opposite_direction(section);
        tile_t *adjacent_tile = pos->hover_adjacent->tile;
        path_type_t old_adjacent_path = adjacent_tile->path[opposite_section];
        adjacent_tile->path[opposite_section] = pos->tile->path[section];
        tile_update_path_count(pos->hover_adjacent->tile);

        level_undo_add_change_paths_event(current_level,

                                          pos->tile,
                                          section,
                                          old_path,
                                          pos->tile->path[section],

                                          adjacent_tile,
                                          opposite_section,
                                          old_adjacent_path,
                                          adjacent_tile->path[opposite_section]);
    } else {
        level_undo_add_change_path_event(current_level,
                                         pos->tile,
                                         section,
                                         old_path,
                                         pos->tile->path[section]);
    }
}

void tile_pos_cycle_path_section(tile_pos_t *pos, hex_direction_t section)
{
    assert_not_null(pos);

    path_type_t new_type = pos->tile->path[section];
    if (mouse_right_click) {
        new_type = new_type == PATH_TYPE_MIN ? PATH_TYPE_MAX : (new_type - 1);
    } else {
        new_type = new_type == PATH_TYPE_MAX ? PATH_TYPE_MIN : (new_type + 1);
    }

    tile_pos_set_path_section(pos, section, new_type);
}

static void tile_pos_modify_center(tile_pos_t *pos)
{
    assert_not_null(pos);

    if (is_any_shift_down()) {
        tile_pos_toggle_hidden(pos, true);
    } else {
        tile_pos_toggle_fixed(pos);
    }
}

void tile_pos_modify_hovered_feature(tile_pos_t *pos)
{
    assert_not_null(pos);

    if (pos->tile->hidden) {
        tile_pos_toggle_hidden(pos, true);
    } else {
        if (pos->hover_center) {
            tile_pos_modify_center(pos);
        } else {
            tile_pos_cycle_path_section(pos, pos->hover_section);
        }
    }
}

void tile_pos_set_hovered_feature(tile_pos_t *pos, path_type_t type)
{
    assert_not_null(pos);

    if (pos->tile->hidden) {
        tile_pos_toggle_hidden(pos, true);
    } else {
        if (pos->hover_center) {
            tile_pos_modify_center(pos);
        } else {
            tile_pos_set_path_section(pos, pos->hover_section, type);
        }
    }
}

void tile_pos_clear(tile_pos_t *pos, struct level *level)
{
    each_direction {
        pos->tile->path[dir] = PATH_TYPE_NONE;
        tile_pos_t *neighbor = level_find_current_neighbor_tile_pos(level, pos, dir);
        if (neighbor) {
            hex_direction_t opp_i = hex_opposite_direction(dir);
            neighbor->tile->path[opp_i] = PATH_TYPE_NONE;
        }
    }
}

void tile_pos_rebuild(tile_pos_t *pos)
{
    assert_not_null(pos);

    pos->ring_radius = hex_axial_distance(pos->position, LEVEL_CENTER_POSITION);

    pos->extra_rotate = 0.0f;
    pos->extra_translate = VEC2_ZERO;
    pos->pop_translate = VEC2_ZERO;
    pos->pop_magnitude = 0.0f;
    pos->pop_out_phase = 0.0f;
    pos->pop_in_phase = 0.0f;
    pos->prev_ring_phase = 0.0f;

    pos->line_width = pos->size / 6.0;
    pos->center_circle_draw_radius = pos->line_width * 1.2;
    pos->center_circle_hover_radius = pos->line_width * 1.75;
    pos->win.center = hex_axial_to_pixel(pos->position, pos->size);

    pos->rel.center = Vector2Zero();

    Vector2 *corners = hex_pixel_corners(pos->win.center, pos->size);
    memcpy(pos->win.corners, corners, 7 * sizeof(Vector2));

    each_direction {
        Vector2 c0 = pos->win.corners[dir];
        Vector2 c1 = pos->win.corners[dir + 1];

        pos->win.midpoints[dir] = Vector2Lerp(c0, c1, 0.5);

        pos->rel.corners[dir]   = Vector2Subtract(pos->win.corners[dir],   pos->win.center);
        pos->rel.midpoints[dir] = Vector2Subtract(pos->win.midpoints[dir], pos->win.center);
    }

    Vector2 cent = Vector2Lerp(pos->win.midpoints[0], pos->win.midpoints[3], 0.5);

    float half_line_width = pos->line_width / 2.0f;

    each_direction {
        pos->win.radial_unit[dir] = Vector2Normalize(Vector2Subtract(pos->win.midpoints[dir], pos->win.center));
        pos->rel.radial_unit[dir] = pos->win.radial_unit[dir];

        Vector2 c0 = pos->win.corners[dir];
        Vector2 c1 = pos->win.corners[dir + 1];

        pos->win.sections[dir].corners[0] = c0;
        pos->win.sections[dir].corners[1] = c1;
        pos->win.sections[dir].corners[2] = cent;

        pos->rel.sections[dir].corners[0] = Vector2Subtract(pos->win.sections[dir].corners[0], pos->win.center);
        pos->rel.sections[dir].corners[1] = Vector2Subtract(pos->win.sections[dir].corners[1], pos->win.center);
        pos->rel.sections[dir].corners[2] = Vector2Subtract(pos->win.sections[dir].corners[2], pos->win.center);

        Vector2 mid_to_corner = Vector2Subtract(pos->win.midpoints[dir], pos->win.sections[dir].corners[0]);
        Vector2 halfpath = Vector2Scale(Vector2Normalize(mid_to_corner), half_line_width);

        pos->win.midpoint_path_cw[dir] = Vector2Add(pos->win.midpoints[dir], halfpath);
        halfpath = Vector2Scale(halfpath, -1.0);
        pos->win.midpoint_path_ccw[dir] = Vector2Add(pos->win.midpoints[dir], halfpath);

        pos->rel.midpoint_path_cw[dir] = Vector2Subtract(pos->rel.midpoint_path_cw[dir], pos->win.center);
        pos->rel.midpoint_path_ccw[dir] = Vector2Subtract(pos->rel.midpoint_path_ccw[dir], pos->win.center);
    }
}

void tile_pos_set_size(tile_pos_t *pos, float tile_pos_size)
{
    assert_not_null(pos);

    pos->size = tile_pos_size;

    tile_pos_rebuild(pos);
}

void tile_pos_reset_win_anim(tile_pos_t *pos)
{
    pos->physics_position = VEC2_ZERO;
    pos->physics_velocity = VEC2_ZERO;
    pos->physics_rotation = 0.0f;
    pos->extra_rotate     = 0.0f;
    pos->extra_translate  = VEC2_ZERO;
    pos->extra_magnitude  = 0.0f;
    pos->pop_out_phase    = 0.0f;
    pos->pop_in_phase     = 0.0f;
    pos->pop_magnitude    = 0.0f;
    pos->pop_translate    = VEC2_ZERO;
    pos->prev_ring_phase  = 0.0f;
}
