/****************************************************************************
 *                                                                          *
 * startup_action.h                                                         *
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

#ifndef STARTUP_ACTION_H
#define STARTUP_ACTION_H

enum startup_action {
    STARTUP_ACTION_NONE = 0,
    STARTUP_ACTION_PLAY,
    STARTUP_ACTION_RANDOM,
    STARTUP_ACTION_EDIT,
    STARTUP_ACTION_CREATE_LEVEL,
    STARTUP_ACTION_PACK_COLLECTION,
    STARTUP_ACTION_UNPACK_COLLECTION
};
typedef enum startup_action startup_action_t;

enum create_level_mode {
    CREATE_LEVEL_MODE_NULL = 0,
    CREATE_LEVEL_MODE_BLANK,
    CREATE_LEVEL_MODE_RANDOM
};
typedef enum create_level_mode create_level_mode_t;

extern bool startup_action_ok;

//create_level_mode_t parse_create_level_mode(const char *str);
bool run_startup_action(void);

#endif /*STARTUP_ACTION_H*/

