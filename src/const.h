/****************************************************************************
 *                                                                          *
 * const.h                                                                  *
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

#ifndef CONST_H
#define CONST_H

#define LEVEL_FILENAME_EXT      "hexlevel"
#define COLLECTION_FILENAME_EXT "hexlevelpack"

#define LEVEL_MIN_RADIUS 1
#define LEVEL_MAX_RADIUS 4

#define TILE_GRID_WIDTH  ((2 * LEVEL_MAX_RADIUS) + 1)
#define TILE_GRID_HEIGHT TILE_GRID_WIDTH

#define LEVEL_CENTER_POSITION  \
    ((hex_axial_t){            \
        .q = LEVEL_MAX_RADIUS, \
        .r = LEVEL_MAX_RADIUS  \
     })

#define NAME_MAXLEN 22
#define UI_NAME_MAXLEN  (6 + NAME_MAXLEN)

#endif /*CONST_H*/

