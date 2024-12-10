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

struct font_handle {
    Font font;
    float size;
    float spacing;
    bool use_color;
    int color;
};
typedef struct font_handle font_handle_t;

extern font_handle_t *current_font;

void load_fonts(void);
void unload_fonts(void);

void set_font(font_handle_t *fh);

#define MeasureTextWithFont(fh, str) \
    MeasureTextEx(fh.font,           \
                  str,               \
                  fh.size,           \
                  fh.spacing)

#define draw_text_with_font(fh, str, position, tint) \
    DrawTextEx(fh.font,                              \
               str,                                  \
               position,                             \
               fh.size,                              \
               fh.spacing,                           \
               tint)

#define deffont(name)                                                  \
                                                                       \
    extern font_handle_t name ## _font;                                \
                                                                       \
    static inline void set_ ## name ## _font(void)                     \
    {                                                                  \
        set_font(&(name ## _font));                                    \
    }                                                                  \
                                                                       \
    static inline Vector2 measure_ ## name ## _text(const char *str)   \
    {                                                                  \
        return MeasureTextEx(name ## _font.font,                       \
                             str,                                      \
                             name ## _font.size,                       \
                             name ## _font.spacing);                   \
    }                                                                  \
                                                                       \
    static inline void draw_ ## name ## _text(const char *str,         \
                                              Vector2 position,        \
                                              Color tint)              \
    {                                                                  \
        DrawTextEx(name ## _font.font,                                 \
                   str,                                                \
                   position,                                           \
                   name ## _font.size,                                 \
                   name ## _font.spacing,                              \
                   tint);                                              \
    }

deffont(gui);
deffont(gui_narrow);
deffont(panel);
deffont(name);
deffont(big_button);

#undef deffont

#define set_default_font() set_gui_font()

#endif /*FONTS_H*/
