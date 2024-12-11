/****************************************************************************
 *                                                                          *
 * anim_fsm.c                                                               *
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
#include "anim_fsm.h"

//#define DEBUG_ANIM_FSM_CALLS

const char *anim_fsm_state_str(anim_fsm_state_switch_mode_t state)
{
    switch (state) {
    case ANIM_FSM_STATE_NEXT:
        return "NEXT";

    case ANIM_FSM_STATE_RESTART:
        return "RESTART";

    case ANIM_FSM_STATE_STAY:
        return "STAY";

    case ANIM_FSM_STATE_STOP:
        return "STOP";

    default:
        return "(INVALID ANIM_FSM_STATE)";
    }
}

void print_anim_fsm(anim_fsm_t *anim_fsm)
{
    if (!anim_fsm) {
        printf(">>> anim_fsm[NULL]\n");
        return;
    }

    if (!anim_fsm->current_state) {
        printf(">>> anim_fsm[%p] NULL CURRENT STATE\n", anim_fsm);
        return;
    }

    printf(">>> anim_fsm[%p] current_state = \"%s\"[%d]\n",
           anim_fsm,
           anim_fsm->current_state->name,
           anim_fsm->current_state_index);

#define pr_field(field)                                 \
    printf("\t->" #field " \t= %f\n", anim_fsm->field);

    pr_field(start_time);
    pr_field(elapsed_time);
    pr_field(state_elapsed_time);
    pr_field(state_start_time);
    pr_field(state_end_time);
    pr_field(state_progress);

#undef pr_field
}

#ifdef DEBUG_ANIM_FSM_CALLS
# define debug_call(source, callback_name)                              \
    printf("anim_fsm_call(state=\"%s\" type=\"%s\" cb_name=\"%s\"\n",   \
           anim_fsm->current_state->name,                               \
           source,                                                      \
           STR(callback_name));
#else
# define debug_call(source, callback_name)
#endif

#define anim_fsm_call(anim_fsm, callback_name) do {                     \
        anim_fsm_callbacks_t *state_cb =                                \
            (anim_fsm)->current_state->callbacks;                       \
        anim_fsm_callbacks_t *common_cb =                               \
            (anim_fsm)->callbacks;                                      \
        if (state_cb && state_cb->callback_name) {                      \
            debug_call("STATE", callback_name);                         \
            state_cb->callback_name(anim_fsm, (anim_fsm)->data);        \
        } else {                                                        \
            if (common_cb && common_cb->callback_name) {                \
                debug_call("COMMON", callback_name);                    \
                common_cb->callback_name(anim_fsm, (anim_fsm)->data);   \
            } else {                                                    \
                debug_call("NULL", callback_name);                      \
            }                                                           \
        }                                                               \
    } while(0)

anim_fsm_callbacks_t null_callbacks = {
    .update = NULL,
    .draw   = NULL,
    .enter  = NULL,
    .exit   = NULL,
};

void init_anim_fsm(anim_fsm_t *anim_fsm, anim_fsm_state_t *states, anim_fsm_callbacks_t *callbacks, void *data)
{
    assert_not_null(anim_fsm);
    assert_not_null(states);

    if (!callbacks) {
        callbacks = &null_callbacks;
    }

    anim_fsm->callbacks = callbacks;
    anim_fsm->data = data;
    anim_fsm->states = states;
    anim_fsm->current_state = NULL;

    anim_fsm->stop_state_index = 0;
    while (anim_fsm->states[anim_fsm->stop_state_index].switch_mode != ANIM_FSM_STATE_STOP) {
         /* printf("skip state[%d] \"%s\"\n", */
         /*       anim_fsm->stop_state_index, */
         /*       anim_fsm->states[anim_fsm->stop_state_index].name); */

        anim_fsm->stop_state_index++;
    }

    //printf("init_anim_fsm(): found stop state at index %d\n", anim_fsm->stop_state_index);

    assert(anim_fsm->stop_state_index > 0);
}

