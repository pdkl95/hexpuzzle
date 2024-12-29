/****************************************************************************
 *                                                                          *
 * win_anim_mode_config.c                                                   *
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
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#include "common.h"
#include "win_anim.h"
#include "win_anim_mode_config.h"

win_anim_mode_config_t win_anim_mode_config[WIN_ANIM_MODE_COUNT];

win_anim_mode_config_t default_win_anim_mode_config[WIN_ANIM_MODE_COUNT] = {
    { /* WIN_ANIM_MODE_SIMPLE */
      .enabled = true,
      .chances = 4,
      .do_fade_rotate = true
    },
    { /* WIN_ANIM_MODE_POPS */
      .enabled = true,
      .chances = 10,
      .do_fade_rotate = true
    },
    { /* WIN_ANIM_MODE_WAVES */
      .enabled = true,
      .chances = 10,
      .do_fade_rotate = true
    },
    { /* WIN_ANIM_MODE_SPIN */
      .enabled = true,
      .chances = 8,
      .do_fade_rotate = true
    }
#ifdef USE_PHYSICS
     ,
    { /* WIN_ANIM_MODE_PHYSICS_FALL */
      .enabled = true,
      .chances = 20,
      .do_fade_rotate = true
    },
    { /* WIN_ANIM_MODE_PHYSICS_SWIRL */
      .enabled = true,
      .chances = 23,
      .do_fade_rotate = true
    }
#endif
};

bool win_anim_mode_config_from_json(win_anim_mode_config_t *config, cJSON *json)
{
    if (!cJSON_IsObject(json)) {
        errmsg("Error parsing mode config JSON: not an Object");
        return false;
    }

    cJSON *chances_json = cJSON_GetObjectItemCaseSensitive(json, "chances");
    if (!cJSON_IsNumber(chances_json)) {
        errmsg("Error parsing mode config JSON: 'chances' is not a Number");
        return false;
    }
    config->chances = chances_json->valueint;

    cJSON *bool_json = NULL;
#define mk_bool_json(field, name)                                       \
    bool_json = cJSON_GetObjectItemCaseSensitive(json, STR(name));      \
    if (bool_json) {                                                    \
        if (cJSON_IsBool(bool_json)) {                                  \
            if (cJSON_IsTrue(bool_json)) {                              \
                config->field = true;                                   \
            } else {                                                    \
                config->field = false;                                  \
            }                                                           \
        } else {                                                        \
            errmsg("mode config JSON field '%s' is not a BOOL",         \
                   STR(name));                                          \
        }                                                               \
    } else {                                                            \
        warnmsg("mode config JSON is missing \"%s\"",                   \
                STR(name));                                             \
    }

    mk_bool_json(enabled, enabled);
    mk_bool_json(do_fade_rotate, do_fade_rotate);

#undef mk_bool_json

    return true;
}

cJSON *win_anim_mode_config_to_json(win_anim_mode_config_t *config)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(json, "chances", config->chances) == NULL) {
        errmsg("Error adding \"chances\" to mode config JSON");
        goto mode_json_error;
    }

    cJSON *bool_json = NULL;
#define mk_bool_json(field, name)                                       \
    if (config->field) {                                               \
        bool_json = cJSON_AddTrueToObject(json, STR(name));    \
    } else {                                                            \
        bool_json = cJSON_AddFalseToObject(json, STR(name));   \
    }                                                                   \
    if (!bool_json) {                                                   \
        errmsg("Error adding bool \"%s\" to mode config JSON", STR(name)); \
        goto mode_json_error;                                             \
    }

    mk_bool_json(enabled, enabled);
    mk_bool_json(do_fade_rotate, do_fade_rotate);
#undef mk_bool_json

    return json;

  mode_json_error:
    if (json) {
        cJSON_Delete(json);
    }
    return NULL;
}

void win_anim_mode_config_reset_to_defaut(win_anim_mode_config_t *config)
{
    memcpy(config,
           default_win_anim_mode_config,
           sizeof(default_win_anim_mode_config));
}

bool win_anim_config_from_json(cJSON *json)
{
    cJSON *modes_json = cJSON_GetObjectItem(json, "modes");
    if (modes_json) {
        for (win_anim_mode_t mode = 0; mode < WIN_ANIM_MODE_COUNT; mode++) {
            const char *mode_name = win_anim_mode_str(mode);
            win_anim_mode_config_t *config = &(win_anim_mode_config[mode]);

            cJSON *config_json = cJSON_GetObjectItem(modes_json, mode_name);
            if (!win_anim_mode_config_from_json(config, config_json)) {
                errmsg("Error parsing program state mode config JSON['win_animation']['modes']['%s']",
                       mode_name);
                return false;
            }
        }
    } else {
        warnmsg("Program state JSON['win_animatiob'] is missing \"modes\"");
    }

    return true;
}

cJSON *win_anim_config_to_json(void)
{
    cJSON *json = cJSON_CreateObject();

    cJSON *modes_json = cJSON_AddObjectToObject(json, "modes");
    if (!modes_json) {
        errmsg("Error adding \"modes\" object to JSON");
        goto config_json_error;
    }

    for (win_anim_mode_t mode = 0; mode < WIN_ANIM_MODE_COUNT; mode++) {
        const char *mode_name = win_anim_mode_str(mode);
        win_anim_mode_config_t *config = &(win_anim_mode_config[mode]);
        cJSON *config_json = win_anim_mode_config_to_json(config);
        if (config_json) {
            if (!cJSON_AddItemToObject(modes_json, mode_name, config_json)) {
                errmsg("Error building mode config JSON for mode %d (\"%s\")",
                       mode, mode_name);
                cJSON_Delete(config_json);
                goto config_json_error;
            }
        } else {
            errmsg("Error building mode config JSON for mode %d (\"%s\")",
                   mode, mode_name);
            goto config_json_error;
        }
    }

    return json;

  config_json_error:
    if (json) {
        cJSON_Delete(json);
    }
    return NULL;
}
