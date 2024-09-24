/****************************************************************************
 *                                                                          *
 * anim_fsm.h                                                               *
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

#ifndef ANIM_FSM_H
#define ANIM_FSM_H

struct anim_fsm;

typedef void (*anim_fsm_update_cb_t)(struct anim_fsm *anim_fsm);
typedef void (*anim_fsm_draw_cb_t)(struct anim_fsm *anim_fsm);
typedef void (*anim_fsm_enter_state_cb_t)(struct anim_fsm *anim_fsm);
typedef void (*anim_fsm_exit_state_cb_t)(struct anim_fsm *anim_fsm);

struct anim_fsm_callbacks {
    anim_fsm_update_cb_t      update;
    anim_fsm_draw_cb_t        draw;
    anim_fsm_enter_state_cb_t enter_state;
    anim_fsm_exit_state_cb_t  exit_state;
};
typedef struct anim_fsm_callbacks anim_fsm_callbacks_t;

enum anim_fsm_state_switch_mode {
    ANIM_STATE_NEXT = 0,
    ANIM_STATE_RESTART,
    ANIM_STATE_STOP
};
typedef enum anim_fsm_state_switch_mode anim_fsm_state_switch_mode_t;

struct anim_fsm_state {
    double length;
    anim_fsm_state_switch_mode_t switch_mode;
    anim_fsm_callbacks_t *callbacks;
};
typedef struct anim_fsm_state anim_fsm_state_t;

struct anim_fsm {
    anim_fsm_callbacks_t *callbacks;
    anim_fsm_state_t *states;
    anim_fsm_state_t *current_state;
    int current_state_index;
    int stop_state_index;

    double start_time;
    double elapsed_time;

    double state_elapsed_time;
    double state_start_time;
    double state_end_time;

    float state_progress;
};
typedef struct anim_fsm anim_fsm_t;

#define anim_fsm_call(anim_fsm, callback_name) do {                     \
        anim_fsm_callbacks_t *state_cb =                                \
            (anim_fsm)->current_state->callbacks;                       \
        anim_fsm_callbacks_t *common_cb =                               \
            (anim_fsm)->callbacks;                                      \
        if (state_cb && state_cb->callback_name) {                      \
            state_cb->callback_name(anim_fsm);                          \
        } else {                                                        \
            if (common_cb && common_cb->callback_name) {                \
                common_cb->callback_name(anim_fsm);                     \
            }                                                           \
        }                                                               \
    } while(0)

anim_fsm_t *create_anim_fsm(anim_fsm_state_t *states, anim_fsm_callbacks_t *callbacks);
void destroy_anim_fsm(anim_fsm_t *anim_fsm);

int anim_fsm_next_state_index(anim_fsm_t *anim_fsm);
void anim_fsm_change_state(anim_fsm_t *anim_fsm, int state_index);

void anim_fsm_update(anim_fsm_t *anim_fsm);
void anim_fsm_draw(anim_fsm_t *anim_fsm);
void anim_fsm_start(anim_fsm_t *anim_fsm);
void anim_fsm_stop(anim_fsm_t *anim_fsm);
void anim_fsm_next_state(anim_fsm_t *anim_fsm);
void anim_fsm_set_state(anim_fsm_t *anim_fsm, int states_index);


#endif /*ANIM_FSM_H*/

