/****************************************************************************
 *                                                                          *
 * color.c                                                                  *
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
#include "color.h"

Color TRANSPARENT_BLACK  = { 0, 0, 0, 0 };
Color LIGHT_ORANGE       = { 255, 249, 187, 255 };
Color CHARTREUSE         = { 194, 241, 146, 255 };
Color DEEP_SKY_BLUE      = { 176, 224, 230, 255 };
Color DODGER_BLUE        = { 0x1E, 0x90, 0xFF, 255 };
Color DEEP_PINK          = { 0xFF, 0x14, 0x93, 255 };
Color CGOLD              = { 0xFF, 0xD7, 0x00, 255 };
Color cursor_outer_color;
Color cursor_inner_color;

Color magenta    = { 214,   2, 112, 255 }; // d60270
Color purple     = { 155,  79, 150, 255 }; // 9b4f96
Color royal_blue = {   0,  56, 168, 255 }; // 0038a8


Color tile_bg_color            = { 0x32, 0x32, 0x32, 0xff };
Color tile_bg_hover_color      = { 0x40, 0x40, 0x40, 0xff };
Color tile_bg_drag_color       = { 0x3b, 0x3b, 0x3b, 0xff };
Color tile_bg_hidden_color     = { 0x32, 0x32, 0x32, 0x33 };
Color tile_center_color        = { 0x70, 0x70, 0x70, 0xff };
Color tile_center_color_hover  = { 0x90, 0x90, 0x90, 0xff };
Color tile_edge_color          = { 0x18, 0x18, 0x18, 0xff };
Color tile_edge_hover_color    = { 0xaa, 0xaa, 0xaa, 0xff };
Color tile_edge_drag_color     = { 0x77, 0x77, 0x77, 0xff };
Color tile_edge_hidden_color   = { 0xe9, 0xdf, 0x9c, 0x44 };
Color tile_edge_finished_color = { 0x9b, 0xff, 0x9b, 0x80 };

Color tile_bg_highlight_color     = { 0xfd, 0xf9, 0x00, 0x4c };
Color tile_bg_highlight_color_dim = { 0xfd, 0xf9, 0x00, 0x2c };

Color path_color_none   = { 0, 0, 0, 0 };
Color path_color_red    = RED;
Color path_color_blue   = BLUE;
Color path_color_yellow = YELLOW;
Color path_color_green  = GREEN;
Color path_highlight_color_red    = { 255,  65,  81, 255 };
Color path_highlight_color_blue   = { 70,  166, 255, 255 };
Color path_highlight_color_yellow = { 255, 253, 127, 255 };
Color path_highlight_color_green  = {  67, 255, 105, 255 };

Color panel_bg_color   = { 0x72, 0x1C, 0xB8, 0xaa };
Color panel_edge_color = { 0x94, 0x83, 0xA2, 0xcc };
Color panel_header_text_color = { 0xD0, 0xC0, 0xFF, 0xff };

void prepare_global_colors()
{
    //cursor_outer_color = DODGER_BLUE;
    cursor_outer_color = CGOLD;
    cursor_outer_color = ColorAlpha(cursor_outer_color, 0.86);

    cursor_inner_color = DEEP_PINK;
    cursor_inner_color = ColorAlpha(cursor_inner_color, 0.65);

    float dim_factor = -0.25;
    path_color_red    = ColorBrightness(path_color_red,    dim_factor);
    path_color_blue   = ColorBrightness(path_color_blue,   dim_factor);
    path_color_yellow = ColorBrightness(path_color_yellow, dim_factor);
    path_color_green  = ColorBrightness(path_color_green,  dim_factor);

}

