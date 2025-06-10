/****************************************************************************
 *                                                                          *
 * generate_level.h                                                         *
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

#ifndef GENERATE_LEVEL_H
#define GENERATE_LEVEL_H

#include "range.h"

enum generate_level_mode {
    GENERATE_LEVEL_BLANK                    = 0,
    GENERATE_LEVEL_RANDOM_CONNECT_TO_POINT  = 1
};
typedef enum generate_level_mode generate_level_mode_t;

struct generate_level_param {
    generate_level_mode_t mode;

    uint64_t seed;
    uint64_t series;

    int tile_radius;

    int_range_t fixed;
    int_range_t hidden;

    int fixed_count;
    int hidden_count;

    long path_density;

    bool color[PATH_TYPE_COUNT];
    int  color_count;

    symmetry_mode_t symmetry_mode;

    bool fill_all_tiles;
};
typedef struct generate_level_param generate_level_param_t;

const char *symmetry_mode_string(symmetry_mode_t mode);
symmetry_mode_t parse_symmetry_mode_string(const char *string);

bool parse_random_seed_str(char *seedstr, uint64_t *dst);

struct level *generate_random_level(generate_level_param_t *param, const char *purpose);
struct level *generate_random_level_simple(const char *purpose);

struct level *generate_blank_level(void);
struct level *generate_random_title_level(void);

#endif /*GENERATE_LEVEL_H*/

