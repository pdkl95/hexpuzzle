/****************************************************************************
 *                                                                          *
 * raylib_gui_numeric.c                                                     *
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
#include "color.h"
#include "raylib_gui_numeric.h"

raylib_gui_numeric_t *create_gui_numeric(const char *label, numeric_t valuep, numeric_t min, numeric_t max, numeric_t step)
{
    raylib_gui_numeric_t *gn = calloc(1, sizeof(raylib_gui_numeric_t));

    gn->value = valuep;
    gn->min   = min;
    gn->max   = max;
    gn->step  = step;

    gn->label_text = strdup(label);

    gn->left_button_text  = strdup(GuiIconText(ICON_ARROW_LEFT_FILL, NULL));
    gn->right_button_text = strdup(GuiIconText(ICON_ARROW_RIGHT_FILL, NULL));

    return gn;
}

raylib_gui_numeric_t *create_gui_numeric_int(const char *label, int *valuep, int min, int max, int step)
{
    return create_gui_numeric(label, numeric_int_ptr(valuep), numeric_int(min), numeric_int(max), numeric_int(step));
}

raylib_gui_numeric_t *create_gui_numeric_float(const char *label, float *valuep, float min, float max, float step)
{
    return create_gui_numeric(label, numeric_float_ptr(valuep), numeric_float(min), numeric_float(max), numeric_float(step));
}

void destroy_gui_numeric(raylib_gui_numeric_t *gn)
{
    if (gn) {
        SAFEFREE(gn->label_text);
        SAFEFREE(gn->left_button_text);
        SAFEFREE(gn->right_button_text);
    }
}

void gui_numeric_resize(raylib_gui_numeric_t *gn, Rectangle *area)
{
    assert_not_null(gn);
    Vector2 label_text_size = measure_gui_text(gn->label_text);

    gn->label_rect.x      = area->x;
    gn->label_rect.y      = area->y;
    gn->label_rect.width  = label_text_size.x + (4 * BUTTON_MARGIN);
    gn->label_rect.height = TOOL_BUTTON_HEIGHT;

    gn->left_button_rect.x      = gn->label_rect.x + gn->label_rect.width + RAYGUI_ICON_SIZE;;
    gn->left_button_rect.y      = gn->label_rect.y;
    gn->left_button_rect.width  = TOOL_BUTTON_WIDTH;
    gn->left_button_rect.height = TOOL_BUTTON_HEIGHT;

    gn->display_rect.x      = gn->left_button_rect.x + gn->left_button_rect.width + BUTTON_MARGIN;
    gn->display_rect.y      = gn->left_button_rect.y;
    gn->display_rect.width  = 2 * TOOL_BUTTON_WIDTH;
    gn->display_rect.height = TOOL_BUTTON_HEIGHT;

    gn->right_button_rect.x      = gn->display_rect.x + gn->display_rect.width + BUTTON_MARGIN;
    gn->right_button_rect.y      = gn->display_rect.y;
    gn->right_button_rect.width  = TOOL_BUTTON_WIDTH;
    gn->right_button_rect.height = TOOL_BUTTON_HEIGHT;

    gn->display_text_location.x = gn->display_rect.x + 8;
    gn->display_text_location.y = gn->display_rect.y + 3;
    gn->display_rect.x      += 1;
    gn->display_rect.width  -= 2;
    gn->display_rect.height += 1;

    gn->display_text_shadow_location = gn->display_text_location;
    gn->display_text_shadow_location.x += 1.0f;
    gn->display_text_shadow_location.y += 1.0f;

    float yoffset = gn->label_rect.height + RAYGUI_ICON_SIZE;
    area->y      += yoffset;
    area->height -= yoffset;
}

numeric_t gui_numeric_set_bounded(raylib_gui_numeric_t *gn, numeric_t new_value)
{
    if (numeric_lt(new_value, gn->min)) {
        numeric_copy(gn->value, gn->min);
    } else if (numeric_gt(new_value, gn->max)) {
        numeric_copy(gn->value, gn->max);
    } else {
        numeric_copy(gn->value, new_value);
    }

    return gn->value;
}

numeric_t gui_numeric_inc(raylib_gui_numeric_t *gn)
{
    return gui_numeric_set_bounded(gn, numeric_add(gn->value, gn->step));
}

numeric_t gui_numeric_dec(raylib_gui_numeric_t *gn)
{
    return gui_numeric_set_bounded(gn, numeric_sub(gn->value, gn->step));
}

bool draw_gui_numeric(raylib_gui_numeric_t *gn)
{
    bool rv = false;

    GuiLabel(gn->label_rect, gn->label_text);

    if (GuiButton(gn->left_button_rect, gn->left_button_text)) {
        gui_numeric_dec(gn);
        rv = true;
    }

    DrawRectangleRec(gn->display_rect, seed_bg_color);

    const char *value_text = numeric_text(gn->value);

    draw_panel_text(value_text,
                    gn->display_text_shadow_location,
                    text_shadow_color);

    draw_panel_text(value_text,
                    gn->display_text_location,
                    RAYWHITE);

    if (GuiButton(gn->right_button_rect, gn->right_button_text)) {
        gui_numeric_inc(gn);
        rv = true;
    }

    return rv;
}
