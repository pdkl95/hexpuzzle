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

extern tile_t *drag_target;

tile_t *init_tile(tile_t *tile, hex_axial_t pos)
{
    assert_not_null(tile);

    tile->enabled = true;
    tile->position = pos;

    for (int i=0; i<6; i++) {
        tile->path[i] = i;
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

void tile_draw(tile_t *tile, float tile_size)
{
    assert_not_null(tile);

    bool drag = (drag_target == tile);
    Vector2 pos = hex_axial_to_pixel(tile->position, tile_size);

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

    int font_size = 13;

    DrawTextDropShadow("q=", pos.x - 16, pos.y - 22, font_size, ColorAlpha(ORANGE, 0.7), BLACK);

    DrawTextDropShadow("r=", pos.x - 6, pos.y + 8, font_size, ColorAlpha(PINK, 0.7), BLACK);

    font_size = 15;
    DrawTextDropShadow(TextFormat("%d", tile->position.q),
             pos.x, pos.y - 23, font_size, ORANGE, BLACK);

    DrawTextDropShadow(TextFormat("%d", tile->position.r),
             pos.x, pos.y + 7, font_size, PINK, BLACK);
}
