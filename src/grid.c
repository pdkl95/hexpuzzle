/****************************************************************************
 *                                                                          *
 * grid.c                                                                   *
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
#include "grid.h"

#define DEBUG_DRAG_AND_DROP 1

static int grid_tile_storage_location(grid_t *grid, hex_axial_t axial)
{
    if ((axial.r < 0) || (axial.r >= grid->tile_grid_height) ||
        (axial.q < 0) || (axial.q >= grid->tile_grid_width)) {
        return -1;
    }
    int loc = (axial.r * grid->tile_grid_width) + axial.q;

    assert(loc >= 0);
    assert(loc < grid->maxtiles);

    return loc;
}

static void grid_add_to_bounding_box(grid_t *grid, tile_t *tile)
{
    Vector2 *corners = tile->corners;
    for (int i=0; i<6; i++) {
        grid->px_min.x = MIN(grid->px_min.x, corners[i].x);
        grid->px_min.y = MIN(grid->px_min.y, corners[i].y);

        grid->px_max.x = MAX(grid->px_max.x, corners[i].x);
        grid->px_max.y = MAX(grid->px_max.y, corners[i].y);
   }
}

void grid_build_tiles(grid_t *grid, int radius)
{
    grid->radius = radius;

    grid->tile_grid_width  = (2 * radius) + 1;
    grid->tile_grid_height = (2 * radius) + 1;
    grid->maxtiles = grid->tile_grid_width * grid->tile_grid_height;

    grid->tiles = calloc(grid->maxtiles, sizeof(tile_t));

    grid->center.q = radius;
    grid->center.r = radius;

    for (int q=0; q<grid->tile_grid_width; q++) {
        for (int r=0; r<grid->tile_grid_height; r++) {
            hex_axial_t pos = {
                .q = q,
                .r = r
            };
            tile_t *tile = grid_get_tile(grid, pos);
            init_tile(tile, pos);

            if (hex_axial_distance(pos, grid->center) <= radius) {
                if (hex_axial_distance(pos, grid->center) > grid->radius) {
                    tile->enabled = false;
                } else {
                    tile->enabled = true;
                }
            } else {
                tile->enabled = false;
            }
        }
    }

    grid->hover       = NULL;
    grid->drag_target = NULL;

    grid_resize(grid);
}

grid_t *create_grid(int radius)
{
    assert(radius > 0);
    assert(radius < 6);

    grid_t *grid = calloc(1, sizeof(grid_t));

    grid->req_tile_size = 60.0f;

    grid->drag_reset_total_frames = 12;
    grid->drag_reset_frames = 0;

    grid->hover = NULL;

    grid->hover_section_adjacency_radius = 12.0;

    grid_build_tiles(grid, radius);

    return grid;
}

void grid_resize(grid_t *grid)
{
    assert_not_null(grid);

    Vector2 window_grid_margin = { 0.8, 0.8 };
    Vector2 window = Vector2Scale(ivector2_to_vector2(window_size), 1.0);
    Vector2 max_grid_size_px = Vector2Multiply(window, window_grid_margin);
    int grid_width_in_hex_radii = 2 + (3 * grid->radius);
    Vector2 max_tile_size = {
        .x =  max_grid_size_px.x / ((float)grid_width_in_hex_radii),
        .y = (max_grid_size_px.y / grid->tile_grid_height) * INV_SQRT_3
    };

    grid->tile_size = MIN(grid->req_tile_size,
                          MIN(max_tile_size.x,
                              max_tile_size.y));

#if 0
    printf(">>>=-- ~ --=<<<\n");
    pvec2(window_grid_margin);
    pvec2(max_grid_size_px);
    pint(grid_width_in_hex_radii);
    pvec2(max_tile_size);
    pfloat(grid->tile_size);
#endif

    grid->px_min.x = (float)window_size.x * 10.0;
    grid->px_min.y = (float)window_size.y * 10.0;

    grid->px_max.x = 0.0f;
    grid->px_max.y = 0.0f;

    for (int i=0; i<grid->maxtiles; i++) {
        tile_t *tile = &grid->tiles[i];
        if (tile->enabled) {
            tile_set_size(tile, grid->tile_size);
            grid_add_to_bounding_box(grid, tile);
        }
    }

    grid->px_bounding_box.x = grid->px_min.x;
    grid->px_bounding_box.y = grid->px_min.y;
    grid->px_bounding_box.width  = grid->px_max.x - grid->px_min.x;
    grid->px_bounding_box.height = grid->px_max.y - grid->px_min.y;

    grid->px_offset.x = ((float)window_size.x - grid->px_bounding_box.width)  / 2;
    grid->px_offset.y = ((float)window_size.y - grid->px_bounding_box.height) / 2;

    grid->px_offset.x -= grid->px_bounding_box.x;
    grid->px_offset.y -= grid->px_bounding_box.y;

#if 0
    printf("-- px --\n");
    printf("window_size = (%d x %d)\n", window_size.x, window_size.y);
    printf("px_min = [ %f, %f ]\n", grid->px_min.x, grid->px_min.y);
    printf("px_max = [ %f, %f ]\n", grid->px_max.x, grid->px_max.y);
    printf("px_bounding_box:\n");
    printrect(grid->px_bounding_box);
    printf("px_offset = [ %f, %f ]\n", grid->px_offset.x, grid->px_offset.y);
#endif
}

void destroy_grid(grid_t *grid)
{
    if (grid) {
        SAFEFREE(grid->tiles);
        SAFEFREE(grid);
    }
}

tile_t *grid_get_tile(grid_t *grid, hex_axial_t axial)
{
    assert_not_null(grid);

    int loc = grid_tile_storage_location(grid, axial);
    if ((loc >= 0) && (loc < grid->maxtiles)) {
        return &grid->tiles[loc];
    } else {
        return NULL;
    }
}

tile_t *grid_find_neighbor_tile(grid_t *grid, tile_t *tile, hex_direction_t section)
{
    section = (section + 1) % 6;
    hex_axial_t neighbor_pos = hex_axial_neighbors(tile->position, section);
    tile_t *neighbor = grid_get_tile(grid, neighbor_pos);
    return neighbor;
}

void grid_check(grid_t *grid)
{
    assert_not_null(grid);
}

void grid_set_hover(grid_t *grid, IVector2 mouse_position)
{
    if (grid) {
        if (grid->hover) {
            if (grid->hover_adjacent) {
                tile_unset_hover_adjacent(grid->hover);
                tile_unset_hover_adjacent(grid->hover_adjacent);
                grid->hover_adjacent = NULL;
            }

            tile_unset_hover(grid->hover);
            grid->hover->hover = false;
        }

        grid->mouse_pos.x = (float)mouse_position.x;
        grid->mouse_pos.y = (float)mouse_position.y;

        if (grid->drag_target) {
            if (grid->drag_reset_frames > 0) {
                float reset_fract = ((float)grid->drag_reset_frames) / ((float)grid->drag_reset_total_frames);
                reset_fract = ease_exponential_in(reset_fract);
                grid->drag_offset = Vector2Scale(grid->drag_reset_vector, reset_fract);
                grid->drag_reset_frames--;
                if (0 == grid->drag_reset_frames) {
                    grid->drag_target = NULL;
                    disable_automatic_events();
                }
            } else {
                grid->drag_offset = Vector2Subtract(grid->mouse_pos, grid->drag_start);
            }
        } else {
            grid->drag_offset.x = 0.0f;
            grid->drag_offset.y = 0.0f;
        }

        Vector2 mouse_tile_pos = Vector2Subtract(grid->mouse_pos, grid->px_offset);
        hex_axial_t mouse_hex = pixel_to_hex_axial(mouse_tile_pos, grid->tile_size);

        grid->hover = grid_get_tile(grid, mouse_hex);

        if (grid->hover) {
            tile_t *tile = grid->hover;
            Vector2 midpoint = tile->midpoints[tile->hover_section];
            midpoint = Vector2Add(midpoint, grid->px_offset);
            if (Vector2Distance(midpoint, grid->mouse_pos) < grid->hover_section_adjacency_radius) {
                grid->hover_adjacent = grid_find_neighbor_tile(grid, tile, tile->hover_section);
                if (grid->hover_adjacent) {
                    grid->hover_section = tile->hover_section;
                    grid->hover_adjacent_section = hex_opposite_direction(grid->hover_section);

                    tile_set_hover_adjacent(grid->hover,          grid->hover_section,          grid->hover_adjacent);
                    tile_set_hover_adjacent(grid->hover_adjacent, grid->hover_adjacent_section, grid->hover);
                }
            }

            tile_set_hover(grid->hover, Vector2Subtract(grid->mouse_pos, grid->px_offset));
        }
    }
}

void grid_drag_start(grid_t *grid)
{
    assert_not_null(grid);

    if (grid->drag_target) {
        grid->drag_target = NULL;
    }

    if (grid->hover) {
#ifdef DEBUG_DRAG_AND_DROP
        printf("drag_stop(): hover = %p\n", grid->hover);
#endif

        if (!grid->hover->fixed) {
            grid->drag_target = grid->hover;
            grid->drag_start  = grid->mouse_pos;
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_start(): drag_target = %p\n", grid->drag_target);
#endif
        }
    }
}

void grid_drop_tile(grid_t *grid, tile_t *drag_target, tile_t *drop_target)
{
    assert_not_null(grid);
    assert_not_null(drag_target);
    assert_not_null(drop_target);

    assert(drag_target->enabled);
    assert(drop_target->enabled);

    if (!drop_target->fixed) {
        tile_swap_attributes(drag_target, drop_target);
        grid_check(grid);
    }
}

void grid_drag_stop(grid_t *grid)
{
    assert_not_null(grid);

    if (grid->drag_target) {
        tile_t *drop_target = grid->hover;

        if (drop_target && !drop_target->fixed) {
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_stop(): drop target\n");
#endif
            grid_drop_tile(grid, grid->drag_target, drop_target);
            grid->drag_target = NULL;
        } else {
#ifdef DEBUG_DRAG_AND_DROP
            printf("drag_stop(): reset\n");
#endif
            grid->drag_reset_frames = grid->drag_reset_total_frames;;
            grid->drag_reset_vector = grid->drag_offset;
            enable_automatic_events();
        }
    } else {
#ifdef DEBUG_DRAG_AND_DROP
        printf("drag_stop(): missing drag target\n");
#endif
    }
}

void grid_modify_hovered_feature(grid_t *grid)
{
    assert_not_null(grid);
    if (grid->hover) {
        tile_modify_hovered_feature(grid->hover);
    }
}

void grid_draw(grid_t *grid)
{
    assert_not_null(grid);

    rlPushMatrix();

    rlTranslatef(grid->px_offset.x,
                 grid->px_offset.y,
                 0.0);

    for (int i=0; i<grid->maxtiles; i++) {
        tile_t *tile = &grid->tiles[i];
        assert_not_null(tile);
        if (tile->enabled) {
            if (tile == grid->drag_target) {
                // defer ubtil after bg tiles are drawn
            } else {
                tile_draw(tile, grid->drag_target);
            }
        }
    }

    if (grid->drag_target) {
        rlPushMatrix();

        rlTranslatef(grid->drag_offset.x,
                     grid->drag_offset.y,
                     0.0);

        tile_draw(grid->drag_target, grid->drag_target);

        rlPopMatrix();
    }

    //DrawRectangleLinesEx(grid->px_bounding_box, 5.0, LIME);

    rlPopMatrix();

#if 0
    if (grid->drag_target) {
        DrawText(TextFormat("drag_target<%d,%d> drag_offset = (%f, %f)",
                            grid->drag_target->position.q, grid->drag_target->position.r,
                            grid->drag_offset.x, grid->drag_offset.y),
                 10, 10, 20, GREEN);
    }
#endif
}

void grid_serialize(grid_t *grid, FILE *f)
{
    fprintf(f, "hexlevel version 1\n");
    fprintf(f, "name \"%s\"\n", grid->name);
    fprintf(f, "radius %d\n", grid->radius);
    fprintf(f, "begin_tiles %d\n", grid->maxtiles);
    for (int i=0; i<grid->maxtiles; i++) {
        tile_serialize(&grid->tiles[i], f);
    }
    fprintf(f, "end_tiles\n");
}

void grid_change_radius(grid_t *grid, int new_radius)
{
    printf("TODI: change grid radius from %d to %d\n", grid->radius, new_radius);

    int old_maxtiles  = grid->maxtiles;
    tile_t *old_tiles = grid->tiles;
    grid->tiles = NULL;

    grid_build_tiles(grid, new_radius);

    for (int i=0; i<old_maxtiles; i++) {
        tile_t *old_tile = &(old_tiles[i]);
        tile_t *new_tile = grid_get_tile(grid, old_tile->position);
        if (new_tile) {
            tile_copy_attributes_except_enabled(new_tile, old_tile);
        }
    }

    free(old_tiles);
}
