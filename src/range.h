/****************************************************************************
 *                                                                          *
 * range.h                                                                  *
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

#ifndef RANGE_H
#define RANGE_H

#include "cJSON/cJSON.h"

struct int_range {
    int min;
    int max;
};
typedef struct int_range int_range_t;

const char *int_range_string(int_range_t *ir);
cJSON *int_range_to_json(int_range_t *ir);
bool int_range_from_json(cJSON *json, int_range_t *ir);
void int_range_clamp(int_range_t *ir, int min, int max);

struct gui_int_range;

struct gui_int_range_spinner {
    char *label;
    Vector2 label_size;
    int *value;
    Rectangle rect;
    Rectangle bg_rect;
    bool edit;
    struct gui_int_range *parent;
};
typedef struct gui_int_range_spinner gui_int_range_spinner_t;

struct gui_int_range {
    int_range_t *range;

    int min;
    int max;

    Rectangle rect;

    char *label_text;
    Rectangle label_rect;

    gui_int_range_spinner_t min_spinner;
    gui_int_range_spinner_t max_spinner;

    char add_button_text[6];
    char sub_button_text[6];
    char first_button_text[6];
    char last_button_text[6];
};
typedef struct gui_int_range gui_int_range_t;

gui_int_range_t *create_gui_int_range(int_range_t *range, const char *label, int min, int max);
void resize_gui_int_range(gui_int_range_t *gui, Rectangle *area);
void destroy_gui_int_range(gui_int_range_t *gui);
void draw_gui_int_range(gui_int_range_t *gui);
void gui_int_range_set_label_width(gui_int_range_t *gui, float width);

#endif /*RANGE_H*/

