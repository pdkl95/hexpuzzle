/****************************************************************************
 *                                                                          *
 * win_anim.c                                                               *
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
#include "shader.h"
#ifdef USE_PHYSICS
#include "physics.h"
#endif

//#define DEBUG_TRACE_WIN_ANIM

extern bool do_postprocessing;
extern float bloom_amount;
extern float warp_amount;

float smooth_change(float current, float target, float step);

const char *win_anim_state_str(win_anim_state_t state);

static void win_anim_set_state(win_anim_t *win_anim, win_anim_state_t new_state)
{
    assert_not_null(win_anim);

#if 0
    printf("win_anim_set_state: %s -> %s\n",
           win_anim_state_str(win_anim->state),
           win_anim_state_str(new_state));
#endif

    win_anim->state = new_state;
    win_anim->state_change_time = GetTime();
}

const char *win_anim_mode_str(win_anim_mode_t mode)
{
    switch (mode) {
    case WIN_ANIM_MODE_NULL:
        return "NULL";

    case WIN_ANIM_MODE_SIMPLE:
        return "SIMPLE";

    case WIN_ANIM_MODE_POPS:
        return "POPS";

    case WIN_ANIM_MODE_WAVES:
        return "WAVES";

    case WIN_ANIM_MODE_SPIN:
        return "SPIN";

#ifdef USE_PHYSICS
    case WIN_ANIM_MODE_PHYSICS_FALL:
        return "PHYSICS_FALL";

    case WIN_ANIM_MODE_PHYSICS_SWIRL:
        return "PHYSICS_SWIRL";
#endif

    default:
        return "(INVALID WIN_ANIM_MODE)";
    }
}

const char *win_anim_state_str(win_anim_state_t state)
{
    switch (state) {
    case WIN_ANIM_STATE_NULL:
        return "NULL";

    case WIN_ANIM_STATE_STANDBY:
        return "STANDBY";

    case WIN_ANIM_STATE_STARTUP:
        return "STARTUP";

    case WIN_ANIM_STATE_RUNNING:
        return "RUNNING";

    case WIN_ANIM_STATE_SHUTDOWN:
        return "SHUTDOWN";

    default:
        return "(INVALID WIN_ANIM_STATE)";
    }
}

void print_win_anim_modes(win_anim_t *win_anim)
{
    for (win_anim_mode_t mode = 0; mode < WIN_ANIM_MODE_COUNT; mode++) {
        win_anim_mode_config_t *config = win_anim->mode_config;
        printf("\tmode[%d] %s %s  \"%s\"\t%d chances (%f%%)\n",
               mode,
               config->enabled ? "ON " : "OFF",
               config->animated ? "ANIMATED" : "STATIC",
               win_anim_mode_str(mode),
               config->chances,
               100.0f * ((float)win_anim_mode_config[mode].chances) / ((float)win_anim->total_mode_chances[config->animated]));
    }
}

void print_win_anim(win_anim_t *win_anim)
{
    printf(">>> win_anim[%p] %s %s\n",
           win_anim,
           win_anim_mode_str(win_anim->mode),
           win_anim_state_str(win_anim->state));

    print_win_anim_modes(win_anim);

    for (int i=0; i<4; i++) {
        printf("\t->fade[%d] = %f\n", i, win_anim->fade[i]);
    }

    printf("\t->start_time = %f\n", win_anim->start_time);
}

static void trigger_pop(tile_pos_t *pos)
{
    if ((pos->pop_out_phase > 0.0) || (pos->pop_in_phase)) {
        return;
    }

    pos->pop_out_phase = 1.0;
    for (int i=0; i<pos->outer_neighbors_count; i++) {
        trigger_pop(pos->outer_neighbors[i]);
    }
}

#ifdef USE_PHYSICS
static void win_anim_update_physics(UNUSED win_anim_t *win_anim)
{
#if 0
    level_t *level = win_anim->level;

    for (int i=0; i<4; i++) {
        cpShape *wall = win_anim.physics->wall[i];

        Vector2 a = cpVectToVector2(cpSegmentShapeGetA(wall));
        Vector2 b = cpVectToVector2(cpSegmentShapeGetB(wall));
        a = Vector2Add(a, window_center);
        b = Vector2Add(b, window_center);
        //a = Vector2Add(a, level->px_offset);
        //b = Vector2Add(b, level->px_offset);
        DrawLineEx(a, b, 8.0, LIME);
    }
#endif
}
#endif

static void win_anim_update_simple(UNUSED win_anim_t *win_anim)
{
}

static void win_anim_update_pops(win_anim_t *win_anim)
{
    level_t *level = win_anim->level;

    float fade_magnitude = win_anim->fade[2];
    //float fade_magnitude = ease_circular_in(anim_fsm->state_progress);
    float  osc_magnitude = win_anim->fade[3];

    //float envelope_variation = (sinf(0.23f * current_time) + 1.0f) * 0.5;
    float envelope_speed = 0.21f;
    //envelope_speed += 0.05f * envelope_variation;

    float envelope_phase = current_time * envelope_speed;

    float e_f = powf(1.0f - sqrtf(1.0f - fabs(sinf(envelope_phase))), 3.0f);
    float simple_envelope = e_f;

    //float e_g = cosf(powf(1.0f - sqrtf(1.0f - (sinf(0.337 * envelope_phase))), 4.0f));
    //float e_h = powf(1.0f - sqrtf(1.0f - fabs(sinf(0.12 * envelope_phase))), 4.0f);

    //float envelope = tanhf(fabs(e_f - e_g + e_h));
    float envelope = e_f;

#define TILE_POP_OUT_STEP 0.2
#define TILE_POP_IN_STEP 0.05
#define TILE_POP_PHASE (TAU/4)
#define TILE_POP_RBG_MASK 0x00000001
#define LEVEL_WIN_OSC_SPIN_RATE 0.0004
#define TILE_POP_AMPLIFY_MIN 60.0
#define TILE_POP_AMPLIFY_MAX 220.0
#define TILE_POP_AMPLIFY_DELTA ((TILE_POP_AMPLIFY_MAX) - (TILE_POP_AMPLIFY_MIN))
#define EVEL_WIN_SPIN_MAX_DECREASE -0.2f
#define EVEL_WIN_SPIN_MAX_INCREASE 10.0f

    float pop_amplify_delta = TILE_POP_AMPLIFY_DELTA * fade_magnitude * simple_envelope;
    float pop_amplify = TILE_POP_AMPLIFY_MIN;
    pop_amplify += pop_amplify_delta;

    level->extra_rotate_level_speed = slew_limit_down(
        level->extra_rotate_level_speed,
        simple_envelope
        * osc_magnitude
        * LEVEL_WIN_OSC_SPIN_RATE
        * pop_amplify,
        0.0015);

    level->extra_rotate_level_velocity = level->extra_rotate_level_speed;
    /* level->extra_rotate_level_velocity = copysignf(level->extra_rotate_level_speed, */
    /*                                                level->fade_rotate_speed); */

    /* float old_erl = level->extra_rotate_level; */
    /* float new_erl = (old_erl + level->extra_rotate_level_velocity) * 0.97; */
    /* float delta_erl = new_erl - old_erl; */
    /* float clamp_delta = Clamp(delta_erl, EVEL_WIN_SPIN_MAX_DECREASE, EVEL_WIN_SPIN_MAX_INCREASE); */
    /* level->extra_rotate_level += clamp_delta; */

    /* printf("erl = %f\tdelta = %f\tvelocity = %f\n", level->extra_rotate_level, delta_erl, level->extra_rotate_level_velocity); */

    level->extra_rotate_level += level->extra_rotate_level_velocity;
    level->extra_rotate_level = fmodf(level->extra_rotate_level, TAU);

    tile_pos_t *center_pos = level_get_center_tile_pos(level);

    float phase = fmodf(current_time * 3.0f, TAU);
    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(win_anim->level->tiles[i]);

        if (tile->enabled) {
            tile_pos_t *pos = tile->unsolved_pos;

            float ring_phase = fmod(phase + pos->radial_angle, TAU);
            if (pos->ring_radius & 0x00000001) {
                ring_phase = TAU - ring_phase;
            }

            float osc_norm = sinf(ring_phase);
            float osc = (osc_norm + 1.0f) * 0.6f;

            float mag = fade_magnitude;
            mag += 12.0f * ((osc_magnitude) * (pos->ring_radius)) * osc;
            mag *= envelope;


            if ((pos->prev_ring_phase < TILE_POP_PHASE) &&
                (ring_phase > TILE_POP_PHASE)) {
                if ((rand() & TILE_POP_RBG_MASK) == TILE_POP_RBG_MASK) {
                    trigger_pop(pos);
                }
            }
            if (pos->pop_out_phase > 0.0f) {
                pos->pop_out_phase -= TILE_POP_OUT_STEP;
                if (pos->pop_out_phase < 0.0f) {
                    pos->pop_out_phase = 0.0f;
                    pos->pop_in_phase = 1.0f;
                }
                pos->pop_magnitude = 1.0f - (pos->pop_out_phase);
            }
            if (pos->pop_in_phase > 0.0f) {
                pos->pop_in_phase -= TILE_POP_IN_STEP;
                if (pos->pop_in_phase < 0.0f) {
                    pos->pop_in_phase = 0.0f;
                }
                pos->pop_magnitude = (pos->pop_in_phase);
            }
            float pop_magnitude = pos->pop_magnitude * fade_magnitude * pop_amplify;

            pos->prev_ring_phase = ring_phase;

            float tmag = tanh(mag);
            float extra_magnitude_target = Lerp(pop_magnitude * tmag, mag + (0.25 * tmag), tmag);
            extra_magnitude_target += 1.0f * pos->ring_radius;
            float spin_boost = 1.0f + (pos->ring_radius /2.0f);
            extra_magnitude_target += spin_boost * pos->ring_radius;
            extra_magnitude_target *= fade_magnitude * osc_magnitude;
            pos->extra_magnitude = slew_limit_down(pos->extra_magnitude,
                                                   extra_magnitude_target,
                                                   1.666);

#if 0
            if (i==LEVEL_MAXTILES/2+7) {
                printf("[%d] pop_out_phase = %f, prev_ring_phase = %f\n", i, pos->pop_out_phase, pos->prev_ring_phase);
                printf("[%d]  pop_in_phase = %f,      ring_phase = %f\n", i, pos->pop_in_phase, ring_phase);
                printf("[%d] pop_magnitude = %f,       extra_mag = %f\n--\n", i, pos->pop_magnitude, mag);
            }
#endif

            //pos->pop_translate   = Vector2Scale(pos->radial_vector_norm, pop_magnitude);

            float rotate_osc = cosf(phase + pos->radial_angle);
            pos->extra_rotate = rotate_osc * 0.12f * osc_magnitude * envelope *
                (((float)pos->ring_radius) / ((float)win_anim->level->radius));
        }
    }


    center_pos->extra_rotate = 0.0f;

    bloom_amount = envelope * fade_magnitude * osc_magnitude;

    float run_time = GetTime() - win_anim->start_time;
    warp_amount = tanhf(0.087 * run_time);

    level_update_tile_pops(win_anim->level);
}

