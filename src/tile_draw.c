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
#include "color.h"

#include "path.h"
#include "tile.h"
#include "tile_pos.h"
#include "tile_draw.h"
#include "level.h"
#include "win_anim.h"

#include "stb/stb_perlin.h"

extern Shader win_border_shader;

static bool either_self_or_adjacent_is_hidden(tile_pos_t *pos)
{
    return pos->tile->hidden || (pos->hover_adjacent && pos->hover_adjacent->tile->hidden);
}

static void draw_adjacency_highlight(tile_pos_t *pos)
{
    if (either_self_or_adjacent_is_hidden(pos)) {
        return;
    }

    Vector2 mid = pos->rel.midpoints[pos->hover_section];
    tile_section_t sec = pos->rel.sections[pos->hover_section];
    Vector2 c0 = Vector2Lerp(sec.corners[0], mid, 0.35);
    Vector2 c1 = Vector2Lerp(sec.corners[1], mid, 0.35);
    DrawTriangle(c0, c1, sec.corners[2], tile_bg_highlight_color_dim);
}

extern bool edit_tool_cycle;
extern bool edit_tool_erase;
extern path_type_t edit_tool_state;

static path_type_t get_next_path(tile_pos_t *pos)
{
    if (edit_tool_cycle) {
        path_type_t next_path = pos->tile->path[pos->hover_section];
        return (1 + next_path) % PATH_TYPE_COUNT;
    } else if (edit_tool_erase) {
        return PATH_TYPE_NONE;
    } else {
        return edit_tool_state;
    }
}

void tile_draw_path(tile_pos_t *pos, bool finished)
{
    each_direction {
        /* colored strips */
        Vector2 mid = pos->rel.midpoints[dir];

        tile_t *tile = pos->swap_target ? pos->swap_target->tile : pos->tile;
        Color pcolor = path_type_color(tile->path[dir]);
        if (!ColorEq(pcolor, path_color_none)) {
            if (finished) {
                pcolor = ColorAlpha(pcolor, 0.666);
            }

            DrawLineEx(pos->rel.center, mid, pos->line_width, pcolor);
#if 0
            float absx= fabs(pos->rel.center.x - mid.x);
            float absy= fabs(pos->rel.center.y - mid.y);
//            if ((absx >= pos->size) || (absy >= pos->size)) {
                printf("DrawLineEx(<%3f,%3f>, <%3f,%3f>, %2f, #%02x%02x%02x%02x) %f\n",
                       pos->rel.center.x, pos->rel.center.y,
                       mid.x, mid.y,
                       pos->line_width,
                       pcolor.r, pcolor.g, pcolor.b, pcolor.a,
                       pos->size);
//            }
#endif
        }

#if 0
        /* section index label */
        Vector2 offset = Vector2Scale(Vector2Subtract(pos->rel.center, mid), 0.2);;
        Vector2 mlabel = Vector2Add(mid, offset);
        DrawTextShadow(TextFormat("%d", dir), mlabel.x - 5, mlabel.y - 9, 18, RAYWHITE);
#endif

#if 0
        /* neighbor hex address label */
        Vector2 offset = Vector2Scale(Vector2Subtract(pos->rel.center, mid), 0.32);;
        Vector2 mlabel = Vector2Add(mid, offset);
        tile_pos_t *n = pos->neighbors[i];
        DrawTextShadow(TextFormat("%d,%d", n->position.q, n->position.r), mlabel.x - 11, mlabel.y - 4, 16, RAYWHITE);
#endif
    }
}

void tile_draw_path_ghost(tile_pos_t *pos)
{
    each_direction {
        /* colored strips */
        Vector2 mid = pos->rel.midpoints[dir];

        Color pcolor = path_type_color(pos->tile->path[dir]);
        if (!ColorEq(pcolor, path_color_none)) {
            pcolor = ColorAlpha(pcolor, 0.666);

            DrawLineEx(pos->rel.center, mid, pos->line_width, pcolor);
#if 0
            float absx= fabs(pos->rel.center.x - mid.x);
            float absy= fabs(pos->rel.center.y - mid.y);
            if ((absx >= pos->size) || (absy >= pos->size)) {
                printf("DrawLineEx(<%3f,%3f>, <%3f,%3f>, %2f, #%02x%02x%02x%02x) %f\n",
                       pos->rel.center.x, pos->rel.center.y,
                       mid.x, mid.y,
                       pos->line_width,
                       pcolor.r, pcolor.g, pcolor.b, pcolor.a,
                       pos->size);
            }
#endif
        }
    }
}

