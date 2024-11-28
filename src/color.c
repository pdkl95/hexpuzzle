/****************************************************************************
 *                                                                          *
 * color.c                                                                  *
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
#include "options.h"
#include "color.h"

Color TRANSPARENT_BLACK  = { 0, 0, 0, 0 };
Color LIGHT_ORANGE       = { 255, 249, 187, 255 };
Color CHARTREUSE         = { 194, 241, 146, 255 };
Color DEEP_SKY_BLUE      = { 176, 224, 230, 255 };
Color DODGER_BLUE        = { 0x1E, 0x90, 0xFF, 255 };
Color DEEP_PINK          = { 0xFF, 0x14, 0x93, 255 };
Color CGOLD              = { 0xFF, 0xD7, 0x00, 255 };
Color cursor_outer_color;
Color cursor_inner_color;

Color magenta    = { 214,   2, 112, 255 }; // d60270
Color purple     = { 155,  79, 150, 255 }; // 9b4f96
Color royal_blue = {   0,  56, 168, 255 }; // 0038a8


Color tile_bg_color            = { 0x32, 0x32, 0x32, 0xff };
Color tile_bg_hover_color      = { 0x40, 0x40, 0x40, 0xff };
Color tile_bg_drag_color       = { 0x3b, 0x3b, 0x3b, 0xff };
Color tile_bg_hidden_color     = { 0x32, 0x32, 0x32, 0x33 };
Color tile_center_color        = { 0x70, 0x70, 0x70, 0xff };
Color tile_center_color_hover  = { 0x90, 0x90, 0x90, 0xff };
Color tile_edge_color          = { 0x18, 0x18, 0x18, 0xff };
Color tile_edge_hover_color    = { 0xaa, 0xaa, 0xaa, 0xff };
Color tile_edge_drag_color     = { 0x77, 0x77, 0x77, 0xff };
Color tile_edge_hidden_color   = { 0xe9, 0xdf, 0x9c, 0x44 };
Color tile_edge_finished_color = { 0x9b, 0xff, 0x9b, 0x80 };

Color tile_bg_highlight_color     = { 0xfd, 0xf9, 0x00, 0x4c };
Color tile_bg_highlight_color_dim = { 0xfd, 0xf9, 0x00, 0x2c };

Color panel_bg_color                = { 0x72, 0x1C, 0xB8, 0xaa };
Color panel_edge_color              = { 0x94, 0x83, 0xA2, 0xcc };
Color panel_header_text_color       = { 0xF0, 0xEA, 0xFF, 0xff };
Color panel_bg_hover_color          = { 0x72, 0x1C, 0xB8, 0xaa };
Color panel_edge_hover_color        = { 0x94, 0x83, 0xA2, 0xcc };
Color panel_header_text_hover_color = { 0xFF, 0xFF, 0xFF, 0xff };
Color panel_header_label_bg_color   = { 0x4B, 0x0C, 0x7F, 0xC0 };

Color error_cross_color = { 0xCC, 0x00, 0x00, 0xDD };

Color text_shadow_color = BLACK;

Color path_color_none = OPTIONS_DEFAULT_PATH_COLOR_0;


#define FEEDBACK_BG_TINT 0xc0
Color feedback_bg_tint_color = { FEEDBACK_BG_TINT, FEEDBACK_BG_TINT, FEEDBACK_BG_TINT, 0xff };


void prepare_global_colors()
{
    //cursor_outer_color = DODGER_BLUE;
    cursor_outer_color = CGOLD;
    cursor_outer_color = ColorAlpha(cursor_outer_color, 0.86);

    cursor_inner_color = DEEP_PINK;
    cursor_inner_color = ColorAlpha(cursor_inner_color, 0.65);

    panel_bg_hover_color          = ColorBrightness(panel_bg_color,   -0.3);
    panel_edge_hover_color        = ColorBrightness(panel_edge_color,  0.3);
    panel_header_text_hover_color = WHITE;

    text_shadow_color = ColorAlpha(BLACK, 0.85);
}

void color_option_set(color_option_t *c_opt, Color new_color)
{
    assert_not_null(c_opt);

    c_opt->color = new_color;
    c_opt->highlight_color      = ColorBrightness(new_color, -0.25);
    c_opt->hover_color          = ColorBrightness(new_color,  0.1);

    c_opt->disabled_color       = ColorBrightness(new_color, -0.07);
    Vector3 hsv = ColorToHSV(c_opt->disabled_color);
    hsv.y *= 0.5;
    c_opt->disabled_color = ColorFromHSV(hsv.x, hsv.y, hsv.z);

    c_opt->disabled_hover_color = ColorLerp(c_opt->color, c_opt->disabled_color, 0.5);

    c_opt->rgba[0] = new_color.r;
    c_opt->rgba[1] = new_color.g;
    c_opt->rgba[2] = new_color.b;
    c_opt->rgba[3] = new_color.a;

    snprintf(c_opt->rgba_string,
             COLOR_OPTION_STRING_LENGTH,
             "#%02x%02x%02x%02x",
             new_color.r,
             new_color.g,
             new_color.b,
             new_color.a);

    snprintf(c_opt->rgb_string,
             COLOR_OPTION_STRING_LENGTH,
             "#%02x%02x%02x",
             new_color.r,
             new_color.g,
             new_color.b);
};

static unsigned char parse_hex_byte(const char *p)
{
    char buf[5] = {'0', 'x', p[0], p[1], '\0'};
    return strtol(buf, NULL, 0);
}

static unsigned char parse_hex_nibble(const char *p)
{
    char buf[5] = {'0', 'x', p[0], p[0], '\0'};
    return strtol(buf, NULL, 0);
}

bool color_option_set_string(color_option_t *c_opt, const char *new_color_text)
{
    assert_not_null(c_opt);
    assert_not_null(new_color_text);

    const char *p = new_color_text;
    if (p[0] == '#') {
        p++;
    } else {
        //errmsg("Cannot parse \"%s\" as a color - missing leading '#'");
        return false;
    }

    Color color = BLACK;

    switch (strlen(p)) {
    case 3:  // RGB
        color.r = parse_hex_nibble(p);
        color.g = parse_hex_nibble(p + 1);
        color.b = parse_hex_nibble(p + 2);
        break;

    case 4:  // RGBA
        color.r = parse_hex_nibble(p);
        color.g = parse_hex_nibble(p + 1);
        color.b = parse_hex_nibble(p + 2);
        color.a = parse_hex_nibble(p + 3);
        break;

    case 6:  // RRGGBB
        color.r = parse_hex_byte(p);
        color.g = parse_hex_byte(p + 2);
        color.b = parse_hex_byte(p + 4);
        color.a = 0xff;
        break;

    case 8:  // RRGGBBAA
        color.r = parse_hex_byte(p);
        color.g = parse_hex_byte(p + 2);
        color.b = parse_hex_byte(p + 4);
        color.a = parse_hex_byte(p + 6);
        break;

    default:
        //errmsg("Cannot parse \"%s\" as a color - bad length");
        return false;
    }

    color_option_set(c_opt, color);

    return true;
}

bool color_eq(Color a, Color b)
{
    return ((a.r == b.r) &&
            (a.g == b.g) &&
            (a.b == b.b) &&
            (a.a == b.a));
}
