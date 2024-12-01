/****************************************************************************
 *                                                                          *
 * background.h                                                             *
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

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "anim_fsm.h"

enum background_mode {
    BG_MODE_INIT = 0,
    BG_MODE_NORMAL,
    BG_MODE_WIN
};
typedef enum background_mode background_mode_t;

struct background_control {
    float amp;
};
typedef struct background_control background_control_t;


struct background {
    background_mode_t mode;

    background_control_t *control;

    background_control_t normal_control;
    background_control_t win_control;

    anim_fsm_t anim_fsm;

    float amp;
    int change_counter_frames;
    float change_per_frame;

    Camera camera;

    Color  minor_color;
    Color hmajor_color;
    Color vmajor_color;
};
typedef struct background background_t;


background_t *create_background(void);
void destroy_background(background_t *bg);

void background_resize(background_t *bg);

void background_set_mode(background_t *bg, background_mode_t new_mode);
void background_draw(background_t *bg);


                     
#endif /*BACKGROUND_H*/

