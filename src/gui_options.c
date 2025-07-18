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

#include "options.h"
#include "color.h"
#include "level.h"
#include "nvdata_finished.h"
#include "gui_options.h"
#include "gui_dialog.h"

Rectangle options_panel_rect;
Rectangle options_tabbar_rect;
Rectangle options_area_rect;
Rectangle options_icon_scale_rect;
Rectangle options_anim_bg_rect;
Rectangle options_anim_win_rect;
Rectangle options_show_level_previews_rect;
Rectangle options_show_tooltips_rect;
Rectangle options_use_two_click_dnd_rect;
Rectangle options_log_finished_levels_rect;
Rectangle options_compress_finished_levels_dat_rect;
Rectangle options_reset_finished_rect;
Rectangle options_use_postprocessing_rect;
Rectangle options_use_solve_timer_rect;
Rectangle options_description_rect;

char options_panel_text[] = "Options";
char options_icon_scale_text[] = "Cursor Size";
char options_anim_bg_text[]  = "Animate Background";
char options_anim_win_text[] = "Animate Winning Levels";
char options_show_level_previews_text[] = "Show Level Previews";
char options_show_tooltips_text[] = "Show Tooltips";
char options_use_two_click_dnd_text[] = "2-Click Drag-and-Drop";
char options_log_finished_levels_text[] = "Log Finished Levels";
char options_compress_finished_levels_dat_text[] = "Compress History Log";
char options_reset_finished_text[] = "Expunge Finished Level Data";
char options_use_postprocessing_text[] = "Use Shader Effects";
char options_use_solve_timer_text[] = "Solve Time Clock";

char options_color_edit_tooltip[] = "Select Color";
char options_color_reset_tooltip[] = "Reset color back to default";
char options_reset_finished_text_tooltip[] = "WARNING: Permanent!";

char options_anim_bg_desc_text[]  = "Disable if the animated background is too distracting.";
char options_anim_win_desc_text[] = "Special effects animation that plays when a level is completed.";
char options_use_postprocessing_desc_text[] = "Postpo4ssing shader effects (blur, warping)";
char options_use_solve_timer_desc_text[] = "A clock that tracks how long it takes to solve each level.";
char options_use_two_click_dnd_desc_text[] = "Click on two tiles to swap them. Alternative to drag-n-drop.";
char options_show_level_previews_desc_text[] = "Show small level previews.";
char options_show_tooltips_desc_text[] = "Show popup tooltips when hovering over some controls.";
char options_log_finished_levels_desc_text[] = "Save the win time and blueprint string of finished levels in a browsable history log.";
char options_compress_finished_levels_dat_desc_text[] = "Disables compression of the history log file \"finished levels.dat\". Unless you are interested in using the history log as raw uncompressed JSON, leave this enabled";

#ifdef USE_PHYSICS
Rectangle options_use_physics_rect;
char options_use_physics_text[] = "Use Physics Engine";
char options_use_physics_desc_text[] = "Some win animations use a physics engine, which can use a lot of CPU time.";
#endif

char options_disabled_reason_wait_events[] = "Always off when -w/--wait-events is enabled";
char options_disabled_reason_no_animate_win[] = "Requires \"Animate Winning Levels\"";

struct gui_bool_option {
    bool *opt;
    bool opt_saved;
    bool opt_have_saved_value;

    Rectangle *rect;
    char *label;
    char *desc;

    Vector2 label_text_size;

    bool disabled;
    const char *disabled_reason;
};
typedef struct gui_bool_option gui_bool_option_t;

struct gui_bool_option_list {
    gui_bool_option_t *options;
    int count;
    float max_label_text_width;
};
typedef struct gui_bool_option_list gui_bool_option_list_t;

enum game_option_index {
    GAME_OPTION_USE_SOLVE_TIMER     = 0,
    GAME_OPTION_USE_TWO_CLICK_DND   = 1,
    GAME_OPTION_SHOW_LEVEL_PREVIEW  = 2
};
typedef enum game_option_index game_option_index_t;