static void win_anim_update_waves(win_anim_t *win_anim)
{
    float fade_magnitude = win_anim->fade[2];

#define WAVES_FREQ_SCALE 2.7
#define WAVES_BLOOM_FREQ_SCALE (3.0 * WAVES_FREQ_SCALE)

    float global_theta = fmodf(WAVES_FREQ_SCALE * current_time, TAU);
    float bloom_theta = fmodf(WAVES_BLOOM_FREQ_SCALE * current_time, TAU);

    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(win_anim->level->tiles[i]);

        if (tile->enabled) {
            tile_pos_t *pos = tile->unsolved_pos;

            float theta = global_theta + pos->radial_angle;

            float raw_wave = sinf(theta);
            float wave = 0.5 * (raw_wave + 1.0);

            pos->extra_magnitude = wave * fade_magnitude * 0.25 * TILE_POP_AMPLIFY_DELTA;

            float rotate_osc = cosf(theta);
            pos->extra_rotate = rotate_osc * 0.12f * wave *
                (((float)pos->ring_radius) / ((float)win_anim->level->radius));
        }
    }

    float bloom_raw_wave = sinf(bloom_theta);
    float bloom_wave = 0.5 * (bloom_raw_wave + 1.0);

    bloom_amount = bloom_wave * fade_magnitude;

    float run_time = GetTime() - win_anim->start_time;
    warp_amount = tanhf(0.037 * run_time);

    level_update_tile_pops(win_anim->level);
}

