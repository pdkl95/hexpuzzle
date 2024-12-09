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

extern bool do_postprocessing;
extern float bloom_amount;
extern float warp_amount;

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
static void win_anim_common_update_physics(UNUSED win_anim_t *win_anim)
{
#if 0
    level_t *level = win_anim->level;

    for (int i=0; i<4; i++) {
        cpShape *wall = level->physics->wall[i];

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

static void win_anim_common_update_pops(win_anim_t *win_anim)
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

static void win_anim_common_update(struct anim_fsm *anim_fsm, void *data)
{
    win_anim_t *win_anim = (win_anim_t *)data;

    win_anim->fade[0] = anim_fsm->state_progress;
    win_anim->fade[1] = ((float)win_anim->level->finished_hue) / 360.0;

#if 0
    printf("progress = %f. hue = %f, fadein = %f\n",
           win_anim->fade[0],
           win_anim->fade[1],
           win_anim->fade[2]);
#endif

    SetShaderValue(win_border_shader,
                   win_border_shader_loc.fade,
                   &(win_anim->fade[0]),
                   SHADER_UNIFORM_VEC4);

    switch (win_anim->mode) {
    case WIN_ANIM_MODE_POPS:
        win_anim_common_update_pops(win_anim);
        break;

#ifdef USE_PHYSICS
    case WIN_ANIM_MODE_PHYSICS:
        win_anim_common_update_physics(win_anim);
        break;
#endif
    }
}

static void win_anim_fade_in_update(struct anim_fsm *anim_fsm, void *data)
{
    win_anim_t *win_anim = (win_anim_t *)data;

    win_anim->fade[2] = anim_fsm->state_progress;
    win_anim->fade[3] = 0.0f;
    win_anim_common_update(anim_fsm, data);
}

static void win_anim_osc_ramp_in_update(struct anim_fsm *anim_fsm, void *data)
{
    win_anim_t *win_anim = (win_anim_t *)data;

    win_anim->fade[2] = 1.0;
    win_anim->fade[3] = anim_fsm->state_progress;
    win_anim_common_update(anim_fsm, data);

    if (anim_fsm->state_progress == 0.0f) {
#ifdef USE_PHYSICS
        if (win_anim->mode == WIN_ANIM_MODE_PHYSICS) {
            assert(win_anim->level->finished);
            physics_start(win_anim->level->physics);
        }
#endif
    }
}

static void win_anim_osc_stay_update(struct anim_fsm *anim_fsm, void *data)
{
    win_anim_t *win_anim = (win_anim_t *)data;

    win_anim->fade[2] = 1.0f;
    win_anim->fade[3] = 1.0f;
    win_anim_common_update(anim_fsm, data);
}

anim_fsm_callbacks_t     fade_in_callbacks = { .update = win_anim_fade_in_update     };
anim_fsm_callbacks_t osc_ramp_in_callbacks = { .update = win_anim_osc_ramp_in_update };
anim_fsm_callbacks_t    osc_stay_callbacks = { .update = win_anim_osc_stay_update    };

anim_fsm_state_t states[] = {
#if 0
    { "FADE_IN",       1/* 6.0 */, ANIM_FSM_STATE_NEXT,     &fade_in_callbacks },
    { "OSC_RAMP_IN",   1/* 8.0 */, ANIM_FSM_STATE_NEXT, &osc_ramp_in_callbacks },
#else
    { "FADE_IN",       6.0, ANIM_FSM_STATE_NEXT,     &fade_in_callbacks },
    { "OSC_RAMP_IN",   8.0, ANIM_FSM_STATE_NEXT, &osc_ramp_in_callbacks },
#endif
    { "OSC_STAY",     10.0, ANIM_FSM_STATE_STAY,    &osc_stay_callbacks },
    { "STOP",          0.0, ANIM_FSM_STATE_STOP,                   NULL }
};

win_anim_t *create_win_anim(struct level *level)
{
    win_anim_t *win_anim = calloc(1, sizeof(win_anim_t));

#ifdef USE_PHYSICS
# if 1
    win_anim->mode = WIN_ANIM_MODE_PHYSICS;
# else
    win_anim->mode = WIN_ANIM_MODE_POPS;
# endif
#else
    win_anim->mode = WIN_ANIM_MODE_POPS;
#endif
    win_anim->running = false;
    win_anim->level = level;

    win_anim->fade[0] = 0.0;
    win_anim->fade[1] = 0.0;
    win_anim->fade[2] = 0.0;
    win_anim->fade[3] = 0.0;

    init_anim_fsm(&win_anim->anim_fsm, states, NULL, win_anim);

#ifdef USE_PHYSICS
    if (!level->physics) {
        level->physics = create_physics(level);
    }
#endif

    return win_anim;
}

void destroy_win_anim(win_anim_t *win_anim)
{
    SAFEFREE(win_anim);
}

void win_anim_update(win_anim_t *win_anim)
{
    assert_not_null(win_anim);

#ifdef USE_PHYSICS
    if (win_anim->mode == WIN_ANIM_MODE_PHYSICS) {
        physics_update(win_anim->level->physics);
    }
#endif

    anim_fsm_update(&win_anim->anim_fsm);
}

void win_anim_draw(win_anim_t *win_anim)
{
    assert_not_null(win_anim);

    anim_fsm_draw(&win_anim->anim_fsm);
}

bool win_anim_running(win_anim_t *win_anim)
{
    assert_not_null(win_anim);

    return win_anim->running;
}


void win_anim_start(win_anim_t *win_anim)
{
    assert_not_null(win_anim);

    if (!win_anim->running) {
        anim_fsm_start(&win_anim->anim_fsm);
        win_anim->running = true;
        win_anim->start_time = GetTime();
    }
}

void win_anim_stop(win_anim_t *win_anim)
{
    assert_not_null(win_anim);

    if (win_anim->running) {
#ifdef USE_PHYSICS
        if (win_anim->mode == WIN_ANIM_MODE_PHYSICS) {
            physics_stop(win_anim->level->physics);
        }
#endif

        win_anim->running = false;
        anim_fsm_stop(&win_anim->anim_fsm);
    }
}
