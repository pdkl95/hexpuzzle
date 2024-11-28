/****************************************************************************
 *                                                                          *
 * level_draw.c                                                             *
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
 * with rocks. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#include "common.h"

#include "tile.h"
#include "tile_pos.h"
#include "tile_draw.h"
#include "level.h"
#include "level_draw.h"
#include "shader.h"
#include "win_anim.h"


static void level_set_fade_transition(level_t *level, tile_pos_t *pos)
{
    tile_pos_t *center_pos = level_get_center_tile_pos(level);
    if (!level->fade_active && center_pos == pos) {
        return;
    }

    Vector2 modded = Vector2Scale(pos->radial_vector, 5.0);
    Vector2 faded  = Vector2Lerp(modded, pos->radial_vector, level->fade_value_eased);

    Vector2 translate = Vector2Subtract(faded, pos->radial_vector);

    rlTranslatef(translate.x,
                 translate.y,
                 0.0);
}

static void level_set_transition(level_t *level, tile_pos_t *pos, bool do_fade, float fade_ammount)
{
    Vector2 extra_tvec = Vector2Scale(pos->extra_translate, fade_ammount);
    Vector2 tvec = Vector2Add(pos->win.center, Vector2Add(extra_tvec, pos->pop_translate));


    rlTranslatef(tvec.x,
                 tvec.y,
                 0.0);

    if (do_fade) {
        level_set_fade_transition(level, pos);
    }

    rlRotatef(TO_DEGREES(pos->extra_rotate), 0.0, 0.0, 1.0);
}

static void level_draw_corner_connections(level_t *level)
{
    BeginShaderMode(win_border_shader);
    {
        for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
            for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
                hex_axial_t addr = {
                    .q = q,
                    .r = r
                };
                tile_pos_t *pos = level_get_current_tile_pos(level, addr);

                rlPushMatrix();

                //level_set_transition(level, pos, do_fade, finished_fade_in);
                tile_draw_corner_connections(pos, level);

                rlPopMatrix();
            }
        }
    }
    EndShaderMode();
}

void level_draw(level_t *level, bool finished)
{
    assert_not_null(level);

    bool do_fade = level_update_fade(level);

    level->finished_hue += FINISHED_HUE_STEP;
    while (level->finished_hue > 360.0f) {
        level->finished_hue -= 360.0f;
    }
    win_anim_update(level->win_anim);

    Color finished_color = ColorFromHSV(level->finished_hue, 0.7, 1.0);
    float finished_fade_in = level->win_anim->fade[2];

    rlPushMatrix();

    if (do_fade) {
        float blend_amount = level->fade_value;// * (1.0f - finished_fade_in);
        glBlendColor(0.0f, 0.0f, 0.0f, blend_amount);
        rlSetBlendMode(RL_BLEND_CUSTOM);
    }

    float level_rotate = level->fade_rotate_level + level->extra_rotate_level;
    rlTranslatef(window_center.x, window_center.y, 0.0);
    rlRotatef(TO_DEGREES(level_rotate), 0.0, 0.0, 1.0);
    rlTranslatef(-window_center.x, -window_center.y, 0.0);

    rlTranslatef(level->px_offset.x, level->px_offset.y, 0.0);

    for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
        for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
            hex_axial_t addr = {
                .q = q,
                .r = r
            };
            tile_pos_t *pos = level_get_current_tile_pos(level, addr);
            assert_not_null(pos);

            if (pos->tile->enabled) {
                if (level->drag_target && pos->tile == level->drag_target->tile) {
                    if (pos->swap_target) {
                        // preview the swap
                        rlPushMatrix();
                        rlTranslatef(pos->win.center.x,
                                     pos->win.center.y,
                                     0.0);

                        tile_draw(pos, level->drag_target, finished, finished_color, finished_fade_in);

                        rlPopMatrix();
                    } else {
                        // defer until after bg tiles are drawn
                    }
                } else {
                    rlPushMatrix();

                    level_set_transition(level, pos, do_fade, finished_fade_in);
                    tile_draw(pos, level->drag_target, finished, finished_color, finished_fade_in);

                    rlPopMatrix();
                }
            }
        }
    }

    if (finished) {
        BeginShaderMode(win_border_shader);
        {
            for (int q=0; q<TILE_LEVEL_WIDTH; q++) {
                for (int r=0; r<TILE_LEVEL_HEIGHT; r++) {
                    hex_axial_t addr = {
                        .q = q,
                        .r = r
                    };
                    tile_pos_t *pos = level_get_current_tile_pos(level, addr);

                    rlPushMatrix();

                    level_set_transition(level, pos, do_fade, finished_fade_in);
                    tile_draw_win_anim(pos, level);

                    rlPopMatrix();
                }
            }
        }
        EndShaderMode();
    }

    //win_anim_draw(level->win_anim);

    if (level->drag_target) {
        rlPushMatrix();

        rlTranslatef(level->drag_offset.x,
                     level->drag_offset.y,
                     0.0);

        rlTranslatef(level->drag_target->win.center.x,
                     level->drag_target->win.center.y,
                     0.0);

        tile_draw_ghost(level->drag_target);

        /* if (finished) { */
        /*     BeginShaderMode(win_border_shader); */
        /*     { */
        /*         tile_draw_win_anim(pos, level); */
        /*     } */
        /*     EndShaderMode(); */
        /* } */

        rlPopMatrix();
    }
    if (finished) {
        level_draw_corner_connections(level);
    }


    //DrawRectangleLinesEx(level->px_bounding_box, 5.0, LIME);
    rlSetBlendMode(RL_BLEND_ALPHA);
    rlPopMatrix();

#if 0
    if (level->drag_target) {
        DrawText(TextFormat("drag_target<%d,%d> drag_offset = (%f, %f)",
                            level->drag_target->position.q, level->drag_target->position.r,
                            level->drag_offset.x, level->drag_offset.y),
                 10, 10, 20, GREEN);
    }
#endif
}

void level_preview(level_t *level, Rectangle rect)
{
    rlPushMatrix();

    float scale_factor = 1.2;
    float half_scale_factor = 0.5 * scale_factor;
    Vector2 new_size = {
        .x = half_scale_factor * rect.width,
        .y = half_scale_factor * rect.height
    };
    Vector2 old_size = {
        .x = 0.5 * rect.width,
        .y = 0.5 * rect.height
    };
    Vector2 delta = Vector2Subtract(new_size, old_size);

    rlTranslatef(rect.x - delta.x,
                 rect.y - delta.y,
                 0.0f);

    rlScalef(scale_factor * (rect.width  / window_size.x),
             scale_factor * (rect.height / window_size.y),
             1.0f);

    used_tiles_t save_currently_used_tiles = level->currently_used_tiles;
    float save_fade_value = level->fade_value;
    float save_fade_value_eased = level->fade_value_eased;
    float save_fade_rotate_speed = level->fade_rotate_speed;

    level->currently_used_tiles = USED_TILES_UNSOLVED;
    level->fade_value = 1.0;
    level->fade_value_eased = 1.0;
    level->fade_rotate_speed = 0.0;

    level_draw(level, false);

    level->currently_used_tiles = save_currently_used_tiles;
    level->fade_value = save_fade_value;
    level->fade_value_eased = save_fade_value_eased;
    level->fade_rotate_speed = save_fade_rotate_speed;

    rlPopMatrix();
}
