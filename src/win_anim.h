/****************************************************************************
 *                                                                          *
 * win_anim.h                                                               *
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

#ifndef WIN_ANIM_H
#define WIN_ANIM_H

#include "anim_fsm.h"

enum win_anim_mode {
    WIN_ANIM_MODE_SIMPLE        = 0,
    WIN_ANIM_MODE_POPS          = 1,
    WIN_ANIM_MODE_WAVES         = 2,
#ifdef USE_PHYSICS
    WIN_ANIM_MODE_PHYSICS_FALL  = 3,
    WIN_ANIM_MODE_PHYSICS_SWIRL = 4
#endif
};
typedef enum win_anim_mode win_anim_mode_t;

#ifdef USE_PHYSICS
# define WIN_ANIM_MODE_COUNT 5
#else
# define WIN_ANIM_MODE_COUNT 3
#endif

struct win_anim {
    win_anim_mode_t mode;
    anim_fsm_t anim_fsm;
    struct level *level;
    bool running;
    float fade[4];
    float start_time;
};
typedef struct win_anim win_anim_t;

void print_win_anim(win_anim_t *win_anim);

win_anim_t *create_win_anim(struct level *level);
void destroy_win_anim(win_anim_t *win_anim);

void win_anim_select_random_mode(win_anim_t *win_anim);
void win_anim_update(win_anim_t *win_anim);
void win_anim_draw(win_anim_t *win_anim);

bool win_anim_running(win_anim_t *win_anim);

void win_anim_start(win_anim_t *win_anim);
void win_anim_stop(win_anim_t *win_anim);

#endif /*WIN_ANIM_H*/

