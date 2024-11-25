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
font_handle_t panel_font;
font_handle_t name_font;
font_handle_t big_button_font;

font_handle_t *current_font;

static inline font_handle_t load_standard_font(int size, int filter)
{
    font_handle_t fh = {
        .size    = size,
        .spacing = 2.0f,
        .font = LoadFontFromMemory(".ttf",
                                   fonts_PalanquinDark_Regular_ttf,
                                   fonts_PalanquinDark_Regular_ttf_len,
                                   2.9 * size,
                                   NULL,
                                   0)
    };

    SetTextureFilter(fh.font.texture, filter);

    return fh;
}

void load_fonts(void)
{
    gui_font        = load_standard_font(26, TEXTURE_FILTER_BILINEAR);
    panel_font      = load_standard_font(32, TEXTURE_FILTER_BILINEAR);
    name_font       = load_standard_font(36, TEXTURE_FILTER_BILINEAR);
    big_button_font = load_standard_font(48, TEXTURE_FILTER_POINT);
}

void unload_fonts(void)
{
    UnloadFont(big_button_font.font);
    UnloadFont(name_font.font);
    UnloadFont(panel_font.font);
    UnloadFont(gui_font.font);
}

void set_font(font_handle_t *fh)
{
    current_font = fh;

    GuiSetFont(fh->font);
    GuiSetStyle(DEFAULT, TEXT_SIZE,    fh->size);
    GuiSetStyle(DEFAULT, TEXT_SPACING, fh->spacing);
}
