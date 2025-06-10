/****************************************************************************
 *                                                                          *
 * blueprint_string.h                                                       *
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

#ifndef BLUEPRINT_STRING_H
#define BLUEPRINT_STRING_H

#include "generate_level.h"

#define BLUEPRINT_STRING_MAXLEN 1024

const char *serialize_generate_level_params(generate_level_param_t param);
bool deserialize_generate_level_params(const char *str, generate_level_param_t *result);;

#endif /*BLUEPRINT_STRING_H*/

