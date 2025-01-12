/****************************************************************************
 *                                                                          *
 * range.h                                                                  *
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

#ifndef RANGE_H
#define RANGE_H

#include "cJSON/cJSON.h"

struct int_range {
    int min;
    int max;
};
typedef struct int_range int_range_t;

const char *int_range_string(int_range_t *ir);
cJSON *int_range_to_json(int_range_t *ir);
bool int_range_from_json(cJSON *json, int_range_t *ir);
void int_range_clamp(int_range_t *ir, int min, int max);

#endif /*RANGE_H*/

