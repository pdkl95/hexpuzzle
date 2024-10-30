/****************************************************************************
 *                                                                          *
 * game_mode.h                                                              *
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

#ifndef GAME_MODE_H
#define GAME_MODE_H

enum game_mode {
    GAME_MODE_NULL = 0,
    GAME_MODE_PLAY_COLLECTION,
    GAME_MODE_EDIT_COLLECTION,
    GAME_MODE_WIN_LEVEL,
    GAME_MODE_PLAY_LEVEL,
    GAME_MODE_EDIT_LEVEL,
    GAME_MODE_BROWSER,
    GAME_MODE_OPTIONS,
    GAME_MODE_RANDOM
};
typedef enum game_mode game_mode_t;

extern game_mode_t game_mode;
extern game_mode_t last_game_mode;

#define play_level_mode    (game_mode == GAME_MODE_PLAY_LEVEL)
#define win_level_mode     (game_mode == GAME_MODE_PLAY_LEVEL)
#define play_mode          (play_level_mode || win_level_mode)

#define edit_mode    (game_mode == GAME_MODE_EDIT_LEVEL)
#define browser_mode (game_mode == GAME_MODE_BROWSER)
#define options_mode (game_mode == GAME_MODE_OPTIONS)
#define random_mode  (game_mode == GAME_MODE_RANDOM)

char *game_mode_t_str(game_mode_t mode);
char *game_mode_str(void);
void set_game_mode(game_mode_t new_mode);
void set_game_mode_save_prev(game_mode_t new_mode);
void prev_game_mode(void);
void toggle_edit_mode(void);

#endif /*GAME_MODE_H*/