gui_bool_option_t _game_options[] = {
    {
        .rect  = &options_use_solve_timer_rect,
        .label = options_use_solve_timer_text,
        .desc  = options_use_solve_timer_desc_text
    },
    {
        .rect  = &options_use_two_click_dnd_rect,
        .label = options_use_two_click_dnd_text,
        .desc  = options_use_two_click_dnd_desc_text
    },
        {
        .rect  = &options_show_level_previews_rect,
        .label = options_show_level_previews_text,
        .desc  = options_show_level_previews_desc_text
    }
};
#define NUM_GAME_OPTIONS ((long)NUM_ELEMENTS(gui_bool_option_t, _game_options))

enum ui_option_index {
#ifdef USE_PHYSICS
    UI_OPTION_SHOW_TOOLTIPS       = 0,
    UI_OPTION_ANIMATE_BG          = 1,
    UI_OPTION_ANIMATE_WIN         = 2,
    UI_OPTION_USE_PHYSICS         = 3,
    UI_OPTION_USE_POSTPROCESSING  = 4
#else
    UI_OPTION_SHOW_TOOLTIPS       = 0,
    UI_OPTION_ANIMATE_BG          = 1,
    UI_OPTION_ANIMATE_WIN         = 2,
    UI_OPTION_USE_POSTPROCESSING  = 3
#endif
};
typedef enum ui_option_index ui_option_index_t;

gui_bool_option_t _ui_options[] = {
    {
        .rect  = &options_show_tooltips_rect,
        .label = options_show_tooltips_text,
        .desc  = options_show_tooltips_desc_text
    },
    {
        .rect  = &options_anim_bg_rect,
        .label = options_anim_bg_text,
        .desc  = options_anim_bg_desc_text,
    },
    {
        .rect  = &options_anim_win_rect,
        .label = options_anim_win_text,
        .desc  = options_anim_win_desc_text
    },
#ifdef USE_PHYSICS
    {
        .rect  = &options_use_physics_rect,
        .label = options_use_physics_text,
        .desc  = options_use_physics_desc_text
    },
#endif
    {
        .rect  = &options_use_postprocessing_rect,
        .label = options_use_postprocessing_text,
        .desc  = options_use_postprocessing_desc_text
    }
};
#define NUM_UI_OPTIONS ((long)NUM_ELEMENTS(gui_bool_option_t, _ui_options))

enum data_option_index {
    DATA_OPTION_LOG_FINISHED_LEVELS          = 0,
    DATA_OPTION_COMPRESS_FINISHED_LEVELS_DAT = 1
};
typedef enum graphics_option_index graphics_option_index_t;

gui_bool_option_t _data_options[] = {
    {
        .rect  = &options_log_finished_levels_rect,
        .label = options_log_finished_levels_text,
        .desc  = options_log_finished_levels_desc_text
    },
    {
        .rect  = &options_compress_finished_levels_dat_rect,
        .label = options_compress_finished_levels_dat_text,
        .desc  = options_compress_finished_levels_dat_desc_text
    }
};
#define NUM_DATA_OPTIONS ((long)NUM_ELEMENTS(gui_bool_option_t, _data_options))


gui_bool_option_list_t game_options = {
    .options = _game_options,
    .count   = NUM_GAME_OPTIONS
};

gui_bool_option_list_t ui_options = {
    .options = _ui_options,
    .count   = NUM_UI_OPTIONS
};

gui_bool_option_list_t data_options = {
    .options = _data_options,
    .count   = NUM_DATA_OPTIONS
};

