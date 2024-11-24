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

Font font20;
Font font18;
Font font16;

int gui_font_size = DEFAULT_GUI_FONT_SIZE;

void load_fonts(void)
{
    font20 = LoadFontFromMemory(".ttf",
                                fonts_PalanquinDark_Regular_ttf,
                                fonts_PalanquinDark_Regular_ttf_len,
                                36, NULL, 0);
    SetTextureFilter(font20.texture, TEXTURE_FILTER_POINT);

    font18 = LoadFontFromMemory(".ttf",
                                fonts_PalanquinDark_Regular_ttf,
                                fonts_PalanquinDark_Regular_ttf_len,
                                2 * gui_font_size, NULL, 0);
//36, NULL, 0);
    SetTextureFilter(font18.texture, TEXTURE_FILTER_BILINEAR);

    font16 = LoadFontFromMemory(".ttf",
                                fonts_PalanquinDark_Regular_ttf,
                                fonts_PalanquinDark_Regular_ttf_len,
                                32, NULL, 0);
    SetTextureFilter(font16.texture, TEXTURE_FILTER_BILINEAR);
}

void unload_fonts(void)
{
    UnloadFont(font16);
    UnloadFont(font18);
    UnloadFont(font20);
}

void set_default_gui_font(void)
{
    GuiSetFont(DEFAULT_GUI_FONT);
    GuiSetStyle(DEFAULT, TEXT_SIZE, gui_font_size);  //DEFAULT_GUI_FONT_SIZE);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
}

void set_gui_font20(void)
{
    GuiSetFont(font20);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 36);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
}

void set_gui_font18(void)
{
    GuiSetFont(font18);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 26);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
}

void set_gui_font16(void)
{
    GuiSetFont(font16);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 32);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
}
