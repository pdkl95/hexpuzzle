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

#include "config.h"

#include "cJSON/cJSON.h"

#ifdef USE_PHYSICS
#include "physics.h"
#endif

enum win_anim_state {
    WIN_ANIM_STATE_NULL     = 0,
    WIN_ANIM_STATE_STANDBY  = 1,
    /*
     * NOTE: all 'running' states MUST
     *       be >= all non-'running' states
     */
#define WIN_ANIM_MIN_RUNNING_STATE WIN_ANIM_STATE_STARTUP
    WIN_ANIM_STATE_STARTUP  = 2,
    WIN_ANIM_STATE_RUNNING  = 3,
    WIN_ANIM_STATE_SHUTDOWN = 4
};
typedef enum win_anim_state win_anim_state_t;

enum win_anim_mode {
    WIN_ANIM_MODE_NULL          = 0,
    WIN_ANIM_MODE_SIMPLE        = 1,
    WIN_ANIM_MODE_POPS          = 2,
    WIN_ANIM_MODE_WAVES         = 3,
    WIN_ANIM_MODE_SPIN          = 4,
#ifdef USE_PHYSICS
    WIN_ANIM_MODE_PHYSICS_FALL  = 5,
    WIN_ANIM_MODE_PHYSICS_SWIRL = 6
#endif
};
typedef enum win_anim_mode win_anim_mode_t;

#define WIN_ANIM_NON_PHYSICS_MODE_COUNT 5

#ifdef USE_PHYSICS
# define WIN_ANIM_PHYSICS_MODE_COUNT 2
#else
# define WIN_ANIM_PHYSICS_MODE_COUNT 0
#endif

#define WIN_ANIM_MODE_COUNT (WIN_ANIM_NON_PHYSICS_MODE_COUNT + WIN_ANIM_PHYSICS_MODE_COUNT)

#include "win_anim_mode_config.h"

struct win_anim {
    int id;
    win_anim_mode_t mode;
    win_anim_mode_config_t *mode_config;
    int total_mode_chances[2];
    bool use_background_3d;
    struct level *level;
    win_anim_state_t state;
    float state_change_time;
    float activation;
    float fade[4];
    float start_time;
    float run_time;
#ifdef USE_PHYSICS
    physics_t physics;
#endif
};
typedef struct win_anim win_anim_t;

const char *win_anim_mode_str(win_anim_mode_t mode);
void print_win_anim(win_anim_t *win_anim);

win_anim_t *create_win_anim(struct level *level);
void cleanup_win_anim(win_anim_t *win_anim);
void destroy_win_anim(win_anim_t *win_anim);

void win_anim_select_random_mode(win_anim_t *win_anim, bool animated);
void win_anim_update(win_anim_t *win_anim);
void win_anim_draw(win_anim_t *win_anim);

void win_anim_start(win_anim_t *win_anim);
void win_anim_stop(win_anim_t *win_anim);

static inline bool win_anim_running(win_anim_t *win_anim)
{
    return (win_anim != NULL) && (win_anim->state >= WIN_ANIM_MIN_RUNNING_STATE);
}

#endif /*WIN_ANIM_H*/
