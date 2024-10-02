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
    pos->hover_adjacent = NULL;
    pos->hover = false;
    pos->hover_center = false;

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

bool tile_pos_check(tile_pos_t *pos)
{
    for (hex_direction_t i=0; i<6; i++) {
        if (pos->tile->path[i] != PATH_TYPE_NONE) {
            hex_direction_t opp_i = hex_opposite_direction(i);
            tile_pos_t *neighbor = pos->neighbors[i];
            if (neighbor) {
                if (pos->tile->path[i] != neighbor->tile->path[opp_i]) {
                    return false;
                }
            }
        }
    }

    return true;
}

void tile_pos_set_hover(tile_pos_t *pos, Vector2 mouse_pos)
{
    assert_not_null(pos);

    Vector2 relvec = Vector2Subtract(mouse_pos, pos->center);
    float theta = atan2f(-relvec.y, -relvec.x);
    theta += TAU/2.0;
    theta = TAU - theta;
    pos->hover = true;
    pos->hover_section = (int)(theta/TO_RADIANS(60.0));
    //printf("tile_pos->hover_section = %d (theta = %f)\n", pos->hover_section, theta);
    pos->hover_center =
        (Vector2DistanceSqr(mouse_pos, pos->center)
         < (pos->center_circle_hover_radius *
            pos->center_circle_hover_radius));
}

void tile_pos_unset_hover(tile_pos_t *pos)
{
    assert_not_null(pos);

    pos->hover = false;
    pos->hover_center = false;

    pos->hover_adjacent = NULL;
}

void tile_pos_toggle_fixed(tile_pos_t *pos)
{
    assert_not_null(pos);

    pos->tile->fixed = !pos->tile->fixed;
}

void tile_pos_toggle_hidden(tile_pos_t *pos)
{
    assert_not_null(pos);

    pos->tile->hidden = !pos->tile->hidden;

    for (hex_direction_t dir=0; dir<6; dir++) {
        hex_direction_t opp_dir =
            hex_opposite_direction(dir);
        tile_pos_t *neighbor = pos->neighbors[dir];

        if (pos->tile->hidden) {
            pos->tile->saved_path[dir] =  pos->tile->path[dir];
            pos->tile->path[dir] = PATH_TYPE_NONE;;
            neighbor->tile->path[opp_dir] = PATH_TYPE_NONE;
        } else {
            pos->tile->path[dir] =  pos->tile->saved_path[dir];
            pos->tile->saved_path[dir] = PATH_TYPE_NONE;;
            neighbor->tile->path[opp_dir] = pos->tile->path[dir];;
        }
    }
}

void tile_pos_set_path_section(tile_pos_t *pos, hex_direction_t section, path_type_t type)
{
    assert_not_null(pos);

    pos->tile->path[section] = type % PATH_TYPE_COUNT;

    if (pos->hover_adjacent) {
        hex_direction_t opposite_section =
            hex_opposite_direction(section);
        pos->hover_adjacent->tile->path[opposite_section] = pos->tile->path[section];
    }
}

void tile_pos_cycle_path_section(tile_pos_t *pos, hex_direction_t section)
{
    assert_not_null(pos);

    tile_pos_set_path_section(pos, section, pos->tile->path[section] + 1);
}

static void tile_pos_modify_center(tile_pos_t *pos)
{
    assert_not_null(pos);

    if (is_any_shift_down()) {
        tile_pos_toggle_hidden(pos);
    } else {
        tile_pos_toggle_fixed(pos);
    }
}

void tile_pos_modify_hovered_feature(tile_pos_t *pos)
{
    assert_not_null(pos);

    if (pos->tile->hidden) {
        tile_pos_toggle_hidden(pos);
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
        tile_pos_toggle_hidden(pos);
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
    for (hex_direction_t i=0; i<6; i++) {
        pos->tile->path[i] = PATH_TYPE_NONE;
        tile_pos_t *neighbor = level_find_current_neighbor_tile_pos(level, pos, i);
        if (neighbor) {
            hex_direction_t opp_i = hex_opposite_direction(i);
            neighbor->tile->path[opp_i] = PATH_TYPE_NONE;
        }
    }
}

void tile_pos_rebuild(tile_pos_t *pos)
{
    assert_not_null(pos);

    pos->line_width = pos->size / 6.0;
    pos->center_circle_draw_radius = pos->line_width * 1.2;
    pos->center_circle_hover_radius = pos->line_width * 1.6;
    pos->center = hex_axial_to_pixel(pos->position, pos->size);

    Vector2 *corners = hex_pixel_corners(pos->center, pos->size);
    memcpy(pos->corners, corners, 7 * sizeof(Vector2));

    for (int i=0; i<6; i++) {
        Vector2 c0 = pos->corners[i];
        Vector2 c1 = pos->corners[i + 1];

        pos->midpoints[i] = Vector2Lerp(c0, c1, 0.5);
    }

    Vector2 cent = Vector2Lerp(pos->midpoints[0], pos->midpoints[3], 0.5);

    for (int i=0; i<6; i++) {
        Vector2 c0 = pos->corners[i];
        Vector2 c1 = pos->corners[i + 1];

        pos->sections[i].corners[0] = c0;
        pos->sections[i].corners[1] = c1;
        pos->sections[i].corners[2] = cent;
    }
}

void tile_pos_set_size(tile_pos_t *pos, float tile_pos_size)
{
    assert_not_null(pos);

    pos->size = tile_pos_size;

    tile_pos_rebuild(pos);
}

