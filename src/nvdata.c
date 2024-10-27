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

#include "cJSON/cJSON.h"

#include "options.h"
#include "nvdata.h"
#include "nvdata_finished.h"

uint16_t state_version = 1;

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

#if defined(PLATFORM_DESKTOP)
    char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    if (xdg_config_home && DirectoryExists(xdg_config_home)) {
        local_config_dir = strdup(xdg_config_home);
        return;
    }

    char *home = getenv("HOME");
    if (!home) {
        errmsg("cannot find nvdata directory - env variable HOME is missing?!");
        return;
    }

    asprintf(&local_config_dir, "%s/.config", PACKAGE_NAME);
#else
# if defined(PLATFORM_WEB)
    asprintf(&local_config_dir, "/%s", progname);
# else
#  error "Unsupported platform! (no file layout information)"
# endif
#endif
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

static bool program_state_from_json(cJSON *json)
{
    if (!cJSON_IsObject(json)) {
        errmsg("Error parsing program state JSON: not an Object");
        return false;
    }

    cJSON *version_json = cJSON_GetObjectItemCaseSensitive(json, "version");
    if (version_json) {
        if (!cJSON_IsNumber(version_json)) {
            errmsg("Error parsing level JSON: 'version' is not a Number");
            return false;
        }

        if (version_json->valueint != state_version) {
            errmsg("Program state JSON is version %d, expected %d",
                   version_json->valueint, state_version);
        }
    } else {
        warnmsg("Program state JSON is missing \"version\"");
    }

    cJSON *window_json = cJSON_GetObjectItemCaseSensitive(json, "window");
    if (window_json) {
        cJSON  *width_json = cJSON_GetObjectItemCaseSensitive(window_json, "width");
        cJSON *height_json = cJSON_GetObjectItemCaseSensitive(window_json, "height");

        if (!width_json) {
            warnmsg("Program state JSON['window'] is missing \"width\"");
        }
        if (!height_json) {
            warnmsg("Program state JSON['window'] is missing \"height\"");
        }

        if (width_json && height_json) {
            bool w_ok = cJSON_IsNumber(width_json);
            bool h_ok = cJSON_IsNumber(height_json);
            if (!w_ok) {
                errmsg("Program state JSON['window'] \"width\" is not a NUMBER");
            }
            if (!h_ok) {
                errmsg("Program state JSON['window'] \"height\" is not a NUMBER");
            }
            if (w_ok && h_ok) {
                int scr_width  = width_json->valueint;
                int scr_height = height_json->valueint;

                CLAMPVAR(scr_width,  500, window_size.x);
                CLAMPVAR(scr_height, 500, window_size.y);

                SetWindowSize(scr_width, scr_height);
            } else {
                return false;
            }
        }

        cJSON *position_x_json = cJSON_GetObjectItemCaseSensitive(window_json, "position_x");
        cJSON *position_y_json = cJSON_GetObjectItemCaseSensitive(window_json, "position_y");

        if (!position_x_json) {
            warnmsg("Program state JSON['window'] is missing \"position_x\"");
        }
        if (!position_y_json) {
            warnmsg("Program state JSON['window'] is missing \"position_y\"");
        }

        if (position_x_json && position_y_json) {
            bool x_ok = cJSON_IsNumber(position_x_json);
            bool y_ok = cJSON_IsNumber(position_y_json);
            if (!x_ok) {
                errmsg("Program state JSON['window'] \"position_x\" is not a NUMBER");
            }
            if (!y_ok) {
                errmsg("Program state JSON['window'] \"position_y\" is not a NUMBER");
            }
            if (x_ok && y_ok) {
                IVector2 wpos = {
                    .x = position_x_json->valueint,
                    .y = position_y_json->valueint
                };

                CLAMPVAR(wpos.x, 0, (window_size.x - 32));
                CLAMPVAR(wpos.y, 0, (window_size.y - 32));

                SetWindowPosition(wpos.x, wpos.y);
            } else {
                return false;
            }
        }
    }

    cJSON *graphics_json = cJSON_GetObjectItemCaseSensitive(json, "graphics");
    if (graphics_json) {
        cJSON *bool_json = NULL;

#define mk_bool_json(field, name)                                           \
    bool_json = cJSON_GetObjectItemCaseSensitive(graphics_json, STR(name)); \
    if (bool_json) {                                                        \
        if (cJSON_IsBool(bool_json)) {                                      \
            if (cJSON_IsTrue(bool_json)) {                                  \
                options->field = true;                                      \
            } else {                                                        \
                options->field = false;                                     \
            }                                                               \
        } else {                                                            \
            errmsg("Program state JSON['graphics'] \"%s\" is not a BOOL",   \
                   STR(name));                                              \
        }                                                                   \
    } else {                                                                \
        warnmsg("Program state JSON['graphgics'] is missing \"%s\"",        \
                STR(name));                                                 \
    }

        if (options->load_state_physics_effects) {
            mk_bool_json(physics_effects, physics_effects);
        }
        if (options->load_state_animate_bg) {
            mk_bool_json(animate_bg, animate_background);
        }
        if (options->load_state_animate_win) {
            mk_bool_json(animate_win, animate_win);
        }
#undef mk_bool_json
    }

    return true;
}

