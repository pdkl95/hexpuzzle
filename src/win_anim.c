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
#include "physac/physac.h"
#include "options.h"
#include "level.h"
#include "win_anim.h"
#include "shader.h"

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
}

static void win_anim_stay_update(struct anim_fsm *anim_fsm, void *data)
{
    win_anim_t *win_anim = (win_anim_t *)data;

    win_anim->fade[2] = 1.0f;
    win_anim_common_update(anim_fsm, data);
}

static void win_anim_fadein_update(struct anim_fsm *anim_fsm, void *data)
{
    win_anim_t *win_anim = (win_anim_t *)data;

    win_anim->fade[2] = anim_fsm->state_progress;
    win_anim_common_update(anim_fsm, data);
}

anim_fsm_callbacks_t fadein_callbacks = { .update = win_anim_fadein_update };
anim_fsm_callbacks_t   stay_callbacks = { .update = win_anim_stay_update   };

anim_fsm_state_t states[] = {
    { "FADEIN",   5.0, ANIM_FSM_STATE_NEXT, &fadein_callbacks },
    { "HUE",     10.0, ANIM_FSM_STATE_STAY,   &stay_callbacks },
    { "STOP",     0.0, ANIM_FSM_STATE_STOP,              NULL }
};

win_anim_t *create_win_anim(struct level *level)
{
    win_anim_t *win_anim = calloc(1, sizeof(win_anim_t));

    win_anim->running = false;
    win_anim->level = level;

    win_anim->fade[0] = 0.0;
    win_anim->fade[1] = 0.0;
    win_anim->fade[2] = 0.0;
    win_anim->fade[3] = 0.0;

    init_anim_fsm(&win_anim->anim_fsm, states, NULL, win_anim);

    return win_anim;
}

void destroy_win_anim(win_anim_t *win_anim)
{
    SAFEFREE(win_anim);
}

void win_anim_update(win_anim_t *win_anim)
{
    assert_not_null(win_anim);

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
    }
}

void win_anim_stop(win_anim_t *win_anim)
{
    assert_not_null(win_anim);

    if (win_anim->running) {
        win_anim->running = false;
        anim_fsm_stop(&win_anim->anim_fsm);
    }
}
