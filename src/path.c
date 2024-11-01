/****************************************************************************
 *                                                                          *
 * path.c                                                                   *
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

#include "common.h"
#include "options.h"
#include "path.h"

char *path_type_name(path_type_t type)
{
    switch (type) {
    default:
        return "(BAD VALUE)";

    case PATH_TYPE_NONE:
        return "NONE";

    case PATH_TYPE_RED:
        return "RED";

    case PATH_TYPE_BLUE:
        return "BLUE";

    case PATH_TYPE_YELLOW:
        return "YELLOW";

    case PATH_TYPE_GREEN:
        return "GREEN";
    }
}