static void load_nvdata_program_state(void)
{
    if (!FileExists(nvdata_state_file_path)) {
        if (options->verbose) {
            infomsg("Skipping load program state: \"%s\" doesn't exist",
                    nvdata_state_file_path);
        }
        return;
    }

    char *str = LoadFileText(nvdata_state_file_path);
    if (!str) {
        errmsg("Failed loading program state file \"%s\"",
               nvdata_state_file_path);
        return;
    }

    cJSON *json = cJSON_Parse(str);
    free(str);
    if (!json) {
        errmsg("Failed parsing program state file \"%s\"",
               nvdata_state_file_path);
        return;
    }

    bool rv = program_state_from_json(json);
    cJSON_Delete(json);

    if (rv) {
        if (options->verbose) {
            infomsg("Successfully loaded program state");
        }
    } else {
        errmsg("Error loading program state");
    }
}

static cJSON *program_state_to_json(void)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(json, "version", state_version) == NULL) {
        errmsg("Error adding \"version\" to JSON");
        goto to_json_error;
    }

    cJSON *window_json = cJSON_AddObjectToObject(json, "window");
    if (!window_json) {
        errmsg("Error adding \"window\" object to JSON");
        goto to_json_error;
    }

    IVector2 wpos = vector2_to_ivector2(GetWindowPosition());
    CLAMPVAR(wpos.x, 0, (window_size.x - 32));
    CLAMPVAR(wpos.y, 0, (window_size.y - 32));

    if (cJSON_AddNumberToObject(window_json, "position_x", wpos.x) == NULL) {
        errmsg("Error adding \"position_x\" to JSON.window");
        goto to_json_error;
    }

    if (cJSON_AddNumberToObject(window_json, "position_y", wpos.y) == NULL) {
        errmsg("Error adding \"position_y\" to JSON.window");
        goto to_json_error;
    }

    int scr_width = GetScreenWidth();
    int scr_height = GetScreenHeight();
    CLAMPVAR(scr_width,  500, window_size.x);
    CLAMPVAR(scr_height, 500, window_size.y);

    if (cJSON_AddNumberToObject(window_json, "width", scr_width) == NULL) {
        errmsg("Error adding \"width\" to JSON.window");
        goto to_json_error;
    }

    if (cJSON_AddNumberToObject(window_json, "height", scr_height) == NULL) {
        errmsg("Error adding \"height\" to JSON.window");
        goto to_json_error;
    }

    cJSON *graphics_json = cJSON_AddObjectToObject(json, "graphics");
    if (!graphics_json) {
        errmsg("Error adding \"graphics\" object to JSON");
        goto to_json_error;
    }

    cJSON *bool_json = NULL;

#define mk_bool_json(field, name)                                       \
    if (options->field) {                                               \
        bool_json = cJSON_AddTrueToObject(graphics_json, STR(name));    \
    } else {                                                            \
        bool_json = cJSON_AddFalseToObject(graphics_json, STR(name));   \
    }                                                                   \
    if (!bool_json) {                                                   \
        errmsg("Error adding bool \"%s\" to JSON", STR(name));          \
        goto to_json_error;                                             \
    }

    mk_bool_json(physics_effects, physics_effects);
    mk_bool_json(animate_bg, animate_background);
    mk_bool_json(animate_win, animate_win);
#undef mk_bool_json

    return json;

  to_json_error:
    if (json) {
        cJSON_Delete(json);
    }
    return NULL;
}

static void save_nvdata_program_state(void)
{
    char *tmpname;
    asprintf(&tmpname, "%s.tmp", nvdata_state_file_path);

    cJSON *json = program_state_to_json();
    if (!json) {
        errmsg("Failed to convert program state to JSON");
        return;
    }

    char *json_str = cJSON_Print(json);
    if (!json_str) {
        errmsg("Failed to print program state JSON to a string");
        cJSON_Delete(json);
        return;
    }

    if (options->verbose) {
        infomsg("Saving program state to: \"%s\"", nvdata_state_file_path);
    }

    SaveFileText(tmpname, json_str);

    free(json_str);
    cJSON_Delete(json);

    if (tmpname) {
        if (-1 == rename(tmpname, nvdata_state_file_path)) {
            errmsg("Error trying to rename \"%s\" to \"%s\" - ",
                   tmpname, nvdata_state_file_path);
        }

        free(tmpname);
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

    if (nvdata_default_browse_path == NULL) {
        asprintf(&nvdata_default_browse_path, "%s/%s",
                 nvdata_dir, NVDATA_DEFAULT_BROWSE_PATH_NAME);
    }

    find_or_create_dir(nvdata_default_browse_path, "level file");


    init_nvdata_finished();
}

void cleanup_nvdata(void)
{
    cleanup_nvdata_finished();

    SAFEFREE(local_config_dir);
    SAFEFREE(nvdata_dir);
    SAFEFREE(nvdata_state_file_path);
    SAFEFREE(nvdata_default_browse_path);

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
