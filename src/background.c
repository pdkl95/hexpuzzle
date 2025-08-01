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

#include "level.h"
#include "win_anim.h"
#include "background.h"
#include "shader.h"

//#define DEBUG_DRAW_LABELS

#define USE_BG_TEXTURE

extern Texture2D bg_grid_texture;
extern float bloom_amount;

extern background_t *background;

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
    bg->hmajor_color = ColorAlpha(bg->hmajor_color, 1.0);
    bg->vmajor_color = ColorAlpha(bg->vmajor_color, 1.0);

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

    Vector2 cpos = VEC2_ZERO;

    memset(&bg->camera, 0, sizeof(Camera));
    bg->camera.position = (Vector3){ cpos.x, cpos.y, -nearfar_dist };
    bg->camera.target = (Vector3){ cpos.x, cpos.y, 4 * nearfar_dist };
    bg->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    bg->camera.fovy = 45.0f;
    //bg->camera.fovy = 90.0f;
    bg->camera.projection = CAMERA_PERSPECTIVE;
}

float smooth_change(float current, float target, float step)
{
    if (current < target) {
        current += step;
    } else if (current > step) {
        current -= step;
    }

    if (fabsf(target - current) <= step) {
        return target;
    } else {
        return current;
    }
}

float limit_rot_x = 0.0f;
float limit_rot_y = 0.0f;
float limit_rot_z = 0.0f;

void background_begin_3d(background_t *bg, float amount)
{
    rlPushMatrix();
    BeginMode3D(bg->camera);

    rlTranslatef(window_center.x,
                 window_center.y,
                 0.0);

    rlRotatef(amount * limit_rot_z, 0.0, 0.0, 1.0);

    rlTranslatef(-window_size.x,
                 -window_size.y,
                 0.0);

    rlRotatef(amount * TO_DEGREES(limit_rot_x), 1.0, 0.0, 0.0);
    rlRotatef(amount * TO_DEGREES(limit_rot_y), 0.0, 1.0, 0.0);
}

void background_end_3d(UNUSED background_t *bg)
{
    EndMode3D();
    rlPopMatrix();
}

void bg_begin_3d(float amount)
{
    background_begin_3d(background, amount);
}

void bg_end_3d(void)
{
    background_end_3d(background);
}

