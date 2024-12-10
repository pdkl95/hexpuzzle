/****************************************************************************
 *                                                                          *
 * gui_random.h                                                             *
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

#ifndef GUI_RANDOM_H
#define GUI_RANDOM_H

void init_gui_random_minimal(void);
void init_gui_random(void);
void cleanup_gui_random(void);
void resize_gui_random(void);
void draw_gui_random(void);

struct level *generate_random_level(void);
struct level *generate_random_title_level(void);

void play_gui_random_level(void);

#endif /*GUI_RANDOM_H*/

