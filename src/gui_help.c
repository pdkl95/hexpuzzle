/****************************************************************************
 *                                                                          *
 * gui_help.c                                                               *
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
#include "gui_help.h"
#include "gui_dialog.h"

bool show_help_box = false;

Rectangle help_box_panel_rect;
Rectangle help_box_area_rect;
Rectangle help_box_button_rect;
Rectangle help_box_key_rect;
Rectangle help_box_desc_rect;

char help_box_button_text_str[] = "Close";
#define HELP_BOX_BUTTON_TEXT_LENGTH (6 + sizeof(help_box_button_text_str))
char help_box_button_text[HELP_BOX_BUTTON_TEXT_LENGTH];

char help_box_panel_text[] = "Help: Hotkeys";

char help_box_hotkeys_title_text[] = "HOTKEYS";

struct hotkey_desc {
    const char *key;
    const char *key2;
    const char *desc;

    float  key_width;
    float key2_width;
    float desc_width;
};
typedef struct hotkey_desc hotkey_desc_t;

#if defined(__APPLE__)
# define CTRL_KEY "Cmd"
#else
# define CTRL_KEY "Ctrl"
#endif

hotkey_desc_t hotkey_desc[] = {
    { .key = "<H>", .key2 = "<F1>",                 .desc = "Show thie help text" },
    { .key = "<U>", .key2 = "<" CTRL_KEY "> + <Z>", .desc = "UEDO" },
    { .key = "<R>", .key2 = "<" CTRL_KEY "> + <Y>", .desc = "REDO" },
    { .key = "<B>",                                 .desc = "Toggle animated background" },
    { .key = "<P>",                                 .desc = "Toggle postprocessing shader" },
    { .key = "<F>",                                 .desc = "Show FPS" },
    { .key = "<Q>", .key2 = "<ESC>",                .desc = "Quit the program" },
    { .key = "<SHIFT> + <R>",                       .desc = "Reset window possition/size" }
};
#define NUM_HOTKEY_DESC ((long)NUM_ELEMENTS(hotkey_desc_t, hotkey_desc))

float max_hotkey_key_width  = 0.0f;
float max_hotkey_key2_width = 0.0f;
float max_hotkey_desc_width = 0.0f;

float hotkey_single_row_height = 0.0f;
float hotkey_double_row_height = 0.0f;

#define HELP_KEY_BG_NARGIN 5.0f
#define KEY_ROUNDNES 0.4f
#define KEY2_VSEP 2.0f

float get_str_render_width(const char *str)
{
    if (str) {
        Vector2 size = measure_gui_text(str);
        return size.x;
    } else {
        return 0.0f;
    }
}

void init_gui_help(void)
{
    memcpy(help_box_button_text,
           GuiIconText(ICON_FILETYPE_INFO, help_box_button_text_str),
           HELP_BOX_BUTTON_TEXT_LENGTH);
}

void cleanup_gui_help(void)
{
}

void resize_gui_help(void)
{
    hotkey_single_row_height = TOOL_BUTTON_HEIGHT + PANEL_INNER_MARGIN;
    hotkey_single_row_height += 7.0f;
    hotkey_double_row_height = hotkey_single_row_height + TOOL_BUTTON_HEIGHT + KEY2_VSEP;

    max_hotkey_key_width  = 0.0f;
    max_hotkey_key2_width = 0.0f;
    max_hotkey_desc_width = 0.0f;

    float text_height = 0.0f;

    for (int i=0; i<NUM_HOTKEY_DESC; i++) {
        hotkey_desc_t *hkd = &(hotkey_desc[i]);

        hkd->key_width  = get_str_render_width(hkd->key);
        hkd->key2_width = get_str_render_width(hkd->key2);
        hkd->desc_width = get_str_render_width(hkd->desc);

        max_hotkey_key_width   = MAX(max_hotkey_key_width,  hkd->key_width);
        max_hotkey_key2_width  = MAX(max_hotkey_key2_width, hkd->key2_width);
        max_hotkey_desc_width  = MAX(max_hotkey_desc_width, hkd->desc_width);

        if (hkd->key2) {
            text_height += hotkey_double_row_height;
        } else {
            text_height += hotkey_single_row_height;
        }
    }

    max_hotkey_key_width = MAX(max_hotkey_key_width, max_hotkey_key2_width);

    help_box_panel_rect = main_gui_area_rect;

    float key_desc_sep = 2.0f * PANEL_INNER_MARGIN;

    float extra_panel_width  = 4.0f * PANEL_INNER_MARGIN;
    float extra_panel_height = 2.0f * PANEL_INNER_MARGIN;

    float panel_width = (4.0f * PANEL_INNER_MARGIN)
        + max_hotkey_key_width
        + max_hotkey_desc_width
        + key_desc_sep
        + (2.0f * HELP_KEY_BG_NARGIN)
        + extra_panel_width;

    float panel_height = text_height
        + (3.0f * PANEL_INNER_MARGIN)
        + TOOL_BUTTON_HEIGHT
        + (float)(RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT - 1)
        + extra_panel_height;

    help_box_panel_rect.width  = panel_width;
    help_box_panel_rect.height = panel_height;

    help_box_panel_rect.x = (window_sizef.x / 2.0f) - (help_box_panel_rect.width  / 2.0f);
    help_box_panel_rect.y = (window_sizef.y / 2.0f) - (help_box_panel_rect.height / 2.0f);

    float panel_bottom = help_box_panel_rect.y + help_box_panel_rect.height;

    help_box_area_rect.x      = help_box_panel_rect.x + PANEL_INNER_MARGIN;
    help_box_area_rect.y      = help_box_panel_rect.y + PANEL_INNER_MARGIN + RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
    help_box_area_rect.width  = help_box_panel_rect.width - key_desc_sep;
    help_box_area_rect.height = panel_bottom - PANEL_INNER_MARGIN - help_box_area_rect.y;

    Vector2 help_box_button_text_size = measure_gui_text(help_box_button_text);

    help_box_button_rect.width  = help_box_button_text_size.x;
    help_box_button_rect.height = TOOL_BUTTON_HEIGHT;
    help_box_button_rect.x =
        help_box_area_rect.x
        + help_box_area_rect.width
        - help_box_button_rect.width;
    help_box_button_rect.y =
        help_box_area_rect.y
        + help_box_area_rect.height
        - help_box_button_rect.height;

    help_box_area_rect.height -= help_box_button_rect.height + PANEL_INNER_MARGIN;

    help_box_key_rect.x      = help_box_area_rect.x + PANEL_INNER_MARGIN;
    help_box_key_rect.y      = help_box_area_rect.y + PANEL_INNER_MARGIN;
    help_box_key_rect.width  = max_hotkey_key_width;
    help_box_key_rect.height = TOOL_BUTTON_HEIGHT;

    help_box_key_rect.x += HELP_KEY_BG_NARGIN * 2.0f;
    help_box_key_rect.y += HELP_KEY_BG_NARGIN;

    help_box_desc_rect.y = help_box_key_rect.y;
    help_box_desc_rect.x = help_box_key_rect.x + help_box_key_rect.width + (2.0f * PANEL_INNER_MARGIN);
    help_box_desc_rect.width  = max_hotkey_desc_width;
    help_box_desc_rect.height = help_box_key_rect.height;
}

void draw_gui_help(void)
{
    if (!show_help_box) {
        return;
    }

    draw_gui_modal_popup_background();
    
    GuiPanel(help_box_panel_rect, help_box_panel_text);

    Vector2  key_pos = getVector2FromRectangle( help_box_key_rect);
    Vector2 desc_pos = getVector2FromRectangle(help_box_desc_rect);

    Color bg_color = ColorAlpha(BLACK, 0.25);
    Color bg_border_color = panel_edge_color;

    for (int i=0; i<NUM_HOTKEY_DESC; i++) {
        hotkey_desc_t *hkd = &(hotkey_desc[i]);
        float width_offset  = max_hotkey_key_width - hkd->key_width;
        float width_offset2 = max_hotkey_key_width - hkd->key2_width;

        Rectangle key_bg = {
            .x = key_pos.x - HELP_KEY_BG_NARGIN + width_offset,
            .y = key_pos.y - HELP_KEY_BG_NARGIN,
            .width  = help_box_key_rect.width + (2.0f * HELP_KEY_BG_NARGIN) - width_offset,
            .height = help_box_key_rect.height
        };

        Rectangle key_bg2 = {
            .x = key_pos.x - HELP_KEY_BG_NARGIN + width_offset2,
            .y = key_pos.y - HELP_KEY_BG_NARGIN + TOOL_BUTTON_HEIGHT + KEY2_VSEP,
            .width  = help_box_key_rect.width + (2.0f * HELP_KEY_BG_NARGIN) - width_offset2,
            .height = help_box_key_rect.height
        };

        DrawRectangleRounded(key_bg, KEY_ROUNDNES, 0, bg_color);
        DrawRectangleRoundedLines(key_bg, KEY_ROUNDNES, 0, 1.0f, bg_border_color);
        if (hkd->key2) {
            DrawRectangleRounded(key_bg2, KEY_ROUNDNES, 0, bg_color);
            DrawRectangleRoundedLines(key_bg2, KEY_ROUNDNES, 0, 1.0f, bg_border_color);
        }

        Vector2 rkey_pos = key_pos;
        rkey_pos.x += width_offset;

        draw_gui_text(hkd->key,  rkey_pos, WHITE);
        draw_gui_text(hkd->desc, desc_pos, WHITE);

        if (hkd->key2) {
            rkey_pos.x = key_pos.x + width_offset2;
            rkey_pos.y = key_pos.y + TOOL_BUTTON_HEIGHT + KEY2_VSEP;

            draw_gui_text(hkd->key2, rkey_pos, WHITE);

            key_pos.y += hotkey_double_row_height;
        } else {
            key_pos.y += hotkey_single_row_height;
        }
        desc_pos.y = key_pos.y;
    }

    GuiUnlock();

    if (GuiButton(help_box_button_rect, help_box_button_text)) {
        show_help_box = false;
    }

    GuiLock();
}
