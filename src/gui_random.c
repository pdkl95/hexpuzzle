/****************************************************************************
 *                                                                          *
 * gui_random.c                                                             *
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
#include "gui_random.h"
#include "color.h"
#include "tile.h"
#include "tile_draw.h"


Rectangle gui_random_panel_rect;
Rectangle gui_random_area_rect;
Rectangle gui_random_play_button_rect;
Rectangle gui_random_radius_rect;
Rectangle gui_random_color_label_rect;

char gui_random_panel_text[] = "Random Level";
char gui_random_play_button_text[] = "Play";
char gui_random_radius_text[] = "Tile Radius  ";
char gui_random_color_label_text[] = "Colors";

int gui_random_radius = LEVEL_MIN_RADIUS;
bool gui_random_color[PATH_TYPE_COUNT];

void init_gui_random(void)
{
    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        gui_random_color[type] = true; 
   }
}

void resize_gui_random(void)
{
    gui_random_panel_rect.width  = window_size.x * 0.4;
    gui_random_panel_rect.height = window_size.y * 0.45;

    MINVAR(gui_random_panel_rect.width,  400);
    MINVAR(gui_random_panel_rect.height, 450);

    gui_random_panel_rect.x = (window_size.x / 2) - (gui_random_panel_rect.width  / 2);
    gui_random_panel_rect.y = (window_size.y / 2) - (gui_random_panel_rect.height / 2);

    float panel_bottom = gui_random_panel_rect.y + gui_random_panel_rect.height;

    gui_random_area_rect.x      = gui_random_panel_rect.x + PANEL_INNER_MARGIN;
    gui_random_area_rect.y      = gui_random_panel_rect.y + PANEL_INNER_MARGIN + RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
    gui_random_area_rect.width  = gui_random_panel_rect.width - (2 * PANEL_INNER_MARGIN);
    gui_random_area_rect.height = panel_bottom - PANEL_INNER_MARGIN - gui_random_area_rect.y;

    Vector2 gui_random_radius_text_size = MeasureGuiText(gui_random_radius_text);

    gui_random_radius_rect.x      = gui_random_area_rect.x + gui_random_radius_text_size.x;
    gui_random_radius_rect.y      = gui_random_area_rect.y;
    gui_random_radius_rect.width  = 90; //gui_random_area_rect.width - gui_random_radius_text_size.x;
    gui_random_radius_rect.height = 30;

    gui_random_area_rect.y      += gui_random_radius_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_radius_rect.height + RAYGUI_ICON_SIZE;

    gui_random_play_button_rect.height = 3 * RAYGUI_ICON_SIZE;
    gui_random_play_button_rect.width  = gui_random_area_rect.width;
    gui_random_play_button_rect.x      = gui_random_area_rect.x;
    gui_random_play_button_rect.y      = gui_random_area_rect.y + gui_random_area_rect.height - gui_random_play_button_rect.height;

    Vector2 color_label_text_size = MeasureGuiText(gui_random_color_label_text);

    gui_random_color_label_rect.x      = gui_random_area_rect.x;
    gui_random_color_label_rect.y      = gui_random_area_rect.y;
    gui_random_color_label_rect.width  = color_label_text_size.x;
    gui_random_color_label_rect.height = TOOL_BUTTON_HEIGHT;


}

static void draw_gui_random_colors(void)
{
    GuiLabel(gui_random_color_label_rect, gui_random_color_label_text);

    Rectangle rect = {
        .x = gui_random_color_label_rect.x + gui_random_color_label_rect.width + RAYGUI_ICON_SIZE,
        .y = gui_random_color_label_rect.y,
        .width  = TOOL_BUTTON_WIDTH,
        .height = TOOL_BUTTON_HEIGHT
    };

    int color_button_segments = 12;
    float color_button_roundness = 0.5;
    float color_button_line_thickness = 1.0;
    float cross_thickness = 2.0;
    Color cross_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    cross_color = ColorAlpha(cross_color, 0.7);
    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        bool hover = CheckCollisionPointRec(mouse_positionf, rect);

        DrawRectangleRounded(rect, color_button_roundness, color_button_segments,
                             hover ? path_type_highlight_color(type) : path_type_color(type));
        if (gui_random_color[type]) {
            DrawRectangleRoundedLines(rect, color_button_roundness, color_button_segments, color_button_line_thickness, WHITE); 
        } else {
            Vector2 s1 = { rect.x,              rect.y               };
            Vector2 e1 = { rect.x + rect.width, rect.y + rect.height };
            Vector2 s2 = { rect.x + rect.width, rect.y               };
            Vector2 e2 = { rect.x,              rect.y + rect.height };
            DrawLineEx(s1, e1, cross_thickness, cross_color);
            DrawLineEx(s2, e2, cross_thickness, cross_color);
        }

        if (hover) {
            if (mouse_left_click) {
                gui_random_color[type] = !gui_random_color[type];
            } else {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            }
        }

        rect.x += TOOL_BUTTON_WIDTH + ICON_BUTTON_SIZE;
    }
}

void draw_gui_random(void)
{
    GuiPanel(gui_random_panel_rect, gui_random_panel_text);

    GuiSpinner(gui_random_radius_rect, gui_random_radius_text, &gui_random_radius, LEVEL_MIN_RADIUS, LEVEL_MAX_RADIUS, false);

    draw_gui_random_colors();

    bool colors_ok = false;
    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        colors_ok = colors_ok || gui_random_color[type];
    }

    if (!colors_ok) {
        GuiDisable();
    }

    if (GuiButton(gui_random_play_button_rect, gui_random_play_button_text)) {
        printf("play\n");
    }

    if (!colors_ok) {
        GuiEnable();
    }
}
