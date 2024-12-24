/****************************************************************************
 *                                                                          *
 * tile_draw.h                                                              *
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

#ifndef TILE_DRAW_H
#define TILE_DRAW_H

#include "tile.h"
#include "tile_pos.h"
#include "level.h"
#include "win_anim.h"

void tile_draw(tile_pos_t *pos, tile_pos_t *drag_target, bool finished, Color finished_color, float finished_fade_in);
void tile_draw_ghost(tile_pos_t *pos);
void tile_draw_win_anim(tile_pos_t *pos);
void tile_draw_corner_connections(tile_pos_t *pos, win_anim_mode_t win_mode);

#endif /*TILE_DRAW_H*/

