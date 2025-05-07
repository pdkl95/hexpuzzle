/****************************************************************************
 *                                                                          *
 * level_draw.h                                                             *
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
 * with rocks. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#ifndef LEVEL_DRAW_H
#define LEVEL_DRAW_H

void level_draw(level_t *level, bool finishe);
void level_preview(level_t *level, Rectangle rect);
void level_draw_for_title(level_t *level, Rectangle rect);

#endif /*LEVEL_DRAW_H*/

