/****************************************************************************
 *                                                                          *
 * gui_title.c                                                              *
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
#include "fonts.h"
#include "level.h"
#include "level_draw.h"
#include "gui_random.h"
#include "gui_title.h"

Rectangle panel_rect;
Rectangle bg_level_rect;
Rectangle title1_text_rect;
Rectangle title2_text_rect;

Vector2 title1_text_position;
Vector2 title2_text_position;
Vector2 title1_text_shadow_position;
Vector2 title2_text_shadow_position;

char title1_text[] = "Hex";
char title2_text[] = "Puzzle";

float hexpanel_size;
float hexpanel_rotation = TO_DEGREES(TAU / 12.0f);

Color title_text_color;
font_handle_t title_font = {0};

float title1_voffset = -35.0f;
float title2_voffset = -28.0f;
float title2_hoffset = 5.0f;

level_t *bg_level = NULL;

Color shade_overlay_color;

void init_gui_title(void)
{
    title_text_color = purple;

    //title_font.font = big_button_font.font;
    //title_font.size = 4.0f * big_button_font.size;
    //title_font.spacing = big_button_font.spacing;

    title_font.font = name_font.font;
    title_font.size = 3.0f * name_font.size;
    title_font.spacing = name_font.spacing;

    title_font.use_color = false;
    title_font.color = 0;

    shade_overlay_color = ColorAlpha(ColorBrightness(text_shadow_color, 0.22), 0.5);
    resize_gui_title();
}

void cleanup_gui_title(void)
{
    if (bg_level) {
        destroy_level(bg_level);
    }
}

void resize_gui_title(void)
{
    hexpanel_size = (window_size.x * 0.65) / 2.0;

    panel_rect.width = window_size.x * 0.65;

    float level_margin = 5.0f;
    bg_level_rect.width  =    panel_rect.width - (2.0 * level_margin);
    bg_level_rect.height = bg_level_rect.width - (2.0 * level_margin);
    bg_level_rect.x      = window_center.x - (bg_level_rect.width  / 2.0f);
    bg_level_rect.y      = window_center.y - (bg_level_rect.height / 2.0f);

    Vector2 title1_text_size = MeasureTextWithFont(title_font, title1_text);
    Vector2 title2_text_size = MeasureTextWithFont(title_font, title2_text);

    title1_text_rect.width  = title1_text_size.x;
    title1_text_rect.height = title1_text_size.y;
    title2_text_rect.width  = title2_text_size.x;
    title2_text_rect.height = title2_text_size.y;

    title1_text_rect.x = window_center.x - (title1_text_rect.width / 2);
    title2_text_rect.x = window_center.x - (title2_text_rect.width / 2) + title2_hoffset;
    title1_text_rect.y = window_center.y - title1_text_rect.height + title1_voffset;
    title2_text_rect.y = window_center.y + title2_voffset;

    title1_text_position = getVector2FromRectangle(title1_text_rect);
    title2_text_position = getVector2FromRectangle(title2_text_rect);

    Vector2 shadow_offset = {
        .x = 2.0f,
        .y = 2.0f
    };
    title1_text_shadow_position = Vector2Add(title1_text_position, shadow_offset);
    title2_text_shadow_position = Vector2Add(title2_text_position, shadow_offset);
}

void draw_gui_title(void)
{
    if (!bg_level) {
        bg_level = generate_random_title_level();
    }

    DrawPoly(       window_center, 6, hexpanel_size + 2.0f, hexpanel_rotation, ColorAlpha(panel_bg_color, 0.15));
    level_preview_solved(bg_level, bg_level_rect);
    DrawPoly(       window_center, 6, hexpanel_size + 2.0f, hexpanel_rotation, shade_overlay_color);

    DrawPolyLinesEx(window_center, 6, hexpanel_size - 0.0f, hexpanel_rotation, 5.0f, text_shadow_color);
    DrawPolyLinesEx(window_center, 6, hexpanel_size - 2.0f, hexpanel_rotation, 1.0f, magenta);
    DrawPolyLinesEx(window_center, 6, hexpanel_size + 2.0f, hexpanel_rotation, 3.0f, royal_blue);

    draw_text_with_font(title_font, title1_text, title1_text_shadow_position, text_shadow_color);
    draw_text_with_font(title_font, title2_text, title2_text_shadow_position, text_shadow_color);

    draw_text_with_font(title_font, title1_text, title1_text_position, title_text_color);
    draw_text_with_font(title_font, title2_text, title2_text_position, title_text_color);
}