//                         8 =    5 2   1
//                           "Color N"'\0'
#define GUI_COLOR_OPTION_LABEL_MAXLEN 8
struct gui_color_option {
    Rectangle label_rect;
    char label_text[GUI_COLOR_OPTION_LABEL_MAXLEN];
    float label_size;

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

#define NUM_TABS 4

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

static void enable_bool_option(gui_bool_option_list_t *list, int index)
{
    assert_not_null(list);

    if (list->options[index].opt_have_saved_value) {
        *list->options[index].opt = list->options[index].opt_saved;
        list->options[index].opt_have_saved_value = false;
    }

    list->options[index].disabled = false;
    list->options[index].disabled_reason = NULL;
}

static void disable_bool_option(gui_bool_option_list_t *list, int index, const char *reason)
{
    assert_not_null(list);

    list->options[index].opt_saved = *list->options[index].opt;
    list->options[index].opt_have_saved_value = true;

    *list->options[index].opt = false;
    list->options[index].disabled = true;
    list->options[index].disabled_reason = reason;
}

static void enable_list_of_bool_options(gui_bool_option_list_t *list)
{
    assert_not_null(list);

    for (int i=0; i<list->count; i++) {
        enable_bool_option(list, i);
    }
}

static void check_disabled_options(void)
{
    enable_list_of_bool_options(&game_options);
    enable_list_of_bool_options(&ui_options);
    enable_list_of_bool_options(&data_options);

    if (options->wait_events) {
        disable_bool_option(&ui_options,
                            UI_OPTION_ANIMATE_BG,
                            options_disabled_reason_wait_events);
        disable_bool_option(&ui_options,
                            UI_OPTION_ANIMATE_WIN,
                            options_disabled_reason_wait_events);
    }

    if (!options->animate_win) {
        disable_bool_option(&ui_options,
                            UI_OPTION_USE_PHYSICS,
                            options_disabled_reason_no_animate_win);
        disable_bool_option(&ui_options,
                            UI_OPTION_USE_POSTPROCESSING,
                            options_disabled_reason_no_animate_win);
    }
}

static void init_bool_option_list(gui_bool_option_list_t *list)
{
    assert_not_null(list);

    for (int i=0; i<list->count; i++) {
        gui_bool_option_t *opt = &(list->options[i]);
        assert_not_null(opt);

        opt->opt_saved = *opt->opt;
        opt->opt_have_saved_value = false;
    }
}

void init_gui_options(void)
{
    options_tabbar_text[0] = "Game";
    options_tabbar_text[1] = "UI";
    options_tabbar_text[2] = "Colors";
    options_tabbar_text[3] = "Data";

    options_active_tab = 0;

    for (int i=1; i<PATH_TYPE_COUNT; i++) {
        snprintf(color_options[i].label_text,
                 GUI_COLOR_OPTION_LABEL_MAXLEN,
                 "Color %d", i);

        snprintf(color_options[i].pick_color_button_text, 7,
                 "%s", GuiIconText(ICON_COLOR_PICKER, NULL));

        snprintf(color_options[i].reset_button_text, 7,
                 "%s", GuiIconText(ICON_UNDO_FILL, NULL));
    }

    _game_options[GAME_OPTION_USE_SOLVE_TIMER    ].opt = &options->use_solve_timer;
    _game_options[GAME_OPTION_USE_TWO_CLICK_DND  ].opt = &options->use_two_click_dnd;
    _game_options[GAME_OPTION_SHOW_LEVEL_PREVIEW ].opt = &options->show_level_previews;

    _ui_options[UI_OPTION_SHOW_TOOLTIPS      ].opt = &options->show_tooltips;
    _ui_options[UI_OPTION_ANIMATE_BG         ].opt = &options->animate_bg;
    _ui_options[UI_OPTION_ANIMATE_WIN        ].opt = &options->animate_win;
    _ui_options[UI_OPTION_USE_PHYSICS        ].opt = &options->use_physics;
    _ui_options[UI_OPTION_USE_POSTPROCESSING ].opt = &options->use_postprocessing;

    _data_options[DATA_OPTION_LOG_FINISHED_LEVELS].opt = &options->log_finished_levels;
    _data_options[DATA_OPTION_COMPRESS_FINISHED_LEVELS_DAT].opt = &options->compress_finished_levels_dat;
    init_bool_option_list(&game_options);
    init_bool_option_list(&ui_options);
    init_bool_option_list(&data_options);

    resize_gui_options();
}

void cleanup_gui_options(void)
{
    SAFEFREE(options_status_on.icon_text);
    SAFEFREE(options_status_off.icon_text);
}

static float resize_bool_option_list(gui_bool_option_list_t *list, Rectangle bounds)
{
    assert_not_null(list);

    list->max_label_text_width = 0.0f;

    for (int i=0; i<list->count; i++) {
        gui_bool_option_t *opt = &(list->options[i]);
        assert_not_null(opt);
        assert_not_null(opt->label);

        opt->label_text_size = measure_gui_text(opt->label);
        list->max_label_text_width = MAX(list->max_label_text_width, opt->label_text_size.x);
    }

    list->max_label_text_width += RAYGUI_ICON_SIZE;;

    for (int i=0; i<list->count; i++) {
        gui_bool_option_t *opt = &(list->options[i]);

        opt->rect->x      = bounds.x;
        opt->rect->y      = bounds.y;
        opt->rect->width  = list->max_label_text_width;
        opt->rect->height = TOOL_BUTTON_HEIGHT;

        bounds.y += opt->rect->height;
        bounds.y += RAYGUI_ICON_SIZE;
    }

    return bounds.y;
}

void resize_gui_options(void)
{
    options_panel_rect.width  = window_size.x * 0.65;
    options_panel_rect.height = (window_size.y * options_panel_rect.x) / window_size.x;

    MINVAR(options_panel_rect.width,  380);
    MINVAR(options_panel_rect.height, 400);

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

    UNUSED
    float game_bool_option_list_bottom = resize_bool_option_list(&game_options, options_area_rect);
    float   ui_bool_option_list_bottom = resize_bool_option_list(&ui_options, options_area_rect);
    float data_bool_option_list_bottom = resize_bool_option_list(&data_options, options_area_rect);

    Vector2 options_reset_finished_text_size = measure_gui_text(options_reset_finished_text);
    options_reset_finished_rect.x = options_area_rect.x;
    options_reset_finished_rect.y = data_bool_option_list_bottom + RAYGUI_ICON_SIZE;
    options_reset_finished_rect.width = options_reset_finished_text_size.x + (4 * BUTTON_MARGIN);;
    options_reset_finished_rect.height = TOOL_BUTTON_HEIGHT;

    Vector2 options_icon_scale_text_size = measure_gui_text(options_icon_scale_text);
    options_icon_scale_rect.x = options_log_finished_levels_rect.x + options_icon_scale_text_size.x;
    options_icon_scale_rect.y = ui_bool_option_list_bottom;
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

    float bool_opt_width =
        options_anim_bg_rect.width +
        options_status_on.icon_text_size.x +
        (2 * PANEL_INNER_MARGIN);

    options_description_rect.x = options_anim_bg_rect.x + bool_opt_width;
    options_description_rect.y = options_area_rect.y;
    options_description_rect.width = options_area_rect.width - bool_opt_width;
    options_description_rect.height = options_area_rect.height;

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

    check_disabled_options();
}

static void draw_bool_opt_list(gui_bool_option_list_t *list)
{
    char *desc = NULL;

    int prev_align = GuiGetStyle(TOGGLE, TEXT_ALIGNMENT);
    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    for (int i=0; i<list->count; i++) {
        gui_bool_option_t *opt = &(list->options[i]);

        if (opt->disabled) {
           GuiDisable();
        }

        bool old_value = *opt->opt;

        GuiToggle(*opt->rect, opt->label, opt->opt);
        show_status_beside(*opt->opt, *opt->rect);

        if (opt->disabled) {
            if (opt->disabled_reason) {
                Rectangle ttrect = *opt->rect;
                ttrect.x -= 100;
                ttrect.x = MAX(ttrect.x, WINDOW_MARGIN);
                ttrect.width += opt->rect->x - ttrect.x;
                tooltip(ttrect, opt->disabled_reason);
            }
            GuiEnable();
        }

        if (*opt->opt != old_value) {
            check_disabled_options();
        }

        if (!gui_dialog_active()
            && CheckCollisionPointRec(mouse_positionf, *opt->rect)) {
            desc = opt->desc;
        }
    }

    if (desc) {
        int old_valign = GuiGetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL);
        int old_wrap   = GuiGetStyle(DEFAULT, TEXT_WRAP_MODE);
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_TOP);
        GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, TEXT_WRAP_WORD);

        GuiTextBox(options_description_rect, desc, 0, false);

        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, old_valign);
        GuiSetStyle(DEFAULT, TEXT_WRAP_MODE, old_wrap);
    }

    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, prev_align);
}