static void win_anim_update_spin(win_anim_t *win_anim)
{
    float envelope_speed = 0.13f;
    float spin_envelope_speed = envelope_speed * 3.0f;

    float envelope_phase = current_time * envelope_speed;
    float envelope = 1.0f - sqrtf(1.0f - fabs(sinf(envelope_phase)));

    envelope = (0.666 * envelope) + 0.333;

    float fade_magnitude = win_anim->fade[2];

    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(win_anim->level->tiles[i]);

        if (tile->enabled) {
            tile_pos_t *pos = tile->unsolved_pos;

            float theta = pos->radial_angle;

            float spin_envelope_phase = (current_time * spin_envelope_speed) + theta - pos->center_distance;
            float spin_envelope = powf(1.0f - sqrtf(1.0f - fabs(sinf(spin_envelope_phase))), 3.0f);
            pos->extra_magnitude = spin_envelope * fade_magnitude * envelope;
            float rotate_speed = spin_envelope * pos->extra_magnitude * 0.333;
            pos->extra_rotate_magnitude = slew_limit_down(pos->extra_rotate_magnitude, rotate_speed, 0.00333);
            pos->extra_rotate += pos->extra_rotate_magnitude;

            pos->extra_magnitude *= (1.0f + (25.0f * spin_envelope));
        }
    }

    level_update_tile_pops(win_anim->level);
}

