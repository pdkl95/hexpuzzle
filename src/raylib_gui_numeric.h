/****************************************************************************
 *                                                                          *
 * raylib_gui_numeric.h                                                     *
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

#ifndef RAYLIB_GUI_NUMERIC_H
#define RAYLIB_GUI_NUMERIC_H

#include "numeric.h"

struct raylib_gui_numeric;

typedef const char *gui_numeric_get_text_cb(struct raylib_gui_numeric *gn);

struct raylib_gui_numeric {
    numeric_t value;

    numeric_t min;
    numeric_t max;
    numeric_t step;

    Rectangle label_rect;
    Rectangle left_button_rect;
    Rectangle display_rect;
    Rectangle right_button_rect;

    Vector2 display_text_location;
    Vector2 display_text_shadow_location;

    char *label_text;
    char *left_button_text;
    char *right_button_text;

    gui_numeric_get_text_cb *get_text;
};
typedef struct raylib_gui_numeric raylib_gui_numeric_t;

const char *gui_numeric_default_get_text(raylib_gui_numeric_t *gn);

raylib_gui_numeric_t *create_gui_numeric(const char *label, numeric_t valuep, numeric_t min, numeric_t max, numeric_t step);
raylib_gui_numeric_t *create_gui_numeric_int(const char *label, int *valuep, int min, int max, int step);
raylib_gui_numeric_t *create_gui_numeric_float(const char *label, float *valuep, float min, float max, float step);
void destroy_gui_numeric(raylib_gui_numeric_t *gn);
void gui_numeric_resize(raylib_gui_numeric_t *gn, Rectangle *area);
bool draw_gui_numeric(raylib_gui_numeric_t *gn);

#endif /*RAYLIB_GUI_NUMERIC_H*/

