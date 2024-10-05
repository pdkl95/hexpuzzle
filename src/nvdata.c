/****************************************************************************
 *                                                                          *
 * nvdata.c                                                                 *
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

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

#include "options.h"
#include "nvdata.h"
#include "nvdata_finished.h"

char *local_config_dir = NULL;
char *nvdata_dir = NULL;
char *nvdata_state_file_path = NULL;

static void find_local_config_dir(void)
{
    if (local_config_dir) {
        FREE(local_config_dir);
    }

    char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    if (xdg_config_home && DirectoryExists(xdg_config_home)) {
        local_config_dir = strdup(xdg_config_home);
        return;
    }

    char *home = getenv("HOME");
    if (!home) {
        errmsg("cannot fine nvdata directory - env variable HOME is missing?!");
        return;
    }

    asprintf(&local_config_dir, "%s/.config", PACKAGE_NAME);
}

static void find_nvdata_dir(void)
{
    if (nvdata_dir) {
        FREE(nvdata_dir);
    }

    if (options->nvdata_dir) {
        nvdata_dir = strdup(options->nvdata_dir);
        return;
    }

    find_local_config_dir();

    asprintf(&nvdata_dir, "%s/%s", local_config_dir, PACKAGE_NAME);

    if (DirectoryExists(nvdata_dir)) {
        infomsg("Found local config dir: \"%s\"", nvdata_dir);
    } else {
        warnmsg("Creating config storage dir: \"%s\"", nvdata_dir);

        if (-1 == mkdir(nvdata_dir, CREATE_DIR_MODE)) {
            errmsg("Error in mkdir(2): %s", strerror(errno));
        }
    }
}

struct nvdata_state {
    int32_t window_size_width;
    int32_t window_size_height;
    int32_t window_position_x;
    int32_t window_position_y;
};
typedef struct nvdata_state nvdata_state_t;

static void nvdata_limit_state(nvdata_state_t *state)
{
    CLAMPVAR(state->window_size_width,  500, window_size.x);
    CLAMPVAR(state->window_size_height, 500, window_size.y);
    CLAMPVAR(state->window_position_x, 0, (window_size.x - 32));
    CLAMPVAR(state->window_position_y, 0, (window_size.y - 32));
}

static void load_nvdata_program_state(void)
{
    if (!FileExists(nvdata_state_file_path)) {
        if (options->verbose) {
            infomsg("Skipping loading state file (\"%s\" doesn't exist)",
                    nvdata_state_file_path);
        }
        return;
    }

    int size;
    unsigned char *data = LoadFileData(nvdata_state_file_path, &size);
    if (size != sizeof(nvdata_state_t)) {
        errmsg("Ignoring state file \"%s\" - size was %d bytes; expected %d bytes.",
               nvdata_state_file_path, size, sizeof(nvdata_state_t));
        return;
    }

    nvdata_state_t state;
    memcpy(&state, data, sizeof(nvdata_state_t));
    UnloadFileData(data);

    nvdata_limit_state(&state);

    SetWindowPosition( state.window_position_x,
                       state.window_position_y );

    SetWindowSize( state.window_size_width,
                   state.window_size_height );
}

static void save_nvdata_program_state(void)
{
    nvdata_state_t state;

    Vector2 wpos = GetWindowPosition();
    state.window_position_x = (int32_t)wpos.x;
    state.window_position_y = (int32_t)wpos.y;

    state.window_size_width  = GetScreenWidth();
    state.window_size_height = GetScreenHeight();

    nvdata_limit_state(&state);

    if (options->verbose) {
        infomsg("Saving program state to \"%s\": ", nvdata_state_file_path);
    }
    bool saveok = SaveFileData(nvdata_state_file_path, &state, sizeof(state));
    if (saveok) {
        if (options->verbose) {
            infomsg("Successfully saved state to \"%s\"", nvdata_state_file_path);
        }
    } else {
        errmsg("Failed to save state to \"%s\"", nvdata_state_file_path);
    }
}

static void load_nvdata_game_metadata(void)
{
    load_nvdata_finished_levels();
}

static void save_nvdata_game_metadata(void)
{
    save_nvdata_finished_levels();
}

void init_nvdata(void)
{
    find_nvdata_dir();

    if (nvdata_dir == NULL) {
        warnmsg("Couldn't find the config-dir; persistent game data will not be loaded!");

        if (local_config_dir == NULL) {
            errmsg("Also couldn't find the directory where programs keep their config data!\nMaybe try providing the path to a local where "  PACKAGE_NAME " config directory should be stored with the --config-dir=/path/.../ option?");
            return;
        }
    }

    if (nvdata_state_file_path == NULL) {
        asprintf(&nvdata_state_file_path, "%s/%s", nvdata_dir, NVDATA_STATE_FILE_NAME);
    }

    if (nvdata_state_finished_levels_file_path == NULL) {
        asprintf(&nvdata_state_finished_levels_file_path, "%s/%s",
                 nvdata_dir, NVDATA_FINISHED_LEVEL_FILE_NAME);
    }


    init_nvdata_finished();
}

void load_nvdata(void)
{
    load_nvdata_program_state();
    load_nvdata_game_metadata();
}

void save_nvdata(void)
{
    if (nvdata_dir && DirectoryExists(nvdata_dir)) {
        save_nvdata_program_state();
        save_nvdata_game_metadata();
    }
}
