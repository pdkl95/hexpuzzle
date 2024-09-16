/****************************************************************************
 *                                                                          *
 * tile_draw.c                                                              *
 *                                                                          *
 * This file is part of hexpuzzle.                                              *
 *                                                                          *
 * hexpuzzle is free software: you can redistribute it and/or                   *
 * modify it under the terms of the GNU General Public License as published *
 * by the Free Software Foundation, either version 3 of the License,        *
 * or (at your option) any later version.                                   *
 *                                                                          *
 * hexpuzzle is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General *
 * Public License for more details.                                         *
 *                                                                          *
 * You should have received a copy of the GNU General Public License along  *
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#include "common.h"
#include "tile.h"
#include "tile_pos.h"
#include "tile_draw.h"

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

Color path_type_highlight_color(path_type_t type)
{
    switch (type) {
    default:
        return path_color_none;

    case PATH_TYPE_RED:
        return path_highlight_color_red;

    case PATH_TYPE_BLUE:
        return path_highlight_color_blue;

    case PATH_TYPE_YELLOW:
        return path_highlight_color_yellow;

    case PATH_TYPE_GREEN:
        return path_highlight_color_green;
    }
}

void tile_draw(tile_pos_t *pos, tile_pos_t *drag_target, bool finished, Color finished_color)
{
    assert_not_null(pos);
    /* drag_target CAN be NULL */

    if (!pos->tile->enabled) {
        return;
    }

    bool drag = (pos == drag_target);
    bool dragged_over = (!drag && drag_target && pos->hover);

    if (pos->tile->hidden) {
        if (edit_mode) {
            float hiddensize = pos->size - (pos->size * 0.08);
            DrawPoly(pos->center, 6, hiddensize, 0.0f,
                     dragged_over
                     ? tile_bg_hover_color
                     : tile_bg_hidden_color);
            DrawPolyLinesEx(pos->center, 6, hiddensize, 0.0f, 2.0f,
                            dragged_over
                            ? tile_edge_hover_color
                            : tile_edge_hidden_color);
        }

        return;
    }

    if (!pos->tile->fixed) {
        /* background */
        DrawPoly(pos->center, 6, pos->size, 0.0f,
                 drag
                 ? tile_bg_drag_color
                 : (pos->hover
                    ? tile_bg_hover_color
                    : tile_bg_color));
    }

    if (pos->hover_adjacent) {
        Vector2 mid = pos->midpoints[pos->hover_section];
        tile_section_t sec = pos->sections[pos->hover_section];
        Vector2 c0 = Vector2Lerp(sec.corners[0], mid, 0.35);
        Vector2 c1 = Vector2Lerp(sec.corners[1], mid, 0.35);
        DrawTriangle(c0, c1, sec.corners[2], tile_bg_highlight_color_dim);

    } else if (edit_mode &&
               pos->hover &&
               !pos->hover_center &&
               !drag_target
    ) {
        /* section highlight */
        tile_section_t sec = pos->sections[pos->hover_section];
        DrawTriangle(sec.corners[0], sec.corners[1], sec.corners[2], tile_bg_highlight_color);
    }

    for (hex_direction_t i=0; i<6; i++) {
        /* colored strips */
        Vector2 mid = pos->midpoints[i];

        DrawLineEx(pos->center, mid, pos->line_width, path_type_color(pos->tile->path[i]));

        if (pos->tile->path[i] != PATH_TYPE_NONE) {
            tile_pos_t *neighbor = pos->neighbors[i];
            if (neighbor) {
                hex_direction_t opposite = hex_opposite_direction(i);
                if (neighbor->tile->path[opposite] == pos->tile->path[i]) {
                    Color highlight_color;
                    float line_width = 1.5;
                    if (finished) {
                        highlight_color = ColorAlpha(finished_color, 0.9);
                        line_width = 2.5;
                    } else {
                        highlight_color = path_type_highlight_color(pos->tile->path[i]);
                    }
                    Vector2 path = Vector2Subtract(mid, pos->center);
                    Vector2 perp = Vector2Normalize((Vector2){ path.y, -path.x});
                    Vector2 shift = Vector2Scale(perp, pos->line_width / 2.0);

                    Vector2 s1 = Vector2Add(pos->center, shift);
                    Vector2 e1 = Vector2Add(mid,         shift);
                    shift = Vector2Negate(shift);
                    Vector2 s2 = Vector2Add(pos->center, shift);
                    Vector2 e2 = Vector2Add(mid,         shift);

                    highlight_color = ColorLerp(highlight_color, WHITE, 0.4);
                    DrawLineEx(s1, e1, line_width, highlight_color);
                    DrawLineEx(s2, e2, line_width, highlight_color);
                }
            }
        }

#if 0
        /* section index label */
        Vector2 offset = Vector2Scale(Vector2Subtract(pos->center, mid), 0.2);;
        Vector2 mlabel = Vector2Add(mid, offset);
        DrawTextShadow(TextFormat("%d", i), mlabel.x - 5, mlabel.y - 9, 18, RAYWHITE);
#endif
    }

    if (pos->hover_adjacent) {
        assert(pos->hover_section >= 0);
        assert(pos->hover_section < 6);
        Vector2 mid = pos->midpoints[pos->hover_section];
        float thickness = 1.0;
        DrawLineEx(pos->center, mid, thickness, WHITE);
    }

    if (!pos->tile->fixed) {
        /* border */
        Color border_color = tile_edge_drag_color;
        float line_width = 2.0f;

        if (!drag) {
            if (pos->hover) {
                border_color = tile_edge_hover_color;
            } else {
                border_color = tile_edge_color;
            }
        }

        if (finished) {
            border_color = finished_color;
            line_width = 2.0;
        }

        DrawPolyLinesEx(pos->center, 6, pos->size, 0.0f, line_width, border_color);
    }

    DrawCircleV(pos->center, pos->center_circle_draw_radius, tile_center_color);

    if (edit_mode && pos->hover_center) {
        DrawCircleV(pos->center, pos->center_circle_draw_radius, tile_bg_highlight_color);
    }

#if 0
    if (drag) {
        return;
    }

    /* show each hex's axial coordinates */
    int font_size = 14;
    const char *coord_text = TextFormat("%d,%d", pos->position.q, pos->position.r);
    int text_width = MeasureText(coord_text, font_size);
    DrawTextDropShadow(coord_text, pos->center.x - (text_width/2), pos->center.y + 14, font_size, WHITE, BLACK);
#endif
}

