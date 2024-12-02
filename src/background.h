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

struct background {
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
void background_draw(background_t *bg);

                     
#endif /*BACKGROUND_H*/

