/****************************************************************************
 *                                                                          *
 * gui_options.c                                                            *
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

#if defined(PLATFORM_DESKTOP)
# include "tinyfiledialogs/tinyfiledialogs.h"
#endif

#include "options.h"
#include "color.h"
#include "level.h"
#include "gui_options.h"
#include "gui_dialog.h"

Rectangle options_panel_rect;
Rectangle options_tabbar_rect;
Rectangle options_area_rect;
Rectangle options_icon_scale_rect;
Rectangle options_anim_bg_rect;
Rectangle options_anim_win_rect;
Rectangle options_show_level_previews_rect;
Rectangle options_reset_finished_rect;

char options_panel_text[] = "Options";
char options_icon_scale_text[] = "Cursor Size";
char options_anim_bg_text[]  = "Animate Background";
char options_anim_win_text[] = "Animate Winning Levels";
char options_show_level_previews_text[] = "Show Level Previews";
char options_reset_finished_text[] = "Reset Level Finished Data";

#ifdef USE_PHYSICS
Rectangle options_use_physics_rect;
char options_use_physics_text[] = "Use Physics Engine";
#endif

//                         8 =    5 2   1
//                           "Color N"'\0'
#define GUI_COLOR_OPTION_LABEL_MAXLEN 8
struct gui_color_option {
    Rectangle label_rect;
    char label_text[GUI_COLOR_OPTION_LABEL_MAXLEN];
    float label_size;
\
    Rectangle color_sample_rect;

    Rectangle pick_color_button_rect;
    char pick_color_button_text[7];

    Rectangle reset_button_rect;
    char reset_button_text[7];

    Rectangle input_rect;
};
typedef struct gui_color_option gui_color_option_t;

gui_color_option_t color_options[PATH_TYPE_COUNT];

struct options_status {
    char *text;
    char *icon_text;
    Vector2 text_size;
    Vector2 icon_text_size;
    Color color;
};
typedef struct options_status options_status_t;

options_status_t options_status_on = {
    .text  = "On",
    .color = GREEN
};
options_status_t options_status_off = {
    .text  = "Off",
    .color = RED
};

#if defined(PLATFORM_DESKTOP)
# define NUM_TABS 3
#else
# define NUM_TABS 2
#endif

const char *options_tabbar_text[NUM_TABS];

int options_active_tab;

#define STATUS_LABEL_ROUNDNESS 0.2
#define STATUS_LABEL_SEGMENTS 12
#define STATUS_LABEL_LINE_THICKNESS 2.0
void show_status(options_status_t *status, Rectangle status_rect)
{
    GuiDrawText(status->icon_text, GetTextBounds(LABEL, status_rect), TEXT_ALIGN_LEFT, status->color);
}

void show_status_beside(bool status_value, Rectangle neighbor)
{
    options_status_t *status =
        status_value
        ? &options_status_on
        : &options_status_off;

    Rectangle status_rect = {
        .x      = neighbor.x + neighbor.width + RAYGUI_ICON_SIZE,
        .y      = neighbor.y,
        .width  = status->icon_text_size.x,
        .height = neighbor.height
    };


    show_status(status, status_rect);
}

void init_gui_options(void)
{
    options_tabbar_text[0] = "Graphics";
#if defined(PLATFORM_DESKTOP)
    options_tabbar_text[1] = "Colors";
    options_tabbar_text[2] = "Data";
#else
    options_tabbar_text[1] = "Data";
#endif

    options_active_tab = 0;

#if defined(PLATFORM_DESKTOP)
    for (int i=1; i<PATH_TYPE_COUNT; i++) {
        snprintf(color_options[i].label_text,
                 GUI_COLOR_OPTION_LABEL_MAXLEN,
                 "Color %d", i);

        snprintf(color_options[i].pick_color_button_text, 7,
                 "%s", GuiIconText(ICON_COLOR_PICKER, NULL));

        snprintf(color_options[i].reset_button_text, 7,
                 "%s", GuiIconText(ICON_UNDO_FILL, NULL));
    }
#endif

    resize_gui_options();
}

void cleanup_gui_options(void)
{
    SAFEFREE(options_status_on.icon_text);
    SAFEFREE(options_status_off.icon_text);
}

void resize_gui_options(void)
{
    options_panel_rect.width  = window_size.x * 0.65;
    options_panel_rect.height = (window_size.y * options_panel_rect.x) / window_size.x;

    MINVAR(options_panel_rect.width,  350);
    MINVAR(options_panel_rect.height, 350);

    options_panel_rect.x = (window_size.x / 2) - (options_panel_rect.width  / 2);
    options_panel_rect.y = (window_size.y / 2) - (options_panel_rect.height / 2);

    float panel_bottom = options_panel_rect.y + options_panel_rect.height;

    options_tabbar_rect.width  = options_panel_rect.width - (2 * PANEL_INNER_MARGIN);
    options_tabbar_rect.height = TOOL_BUTTON_HEIGHT;
    options_tabbar_rect.x = options_panel_rect.x + PANEL_INNER_MARGIN;
    options_tabbar_rect.y = options_panel_rect.y + PANEL_INNER_MARGIN + TOOL_BUTTON_HEIGHT;

    options_area_rect.x      = options_tabbar_rect.x + PANEL_INNER_MARGIN;
    options_area_rect.y      = options_tabbar_rect.y + options_tabbar_rect.height + (2 * RAYGUI_ICON_SIZE);
    options_area_rect.width  = options_tabbar_rect.width - (2 * PANEL_INNER_MARGIN);
    options_area_rect.height = panel_bottom - (2 * RAYGUI_ICON_SIZE) - options_area_rect.y;

    Vector2 options_reset_finished_text_size = measure_gui_text(options_reset_finished_text);
    options_reset_finished_rect.x = options_area_rect.x;
    options_reset_finished_rect.y = options_area_rect.y;
    options_reset_finished_rect.width = options_reset_finished_text_size.x + (4 * BUTTON_MARGIN);;
    options_reset_finished_rect.height = TOOL_BUTTON_HEIGHT;

#define mktext(name) \
    Vector2 options_##name##_text_size = measure_gui_text(options_##name##_text); \
    anim_text_size = MAX(anim_text_size, options_##name##_text_size.x);

    float anim_text_size = 0.0f;
    mktext(anim_bg);
    mktext(anim_win);
#ifdef USE_PHYSICS
    mktext(use_physics);
#endif
    mktext(show_level_previews);
#undef mktext

    anim_text_size += 4 * BUTTON_MARGIN;

    options_anim_bg_rect.x = options_area_rect.x;
    options_anim_bg_rect.y = options_area_rect.y;
    options_anim_bg_rect.width = anim_text_size;
    options_anim_bg_rect.height = TOOL_BUTTON_HEIGHT;

    options_anim_win_rect.x = options_area_rect.x;
    options_anim_win_rect.y = options_anim_bg_rect.y + options_anim_bg_rect.height + RAYGUI_ICON_SIZE;
    options_anim_win_rect.width = anim_text_size;
    options_anim_win_rect.height = TOOL_BUTTON_HEIGHT;

#ifdef USE_PHYSICS
    options_use_physics_rect.x = options_area_rect.x;
    options_use_physics_rect.y = options_anim_win_rect.y + options_anim_win_rect.height + RAYGUI_ICON_SIZE;
    options_use_physics_rect.width = anim_text_size;
    options_use_physics_rect.height = TOOL_BUTTON_HEIGHT;
#endif

    options_show_level_previews_rect.x = options_area_rect.x;
#ifdef USE_PHYSICS
    options_show_level_previews_rect.y = options_use_physics_rect.y + options_use_physics_rect.height + RAYGUI_ICON_SIZE;
#else
    options_show_level_previews_rect.y = options_anim_win_rect.y + options_anim_win_rect.height + RAYGUI_ICON_SIZE;
#endif
    options_show_level_previews_rect.width = anim_text_size;
    options_show_level_previews_rect.height = TOOL_BUTTON_HEIGHT;

    Vector2 options_icon_scale_text_size = measure_gui_text(options_icon_scale_text);
    options_icon_scale_rect.x = options_show_level_previews_rect.x + options_icon_scale_text_size.x;
    options_icon_scale_rect.y = options_show_level_previews_rect.y + options_show_level_previews_rect.height + RAYGUI_ICON_SIZE;
    options_icon_scale_rect.width = 90;
    options_icon_scale_rect.height = TOOL_BUTTON_HEIGHT;
    options_icon_scale_rect.x += (3 * BUTTON_MARGIN) - 1;

    options_status_on.text_size  = measure_gui_text(options_status_on.text);
    options_status_off.text_size = measure_gui_text(options_status_off.text);

    options_status_on.text_size.x = options_status_off.text_size.x =
        MAX(options_status_on.text_size.x, options_status_off.text_size.x);
    options_status_on.text_size.y = options_status_off.text_size.y =
        MAX(options_status_on.text_size.y, options_status_off.text_size.y);

    SAFEFREE(options_status_on.icon_text);
    SAFEFREE(options_status_off.icon_text);
    options_status_on.icon_text  = strdup(GuiIconText(ICON_OK_TICK,  options_status_on.text));
    options_status_off.icon_text = strdup(GuiIconText(ICON_CROSS, options_status_off.text));

    options_status_on.icon_text_size  = measure_gui_text(options_status_on.icon_text);
    options_status_off.icon_text_size = measure_gui_text(options_status_off.icon_text);

    options_status_on.icon_text_size.x = options_status_off.icon_text_size.x =
        MAX(options_status_on.icon_text_size.x, options_status_off.icon_text_size.x);
    options_status_on.icon_text_size.y = options_status_off.icon_text_size.y =
        MAX(options_status_on.icon_text_size.y, options_status_off.icon_text_size.y);

#if defined(PLATFORM_DESKTOP)
    float max_label_text_size = 0.0f;

    for (int i=1; i<PATH_TYPE_COUNT; i++) {
        Vector2 label_text_size = measure_gui_text(color_options[i].label_text);
        color_options[i].label_size = label_text_size.x;
        if (label_text_size.x > max_label_text_size) {
            max_label_text_size = label_text_size.x;
        }
    }

    int color_opt_y_offset = 0;

    float area_rhs = options_area_rect.x + options_area_rect.width;

    for (int i=1; i<PATH_TYPE_COUNT; i++) {
        color_options[i].label_rect.y      = options_area_rect.y + color_opt_y_offset;
        color_options[i].label_rect.width  = max_label_text_size + (2 * BUTTON_MARGIN);;
        color_options[i].label_rect.height = TOOL_BUTTON_HEIGHT;
        color_options[i].label_rect.x      = options_area_rect.x
            + color_options[i].label_rect.width
            - color_options[i].label_size;

        color_options[i].color_sample_rect.x      = color_options[i].label_rect.x + color_options[i].label_rect.width + RAYGUI_ICON_SIZE;
        color_options[i].color_sample_rect.y      = options_area_rect.y + color_opt_y_offset;
        color_options[i].color_sample_rect.width  = TOOL_BUTTON_WIDTH * 3;
        color_options[i].color_sample_rect.height = TOOL_BUTTON_HEIGHT;

        color_options[i].pick_color_button_rect.x      = color_options[i].color_sample_rect.x + color_options[i].color_sample_rect.width + RAYGUI_ICON_SIZE;
        color_options[i].pick_color_button_rect.y      = options_area_rect.y + color_opt_y_offset;
        color_options[i].pick_color_button_rect.width  = TOOL_BUTTON_WIDTH;
        color_options[i].pick_color_button_rect.height = TOOL_BUTTON_HEIGHT;

        color_options[i].reset_button_rect.x      = color_options[i].pick_color_button_rect.x + color_options[i].pick_color_button_rect.width + RAYGUI_ICON_SIZE;
        color_options[i].reset_button_rect.y      = options_area_rect.y + color_opt_y_offset;
        color_options[i].reset_button_rect.width  = TOOL_BUTTON_WIDTH;
        color_options[i].reset_button_rect.height = TOOL_BUTTON_HEIGHT;

        color_options[i].input_rect.x      = color_options[i].reset_button_rect.x + color_options[i].reset_button_rect.width + RAYGUI_ICON_SIZE;
        color_options[i].input_rect.y      = options_area_rect.y + color_opt_y_offset;
        color_options[i].input_rect.width  = area_rhs - color_options[i].input_rect.x;
        color_options[i].input_rect.height = TOOL_BUTTON_HEIGHT;

        color_opt_y_offset += TOOL_BUTTON_HEIGHT + RAYGUI_ICON_SIZE;
    }
#endif
}

void draw_gui_graphics_options(void)
{
    int prev_align = GuiGetStyle(TOGGLE, TEXT_ALIGNMENT);
    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    GuiToggle(options_anim_bg_rect,             options_anim_bg_text,             &options->animate_bg);
    GuiToggle(options_anim_win_rect,            options_anim_win_text,            &options->animate_win);
#ifdef USE_PHYSICS
    GuiToggle(options_use_physics_rect,         options_use_physics_text,         &options->use_physics);
#endif
    GuiToggle(options_show_level_previews_rect, options_show_level_previews_text, &options->show_level_previews);

    show_status_beside(options->animate_bg,          options_anim_bg_rect);
    show_status_beside(options->animate_win,         options_anim_win_rect);
#ifdef USE_PHYSICS
    show_status_beside(options->use_physics,         options_use_physics_rect);
#endif
    show_status_beside(options->show_level_previews, options_show_level_previews_rect);

    int old_cursor_scale = options->cursor_scale;
    int cursor_scale     = options->cursor_scale;
    GuiSpinner(options_icon_scale_rect, options_icon_scale_text, &cursor_scale, CURSOR_MIN_SCALE, CURSOR_MAX_SCALE, false);
    if (old_cursor_scale != cursor_scale) {
        options->cursor_scale = cursor_scale;
    }

    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, prev_align);
}

#if defined(PLATFORM_DESKTOP)
static void pick_color_finished(gui_dialog_t *dialog)
{
    color_option_set(dialog->color_opt, dialog->color);
}

void draw_gui_color_option(gui_color_option_t *gui_opt, color_option_t *c_opt)
{
    GuiLabel(gui_opt->label_rect, gui_opt->label_text);

    bool hover = CheckCollisionPointRec(mouse_positionf, gui_opt->color_sample_rect);
    Color samp_color = hover ? c_opt->highlight_color : c_opt->color;
    Color edge_color = hover ? panel_edge_hover_color : panel_edge_color;
    DrawRectangleRounded(gui_opt->color_sample_rect, PANEL_ROUNDNES, 0, samp_color);
    DrawRectangleRoundedLines(gui_opt->color_sample_rect, PANEL_ROUNDNES, 0, 2.0, edge_color);

    if (GuiButton(gui_opt->pick_color_button_rect, gui_opt->pick_color_button_text)) {
        gui_dialog_pick_color(c_opt, pick_color_finished);
    }

    bool do_disable = color_eq(c_opt->color, c_opt->default_color);
    if (do_disable) {
        GuiDisable();
    }
    if (GuiButton(gui_opt->reset_button_rect, gui_opt->reset_button_text)) {
        color_option_set(c_opt, c_opt->default_color);
    }
    if (do_disable) {
        GuiEnable();
    }
}

void draw_gui_color_options(void)
{
    for (int i=1; i<PATH_TYPE_COUNT; i++) {
        draw_gui_color_option(&(color_options[i]),
                              &(options->path_color[i]));
    }
}
#endif

void draw_gui_data_options(void)
{
    if (GuiButton(options_reset_finished_rect, options_reset_finished_text)) {
        printf("reset\n");
    }
}

void draw_gui_options(void)
{
    GuiPanel(options_panel_rect, options_panel_text);

    GuiSimpleTabBar(options_tabbar_rect, options_tabbar_text, NUM_TABS, &options_active_tab);

    switch (options_active_tab) {
    case 0:
        draw_gui_graphics_options();
        break;

    case 1:
#if defined(PLATFORM_DESKTOP)
        draw_gui_color_options();
        break;

    case 2:
#endif
        draw_gui_data_options();
        break;
    }
}
