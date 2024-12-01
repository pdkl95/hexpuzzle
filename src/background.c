/****************************************************************************
 *                                                                          *
 * background.c                                                             *
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

#include "options.h"

#include "anim_fsm.h"
#include "level.h"
#include "win_anim.h"
#include "background.h"

extern float bloom_amount;

void lerp_background_control(background_control_t *dst,
                             background_control_t *start,
                             background_control_t *end,
                             float t)
{
#define lerp_float(field) dst->field = Lerp(start->field, end->field, t)
    lerp_float(amp);
#undef lerp_float
}

background_t *create_background(void)
{
    background_t *bg = calloc(1, sizeof(background_t));

    bg->amp = 1.0;
    bg->change_counter_frames = 3 * options->max_fps;
    bg->change_per_frame = 1.0f / ((float)bg->change_counter_frames);

    bg->minor_color  = ColorBrightness(purple,     -0.4);
    bg->hmajor_color = ColorBrightness(magenta,    -0.15);
    bg->vmajor_color = ColorBrightness(royal_blue, -0.15);

    bg->minor_color  = ColorAlpha(bg->minor_color,  0.65);
    bg->hmajor_color = ColorAlpha(bg->hmajor_color, 0.95);
    bg->vmajor_color = ColorAlpha(bg->vmajor_color, 0.95);

    background_resize(bg);
    return bg;
}

void destroy_background(background_t *bg)
{
    SAFEFREE(bg);
}

void background_resize(background_t *bg)
{
    float nearfar_dist = MIN(window_size.x, window_size.y);

    //Vector2 cpos = window_size;
    //Vector2 cpos = window_center;
    Vector2 cpos = VEC2_ZERO;

    memset(&bg->camera, 0, sizeof(Camera));
    bg->camera.position = (Vector3){ cpos.x, cpos.y, -nearfar_dist };
    bg->camera.target = (Vector3){ cpos.x, cpos.y, 4 * nearfar_dist };
    bg->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    bg->camera.fovy = 45.0f;
    bg->camera.projection = CAMERA_PERSPECTIVE;
}

void background_set_mode(background_t *bg, background_mode_t new_mode)
{
    bg->mode = new_mode;
}

void background_draw(background_t *bg)
{
    bool draw_labels = false;;
    bool animate_bg = !options->wait_events && options->animate_bg;

    float fade = 0.0f;
    if (current_level && current_level->finished) {
        fade = 0.5 * (current_level->win_anim->fade[2] + current_level->win_anim->fade[3]);
    }

    rlPushMatrix();

    BeginMode3D(bg->camera);

    Vector2 hwin = {
        .x = window_size.x / 2.0,
        .y = window_size.y / 2.0
    };

#define cart_bg_normal_speed   1.0
#define cart_bg_finished_speed 3.5
#define cart_bg_delta_speed ((cart_bg_finished_speed) - (cart_bg_normal_speed))
#define cart_bg_accel_duration 10.0
#define cart_bg_delta_speed_per_second ((cart_bg_delta_speed) / (cart_bg_accel_duration))
#define cart_bg_delta_speed_per_frame ((cart_bg_delta_speed_per_second) / (options->max_fps))

    int minor_size = 25;
    int minor_per_major = 4;
    int major_size = minor_size * minor_per_major;
    float wrap_size = (float)major_size;
    float half_wrap = wrap_size / 2.0;
    float minor_thickness = 3.0;
    float major_thickness = 4.0;

    int margin_size = 2 * major_size;
    int xmin = -margin_size;
    int ymin = -margin_size;
    int xmax = window_size.x + margin_size;
    int ymax = window_size.y + margin_size;

    static float speed = cart_bg_normal_speed;
    static float angle = 0.0;
    static float prev_angle = 0.0f;
    static float next_angle = 0.0f;
    static float change_fract = 0.0f;
    static int change_counter = 0;
    //static float rot_direction = 1.0;

    static Vector2 off = { 1.0, 0.0 };
    static Vector2 dir = { 1.0, 0.0 };

    if (animate_bg) {
#if 1
        if (win_level_mode) {
#if 0
            if (bg->change_counter_frames > cart_bg_finished_change_frames) {
                bg->change_counter_frames--;
            }

            if (bg->lerp_counter_frames > cart_bg_finished_lerp_frames) {
                bg->lerp_counter_frames--;
            }
#endif
            if (bg->amp < cart_bg_finished_speed) {
                bg->amp += cart_bg_delta_speed_per_frame;
            }
            //printf("(WIN) amp = %f\n", bg->amp);
        } else {
#if 0
            if (bg->change_counter_frames < cart_bg_normal_change_frames) {
                bg->change_counter_frames++;
            }

            if (bg->lerp_counter_frames > cart_bg_normal_lerp_frames) {
                bg->lerp_counter_frames++;
            }
#endif
            if (bg->amp > cart_bg_normal_speed) {
                bg->amp -= cart_bg_delta_speed_per_frame * 4.0;
            }
            //printf("(NOR) amp = %f\n", bg->amp);
        }
#endif

        if (0 == change_counter) {
            prev_angle = next_angle;
            next_angle += 0.5 * normal_rng();
            next_angle = fmodf(next_angle, TAU);
            //printf("next_angle = %f\n", next_angle);
            change_fract = 0.0f;
            change_counter = bg->change_counter_frames;
            //float angle_dist = prev_angle < angle ? (TAU - angle + prev_angle) : (prev_angle - angle);
            //rot_direction = angle_dist < TAU / 2.0 ? -1.0 : 1.0;

        } else {
            change_counter--;
        }

        change_fract += bg->change_per_frame;
        float eased_change_fract = ease_quartic_inout(change_fract);
        angle = Lerp(prev_angle, next_angle, eased_change_fract);

        //printf("fract = %f, angle = %f\n", change_fract, angle);

        dir = Vector2Rotate((Vector2) { .x = 0.0f, .y = 1.0f }, angle);
        dir = Vector2Scale(dir, speed);

        rlTranslatef(hwin.x,
                     hwin.y,
                     0.0);

        float bloom_fade = 0.5 * bloom_amount;
        bloom_fade += 0.5;
        float circ_mag = TAU/36.0f;
        circ_mag *= bloom_fade;
        circ_mag *= fade;
        //float circ_time_scale = 3.0;
        /* float rot_x = circ_mag * cosf(current_time / circ_time_scale); */
        /* float rot_y = circ_mag * sinf(current_time / circ_time_scale); */
        float rot_x = circ_mag * -dir.x;
        float rot_y = circ_mag * -dir.y;

        float rot_z = 2.0 * sinf(current_time / 10.0);
        rot_z += 1.0f - bloom_fade;

        rlRotatef(rot_z, 0.0, 0.0, 1.0);

        rlTranslatef(-window_size.x,
                     -window_size.y,
                     0.0);
        /* rlTranslatef(-hwin.x, */
        /*              -hwin.y, */
        /*              0.0); */
        rlRotatef(TO_DEGREES(rot_x), 1.0, 0.0, 0.0);
        rlRotatef(TO_DEGREES(rot_y), 0.0, 1.0, 0.0);

        off = Vector2Add(off, Vector2Scale(dir, speed * bg->amp));

        if      (off.x < -half_wrap) { off.x += wrap_size; }
        else if (off.x >  half_wrap) { off.x -= wrap_size; }
        if      (off.y < -half_wrap) { off.y += wrap_size; }
        else if (off.y >  half_wrap) { off.y -= wrap_size; }
    }

    for (int x=xmin; x<xmax; x += minor_size) {
        DrawLineEx((Vector2){x+off.x, 0},
                   (Vector2){x+off.x, window_size.y},
                   minor_thickness,
                   bg->minor_color);
    }

    for (int y=ymin; y<ymax; y += minor_size) {
        DrawLineEx((Vector2){0, y+off.y},
                   (Vector2){window_size.x, y+off.y},
                   minor_thickness,
                   bg->minor_color);
    }

    for (int x=xmin; x<xmax; x += major_size) {
        DrawLineEx((Vector2){x+off.x, 0},
                   (Vector2){x+off.x, window_size.y},
                   major_thickness,
                   bg->vmajor_color);
        if (draw_labels) {
            DrawText(TextFormat("%d", x), (float)x + 3.0, 8.0, 16, YELLOW);
        }
    }

    for (int y=ymin; y<ymax; y += major_size) {
        DrawLineEx((Vector2){0, y+off.y},
                   (Vector2){window_size.x, y+off.y},
                   major_thickness,
                   bg->hmajor_color);
        if (draw_labels) {
            DrawText(TextFormat("%d", y), 3.0, (float)y + 3.9, 16, YELLOW);
        }
    }

    EndMode3D();

    rlPopMatrix();
}

