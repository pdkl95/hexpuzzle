/****************************************************************************
 *                                                                          *
 * tile.c                                                                   *
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

Vector2 tile_origin;

Color tile_bg_color           = { 0x32, 0x32, 0x32, 0xff };
Color tile_bg_hover_color     = { 0x50, 0x50, 0x50, 0xff };
Color tile_bg_drag_color      = { 0x40, 0x40, 0x40, 0xff };
Color tile_center_color       = { 0x70, 0x70, 0x70, 0xff };
Color tile_center_color_hover = { 0x90, 0x90, 0x90, 0xff };
Color tile_edge_color         = { 0x18, 0x18, 0x18, 0xff };
Color tile_edge_hover_color   = { 0xaa, 0xaa, 0xaa, 0xff };
Color tile_edge_drag_color    = { 0x77, 0x77, 0x77, 0xff };

Color path_color_none   = { 0, 0, 0, 0 };
Color path_color_red    = RED;
Color path_color_blue   = BLUE;
Color path_color_yellow = YELLOW;
Color path_color_green  = GREEN;

Color path_type_color(path_type_t type)
{
    switch (type) {
    default:
        return path_color_none;

    case PATH_TYPE_RED:
        return path_color_red;

    case PATH_TYPE_BLUE:
        return path_color_blue;

    case PATH_TYPE_YELLOW:
        return path_color_yellow;

    case PATH_TYPE_GREEN:
        return path_color_green;
    }
}

tile_t *init_tile(tile_t *tile, hex_axial_t pos)
{
    assert_not_null(tile);

    tile->enabled = true;
    tile->position = pos;

    for (int i=0; i<6; i++) {
        tile->path[i] = rand() % PATH_TYPE_MAX;
    }

    return tile;
}

tile_t *create_tile(void)
{
    tile_t *tile = calloc(1, sizeof(tile_t));
    hex_axial_t pos = {0};
    return init_tile(tile, pos);
}

void destroy_tile(tile_t *tile)
{
    SAFEFREE(tile);
}

Vector2 *tile_corners(tile_t *tile, float tile_size)
{
    Vector2 pos = hex_axial_to_pixel(tile->position, tile_size);
    return hex_pixel_corners(pos, tile_size);
}

void tile_set_hover(tile_t *tile, Vector2 mouse_pos, float tile_size)
{
    tile->hover = true;
    tile->hover_section = pixel_to_hex_axial_section(mouse_pos, tile_size);
}

void tile_unset_hover(tile_t *tile)
{
    tile->hover = false;
}

void tile_cycle_path_section(tile_t *tile, hex_direction_t section)
{
    tile->path[section]++;
    if (tile->path[section] > PATH_TYPE_MAX) {
        tile->path[section] = PATH_TYPE_NONE;
    }
}

void tile_cycle_hovered_path_section(tile_t *tile)
{
    tile_cycle_path_section(tile, tile->hover_section);
}

void tile_draw(tile_t *tile, float tile_size, bool drag)
{
    assert_not_null(tile);

    Vector2 pos = hex_axial_to_pixel(tile->position, tile_size);

    DrawPoly(pos, 6, tile_size, 0.0f,
             drag
             ? tile_bg_drag_color
             : (tile->hover
                ? tile_bg_hover_color
                : tile_bg_color));

    if (tile->hover) {
        Vector2 *corners = hex_pixel_corners(pos, tile_size);
        Vector2 c1 = corners[tile->hover_section];
        Vector2 c2 = corners[tile->hover_section + 1];
        DrawTriangle(pos, c1, c2, ColorAlpha(YELLOW, 0.3));
    }

    Vector2 *midpoints = hex_axial_pixel_edge_midpoints(tile->position, tile_size);
    for (int i=0; i<6; i++) {
        DrawLineEx(pos, midpoints[i], 5.0, path_type_color(tile->path[i]));
    }

    DrawPolyLinesEx(pos, 6, tile_size, 0.0f, 2.0f,
                    drag
                    ? tile_edge_drag_color
                    : (tile->hover
                       ? tile_edge_hover_color
                       : tile_edge_color));


    DrawCircleV(pos, tile_size / 6.0, tile_center_color);

#if 0
    if (drag) {
        return;
    }

    /* show each hex's axial coordinates */
    int font_size = 14;
    const char *coord_text = TextFormat("%d,%d", tile->position.q, tile->position.r);
    int text_width = MeasureText(coord_text, font_size);
    DrawTextDropShadow(coord_text, pos.x - (text_width/2), pos.y + 14, font_size, WHITE, BLACK);
#endif
}