void init_win_anim(win_anim_t *win_anim, level_t *level)
{
    assert_not_null(win_anim);
    assert_not_null(level);

    static int id_seq = 0;
    id_seq++;
    win_anim->id = id_seq;

    win_anim->level = level;

    win_anim_select_random_mode(win_anim, options->animate_win);

#ifdef DEBUG_TRACE_WIN_ANIM
#ifndef DEBUG_BUILD
    if (options->verbose) {
#endif
        infomsg(" INIT win_anim[%02d] %s", win_anim->id, win_anim_mode_str(win_anim->mode));
#ifndef DEBUG_BUILD
    }
#endif
#endif

    win_anim_set_state(win_anim, WIN_ANIM_STATE_STANDBY);
    win_anim->activation = 0.0f;

    win_anim->fade[0] = 0.0;
    win_anim->fade[1] = 0.0;
    win_anim->fade[2] = 0.0;
    win_anim->fade[3] = 0.0;

    switch (win_anim->mode) {
#ifdef USE_PHYSICS
    case WIN_ANIM_MODE_PHYSICS_FALL:
        fallthrough;
    case WIN_ANIM_MODE_PHYSICS_SWIRL:
        init_physics(&win_anim->physics, level);
        break;
#endif
    default:
        break;
    }
}

void win_anim_select_random_mode(win_anim_t *win_anim, bool animated)
{
#ifdef DEBUG_TRACE_WIN_ANIM
    win_anim_mode_t old_mode = win_anim->mode;
#endif

    win_anim->total_mode_chances[animated] = 0;
    for (win_anim_mode_t mode = 0; mode < WIN_ANIM_MODE_COUNT; mode++) {
        if (win_anim_mode_config[mode].enabled) {
            win_anim->total_mode_chances[win_anim_mode_config[mode].animated] +=
                win_anim_mode_config[mode].chances;
        }
    }

    int roll = global_rng_get(win_anim->total_mode_chances[animated]);
    for (win_anim_mode_t mode = 0; mode < WIN_ANIM_MODE_COUNT; mode++) {
        assert(roll >= 0);
        if (win_anim_mode_config[mode].enabled &&
            (win_anim_mode_config[mode].animated == animated)) {
            if (roll <= win_anim_mode_config[mode].chances) {
                win_anim->mode = mode;
                goto finish_select_random_mode;
            } else {
                roll -= win_anim_mode_config[mode].chances;
            }
        }
    }

    assert(false && "RNG went past the end of the mode table");
    __builtin_unreachable();

  finish_select_random_mode:
    //win_anim->mode = WIN_ANIM_MODE_NULL;
    //win_anim->mode = WIN_ANIM_MODE_SIMPLE;
    //win_anim->mode = WIN_ANIM_MODE_POPS;
    //win_anim->mode = WIN_ANIM_MODE_WAVES;
    //win_anim->mode = WIN_ANIM_MODE_SPIN;
    //win_anim->mode = WIN_ANIM_MODE_PHYSICS_SWIRL;
    //win_anim->mode = WIN_ANIM_MODE_PHYSICS_FALL;

    win_anim->mode_config = &(win_anim_mode_config[win_anim->mode]);

#ifdef DEBUG_TRACE_WIN_ANIM
    infomsg("win_anim[%02d] NEW RANDOM MODE <old:%s> => <new:%s>", win_anim->id, win_anim_mode_str(old_mode), win_anim_mode_str(win_anim->mode));
#endif
}