void tile_draw_path_highlight(tile_pos_t *pos, bool finished, Color finished_color)
{
    each_direction {
        /* colored strips */
        Vector2 mid = pos->rel.midpoints[dir];

        if (pos->tile->path[dir] != PATH_TYPE_NONE) {
            tile_pos_t *neighbor = pos->neighbors[dir];
            if (neighbor) {
                hex_direction_t opposite = hex_opposite_direction(dir);
                if (neighbor->tile->path[opposite] == pos->tile->path[dir]) {
                    Color highlight_color;
                    float line_width = 1.5;
                    if (finished) {
                        highlight_color = ColorAlpha(finished_color,- pos->pop_magnitude);
                        line_width = 2.5;
                    } else {
                        highlight_color = path_type_highlight_color(pos->tile->path[dir]);
                    }

                    Vector2 offset_center = Vector2MoveTowards(pos->rel.center, mid, pos->center_circle_draw_radius);
                    Vector2 path = Vector2Subtract(mid, offset_center);
                    Vector2 perp = Vector2Normalize((Vector2){ path.y, -path.x});
                    Vector2 shift = Vector2Scale(perp, pos->line_width / 2.0);

                    Vector2 s1 = Vector2Add(offset_center, shift);
                    Vector2 e1 = Vector2Add(mid,           shift);
                    shift = Vector2Negate(shift);
                    Vector2 s2 = Vector2Add(offset_center, shift);
                    Vector2 e2 = Vector2Add(mid,           shift);

                    highlight_color = ColorLerp(highlight_color, WHITE, 0.4);
                    DrawLineEx(s1, e1, line_width, highlight_color);
                    DrawLineEx(s2, e2, line_width, highlight_color);
                }
            }
        }
    }
}

void tile_draw(tile_pos_t *pos, tile_pos_t *drag_target, bool finished, Color finished_color, float finished_fade_in)
{
    assert_not_null(pos);
    /* drag_target CAN be NULL */

    tile_t *tile = pos->tile;

    if (!tile->enabled) {
        return;
    }

    bool drag = (pos == drag_target) && !edit_mode_solved;
    bool dragged_over = false;

    if (pos->swap_target) {
        if (pos->swap_target == drag_target) {
            dragged_over = true;
        }
    }

    if (tile->hidden) {
        if (edit_mode) {
            float hiddensize = pos->size - (pos->size * 0.08);
            DrawPoly(pos->rel.center, 6, hiddensize, 0.0f,
                     dragged_over
                     ? tile_bg_hover_color
                     : tile_bg_hidden_color);
            DrawPolyLinesEx(pos->rel.center, 6, hiddensize, 0.0f, 2.0f,
                            dragged_over
                            ? tile_edge_hover_color
                            : tile_edge_hidden_color);
        }

        return;
    }

    if (!tile->fixed) {
        /* background */
        Color bgcolor = tile_bg_color;

        if (!dragged_over && (pos->hover && !edit_mode_solved)) {
            bgcolor = tile_bg_hover_color;
        }

        if (drag) {
            bgcolor = tile_bg_drag_color;
        }

#ifdef RANDOM_GEN_DEBUG
        if (tile->start_for_path_type != PATH_TYPE_NONE) {
            bgcolor = ColorLerp(bgcolor, path_type_color(tile->start_for_path_type), 0.12);
        }
#endif

        if (finished) {
            float alpha = (1.0f + sinf(current_time * 0.666)) / 2.0f;
            alpha = (alpha * 0.7) + 0.3;
            Color new_bgcolor = ColorAlpha(bgcolor, alpha);
            bgcolor = ColorLerp(bgcolor, new_bgcolor, finished_fade_in);
        }

#ifdef DEBUG_ID_AND_DIR
        if (pos->tile->id == debug_id) {
            bgcolor = ColorLerp(bgcolor, WHITE, 0.333);
        }
#endif

        DrawPoly(pos->rel.center, 6, pos->size, 0.0f, bgcolor);
    }

    bool edit_solved_not_center = edit_mode_solved && !pos->hover_center;

    if (edit_solved_not_center && !drag && !drag_target) {
        if (pos->hover_adjacent || pos->hover) {
            /* section highlight */
            //tile_section_t sec = pos->sections[pos->hover_section];
            //DrawTriangle(sec.corners[0], sec.corners[1], sec.corners[2], tile_bg_highlight_color);
            draw_adjacency_highlight(pos);
        }
    }

    tile_draw_path(pos, finished);
    if (!finished) {
        tile_draw_path_highlight(pos, finished, finished_color);
    }

    if (edit_solved_not_center && pos->hover_adjacent
        && !either_self_or_adjacent_is_hidden(pos)) {
        assert(pos->hover_section >= 0);
        assert(pos->hover_section < 6);
        Vector2 mid = pos->rel.midpoints[pos->hover_section];
        float thickness = 3.0;
        path_type_t next_path = get_next_path(pos);
        Color next_color = path_type_highlight_color(next_path);
        if (next_path == PATH_TYPE_NONE) {
            next_color = tile_bg_color;
        }
        DrawLineEx(pos->rel.center, mid, thickness, next_color);
    }

    if (!tile->fixed) {
        /* border */
        Color border_color = tile_edge_drag_color;
        float line_width = 2.0f;

        if (!drag) {
            if (pos->hover && !edit_mode_solved) {
                border_color = tile_edge_hover_color;
            } else {
                border_color = tile_edge_color;
            }
        }

        if (edit_mode_solved) {
            border_color = tile_edge_color;
        }

        if (finished) {
            /* skip */
        } else {
            DrawPolyLinesEx(pos->rel.center, 6, pos->size, 0.0f, line_width, border_color);
        }
    }

    if ((tile->path_count > 0) || edit_mode_solved) {
        if (finished) {
            Color cent_color = ColorLerp(tile_center_color, finished_color, 0.5);
            cent_color = ColorBrightness(cent_color, -0.6);
            DrawCircleV(pos->rel.center, pos->center_circle_draw_radius, cent_color);
        } else {
            DrawCircleV(pos->rel.center, pos->center_circle_draw_radius, tile_center_color);

            if (edit_mode_solved && pos->hover_center) {
                DrawCircleV(pos->rel.center, pos->center_circle_draw_radius, tile_bg_highlight_color);
            }
        }
    }

    //DrawLineEx(VEC2_ZERO, Vector2Scale(pos->radial_vector, 0.5), 3.0, LIME);

#if 0
    if (drag) {
        return;
    }

    /* show each hex's axial coordinates */
    int font_size = GuiGetStyle(DEFAULT, TEXT_SIZE);
    const char *coord_text = TextFormat("#%d: %d,%d", pos->tile->id, pos->position.q, pos->position.r);
    Vector2 text_size = MeasureTextEx(DEFAULT_GUI_FONT, coord_text, font_size, 1.0);
    DrawTextDropShadow(coord_text, pos->rel.center.x - (text_size.x/2), pos->rel.center.y + 14, font_size, WHITE, BLACK);
#endif
}

