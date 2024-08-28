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
#include "tile.h"

float tile_size = 60.0f;
Vector2 tile_origin;

Color tile_bg_color           = { 0x32, 0x32, 0x32, 0xff };
Color tile_bg_hover_color     = { 0x50, 0x50, 0x50, 0xff };
Color tile_bg_drag_color      = { 0x40, 0x40, 0x40, 0xff };
Color tile_center_color       = { 0x70, 0x70, 0x70, 0xff };
Color tile_center_color_hover = { 0x90, 0x90, 0x90, 0xff };
Color tile_edge_color         = { 0x18, 0x18, 0x18, 0xff };
Color tile_edge_hover_color   = { 0xaa, 0xaa, 0xaa, 0xff };
Color tile_edge_drag_color    = { 0x77, 0x77, 0x77, 0xff };

extern tile_t *drag_target;

tile_t *create_tile(void)
{
    tile_t *tile = calloc(1, sizeof(tile_t));
    return tile;
}

void destroy_tile(tile_t *tile)
{
    SAFEFREE(tile);
}

void tile_draw(tile_t *tile, Vector2 offset)
{
    bool drag = (drag_target == tile);
    Vector2 pos = hex_axial_to_pixel(tile->position, tile_size);
    pos = Vector2Add(pos, tile_origin);
    pos = Vector2Add(pos, offset);

    //Vector2 *corners = hex_pixel_corners(pos, tile_size);
    //DrawTriangleFan(corners, 7, tile->hover ? tile_bg_hover_color : tile_bg_color);
    //DrawLineStrip(corners, 7, RED);

    DrawPoly(pos, 6, tile_size, 0.0f,
             drag
             ? tile_bg_drag_color
             : (tile->hover
                ? tile_bg_hover_color
                : tile_bg_color));

    DrawPolyLinesEx(pos, 6, tile_size, 0.0f, 2.0f,
                    drag
                    ? tile_edge_drag_color
                    : (tile->hover
                       ? tile_edge_hover_color
                       : tile_edge_color));

    DrawCircleV(pos, tile_size / 6.0, tile_center_color);
}
