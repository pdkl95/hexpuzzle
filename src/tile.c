/****************************************************************************
 *                                                                          *
 * tile.c                                                                   *
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
#include "raylib_helper.h"
#include "tile.h"

Vector2 tile_origin;

Color tile_bg_color           = { 0x32, 0x32, 0x32, 0xff };
Color tile_bg_hover_color     = { 0x40, 0x40, 0x40, 0xff };
Color tile_bg_drag_color      = { 0x3b, 0x3b, 0x3b, 0xff };
Color tile_bg_hidden_color    = { 0x32, 0x32, 0x32, 0x33 };
Color tile_center_color       = { 0x70, 0x70, 0x70, 0xff };
Color tile_center_color_hover = { 0x90, 0x90, 0x90, 0xff };
Color tile_edge_color         = { 0x18, 0x18, 0x18, 0xff };
Color tile_edge_hover_color   = { 0xaa, 0xaa, 0xaa, 0xff };
Color tile_edge_drag_color    = { 0x77, 0x77, 0x77, 0xff };
Color tile_edge_hidden_color  = { 0xe9, 0xdf, 0x9c, 0x44 };

Color tile_bg_highlight_color = { 0xfd, 0xf9, 0x00, 0x4c };

Color path_color_none   = { 0, 0, 0, 0 };
Color path_color_red    = RED;
Color path_color_blue   = BLUE;
Color path_color_yellow = YELLOW;
Color path_color_green  = GREEN;

Color path_type_color(path_type_t type)
{
    switch (type) {
    default:
        return path_color_none;

    case PATH_TYPE_RED:
        return path_color_red;

    case PATH_TYPE_BLUE:
        return path_color_blue;

    case PATH_TYPE_YELLOW:
        return path_color_yellow;

    case PATH_TYPE_GREEN:
        return path_color_green;
    }
}

tile_t *init_tile(tile_t *tile, hex_axial_t pos)
{
    assert_not_null(tile);

    tile->enabled = true;
    tile->fixed = false;
    tile->hidden = false;

    tile->position = pos;

#if 0
    for (int i=0; i<6; i++) {
        tile->path[i] = rand() % PATH_TYPE_MAX;
    }
#endif

    return tile;
}

tile_t *create_tile(void)
{
    tile_t *tile = calloc(1, sizeof(tile_t));
    hex_axial_t pos = {0};
    return init_tile(tile, pos);
}

static void tile_set_flag_from_char(tile_t *tile, char c)
{
    assert_not_null(tile);

    switch (c) {
    case 'e': tile->enabled = false;  break;
    case 'E': tile->enabled = true;   break;
    case 'f': tile->fixed   = false;  break;
    case 'F': tile->fixed   = true;   break;
    case 'h': tile->hidden  = false;  break;
    case 'H': tile->hidden  = true;   break;

    default:
        warnmsg("Invalid flag character: '%c'", c);
        break;
    }
}

tile_t *create_tile_from_serialized_strings(char *addr, char *path, char *flags)
{
    assert_not_null(addr);
    assert_not_null(path);
    assert_not_null(flags);
    assert(strlen(addr)  >= 3);
    assert(strlen(path)  == 6);
    assert(strlen(flags) == 3);

#if 0
    printf("Creating tile from: addr=\"%s\" path=\"%s\" flags=\"%s\"\n",
           addr, path, flags);
#endif

    tile_t *tile = create_tile();

    hex_axial_t pos = {0};
    char *p = addr;
    pos.q = (int)strtol(addr, &p, 10);
    p++;
    pos.r = (int)strtol(p, NULL, 10);

    tile->position = pos;

    for (int i=0; i<6; i++) {
        char digit[2];
        digit[0] = path[i];
        digit[1] = '\0';

        tile->path[i] = (int)strtol(digit, NULL, 10);
    }

    tile_set_flag_from_char(tile, flags[0]);
    tile_set_flag_from_char(tile, flags[1]);
    tile_set_flag_from_char(tile, flags[2]);

    return tile;
}

void destroy_tile(tile_t *tile)
{
    if (tile) {
        if (tile->next) {
            destroy_tile(tile);
        }

        SAFEFREE(tile);
    }
}

void tile_copy_attributes(tile_t *dst, tile_t *src)
{
    assert_not_null(dst);
    assert_not_null(src);

    dst->enabled = src->enabled;
    dst->fixed   = src->fixed;
    dst->hidden  = src->hidden;

    for (int i=0; i<6; i++) {
        dst->path[i] = src->path[i];
    }
}

void tile_swap_attributes(tile_t *a, tile_t *b)
{
    assert_not_null(a);
    assert_not_null(b);

    tile_t tmp;
    tile_copy_attributes(&tmp, a);
    tile_copy_attributes(a, b);
    tile_copy_attributes(b, &tmp);
}

void tile_set_hover(tile_t *tile, Vector2 mouse_pos)
{
    assert_not_null(tile);

    Vector2 relvec = Vector2Subtract(mouse_pos, tile->center);
    float theta = atan2f(-relvec.y, -relvec.x);
    theta += TAU/2.0;
    theta = TAU - theta;
    tile->hover = true;
    tile->hover_section = (int)(theta/TO_RADIANS(60.0));
    tile->hover_center =
        (Vector2DistanceSqr(mouse_pos, tile->center)
         < (tile->center_circle_hover_radius *
            tile->center_circle_hover_radius));
}

void tile_unset_hover(tile_t *tile)
{
    assert_not_null(tile);

    tile->hover = false;
    tile->hover_center = false;
}

void tile_toggle_fixed(tile_t *tile)
{
    assert_not_null(tile);

    tile->fixed = !tile->fixed;
}

void tile_toggle_hidden(tile_t *tile)
{
    assert_not_null(tile);

    tile->hidden = !tile->hidden;
}

void tile_cycle_path_section(tile_t *tile, hex_direction_t section)
{
    assert_not_null(tile);

    tile->path[section]++;
    if (tile->path[section] > PATH_TYPE_MAX) {
        tile->path[section] = PATH_TYPE_NONE;
    }
}

void tile_modify_hovered_feature(tile_t *tile)
{
    assert_not_null(tile);

    if (tile->hidden) {
        tile_toggle_hidden(tile);
    } else {
        if (tile->hover_center) {
            if (IsKeyDown(KEY_LEFT_SHIFT) ||
                IsKeyDown(KEY_RIGHT_SHIFT)) {
                tile_toggle_hidden(tile);
            } else {
                tile_toggle_fixed(tile);
            }
        } else {
            tile_cycle_path_section(tile, tile->hover_section);
        }
    }
}

void tile_set_size(tile_t *tile, float tile_size)
{
    assert_not_null(tile);

    tile->size = tile_size;
    tile->line_width = tile->size / 6.0;
    tile->center_circle_draw_radius = tile->line_width * 1.2;
    tile->center_circle_hover_radius = tile->line_width * 1.6;
    tile->center = hex_axial_to_pixel(tile->position, tile->size);

    Vector2 *corners = hex_pixel_corners(tile->center, tile->size);
    memcpy(tile->corners, corners, 7 * sizeof(Vector2));

    Vector2 cent = Vector2Lerp(tile->midpoints[0], tile->midpoints[3], 0.5);

    for (int i=0; i<6; i++) {
        Vector2 c0 = tile->corners[i];
        Vector2 c1 = tile->corners[i + 1];

        tile->midpoints[i] = Vector2Lerp(c0, c1, 0.5);

        tile->sections[i].corners[0] = c0;
        tile->sections[i].corners[1] = c1;
        tile->sections[i].corners[2] = cent;
    }
}

void tile_draw(tile_t *tile, tile_t *drag_target)
{
    assert_not_null(tile);
    /* drag_target CAN be NULL */

    bool drag = (tile == drag_target);
    bool dragged_over = (!drag && drag_target && tile->hover);

    if (tile->hidden) {
        if (edit_mode) {
            float hiddensize = tile->size - (tile->size * 0.08);
            DrawPoly(tile->center, 6, hiddensize, 0.0f,
                     dragged_over
                     ? tile_bg_hover_color
                     : tile_bg_hidden_color);
            DrawPolyLinesEx(tile->center, 6, hiddensize, 0.0f, 2.0f,
                            dragged_over
                            ? tile_edge_hover_color
                            : tile_edge_hidden_color);
        }

        return;
    }

    if (!tile->fixed) {
        /* background */
        DrawPoly(tile->center, 6, tile->size, 0.0f,
                 drag
                 ? tile_bg_drag_color
                 : (tile->hover
                    ? tile_bg_hover_color
                    : tile_bg_color));
    }

    if (edit_mode &&
        tile->hover &&
        !tile->hover_center &&
        !drag_target
    ) {
        /* section highlight */
        tile_section_t sec = tile->sections[tile->hover_section];
        DrawTriangle(sec.corners[0], sec.corners[1], sec.corners[2], tile_bg_highlight_color);
    }

    for (int i=0; i<6; i++) {
        /* colored strips */
        Vector2 mid = tile->midpoints[i];
        DrawLineEx(tile->center, mid, tile->line_width, path_type_color(tile->path[i]));

#if 0
        /* section index label */
        Vector2 offset = Vector2Scale(Vector2Subtract(tile->center, mid), 0.2);;
        Vector2 mlabel = Vector2Add(mid, offset);
        DrawTextShadow(TextFormat("%d", i), mlabel.x - 5, mlabel.y - 9, 18, RAYWHITE);
#endif
    }

    if (!tile->fixed) {
        /* border */
        DrawPolyLinesEx(tile->center, 6, tile->size, 0.0f, 2.0f,
                        drag
                        ? tile_edge_drag_color
                        : (tile->hover
                           ? tile_edge_hover_color
                           : tile_edge_color));
    }

    DrawCircleV(tile->center, tile->center_circle_draw_radius, tile_center_color);

    if (edit_mode && tile->hover_center) {
        DrawCircleV(tile->center, tile->center_circle_draw_radius, tile_bg_highlight_color);
    }

#if 0
    if (drag) {
        return;
    }

    /* show each hex's axial coordinates */
    int font_size = 14;
    const char *coord_text = TextFormat("%d,%d", tile->position.q, tile->position.r);
    int text_width = MeasureText(coord_text, font_size);
    DrawTextDropShadow(coord_text, tile->center.x - (text_width/2), tile->center.y + 14, font_size, WHITE, BLACK);
#endif
}

void tile_serialize(tile_t *tile, FILE *f)
{
    fprintf(f, "tile %d,%d %d%d%d%d%d%d %s%s%s\n",
            tile->position.q,
            tile->position.r,

            tile->path[0],
            tile->path[1],
            tile->path[2],
            tile->path[3],
            tile->path[4],
            tile->path[5],

            tile->enabled ? "E" : "e",
            tile->fixed   ? "F" : "f",
            tile->hidden  ? "H" : "h");
}