static void draw_gui_game_options(void)
{
    draw_bool_opt_list(&game_options);
}

static void draw_gui_ui_options(void)
{
    draw_bool_opt_list(&ui_options);

    int prev_align = GuiGetStyle(TOGGLE, TEXT_ALIGNMENT);
    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    int old_cursor_scale = options->cursor_scale;
    int cursor_scale     = options->cursor_scale;
    GuiSpinner(options_icon_scale_rect, options_icon_scale_text, &cursor_scale, CURSOR_MIN_SCALE, CURSOR_MAX_SCALE, false);
    if (old_cursor_scale != cursor_scale) {
        options->cursor_scale = cursor_scale;
    }

    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, prev_align);
}

static void pick_color_finished(gui_dialog_t *dialog, UNUSED void *data)
{
    color_option_set(dialog->color_opt, dialog->color);
}

static void draw_gui_color_option(gui_color_option_t *gui_opt, color_option_t *c_opt)
{
    GuiLabel(gui_opt->label_rect, gui_opt->label_text);

    bool hover = CheckCollisionPointRec(mouse_positionf, gui_opt->color_sample_rect);
    Color samp_color = hover ? c_opt->highlight_color : c_opt->color;
    Color edge_color = hover ? panel_edge_hover_color : panel_edge_color;
    DrawRectangleRounded(gui_opt->color_sample_rect, PANEL_ROUNDNES, 0, samp_color);
    DrawRectangleRoundedLines(gui_opt->color_sample_rect, PANEL_ROUNDNES, 0, 2.0, edge_color);

    tooltip(gui_opt->pick_color_button_rect, options_color_edit_tooltip);

    if (GuiButton(gui_opt->pick_color_button_rect, gui_opt->pick_color_button_text)) {
        gui_dialog_pick_color(c_opt, pick_color_finished, NULL);
    }

    bool do_disable = color_eq(c_opt->color, c_opt->default_color);
    if (do_disable) {
        GuiDisable();
    } else {
        tooltip(gui_opt->reset_button_rect, options_color_reset_tooltip);
    }

    if (GuiButton(gui_opt->reset_button_rect, gui_opt->reset_button_text)) {
        color_option_set(c_opt, c_opt->default_color);
    }
    if (do_disable) {
        GuiEnable();
    }
}

