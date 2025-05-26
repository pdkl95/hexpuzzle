/****************************************************************************
 *                                                                          *
 * fonts.c                                                                  *
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
#include "fonts.h"
#include "data_fonts.h"

font_handle_t gui_font;
font_handle_t gui_narrow_font;
font_handle_t panel_font;
font_handle_t name_font;
font_handle_t big_button_font;

font_handle_t *current_font;

bool restore_color_next_change = false;
int restore_color;

static inline font_handle_t load_font(int size, int filter, unsigned char *comp_data, unsigned int comp_data_length)
{
    int font_data_length = 0;
    unsigned char *font_data = DecompressData(comp_data, comp_data_length, &font_data_length);
    assert_not_null(font_data);

    font_handle_t fh = {
        .size    = size,
        .spacing = 2.0f,
        .font = LoadFontFromMemory(".ttf",
                                   font_data,
                                   font_data_length,
                                   2.0 * size,
                                   NULL,
                                   0)
    };

    if (filter == TEXTURE_FILTER_TRILINEAR) {
        GenTextureMipmaps(&fh.font.texture);
    }

    SetTextureFilter(fh.font.texture, filter);

    MemFree(font_data);

    return fh;
}

static inline void set_font_color(font_handle_t *fh, int color)
{
    fh->color = color;
    fh->use_color = true;
}

void load_fonts(void)
{
    gui_font        = load_font(16, TEXTURE_FILTER_TRILINEAR,
                                UBUNTU_MEDIUM_DATA,
                                UBUNTU_MEDIUM_DATA_SIZE);

    gui_narrow_font = load_font(15, TEXTURE_FILTER_TRILINEAR,
                                CABIN_CONDENSED_BOLD_DATA,
                                CABIN_CONDENSED_BOLD_DATA_SIZE);

    panel_font      = load_font(20, TEXTURE_FILTER_BILINEAR,
                                UBUNTU_MEDIUM_DATA,
                                UBUNTU_MEDIUM_DATA_SIZE);

    name_font       = load_font(36, TEXTURE_FILTER_BILINEAR,
                                UBUNTU_MEDIUM_DATA,
                                UBUNTU_MEDIUM_DATA_SIZE);

    big_button_font = load_font(42, TEXTURE_FILTER_BILINEAR,
                                JOCKEYONE_REGULAR_DATA,
                                JOCKEYONE_REGULAR_DATA_SIZE);

    set_font_color(&big_button_font, 0xc1c1c1ff);
}

void unload_fonts(void)
{
    UnloadFont(big_button_font.font);
    UnloadFont(name_font.font);
    UnloadFont(panel_font.font);
    UnloadFont(gui_narrow_font.font);
    UnloadFont(gui_font.font);
}

void set_font(font_handle_t *fh)
{
    if (restore_color_next_change) {
        restore_color_next_change = false;
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, restore_color);
    }

    current_font = fh;

    GuiSetFont(fh->font);
    GuiSetStyle(DEFAULT, TEXT_SIZE,    fh->size);
    GuiSetStyle(DEFAULT, TEXT_SPACING, fh->spacing);

    if (fh->use_color) {
        restore_color = GuiGetStyle(BUTTON, TEXT_COLOR_NORMAL);
        restore_color_next_change = true;

        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, fh->color);
    }
}
