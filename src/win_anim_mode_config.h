/****************************************************************************
 *                                                                          *
 * win_anim_mode_config.h                                                   *
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

#ifndef WIN_ANIM_MODE_CONFIG_H
#define WIN_ANIM_MODE_CONFIG_H

struct win_anim_mode_config {
    bool enabled;
    int chances;

    bool do_fade_rotate;
};
typedef struct win_anim_mode_config win_anim_mode_config_t;

bool win_anim_mode_config_from_json(win_anim_mode_config_t *config, cJSON *json);
cJSON *win_anim_mode_config_to_json(win_anim_mode_config_t *config);
void win_anim_mode_config_reset_to_defaut(win_anim_mode_config_t *config);

extern win_anim_mode_config_t win_anim_mode_config[WIN_ANIM_MODE_COUNT];

bool win_anim_config_from_json(cJSON *json);
cJSON *win_anim_config_to_json(void);

#endif /*WIN_ANIM_MODE_CONFIG_H*/

