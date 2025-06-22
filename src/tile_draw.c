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
#ifdef USE_PHYSICS
#include "physics.h"
#endif

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
        tile_pos_t *n = pos->neighbors[dir];
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

    float fade = 1.0;
    if (current_level && current_level->win_anim) {
        fade = current_level->win_anim->fade[2];
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
            float cent_color_fade = current_level ? fade : 0.5;
            float darken = -0.75 * fade;
            Color cent_color = ColorLerp(tile_center_color, finished_color, cent_color_fade);
            cent_color = ColorBrightness(cent_color, darken);
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
    rlPushMatrix();
    rlRotatef(TO_DEGREES(-pos->extra_rotate), 0.0, 0.0, 1.0);

#if 0
#ifdef USE_PHYSICS
    DrawLineEx(VEC2_ZERO, Vector2Scale(pos->physics_velocity, 0.2), 3.0, PINK);

    if (pos->tile->physics_tile) {
        float energy = cpBodyKineticEnergy(pos->tile->physics_tile->body);
        energy *= 0.0001;
        DrawCircleGradient(0.0f, 0.0f, energy, BLANK, YELLOW);
    }
#endif
#endif

#if 0
    /* show each hex's axial coordinates */
    int font_size = GuiGetStyle(DEFAULT, TEXT_SIZE);

    const char *coord_text1 = TextFormat("#%d: %d,%d", pos->tile->id, pos->position.q, pos->position.r);
    Vector2 text_size1 = measure_gui_text(coord_text1);
#if 0
#ifdef USE_PHYSICS
    Vector2 pp = pos->physics_position;
    pp = Vector2Subtract(pp, window_center);
    if (current_level) {
        //pp = Vector2Subtract(pp, current_level->px_offset);
        pp = Vector2Add(pp, current_level->px_offset);
    }
    const char *coord_text2 = TextFormat("phy: %3.2f,%3.2f", pp.x, pp.y);
    Vector2 text_size2 = measure_gui_text(coord_text2);
    const char *coord_text3 = TextFormat("a = %3.2f", atan2f(pp.y, pp.x) );
    Vector2 text_size3 = measure_gui_text(coord_text3);
#endif
#endif

    float yoffset = 14;
    DrawTextDropShadow(coord_text1, pos->rel.center.x - (text_size1.x/2), pos->rel.center.y + yoffset, font_size, WHITE, BLACK);
#if 0
#ifdef USE_PHYSICS
    float sep = 1;
    DrawTextDropShadow(coord_text2, pos->rel.center.x - (text_size2.x/2), pos->rel.center.y + yoffset + text_size1.y + sep, font_size, WHITE, BLACK);
    DrawTextDropShadow(coord_text3, pos->rel.center.x - (text_size3.x/2), pos->rel.center.y + yoffset - text_size1.y - sep, font_size, WHITE, BLACK);
    //Vector2 lineend = Vector2Add(pos->rel.center, (Vector2) { .x = -pp.x, -pp.y });
    //DrawLineEx(pos->rel.center, lineend, 3.0, PINK);
#endif
#endif
#endif

    rlPopMatrix();
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

static Color get_win_border_color(tile_pos_t *pos)
{
    int offset = 3 - (pos->ring_radius % 3);

    Color color = {0};

    color.r = (255/3) * offset;
    color.g = 0;
    color.b = (unsigned char)(255.0f * tile_draw_hash_wave(pos));
    color.a = pos->tile->hidden ? 255 : 0;

    return color;
}

void tile_draw_win_anim(tile_pos_t *pos)
{
    if (!pos->tile->enabled) {
        return;
    }

    Color color = get_win_border_color(pos);

    tile_draw_path_highlight(pos, true, color);

    float line_width = 2.0;

    DrawPolyLinesEx(pos->rel.center, 6, pos->size, 0.0f, line_width, color);
}

extern float bloom_amount;
#define MIN_CORNER_DIST_SQR 150.0f
void tile_draw_corner_connections(tile_pos_t *pos, win_anim_mode_t win_mode)
{
    tile_t *tile = pos->tile;
    if (!tile->enabled || tile->hidden) {
        return;
    }

    float fade = 1.0;
    if (current_level) {
        if (current_level->win_anim) {
            fade = current_level->win_anim->fade[2] * current_level->fade.value_eased_in;
        }
    }

#ifdef USE_PHYSICS
    physics_tile_t *pt = pos->tile->physics_tile;
#if 0
    if (!pt) {
        return;
    }
#endif
#endif
    Color color = get_win_border_color(pos);

    each_direction {
        hex_direction_t opposite_dir = hex_opposite_direction(dir);
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (!neighbor || !neighbor->tile->enabled || neighbor->tile->hidden) {
            continue;
        }

        Vector2 save_pos_extra_translate = pos->extra_translate;
        Vector2 save_neighbor_extra_translate = neighbor->extra_translate;
        pos->extra_translate = Vector2Scale(pos->extra_translate, fade);
        neighbor->extra_translate = Vector2Scale(neighbor->extra_translate, fade);

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

        //bool is_pop = false;
        bool is_pop = win_mode == WIN_ANIM_MODE_POPS;

        if (is_pop) {
            float cdist_sqr = Vector2DistanceSqr(p1, p2);
            if (cdist_sqr > MIN_CORNER_DIST_SQR) {
                //color.a = (unsigned char)(255.0f * MAX(pos->extra_magnitude, neighbor->extra_magnitude));
                //color.b = 0.0;

                float thickness = 0.75;
                thickness += pos->extra_magnitude * 0.05;
                DrawSplineSegmentCatmullRom(p0, p1, p2, p3, thickness, color);
            }
        }

        if (dir > 2) {
            // only one side of each path
            goto pos_neighbor_cleanup;
        }

        if (tile->path[dir] != PATH_TYPE_NONE) {
            Vector2 pos_m_p = pos->win.midpoints[dir]; 
            pos_m_p = Vector2RotateAroundPoint(pos_m_p,  pos->extra_rotate, pos->win.center);
            pos_m_p = Vector2Add(pos_m_p, pos->extra_translate);

            Vector2 nbr_m_p = neighbor->win.midpoints[opposite_dir];
            nbr_m_p = Vector2RotateAroundPoint(nbr_m_p,  neighbor->extra_rotate, neighbor->win.center);
            nbr_m_p = Vector2Add(nbr_m_p, neighbor->extra_translate);

            float outside_dist = Vector2Distance(pos_m_p, nbr_m_p) * bloom_amount;  //0.7;
#ifdef USE_PHYSICS
            if (pt && !pt->path_has_spring[dir]) {
                outside_dist *= 0.3;
            }
#endif
            //float outside_scale = pos->extra_magnitude * 0.05;
            Vector2 outside          = Vector2Scale(     pos->win.radial_unit[dir], outside_dist);
            Vector2 neighbor_outside = Vector2Scale(neighbor->win.radial_unit[opposite_dir], outside_dist);

            Vector2 pos_m_c = Vector2Add(pos->win.midpoints[dir], outside);
            pos_m_c = Vector2RotateAroundPoint(pos_m_c,  pos->extra_rotate, pos->win.center);
            pos_m_c = Vector2Add(pos_m_c, pos->extra_translate);
            Vector2 nbr_m_c = Vector2Add(neighbor->win.midpoints[opposite_dir], neighbor_outside);
            nbr_m_c = Vector2RotateAroundPoint(nbr_m_c,  neighbor->extra_rotate, neighbor->win.center);
            nbr_m_c = Vector2Add(nbr_m_c, neighbor->extra_translate);

#ifdef DEBUG_ID_AND_DIR
            if (debug_dir == (int)dir && debug_id == pos->tile->id) {
                float r = 12.0f;

                DrawCircleV(pos_cw1_p,  r, RED);
                DrawCircleV(pos_cw1_c,  r, PINK);
                DrawCircleV(nbr_ccw1_c, r, LIME);
                DrawCircleV(nbr_ccw1_p, r, GREEN);
            }
#endif

            color.g = options->path_color[tile->path[dir]].hue;
            float thickness = pos->line_width;

            thickness = Lerp(0.333 * thickness,
                             thickness,
                             smoothstep(0.5, 1.0, 1.0f / outside_dist));

            color.a = 0.0;
            if (is_pop) {
                color.a = fade;
                thickness = pos->line_width * 0.75;
            }

#ifdef USE_PHYSICS
            if (pt && !pt->path_has_spring[dir]) {
#endif
                thickness *= 0.3334 * fade;
                color.r = fade;
                color.b = 0.0;
                color.a = 0.0;
#ifdef USE_PHYSICS
            }
#endif

#if 0
            Vector2 pos_cw1_p;
            Vector2 pos_cw1_c;
            Vector2 nbr_ccw1_c;
            Vector2 nbr_ccw1_p;
            Vector2 pos_ccw2_p;
            Vector2 pos_ccw2_c;
            Vector2 nbr_cw2_c;
            Vector2 nbr_cw2_p;

            float edge_thickness = 25.0;
#endif
            float rand_thickness = 2.0;

            switch (win_mode) {
            case WIN_ANIM_MODE_POPS:
                fallthrough;
            case WIN_ANIM_MODE_WAVES:
#if 0
                pos_cw1_p  = pos->win.midpoint_path_cw[dir];
                pos_cw1_c  = Vector2Add(pos->win.midpoint_path_cw[dir], outside);
                nbr_ccw1_c = Vector2Add(neighbor->win.midpoint_path_ccw[opposite_dir], neighbor_outside);
                nbr_ccw1_p = neighbor->win.midpoint_path_ccw[opposite_dir];
                pos_cw1_p  = Vector2RotateAroundPoint(pos_cw1_p,  pos->extra_rotate, pos->win.center);
                pos_cw1_p  = Vector2Add(pos_cw1_p,  pos->extra_translate);
                pos_cw1_c  = Vector2RotateAroundPoint(pos_cw1_c,  pos->extra_rotate, pos->win.center);
                pos_cw1_c  = Vector2Add(pos_cw1_c,  pos->extra_translate);
                nbr_ccw1_c = Vector2RotateAroundPoint(nbr_ccw1_c,  neighbor->extra_rotate, neighbor->win.center);
                nbr_ccw1_c = Vector2Add(nbr_ccw1_c, neighbor->extra_translate);
                nbr_ccw1_p = Vector2RotateAroundPoint(nbr_ccw1_p,  neighbor->extra_rotate, neighbor->win.center);
                nbr_ccw1_p = Vector2Add(nbr_ccw1_p, neighbor->extra_translate);

                pos_ccw2_p = pos->win.midpoint_path_ccw[dir];
                pos_ccw2_c = Vector2Add(pos->win.midpoint_path_ccw[dir], outside);
                nbr_cw2_c  = Vector2Add(neighbor->win.midpoint_path_cw[opposite_dir], neighbor_outside);
                nbr_cw2_p  = neighbor->win.midpoint_path_cw[opposite_dir];
                pos_ccw2_p = Vector2RotateAroundPoint(pos_ccw2_p,  pos->extra_rotate, pos->win.center);
                pos_ccw2_p = Vector2Add(pos_ccw2_p,  pos->extra_translate);
                pos_ccw2_c = Vector2RotateAroundPoint(pos_ccw2_c,  pos->extra_rotate, pos->win.center);
                pos_ccw2_c = Vector2Add(pos_ccw2_c,  pos->extra_translate);
                nbr_cw2_c  = Vector2RotateAroundPoint(nbr_cw2_c,  neighbor->extra_rotate, neighbor->win.center);
                nbr_cw2_c  = Vector2Add(nbr_cw2_c, neighbor->extra_translate);
                nbr_cw2_p  = Vector2RotateAroundPoint(nbr_cw2_p,  neighbor->extra_rotate, neighbor->win.center);
                nbr_cw2_p  = Vector2Add(nbr_cw2_p, neighbor->extra_translate);
#endif

#if 0
                DrawSplineSegmentBezierCubic(
                    pos_cw1_p,
                    pos_cw1_c,
                    nbr_ccw1_c,
                    nbr_ccw1_p,
                    edge_thickness,
                    color);

                DrawSplineSegmentBezierCubic(
                    pos_ccw2_p,
                    pos_ccw2_c,
                    nbr_cw2_c,
                    nbr_cw2_p,
                    edge_thickness,
                    color);
#else
                rand_thickness = 2.0 + (drand48() * 4.0);
                DrawSplineSegmentBezierCubic(
                    pos_m_p,
                    pos_m_c,
                    nbr_m_c,
                    nbr_m_p,
                    rand_thickness,
                    color);

#if 0
                color.r = rand();
                color.b = 0;
                color.a = 255;
                DrawLineEx(pos_cw1_c, nbr_ccw1_c, edge_thickness, color);
                DrawLineEx(pos_ccw2_c, nbr_cw2_c, edge_thickness, color);
#endif
#endif
                break;

#ifdef USE_PHYSICS
            case WIN_ANIM_MODE_PHYSICS_FALL:
                fallthrough;
            case WIN_ANIM_MODE_PHYSICS_SWIRL:
                thickness *= 1.25;
                fallthrough;
#endif
            default:
                DrawSplineSegmentBezierCubic(
                    pos_m_p,
                    pos_m_c,
                    nbr_m_c,
                    nbr_m_p,
                    thickness,
                    color);
                break;
            }

        }

      pos_neighbor_cleanup:
        pos->extra_translate = save_pos_extra_translate;
        neighbor->extra_translate = save_neighbor_extra_translate;
    }
}
