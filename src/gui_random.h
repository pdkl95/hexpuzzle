/****************************************************************************
 *                                                                          *
 * gui_random.h                                                             *
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

#ifndef GUI_RANDOM_H
#define GUI_RANDOM_H

#include "range.h"

enum symmetry_mode {
    SYMMETRY_MODE_NONE = 0,
    SYMMETRY_MODE_REFLECT,
    SYMMETRY_MODE_ROTATE
};
typedef enum symmetry_mode symmetry_mode_t;

const char *symmetry_mode_string(symmetry_mode_t mode);
symmetry_mode_t parse_symmetry_mode_string(const char *string);

struct generate_features {
    char *name;
    int_range_t fixed;
    int_range_t hidden;
    symmetry_mode_t symmetry_mode;
    float minimum_path_density;
};
typedef struct generate_features generate_features_t;

void print_generate_features(generate_features_t *features);
cJSON *generate_features_to_json(generate_features_t features);
bool generate_features_from_json(cJSON *json, generate_features_t *features);

extern generate_features_t *generate_feature_options;
extern int num_generate_feature_options;

cJSON *generate_feature_options_to_json(void);
bool generate_feature_options_from_json(cJSON *json);

void init_gui_random_minimal(void);
void init_gui_random(void);
void cleanup_gui_random(void);
void resize_gui_random(void);
void draw_gui_random(void);

struct level *generate_random_level(void);
struct level *generate_random_title_level(void);

void regen_level_preview(void);

void save_gui_random_level(void);
void play_gui_random_level(void);
void play_gui_random_level_preview(void);
bool parse_random_seed_str(char *seedstr);

extern struct level *gui_random_level;
extern struct level *gui_random_level_preview;

#endif /*GUI_RANDOM_H*/