static void draw_gui_color_options(void)
{
    for (int i=1; i<PATH_TYPE_COUNT; i++) {
        draw_gui_color_option(&(color_options[i]),
                              &(options->path_color[i]));
    }
}

static void expunge_finished_level_dat_dialog_cb(gui_dialog_t *dialog, UNUSED void *data)
{
    if (dialog->status) {
        if (reset_nvdata_finished_levels()) {
            popup_message("Erased finished level data.");
        }
    }
}

static void draw_gui_data_options(void)
{
    bool do_disable = !have_nvdata_finished_levels_data();

    bool old_compress_finished_levels_dat = options->compress_finished_levels_dat;

    draw_bool_opt_list(&data_options);

    if (old_compress_finished_levels_dat != options->compress_finished_levels_dat) {
        force_save_nvdata_finished_levels();
    }

    if (do_disable) {
        GuiDisable();
    } else {
        tooltip(options_reset_finished_rect, options_reset_finished_text_tooltip);
    }

    if (GuiButton(options_reset_finished_rect, options_reset_finished_text)) {
        if (!current_dialog) {
            gui_dialog_ask_yn(
                "Expunge finished level log?",
                "Permanently delete the finished level log (the history tab in the browser)?",
                expunge_finished_level_dat_dialog_cb,
                NULL);
        }
    }

    if (do_disable) {
        GuiEnable();
    }
}

void draw_gui_options(void)
{
    GuiPanel(options_panel_rect, options_panel_text);

    GuiSimpleTabBar(options_tabbar_rect, options_tabbar_text, NUM_TABS, &options_active_tab);

    switch (options_active_tab) {
    case 0:
        draw_gui_game_options();
        break;

    case 1:
        draw_gui_ui_options();
        break;

    case 2:
        draw_gui_color_options();
        break;

    case 3:
        draw_gui_data_options();
        break;
    }
}