anim_fsm_t *create_anim_fsm(anim_fsm_state_t *states, anim_fsm_callbacks_t *callbacks, void *data)
{
    assert_not_null(states);

    anim_fsm_t *anim_fsm = calloc(1, sizeof(anim_fsm_t));

    init_anim_fsm(anim_fsm, states, callbacks, data);

    return anim_fsm;
}

void destroy_anim_fsm(anim_fsm_t *anim_fsm)
{
    SAFEFREE(anim_fsm);
}

int anim_fsm_next_state_index(anim_fsm_t *anim_fsm)
{
    assert_not_null(anim_fsm);

    if (!anim_fsm->current_state) {
        return anim_fsm->stop_state_index;
    }

    switch (anim_fsm->current_state->switch_mode) {
    case ANIM_FSM_STATE_NEXT:
        return anim_fsm->current_state_index + 1;

    case ANIM_FSM_STATE_RESTART:
        return 0;

    case ANIM_FSM_STATE_STAY:
        return anim_fsm->current_state_index;

    case ANIM_FSM_STATE_STOP:
        /* fall through */
    default:
        return anim_fsm->stop_state_index;
    }
}

void anim_fsm_change_state(anim_fsm_t *anim_fsm, int state_index)
{
    assert_not_null(anim_fsm);

    anim_fsm_state_t *next_state =  &(anim_fsm->states[state_index]);

    if (anim_fsm->current_state) {
        if (next_state == anim_fsm->current_state) {
            return;
        } else {
            anim_fsm_call(anim_fsm, exit);
        }
    }

    anim_fsm->current_state_index = state_index;
    if (state_index == anim_fsm->stop_state_index) {
        anim_fsm->current_state = NULL;
    } else {
        anim_fsm->current_state = next_state;

        anim_fsm->state_progress = 0.0f;
        anim_fsm->state_elapsed_time = 0.0;
        anim_fsm->state_start_time = current_time;
        anim_fsm->state_end_time = anim_fsm->state_start_time \
            + anim_fsm->current_state->length;

        anim_fsm_call(anim_fsm, enter);
    }
}

void anim_fsm_update(anim_fsm_t *anim_fsm)
{
    assert_not_null(anim_fsm);

    if (!anim_fsm->current_state) {
        return;
    }

    anim_fsm->elapsed_time = current_time - anim_fsm->start_time;

    if (current_time >= anim_fsm->state_end_time) {
        anim_fsm_next_state(anim_fsm);
    } else {
        anim_fsm->state_elapsed_time = current_time - anim_fsm->state_start_time;
        anim_fsm->state_progress = (float)(anim_fsm->state_elapsed_time / anim_fsm->current_state->length);
    }

    anim_fsm_call(anim_fsm, update);
}

void anim_fsm_draw(anim_fsm_t *anim_fsm)
{
    assert_not_null(anim_fsm);
    anim_fsm_call(anim_fsm, draw);
}

void anim_fsm_start(anim_fsm_t *anim_fsm)
{
    assert_not_null(anim_fsm);
    anim_fsm->start_time = current_time;
    anim_fsm_change_state(anim_fsm, 0);
}

void anim_fsm_stop(anim_fsm_t *anim_fsm)
{
    assert_not_null(anim_fsm);
    anim_fsm_change_state(anim_fsm, anim_fsm->stop_state_index);
}

void anim_fsm_next_state(anim_fsm_t *anim_fsm)
{
    assert_not_null(anim_fsm);
    anim_fsm_change_state(anim_fsm, anim_fsm_next_state_index(anim_fsm));
}

void anim_fsm_set_state(anim_fsm_t *anim_fsm, int state_index)
{
    assert_not_null(anim_fsm);
    anim_fsm_change_state(anim_fsm, state_index);
}


void anim_fsm_shift_current_time_preserving_progress(anim_fsm_t *anim_fsm, float delta_time)
{
    anim_fsm->start_time += delta_time;
    anim_fsm->state_start_time += delta_time;
    anim_fsm->state_end_time += delta_time;
}