win_anim_t *create_win_anim(struct level *level)
{
    win_anim_t *win_anim = calloc(1, sizeof(win_anim_t));
    init_win_anim(win_anim, level);
    return win_anim;
}

void cleanup_win_anim(win_anim_t *win_anim)
{
    if (win_anim) {
        switch (win_anim->mode) {
#ifdef USE_PHYSICS
        case WIN_ANIM_MODE_PHYSICS_FALL:
            fallthrough;
        case WIN_ANIM_MODE_PHYSICS_SWIRL:
            cleanup_physics(&win_anim->physics);
            break;
#endif
        default:
            break;
        }
    }
}

void destroy_win_anim(win_anim_t *win_anim)
{
    if (win_anim) {
        cleanup_win_anim(win_anim);

#ifdef DEBUG_TRACE_WIN_ANIM
        infomsg("DESTR win_anim[%02d] %s", win_anim->id, win_anim_mode_str(win_anim->mode));
#endif
        FREE(win_anim);
    }
}

static bool win_anim_state_progress(win_anim_t *win_anim, float *progress, float inverse_state_duration)
{
    float cur_time = GetTime();
    float elapsed_time = cur_time - win_anim->state_change_time;
    float state_progress = elapsed_time * inverse_state_duration;
    *progress = Clamp(state_progress, 0.0f, 1.0f);

    return state_progress >= 1.0f;
}

#define WIN_ANIM_STARTUP_TIME_INVERSE  (1.0f / WIN_ANIM_STARTUP_TIME)
#define WIN_ANIM_SHUTDOWN_TIME_INVERSE (1.0f / WIN_ANIM_SHUTDOWN_TIME)

void win_anim_update(win_anim_t *win_anim)
{
    if (!win_anim || !win_level_mode ||
        (win_anim->mode_config && !win_anim->mode_config->animated)) {
        return;
    }

    switch (win_anim->state) {
    case WIN_ANIM_STATE_NULL:
        assert(false && "win_anim_update called before win_anim_init");
        return;

    case WIN_ANIM_STATE_STANDBY:
        win_anim->activation = 0.0f;
        return;

    case WIN_ANIM_STATE_STARTUP:
        if (!win_anim->mode_config->do_fade ||
            win_anim_state_progress(win_anim, &win_anim->activation, WIN_ANIM_STARTUP_TIME_INVERSE)) {
            win_anim_set_state(win_anim, WIN_ANIM_STATE_RUNNING);
            win_anim->activation = 1.0f;
        }
        break;

    case WIN_ANIM_STATE_RUNNING:
        win_anim->activation = 1.0f;
        break;

    case WIN_ANIM_STATE_SHUTDOWN:
        if (!win_anim->mode_config->do_fade ||
            win_anim_state_progress(win_anim, &win_anim->activation, WIN_ANIM_SHUTDOWN_TIME_INVERSE)) {
            win_anim_set_state(win_anim, WIN_ANIM_STATE_STANDBY);
            win_anim->activation = 0.0f;
        } else {
            win_anim->activation = 1.0f - win_anim->activation;
        }
        break;
    }

    win_anim->run_time = current_time - win_anim->start_time;

    win_anim->fade[0] = fmodf(win_anim->run_time, 10.0f);
    win_anim->fade[1] = ((float)win_anim->level->finished_hue) / 360.0;
    win_anim->fade[2] = smoothstep(0.0f,  5.0f, win_anim->run_time);
    win_anim->fade[3] = smoothstep(3.5f, 8.0f, win_anim->run_time);

    float mixed_fade = 0.5f * (win_anim->fade[2] + win_anim->fade[3]);

    if (win_anim->use_background_3d) {
        win_anim->level->background_transform_amount = mixed_fade;
    } else {
        win_anim->level->background_transform_amount = 0.0f;
    }

    if (win_anim->mode != WIN_ANIM_MODE_NULL) {
#if 0
        printf("progress = %3.2f. hue = %3.2f, fadein = %3.2f, other = %3.2f\n",
               win_anim->fade[0],
               win_anim->fade[1],
               win_anim->fade[2],
               win_anim->fade[3]);
#endif

        SetShaderValue(win_border_shader,
                       win_border_shader_loc.fade,
                       &(win_anim->fade[0]),
                       SHADER_UNIFORM_VEC4);
    }

    bloom_amount = fminf(win_anim->run_time, 1.0f);
    warp_amount = smooth_change(warp_amount, 0.0f, 0.01);

    switch (win_anim->mode) {
    case WIN_ANIM_MODE_NULL:
        break;

    case WIN_ANIM_MODE_SIMPLE:
        win_anim_update_simple(win_anim);
        break;

    case WIN_ANIM_MODE_POPS:
        win_anim_update_pops(win_anim);
        break;

    case WIN_ANIM_MODE_WAVES:
        win_anim_update_waves(win_anim);
        break;

    case WIN_ANIM_MODE_SPIN:
        win_anim_update_spin(win_anim);
        break;

#ifdef USE_PHYSICS
    case WIN_ANIM_MODE_PHYSICS_FALL:
        fallthrough;
    case WIN_ANIM_MODE_PHYSICS_SWIRL:
        physics_update(&win_anim->physics, win_anim->activation);
        win_anim_update_physics(win_anim);
        break;
#endif
    }
}