void tile_draw_ghost(tile_pos_t *pos)
{
    DrawPoly(pos->rel.center, 6, pos->size, 0.0f, ColorAlpha(tile_bg_color, 0.4));
    tile_draw_path_ghost(pos);
    DrawCircleV(pos->rel.center, pos->center_circle_draw_radius, ColorAlpha(tile_center_color, 0.666));
    DrawPolyLinesEx(pos->rel.center, 6, pos->size, 0.0f, 2.0, ColorAlpha(tile_edge_drag_color, 0.7));
}

static float tile_draw_hash_wave(tile_pos_t *pos)
{
    return stb_perlin_noise3(pos->win.center.x,
                             pos->win.center.y,
                             current_time,
                             0, 0, 0);
}

static Color get_win_border_color(tile_pos_t *pos, level_t *level)
{
    int offset = 3 - (pos->ring_radius % 3);

    Color color = {0};

    color.r = (255/3) * offset;
    color.g = (255 * pos->ring_radius) / level->radius;
    color.b = (unsigned char)(255.0f * tile_draw_hash_wave(pos));
    color.a = pos->tile->hidden ? 255 : 0;

    return color;
}

void tile_draw_win_anim(tile_pos_t *pos, level_t *level)
{
    if (!pos->tile->enabled) {
        return;
    }

    Color color = get_win_border_color(pos, level);

    tile_draw_path_highlight(pos, true, color);

    float line_width = 2.0;

    DrawPolyLinesEx(pos->rel.center, 6, pos->size, 0.0f, line_width, color);
}

void tile_draw_corner_connections(tile_pos_t *pos, level_t *level)
{
    tile_t *tile = pos->tile;
    if (!tile->enabled || tile->hidden) {
        return;
    }

    each_direction {
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (!neighbor || !neighbor->tile->enabled || neighbor->tile->hidden) {
            continue;
        }

        //int p0_corner_index = (dir + 2) % 6;

        int p1_corner_index = (dir + 1) % 6;
        int p2_corner_index = (dir + 3) % 6;

        //int p3_corner_index = (dir + 2) % 6;

        //Vector2 p0 = pos->win.corners[p0_corner_index];
        Vector2 p0 = pos->win.center;

        Vector2 p1 = pos->win.corners[p1_corner_index];
        Vector2 p2 = neighbor->win.corners[p2_corner_index];

        //Vector2 p3 = neighbor->win.corners[p3_corner_index];
        Vector2 p3 = neighbor->win.center;

        p0 = Vector2Add(p0, pos->extra_translate);
        p1 = Vector2Add(p1, pos->extra_translate);
        p2 = Vector2Add(p2, neighbor->extra_translate);
        p3 = Vector2Add(p3, neighbor->extra_translate);

        Color color = get_win_border_color(pos, level) ;
        color.a = (unsigned char)(255.0f * MAX(pos->extra_magnitude, neighbor->extra_magnitude));

        DrawSplineSegmentCatmullRom(p0, p1, p2, p3, 3.0, color);

#ifdef DEBUG_ID_AND_DIR
        if (debug_dir == (int)dir && debug_id == pos->tile->id) {
            float r = 12.0f;
            DrawCircleV(p0, r, PINK);
            DrawCircleV(p1, r, RED);
            DrawCircleV(p2, r, GREEN);
            DrawCircleV(p3, r, LIME);
        }
#endif
    }
}
