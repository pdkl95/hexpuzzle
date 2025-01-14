/****************************************************************************
 *                                                                          *
 * range.c                                                                  *
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
#include "range.h"

char gui_min_spinner_label[] = "Min";
char gui_max_spinner_label[] = "Max";

#define RANGE_STR_MAX 256
const char *int_range_string(int_range_t *ir)
{
    static char buf[RANGE_STR_MAX];
    if (ir->min == ir->max) {
        snprintf(buf, RANGE_STR_MAX, "%d", ir->max);
    } else {
        snprintf(buf, RANGE_STR_MAX, "%d..%d", ir->min, ir->max);
    }

    return buf;
}

cJSON *int_range_to_json(int_range_t *ir)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(json, "min", ir->min) == NULL) {
        goto int_range_json_error;
    }
    
    if (cJSON_AddNumberToObject(json, "max", ir->max) == NULL) {
        goto int_range_json_error;
    }

    return json;

  int_range_json_error:
    cJSON_Delete(json);
    return NULL;
}

bool int_range_from_json(cJSON *json, int_range_t *ir)
{
    if (!cJSON_IsObject(json)) {
        errmsg("int range JSON should be an Object");
        return false;
    }

    cJSON *min_json = cJSON_GetObjectItem(json, "min");
    if (!min_json) {
        errmsg("int range JSON Object should have Number 'min'");
        return false;
    }
    if (!cJSON_IsNumber(min_json)) {
        errmsg("int range JSON Object item 'min' is not a Number");
        return false;
    }

    cJSON *max_json = cJSON_GetObjectItem(json, "max");
    if (!max_json) {
        errmsg("int range JSON Object should have Number 'max'");
        return false;
    }
    if (!cJSON_IsNumber(max_json)) {
        errmsg("int range JSON Object item 'max' is not a Number");
        return false;
    }

    ir->min = min_json->valueint;
    ir->max = max_json->valueint;

    return true;
}

void int_range_clamp(int_range_t *ir, int min, int max)
{
    if (ir->min > ir->max) {
        int tmp = ir->min;
        ir->min = ir->max;
        ir->max = tmp;
    }

    CLAMPVAR(ir->min, min, max);
    CLAMPVAR(ir->max, min, max);
}

gui_int_range_t *create_gui_int_range(int_range_t *range, const char *label, int min, int max)
{
    gui_int_range_t *gui = calloc(1, sizeof(gui_int_range_t));

    gui->range = range;
    gui->label_text = strdup(label);

    gui->min  = min;
    gui->max  = max;

    memcpy(gui->add_button_text,   GuiIconText(ICON_PLAYER_PLAY_BACK, NULL), 6);
    memcpy(gui->sub_button_text,   GuiIconText(ICON_PLAYER_PLAY,      NULL), 6);
    memcpy(gui->first_button_text, GuiIconText(ICON_PLAYER_PREVIOUS,  NULL), 6);
    memcpy(gui->last_button_text,  GuiIconText(ICON_PLAYER_NEXT,      NULL), 6);

    gui->min_spinner.parent     = gui;
    gui->max_spinner.parent     = gui;
    gui->min_spinner.value      = &range->min;
    gui->max_spinner.value      = &range->max;
    gui->min_spinner.label      = gui_min_spinner_label;
    gui->max_spinner.label      = gui_max_spinner_label;
    gui->min_spinner.label_size = measure_gui_text(gui->min_spinner.label);
    gui->max_spinner.label_size = measure_gui_text(gui->max_spinner.label);
    gui->min_spinner.edit       = false;
    gui->max_spinner.edit       = false;

    return gui;
}

void resize_gui_int_range(gui_int_range_t *gui, Rectangle *area)
{
    Vector2 label_text_sise = measure_gui_text(gui->label_text);
    gui->label_rect.width  = label_text_sise.x;
    gui->label_rect.height = label_text_sise.y;
    gui->label_rect.x      = area->x;
    gui->label_rect.y      = area->y;

    gui->min_spinner.rect.y      = gui->label_rect.y;
    gui->max_spinner.rect.y      = gui->label_rect.y;
    gui->min_spinner.rect.height = gui->label_rect.height;
    gui->max_spinner.rect.height = gui->label_rect.height;

    gui->min_spinner.rect.width = 5 * RAYGUI_ICON_SIZE;;
    gui->max_spinner.rect.width = gui->min_spinner.rect.width;

    gui->min_spinner.rect.x =
        gui->label_rect.x
        + gui->label_rect.width
        + PANEL_INNER_MARGIN
        + gui->min_spinner.label_size.x
        + RAYGUI_ICON_SIZE;

    gui->max_spinner.rect.x =
        gui->min_spinner.rect.x
        + gui->max_spinner.rect.width
        + PANEL_INNER_MARGIN
        + gui->max_spinner.label_size.x
        + RAYGUI_ICON_SIZE;

    gui->min_spinner.bg_rect = gui->min_spinner.rect;
    gui->max_spinner.bg_rect = gui->max_spinner.rect;

    gui->min_spinner.bg_rect.x -= gui->min_spinner.label_size.x;
    gui->max_spinner.bg_rect.x -= gui->max_spinner.label_size.x;
    gui->min_spinner.bg_rect.width = gui->min_spinner.label_size.x;
    gui->max_spinner.bg_rect.width = gui->max_spinner.label_size.x;

    float yoffset = gui->label_rect.height;
    yoffset += PANEL_INNER_MARGIN;
    area->y      += yoffset;
    area->height -= yoffset;
}

void destroy_gui_int_range(gui_int_range_t *gui)
{
    if (gui) {
        SAFEFREE(gui->label_text);
    }
}

static bool draw_gui_imt_range_spinner(gui_int_range_spinner_t *gui_spin)
{
    DrawRectangleRounded(gui_spin->bg_rect, 0.4, 4, text_shadow_color);

    int old = *gui_spin->value;

    if (GuiSpinner(gui_spin->rect, gui_spin->label, gui_spin->value, gui_spin->parent->min, gui_spin->parent->max, gui_spin->edit)) {
        gui_spin->edit = !gui_spin->edit;
    }

    return old != *gui_spin->value;
}

bool draw_gui_int_range(gui_int_range_t *gui)
{
    bool rv = false;

    GuiLabel(gui->label_rect, gui->label_text);

    rv = draw_gui_imt_range_spinner(&gui->min_spinner) || rv;
    rv = draw_gui_imt_range_spinner(&gui->max_spinner) || rv;

    return rv;
}

void gui_int_range_set_label_width(gui_int_range_t *gui, float width)
{
    float delta = width - gui->label_rect.width;
    assert(delta >= 0.0f);

    gui->label_rect.width = width;
    gui->min_spinner.rect.x += delta;
    gui->max_spinner.rect.x += delta;

    gui->min_spinner.bg_rect.x = gui->min_spinner.rect.x - gui->min_spinner.label_size.x;
    gui->max_spinner.bg_rect.x = gui->max_spinner.rect.x - gui->max_spinner.label_size.x;

    gui->min_spinner.bg_rect.x -= PANEL_INNER_MARGIN;
    gui->max_spinner.bg_rect.x -= PANEL_INNER_MARGIN - 1;
    gui->min_spinner.bg_rect.width += PANEL_INNER_MARGIN - 3;
    gui->max_spinner.bg_rect.width += PANEL_INNER_MARGIN - 4;
}
