/****************************************************************************
 *                                                                          *
 * fonts.h                                                                  *
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

#ifndef FONTS_H
#define FONTS_H

extern Font font16;
extern Font font18;
extern Font font20;

void load_fonts(void);
void unload_fonts(void);

void set_gui_font20(void);
void set_gui_font18(void);
void set_gui_font16(void);
void set_default_gui_font(void);

#define MeasureGuiText(str)                  \
    MeasureTextEx(DEFAULT_GUI_FONT,          \
                  str,                       \
                  DEFAULT_GUI_FONT_SIZE,     \
                  DEFAULT_GUI_FONT_SPACING)

#endif /*FONTS_H*/

