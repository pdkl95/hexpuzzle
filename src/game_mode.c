/****************************************************************************
 *                                                                          *
 * game_mode.c                                                              *
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

#include "common.h"
#include "level.h"
#include "game_mode.h"
#include "gui_collection.h"

//#define DEBUG_GAME_MODE

game_mode_t game_mode = GAME_MODE_NULL;
game_mode_t last_game_mode = GAME_MODE_NULL;
game_mode_t options_restore_mode = GAME_MODE_NULL;

char *game_mode_t_str(game_mode_t mode)
{
    switch (mode) {
    case GAME_MODE_NULL:
        return "NULL";

    case GAME_MODE_PLAY_COLLECTION:
        return "PLAY_COLLECTION";

    case GAME_MODE_EDIT_COLLECTION:
        return "EDIT_COLLECTION";

    case GAME_MODE_WIN_LEVEL:
        return "WIN_LEVEL";

    case GAME_MODE_PLAY_LEVEL:
        return "PLAY_LEVEL";

    case GAME_MODE_EDIT_LEVEL:
        return "EDIT_LEVEL";

    case GAME_MODE_TITLE:
        return "TITLE";

    case GAME_MODE_BROWSER:
        return "BROWSER";

    case GAME_MODE_OPTIONS:
        return "OPTIONS";

    case GAME_MODE_RANDOM:
        return "RANDOM";

    default:
        __builtin_unreachable();
    }
}

static void on_enter_play_collection_mode(void)
{
    gui_collection_update_level_preview();
}

static void on_enter_edit_collection_mode(void)
{
    gui_collection_update_level_preview();
}

static void on_enter_win_level_mode(void)
{
    enable_postprocessing();
}

static void on_exit_win_level_mode(void)
{
    disable_postprocessing();
    if (current_level) {
        level_reset_win_anim(current_level);
    }
}

static void _set_game_mode(game_mode_t new_mode)
{
#ifdef DEBUG_GAME_MODE
    printf("--\nGM: _SET(%s)\told = %s\tlast = %s\n",
           game_mode_t_str(new_mode),
           game_mode_t_str(game_mode),
           game_mode_t_str(last_game_mode));
#endif

    if (new_mode == game_mode) {
        return;
    }

    game_mode_t old_mode = game_mode;

    switch (new_mode) {
    case GAME_MODE_OPTIONS:
        options_restore_mode = old_mode;
        game_mode = GAME_MODE_OPTIONS;
        return;

    default:
        break;
    }

    switch (old_mode) {
    case GAME_MODE_OPTIONS:
        options_restore_mode = GAME_MODE_NULL;
        break;

    case GAME_MODE_PLAY_LEVEL:
        if (new_mode == GAME_MODE_WIN_LEVEL) {
            /* preserve last_game_mode */
        } else {
            last_game_mode = old_mode;
        }
        break;


    case GAME_MODE_WIN_LEVEL:
        if (new_mode == GAME_MODE_PLAY_LEVEL) {
            /* preserve last_game_mode */
        } else {
            last_game_mode = old_mode;
        }
        break;

    default:
        last_game_mode = old_mode;
        break;
    }

    /* exit events */
    switch (old_mode) {
    case GAME_MODE_WIN_LEVEL:
        on_exit_win_level_mode();
        break;

    default:
        break;
    }

    /* enter events */
    switch (new_mode) {
    case GAME_MODE_PLAY_COLLECTION:
        on_enter_play_collection_mode();
        break;

    case GAME_MODE_EDIT_COLLECTION:
        on_enter_edit_collection_mode();
        break;

    case GAME_MODE_WIN_LEVEL:
        on_enter_win_level_mode();
        break;

    default:
        break;
    }

    game_mode = new_mode;
}

void set_game_mode(game_mode_t new_mode)
{
    _set_game_mode(new_mode);

#ifdef DEBUG_GAME_MODE
    printf("GM:   >> SET game_mode = %s (prev = %s)\n",
           game_mode_t_str(game_mode),
           game_mode_t_str(last_game_mode));
#endif
}

void prev_game_mode(void)
{
    switch (game_mode) {
    case GAME_MODE_OPTIONS:
        _set_game_mode(options_restore_mode);
        break;

    default:
        _set_game_mode(last_game_mode);
        break;
    }

#ifdef DEBUG_GAME_MODE
    printf("GM:   >> POP game_mode = %s (prev = %s)\n",
           game_mode_t_str(game_mode),
           game_mode_t_str(last_game_mode));
#endif
}

char *game_mode_str(void)
{
    return game_mode_t_str(game_mode);
}

void toggle_edit_mode(void)
{
    switch (game_mode) {
    case GAME_MODE_NULL:
        warnmsg("Trying to toggle edit mode in NULL game mode?!");
        break;

    case GAME_MODE_PLAY_COLLECTION:
        set_game_mode(GAME_MODE_EDIT_COLLECTION);
        break;

    case GAME_MODE_EDIT_COLLECTION:
        set_game_mode(GAME_MODE_PLAY_COLLECTION);
        break;

    case GAME_MODE_WIN_LEVEL:
        /* fall through */
    case GAME_MODE_PLAY_LEVEL:
        set_game_mode(GAME_MODE_EDIT_LEVEL);
        break;

    case GAME_MODE_EDIT_LEVEL:
        set_game_mode(GAME_MODE_PLAY_LEVEL);
        break;

    default:
        /* do nothing */
        __builtin_unreachable();
    }
}

