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
#include "gui_options.h"

Rectangle options_panel_rect;
Rectangle options_tabbar_rect;
Rectangle options_area_rect;
Rectangle options_anim_bg_rect;
Rectangle options_anim_win_rect;
Rectangle options_reset_finished_rect;

char options_panel_text[] = "Options";
char options_anim_bg_text[]  = "Animate Background";
char options_anim_win_text[] = "Animate Winning Levels";
char options_reset_finished_text[] = "Reset Level Finished Data";

#define NUM_TABS 2
const char *options_tabbar_text[NUM_TABS];

int options_active_tab;

void init_gui_options(void)
{
    options_tabbar_text[0] = "Graphics";
    options_tabbar_text[1] = "Data";

    options_active_tab = 0;

    resize_gui_options();
}

void resize_gui_options(void)
{
    options_panel_rect.width  = window_size.x * 0.65;
    options_panel_rect.height = (window_size.y * options_panel_rect.x) / window_size.x;

    MINVAR(options_panel_rect.width,  350);
    MINVAR(options_panel_rect.height, 250);

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

    Vector2 options_reset_finished_text_size = MeasureGuiText(options_reset_finished_text);
    options_reset_finished_rect.x = options_area_rect.x;
    options_reset_finished_rect.y = options_area_rect.y;
    options_reset_finished_rect.width = options_reset_finished_text_size.x;
    options_reset_finished_rect.height = TOOL_BUTTON_HEIGHT;

    Vector2 options_anim_bg_text_size = MeasureGuiText(options_anim_bg_text);
    Vector2 options_anim_win_text_size = MeasureGuiText(options_anim_win_text);
    float anim_text_size = MAX(options_anim_bg_text_size.x, options_anim_win_text_size.x);

    options_anim_bg_rect.x = options_area_rect.x;
    options_anim_bg_rect.y = options_area_rect.y;
    options_anim_bg_rect.width = anim_text_size;
    options_anim_bg_rect.height = TOOL_BUTTON_HEIGHT;

    options_anim_win_rect.x = options_area_rect.x;
    options_anim_win_rect.y = options_anim_bg_rect.y + options_anim_bg_rect.height + RAYGUI_ICON_SIZE;
    options_anim_win_rect.width = anim_text_size;
    options_anim_win_rect.height = TOOL_BUTTON_HEIGHT;
}

void draw_gui_graphics_options(void)
{
    int prev_align = GuiGetStyle(TOGGLE, TEXT_ALIGNMENT);
    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    GuiToggle(options_anim_bg_rect,  options_anim_bg_text,  &options->animate_bg);
    GuiToggle(options_anim_win_rect, options_anim_win_text, &options->animate_win);

    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, prev_align);
}

void draw_gui_data_options(void)
{
    if (GuiButton(options_reset_finished_rect, options_reset_finished_text)) {
        printf("reset\n");
    }
}

void draw_gui_options(void)
{
    GuiPanel(options_panel_rect, options_panel_text);

    GuiSimpleTabBar(options_tabbar_rect, options_tabbar_text, 2, &options_active_tab);

    switch (options_active_tab) {
    case 0:
        draw_gui_graphics_options();
        break;

    case 1:
        draw_gui_data_options();
        break;
    }
}

