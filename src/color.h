/****************************************************************************
 *                                                                          *
 * color.h                                                                  *
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

#ifndef COLOR_H
#define COLOR_H

//             rrggbbaa
//      e.g. "#abcdef12"             4x(2 chars) + '#' + '\0'
#define COLOR_OPTION_STRING_LENGTH 10
struct color_option {
    Color color;
    Color highlight_color;
    Color hover_color;
    Color disabled_hover_color;
    Color disabled_color;
    Color default_color;
    char rgb_string[COLOR_OPTION_STRING_LENGTH];
    char rgba_string[COLOR_OPTION_STRING_LENGTH];
    unsigned char rgba[4];
};
typedef struct color_option color_option_t;

void color_option_set(color_option_t *c_opt, Color new_color);
bool color_option_set_string(color_option_t *c_opt, const char *new_color_text);

extern Color TRANSPARENT_BLACK;
extern Color LIGHT_ORANGE;
extern Color CHARTREUSE;
extern Color DEEP_SKY_BLUE;
extern Color DODGER_BLUE;
extern Color DEEP_PINK;
extern Color CGOLD;

extern Color cursor_outer_color;
extern Color cursor_inner_color;

extern Color magenta;
extern Color purple;
extern Color royal_blue;


extern Color tile_bg_color;
extern Color tile_bg_hover_color;
extern Color tile_bg_drag_color;
extern Color tile_bg_hidden_color;
extern Color tile_center_color;
extern Color tile_center_color_hover;
extern Color tile_edge_color;
extern Color tile_edge_hover_color;
extern Color tile_edge_drag_color;
extern Color tile_edge_hidden_color;
extern Color tile_edge_finished_color;

extern Color tile_bg_highlight_color;
extern Color tile_bg_highlight_color_dim;

extern Color panel_bg_color;
extern Color panel_edge_color;
extern Color panel_header_text_color;
extern Color panel_bg_hover_color;
extern Color panel_edge_hover_color;
extern Color panel_header_text_hover_color;
extern Color panel_header_label_bg_color;

extern Color error_cross_color;
extern Color text_shadow_color;

extern Color path_color_none;

extern Color feedback_bg_tint_color;

void prepare_global_colors();
bool color_eq(Color a, Color b);

#endif /*COLOR_H*/

