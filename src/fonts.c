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

Font font16;
Font font18;
Font font20;

void load_fonts(void)
{
    //font20 = LoadFont("fonts/terminus-20.png");
    font20 = LoadFontEx("fonts/TerminusMedium-4.38.ttf", 20, NULL, 0);
    SetTextureFilter(font20.texture, TEXTURE_FILTER_POINT);

    //font18 = LoadFont("fonts/fira-18.png");
    font18 = LoadFontEx("fonts/FiraSansOT-Medium.otf", 36, NULL, 0);
    SetTextureFilter(font18.texture, TEXTURE_FILTER_BILINEAR);

    //font16 = LoadFont("fonts/sourcecodesanspro-semibold-16.png");
    font16 = LoadFontEx("fonts/SourceSansPro-Semibold.ttf", 32, NULL, 0);
    //font16 = LoadFontEx("fonts/TerminusMedium-4.38.ttf", 16, NULL, 0);
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
    GuiSetStyle(DEFAULT, TEXT_SIZE, DEFAULT_GUI_FONT_SIZE);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
}

#define def_set_gui_font(size)                  \
    void set_gui_font##size(void)               \
    {                                           \
        GuiSetFont(font20);                     \
        GuiSetStyle(DEFAULT, TEXT_SIZE, size);  \
        GuiSetStyle(DEFAULT, TEXT_SPACING, 1);  \
    }
def_set_gui_font(16)
def_set_gui_font(18)
def_set_gui_font(20)
#undef def_set_gui_font