void background_draw(background_t *bg)
{
    bool animate_bg = !options->wait_events && options->animate_bg;

    static float fade = 0.0f;
    float fade_target = 0.0f;

    static float bgwarp = 0.0f;
    float bgwarp_target = 0.0f;

    bool current_finished_level = current_level && current_level->finished;

    if (current_finished_level) {
        fade_target = 0.5 * (current_level->win_anim->fade[2] + current_level->win_anim->fade[3]);
    }
    fade = smooth_change(fade,
                         fade_target,
                         0.02f);

    if (current_finished_level) {
        bgwarp = fade;

        switch (current_level->win_anim->mode) {
        case WIN_ANIM_MODE_WAVES:
            bgwarp_target = 0.0f;
            break;

#ifdef USE_PHYSICS
        case WIN_ANIM_MODE_PHYSICS_FALL:
            bgwarp_target = 0.0f;
            break;
#endif
        default:
            break;
        }
    }

    bgwarp = smooth_change(bgwarp,
                           bgwarp_target,
                           0.02f);

    SetShaderValue(background_shader, background_shader_loc.warp, &bgwarp, SHADER_UNIFORM_FLOAT);

#define cart_bg_normal_speed   1.0
#define cart_bg_finished_speed 3.5
#define cart_bg_delta_speed ((cart_bg_finished_speed) - (cart_bg_normal_speed))
#define cart_bg_accel_duration 10.0
#define cart_bg_delta_speed_per_second ((cart_bg_delta_speed) / (cart_bg_accel_duration))
#define cart_bg_delta_speed_per_frame ((cart_bg_delta_speed_per_second) / (options->max_fps))

    int minor_size = 15;
    int minor_per_major = 4;
    int major_size = minor_size * minor_per_major;
    float wrap_size = 75.0f;  //(float)major_size;
    //float half_wrap = wrap_size / 2.0;

#ifndef USE_BG_TEXTURE
    float minor_thickness = 3.0;
    float major_thickness = 4.0;
#endif

    int margin = 5 * major_size;
    int xmin = -margin;
    int ymin = -margin;
    int xmax = window_size.x + margin;
    int ymax = window_size.y + margin;

    static float bloom_fade = 0.0f;
    static float speed = cart_bg_normal_speed;
    static float angle = 0.0;
    static float prev_angle = 0.0f;
    static float next_angle = 0.0f;
    static float finished_fract = 0.0f;
    static float change_fract = 0.0f;
    static int change_counter = 0;
    //static float rot_direction = 1.0;

    static Vector2 off = { 1.0, 0.0 };
    static Vector2 dir = { 1.0, 0.0 };

    if (animate_bg) {
        bg->amp = smooth_change(bg->amp,
                                win_level_mode
                                ? cart_bg_finished_speed
                                : cart_bg_normal_speed,
                                cart_bg_delta_speed_per_frame);

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

        float bloom_fade_target = 0.5 * bloom_amount;
        bloom_fade_target += 0.5;
        bloom_fade = smooth_change(bloom_fade,
                                   bloom_fade_target,
                                   0.005f);

        float circ_time_scale = 3.0;
        float circ_mag_target = TAU/36.0f;

        float rot_x, rot_y; 
        if (current_level) {
            finished_fract = smooth_change(finished_fract,
                                           current_level->finished_fract,
                                           0.005f);
            circ_mag_target *= finished_fract;
        } else {
            circ_mag_target *= bloom_fade;
            circ_mag_target *= fade;
        }

        static float circ_mag = 0.0f;
        circ_mag = smooth_change(circ_mag,
                                 circ_mag_target,
                                 0.005f);

        if (current_level) {
            rot_x = circ_mag * cosf(current_time / circ_time_scale);
            rot_y = circ_mag * sinf(current_time / circ_time_scale);
        } else {
            rot_x = circ_mag * -dir.x;
            rot_y = circ_mag * -dir.y;
        }

        float rot_z = 2.0 * sinf(current_time / 10.0);
        rot_z += 1.0f - (fade * bloom_fade);

        rot_z -= TAU/10.0f;

        float scale_rot = 3.0f;
        rot_z *= scale_rot * fade;

        float turn_limit = 0.025 * (TAU/options->max_fps);
        limit_rot_x = slew_limit(limit_rot_x, rot_x, turn_limit);
        limit_rot_y = slew_limit(limit_rot_y, rot_y, turn_limit);
        limit_rot_z = slew_limit(limit_rot_z, rot_z, turn_limit);

        background_begin_3d(bg, 1.0f);

        off = Vector2Add(off, Vector2Scale(dir, speed * bg->amp));
        off.x = fmodf(off.x, wrap_size);
        off.y = fmodf(off.y, wrap_size);

        //printf("off=(%4f,%4f)\n", off.x, off.y);
    }

#ifdef USE_BG_TEXTURE
/*
 * TEXTURE MODE
 */
    int texture_size = 75;

    Rectangle src = {
        .x = 0.0f,
        .y = 0.0f,
        .width  = texture_size,
        .height = texture_size
    };
    Rectangle dst = {
        .width  = texture_size,
        .height = texture_size
    };

    for (int y=ymin; y<ymax; y += texture_size) {
        for (int x=xmin; x<xmax; x += texture_size) {
            dst.x = x + off.x;
            dst.y = y + off.y;
            DrawTexturePro(bg_grid_texture, src, dst, VEC2_ZERO, 0.0f, WHITE); 
        }
    }

#else
/*
 * LINE-DRAWING MODE
 */

#if 1
    for (int x=xmin, n=0; x<xmax; x += minor_size, n = (n + 1) % minor_per_major) {
        if (n) {
            DrawLineEx((Vector2){ x+off.x, ymin },
                       (Vector2){ x+off.x, ymax },
                       minor_thickness,
                       bg->minor_color);
        }
    }

    for (int y=ymin, n=0; y<ymax; y += minor_size, n = (n + 1) % minor_per_major) {
        if (n) {
            DrawLineEx((Vector2){ xmin, y+off.y },
                       (Vector2){ xmax, y+off.y },
                       minor_thickness,
                       bg->minor_color);
        }
    }
#endif

#if 1
    for (int x=xmin, n=0; x<xmax; x += major_size, n = (n + 1) % minor_per_major) {
        DrawLineEx((Vector2){ x+off.x, ymin },
                   (Vector2){ x+off.x, ymax },
                   major_thickness,
                   bg->vmajor_color);
#ifdef DEBUG_DRAW_LABELS
        DrawText(TextFormat("%d", x), (float)x + 3.0, 8.0, 16, YELLOW);
#endif
    }

    for (int y=ymin, n=0; y<ymax; y += major_size, n = (n + 1) % minor_per_major) {
        DrawLineEx((Vector2){ xmin, y+off.y },
                   (Vector2){ xmax, y+off.y },
                   major_thickness,
                   bg->hmajor_color);
#ifdef DEBUG_DRAW_LABELS
        DrawText(TextFormat("%d", y), 3.0, (float)y + 3.9, 16, YELLOW);
#endif
    }
#endif
#endif

    background_end_3d(bg);
}

