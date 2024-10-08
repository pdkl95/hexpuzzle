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
#include <arpa/inet.h>

#include "options.h"
#include "nvdata.h"
#include "nvdata_finished.h"

char *local_config_dir = NULL;
char *nvdata_dir = NULL;
char *nvdata_state_file_path = NULL;
char *nvdata_default_browse_path = NULL;

static void find_or_create_dir(const char *path, const char *desc)
{
    if (DirectoryExists(path)) {
        infomsg("Found %s dir: \"%s\"", desc, path);
    } else {
        warnmsg("Creating %s dir: \"%s\"", desc, path);

        if (-1 == mkdir(path, CREATE_DIR_MODE)) {
            errmsg("Error creating \"%s\" in mkdir(2): %s", path, strerror(errno));
        }
    }
}

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

    find_or_create_dir(nvdata_dir, "local storage");
}

/* increment when struct nvdata_state changes to invalidate
   any old (incompatable) state file loading */
uint16_t state_version = 2;

struct nvdata_state {
    uint16_t window_size_width;
    uint16_t window_size_height;
    uint16_t window_position_x;
    uint16_t window_position_y;

    uint16_t animate_bg;
    uint16_t animate_win;
};
typedef struct nvdata_state nvdata_state_t;

static nvdata_state_t nvdata_state_hton(nvdata_state_t hstate)
{
    nvdata_state_t nstate;

    nstate.window_size_width  = htons(hstate.window_size_width);
    nstate.window_size_height = htons(hstate.window_size_height);
    nstate.window_position_x  = htons(hstate.window_position_x);
    nstate.window_position_y  = htons(hstate.window_position_y);
    nstate.animate_bg         = htons(hstate.animate_bg);
    nstate.animate_win        = htons(hstate.animate_win);

    return nstate;
}

static nvdata_state_t nvdata_state_ntoh(nvdata_state_t nstate)
{
    nvdata_state_t hstate;

    hstate.window_size_width  = ntohs(nstate.window_size_width);
    hstate.window_size_height = ntohs(nstate.window_size_height);
    hstate.window_position_x  = ntohs(nstate.window_position_x);
    hstate.window_position_y  = ntohs(nstate.window_position_y);
    hstate.animate_bg         = ntohs(nstate.animate_bg);
    hstate.animate_win        = ntohs(nstate.animate_win);

    return hstate;
}

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

    FILE *f = fopen(nvdata_state_file_path, "r");
    if (NULL == f) {
        errmsg("error opening \"%s\": %s", nvdata_state_file_path, strerror(errno));
        return;
    }

    uint16_t nfile_state_version;
    size_t rdsize = sizeof(nfile_state_version);
    if (rdsize != fread(&nfile_state_version, 1, rdsize, f)) {
        errmsg("fread(state_version) failed: %s", strerror(errno));
        fclose(f);
        return;
    }

    uint16_t file_state_version = ntohs(nfile_state_version);

    if (file_state_version != state_version) {
        warnmsg("Cannot load old version %d state file \"%s\"; expected version %d",
                file_state_version, nvdata_state_file_path, state_version);
        fclose(f);
        return;
    }

    nvdata_state_t nstate;
    rdsize = sizeof(nstate);
    if (rdsize != fread(&nstate, 1, rdsize, f)) {
        errmsg("fread(state) failed: %s", strerror(errno));
        fclose(f);
        return;
    }

    nvdata_state_t state = nvdata_state_ntoh(nstate);
    nvdata_limit_state(&state);

    SetWindowPosition( state.window_position_x,
                       state.window_position_y );

    SetWindowSize( state.window_size_width,
                   state.window_size_height );

    if (options->load_state_animate_bg) {
        options->animate_bg  = state.animate_bg;
    }
    if (options->load_state_animate_win) {
        options->animate_win = state.animate_win;
    }
}

static void save_nvdata_program_state(void)
{
    nvdata_state_t hstate;

    Vector2 wpos = GetWindowPosition();
    hstate.window_position_x = (int32_t)wpos.x;
    hstate.window_position_y = (int32_t)wpos.y;

    hstate.window_size_width  = GetScreenWidth();
    hstate.window_size_height = GetScreenHeight();

    hstate.animate_bg  = options->animate_bg;
    hstate.animate_win = options->animate_win;

    nvdata_limit_state(&hstate);
    nvdata_state_t nstate = nvdata_state_hton(hstate);

    if (options->verbose) {
        infomsg("Saving program state to \"%s\": ", nvdata_state_file_path);
    }

    FILE *f = fopen(nvdata_state_file_path, "w");
    if (NULL == f) {
        errmsg("Failed to save state to \"%s\": %s",
               nvdata_state_file_path, strerror(errno));
        return;
    }

    uint16_t nstate_version = htons(state_version);
    size_t wrsize = sizeof(nstate_version);
    if (wrsize != fwrite(&nstate_version, 1, wrsize, f)) {
        errmsg("fwrite(state_version) failed: %s", strerror(errno));
        goto cleanup;
    }

    wrsize = sizeof(nstate);
    if (wrsize != fwrite(&nstate, 1, wrsize, f)) {
        errmsg("fwrite(state) failed: %s", strerror(errno));
        goto cleanup;
    }

    if (options->verbose) {
        infomsg("Successfully saved state to \"%s\"", nvdata_state_file_path);
    }

  cleanup:
    fclose(f);
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

    if (nvdata_default_browse_path == NULL) {
        asprintf(&nvdata_default_browse_path, "%s/%s",
                 nvdata_dir, NVDATA_DEFAULT_BROWSE_PATH_NAME);
    }

    find_or_create_dir(nvdata_default_browse_path, "level file");


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
