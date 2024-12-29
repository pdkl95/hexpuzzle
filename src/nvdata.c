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
#include <time.h>

#include "cJSON/cJSON.h"

#include "options.h"
#include "level.h"
#include "win_anim.h"
#include "win_anim_mode_config.h"

#include "nvdata.h"
#include "nvdata_finished.h"

uint16_t state_version = 1;

char *local_config_dir = NULL;
char *nvdata_dir = NULL;
char *nvdata_state_file_path = NULL;
char *nvdata_default_browse_path = NULL;
char *nvdata_saved_current_level_path = NULL;

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

    safe_asprintf(&local_config_dir, "%s/.config", PACKAGE_NAME);
#else
# if defined(PLATFORM_WEB)
    safe_asprintf(&local_config_dir, "/%s", progname);
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

    safe_asprintf(&nvdata_dir, "%s/%s", local_config_dir, PACKAGE_NAME);

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
            errmsg("Error parsing program state: JSON['version'] is not a Number");
            return false;
        }

        if (version_json->valueint != state_version) {
            errmsg("Program state JSON is version %d, expected %d",
                   version_json->valueint, state_version);
        }
    } else {
        warnmsg("Program state JSON is missing \"version\"");
    }

    cJSON *current_level_save_file_json = cJSON_GetObjectItemCaseSensitive(json, "current_level_save_file");
    if (current_level_save_file_json) {
        if (!cJSON_IsString(current_level_save_file_json)) {
            errmsg("Error parsing program state: JSON['current_level_save_file_json'] is not a String");
            return false;
        }

        char *filename = cJSON_GetStringValue(current_level_save_file_json);
        if (filename) {
            infomsg("Loading current level save file \"%s\"", filename);

            if (current_level) {
                errmsg("Cannot load - current_level is ocupied ");
            } else {
                level_t *level = load_level_file(filename);
                if (level) {
                    infomsg("Level loaded. Removing transient save file \"%s\"", filename);
                    level_play(level);
                    unlink(filename);
                } else {
                    errmsg("Load failed!");
                }
            }
        } else {
            errmsg("Error parsing program state: JSON['current_level_save_file_json'] is NULL");
        }
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
                errmsg("Program state JSON['window']['width'] is not a NUMBER");
            }
            if (!h_ok) {
                errmsg("Program state JSON['window']['height'] is not a NUMBER");
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
                errmsg("Program state JSON['window']['position_x'] is not a NUMBER");
            }
            if (!y_ok) {
                errmsg("Program state JSON['window']['position_y'] is not a NUMBER");
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

    if (options->load_color_opt) {
        cJSON *colors_json = cJSON_GetObjectItemCaseSensitive(json, "colors");
        if (colors_json) {
            for (int i=1; i<PATH_TYPE_COUNT; i++) {
                const char *field = TextFormat("path_%d", i);
                cJSON *path_json = cJSON_GetObjectItemCaseSensitive(colors_json, field);
                if (path_json) {
                    if (!cJSON_IsString(path_json)) {
                        errmsg("Program state JSON['colors']['%s'] is not a STRING", field);
                        return false;
                    }

                    color_option_set_string(&(options->path_color[i]), path_json->valuestring);
                } else {
                    warnmsg("Program state JSON['window'] is missing \"%s\"", field);
                }
            }
        }
    }

    cJSON *ui_json = cJSON_GetObjectItemCaseSensitive(json, "ui");
    if (ui_json) {
        cJSON *cursor_size_json = cJSON_GetObjectItemCaseSensitive(ui_json, "cursor_size");
        if (cursor_size_json) {
            if (cJSON_IsNumber(cursor_size_json)) {
                int value = cursor_size_json->valueint;
                CLAMPVAR(value, CURSOR_MIN_SCALE, CURSOR_MAX_SCALE);
                options->cursor_scale = value;
            } else {
                errmsg("Program state JSON['ui']['cursor_size'] is not a NUMBER");
            }
        } else {
            warnmsg("Program state JSON['ui'] is missing \"cursor_size\"");
        }

        cJSON *double_click_ms_json = cJSON_GetObjectItemCaseSensitive(ui_json, "double_click_ms");
        if (double_click_ms_json) {
            if (cJSON_IsNumber(double_click_ms_json)) {
                int value = double_click_ms_json->valueint;
                CLAMPVAR(value, DOUBLE_CLICK_MS_MIN, DOUBLE_CLICK_MS_MAX);
                options->double_click_ms = value;
            } else {
                errmsg("Program state JSON['ui']['double_click_ms'] is not a NUMBER");
            }
        } else {
            warnmsg("Program state JSON['ui'] is missing \"double_click_ms\"");
        }

        cJSON *max_win_radius_json = cJSON_GetObjectItemCaseSensitive(ui_json, "max_win_radius");
        if (max_win_radius_json) {
            if (cJSON_IsNumber(max_win_radius_json)) {
                int value = max_win_radius_json->valueint;
                CLAMPVAR(value, LEVEL_MIN_RADIUS, LEVEL_MAX_RADIUS);
                options->max_win_radius = value;
            } else {
                errmsg("Program state JSON['ui']['max_win_radius'] is not a NUMBER");
            }
        } else {
            warnmsg("Program state JSON['ui'] is missing \"max_win_radius\"");
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
            errmsg("Program state JSON['graphics']['%s'] is not a BOOL",    \
                   STR(name));                                              \
        }                                                                   \
    } else {                                                                \
        warnmsg("Program state JSON['graphics'] is missing \"%s\"",         \
                STR(name));                                                 \
    }

        if (options->load_state_animate_bg) {
            mk_bool_json(animate_bg, animate_background);
        }
        if (options->load_state_animate_win) {
            mk_bool_json(animate_win, animate_win);
        }
        if (options->load_state_use_physics) {
            mk_bool_json(use_physics, use_physics);
        }
        if (options->load_state_show_level_previews) {
            mk_bool_json(show_level_previews, show_level_previews);
        }
#undef mk_bool_json
    }

    cJSON *win_anim_json = cJSON_GetObjectItem(json, "win_animation");

    if (win_anim_json) {
        if (!win_anim_config_from_json(win_anim_json)) {
            errmsg("Error parsing program state JSON['win_animation']");
            return false;
        }
    } else {
        warnmsg("Program state JSON is missing \"win_animation\"");
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

    if (nvdata_saved_current_level_path) {
        if (cJSON_AddStringToObject(json, "current_level_save_file", nvdata_saved_current_level_path) == NULL) {
            errmsg("Error adding \"current_level_save_file\" to JSON");
            goto to_json_error;
        }
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

    cJSON *colors_json = cJSON_AddObjectToObject(json, "colors");
    if (!colors_json) {
        errmsg("Error adding \"colors\" object to JSON");
        goto to_json_error;
    }

    for (int i=1; i<PATH_TYPE_COUNT; i++) {
        const char *field = TextFormat("path_%d", i);
        if (cJSON_AddStringToObject(colors_json, field, options->path_color[i].rgb_string) == NULL) {
            errmsg("Error adding \"height\" to JSON.window");
            goto to_json_error;
        }
    }

    cJSON *ui_json = cJSON_AddObjectToObject(json, "ui");
    if (!ui_json) {
        errmsg("Error adding \"ui\" object to JSON");
        goto to_json_error;
    }

    if (cJSON_AddNumberToObject(ui_json, "cursor_size", options->cursor_scale) == NULL) {
        errmsg("Error adding \"cursor_size\" to JSON.ui");
        goto to_json_error;
    }

    if (cJSON_AddNumberToObject(ui_json, "double_click_ms", options->double_click_ms) == NULL) {
        errmsg("Error adding \"double_click_ms\" to JSON.ui");
        goto to_json_error;
    }

    if (cJSON_AddNumberToObject(ui_json, "max_win_radius", options->max_win_radius) == NULL) {
        errmsg("Error adding \"max_win_radius\" to JSON.ui");
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
        errmsg("Error adding bool \"%s\" to JSON.graphics", STR(name)); \
        goto to_json_error;                                             \
    }

    mk_bool_json(animate_bg, animate_background);
    mk_bool_json(animate_win, animate_win);
    mk_bool_json(use_physics, use_physics);
    mk_bool_json(show_level_previews, show_level_previews);
#undef mk_bool_json

    cJSON *win_anim_json = win_anim_config_to_json();
    if (win_anim_json) {
        if (!cJSON_AddItemToObject(json, "win_animation", win_anim_json)) {
            errmsg("Error adding win_animation config JSON");
            cJSON_Delete(win_anim_json);
            goto to_json_error;
        }
    } else {
        errmsg("Error building win_animation config JSON");
        goto to_json_error;
    }
\
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
    safe_asprintf(&tmpname, "%s.tmp", nvdata_state_file_path);

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
        safe_asprintf(&nvdata_state_file_path, "%s/%s", nvdata_dir, NVDATA_STATE_FILE_NAME);
    }

    if (nvdata_state_finished_levels_file_path == NULL) {
        safe_asprintf(&nvdata_state_finished_levels_file_path, "%s/%s",
                 nvdata_dir, NVDATA_FINISHED_LEVEL_FILE_NAME);
    }

    if (nvdata_default_browse_path == NULL) {
        safe_asprintf(&nvdata_default_browse_path, "%s/%s",
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

void save_current_level_with_nvdata(void)
{
    assert_not_null(current_level);

    SAFEFREE(nvdata_saved_current_level_path);

#define TIMEBUF_SIZE 200
    char timebuf[TIMEBUF_SIZE];
    struct tm *tm_info;

    time_t now = time(NULL);
    tm_info = localtime(&now);
    strftime(timebuf, TIMEBUF_SIZE, "%Y%m%d-%H%M%S", tm_info);

    safe_asprintf(&nvdata_saved_current_level_path, "%s/%s-%s.%s",
                  nvdata_dir, NVDATA_SAVED_CURRENT_LEVEL_FILE_NAME_PREFIX,
                  timebuf, LEVEL_FILENAME_EXT);

    infomsg("Saving current level to: %s", nvdata_saved_current_level_path);

    level_save_to_filename(current_level, nvdata_saved_current_level_path);
}