void win_anim_draw(UNUSED win_anim_t *win_anim)
{
    assert_not_null(win_anim);
}

void win_anim_start(win_anim_t *win_anim)
{
    assert_not_null(win_anim);

    if ((win_anim->state == WIN_ANIM_STATE_STANDBY) ||
        (win_anim->state == WIN_ANIM_STATE_SHUTDOWN)) {

        if (options->verbose) {
            infomsg("START win_anim[%02d] %s", win_anim->id, win_anim_mode_str(win_anim->mode));
        }

        win_anim_set_state(win_anim, WIN_ANIM_STATE_STARTUP);
        win_anim->start_time = win_anim->state_change_time;
        win_anim->use_background_3d = false;

        win_anim->level->fade.do_rotate = win_anim->mode_config->do_fade_rotate;

        switch (win_anim->mode) {
        case WIN_ANIM_MODE_SIMPLE:
            fallthrough;
        case WIN_ANIM_MODE_WAVES:
            if (global_rng_bool(11, 11)) {
                win_anim->use_background_3d = true;
            }
            break;

#ifdef USE_PHYSICS
        case WIN_ANIM_MODE_PHYSICS_FALL:
            fallthrough;
        case WIN_ANIM_MODE_PHYSICS_SWIRL:
            physics_start(&win_anim->physics);
            break;
#endif
        default:
            break;
        }
    }
}

void win_anim_stop(win_anim_t *win_anim)
{
    assert_not_null(win_anim);

    if ((win_anim->state == WIN_ANIM_STATE_STARTUP) ||
        (win_anim->state == WIN_ANIM_STATE_RUNNING)) {

#ifdef DEBUG_TRACE_WIN_ANIM
#ifndef DEBUG_BUILD
        if (options->verbose) {
#endif
            infomsg(" STOP win_anim[%02d] %s", win_anim->id, win_anim_mode_str(win_anim->mode));
#ifndef DEBUG_BUILD
        }
#endif
#endif

        switch (win_anim->mode) {
#ifdef USE_PHYSICS
        case WIN_ANIM_MODE_PHYSICS_FALL:
            fallthrough;
        case WIN_ANIM_MODE_PHYSICS_SWIRL:
            physics_stop(&win_anim->physics);
            break;
#endif
        default:
            break;
        }

        win_anim_set_state(win_anim, WIN_ANIM_STATE_SHUTDOWN);

        level_reset_win_anim(win_anim->level);
    }
}
