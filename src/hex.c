/****************************************************************************
 *                                                                          *
 * hex.c                                                                    *
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

#include <math.h>

#include "common.h"
#include "hex.h"

hex_axial_t hex_axial_direction_vectors[6] = {
    { .q =  1, .r =  0 },
    { .q =  1, .r = -1 },
    { .q =  0, .r = -1 },
    { .q = -1, .r =  0 },
    { .q = -1, .r =  1 },
    { .q =  0, .r =  1 }
};

hex_cube_t hex_cube_direction_vectors[6] = {
    { .q =  1, .r =  0, .s = -1 },
    { .q =  1, .r = -1, .s =  0 },
    { .q =  0, .r = -1, .s =  1 },
    { .q = -1, .r =  0, .s =  1 },
    { .q = -1, .r =  1, .s =  0 },
    { .q =  0, .r =  1, .s = -1 }
};

bool hex_axial_eq(hex_axial_t a, hex_axial_t b)
{
    return ((a.q == b.q) && (a.r == b.r));
}

hex_cube_t hex_cube_add(hex_cube_t a, hex_cube_t b)
{
    hex_cube_t result = {
        .q = a.q + b.q,
        .r = a.r + b.r,
        .s = a.s + b.s
    };
    return result;
}


hex_axial_t hex_axial_add(hex_axial_t a, hex_axial_t b)
{
    hex_axial_t result = {
        .q = a.q + b.q,
        .r = a.r + b.r
    };
    return result;
}

hex_cube_t hex_cube_subtract(hex_cube_t a, hex_cube_t b)
{
    hex_cube_t result = {
        .q = a.q - b.q,
        .r = a.r - b.r,
        .s = a.s - b.s
    };
    return result;
}

hex_axial_t hex_axial_subtract(hex_axial_t a, hex_axial_t b)
{
    hex_axial_t result = {
        .q = a.q - b.q,
        .r = a.r - b.r
    };
    return result;
}

int hex_cube_distance(hex_cube_t a, hex_cube_t b)
{
    hex_cube_t vec = hex_cube_subtract(a, b);
    return (abs(vec.q) + abs(vec.r) + abs(vec.s)) / 2;
}

int hex_axial_distance(hex_axial_t a, hex_axial_t b)
{
    hex_axial_t vec = hex_axial_subtract(a, b);
    return (abs(vec.q) + abs(vec.q + vec.r) + abs(vec.r)) / 2;
}

Vector2 hex_axial_to_pixel(hex_axial_t axial, float size)
{
    Vector2 px = {
        .x = size * ((3.0/2.0) * (float)axial.q),
        .y = size * ((SQRT_3_2 * (float)axial.q) + (SQRT_3 * (float)axial.r))
    };
    return px;
}

hex_axial_t hex_axial_round(hex_axialf_t axialf)
{
    return hex_cube_to_axial(hex_cube_round(hex_axialf_to_cubef(axialf)));
}

hex_cube_t hex_cube_round(hex_cubef_t cubef)
{
    int q = roundf(cubef.q);
    int r = roundf(cubef.r);
    int s = roundf(cubef.r);

    float dq = fabs(((float)q) - cubef.q);
    float dr = fabs(((float)r) - cubef.r);
    float ds = fabs(((float)s) - cubef.s);

    if ((dq > dr) && (dq > ds)) {
        q = -r - s;
    } else if (dr > ds) {
        r = -q - s;
    } else {
        s = -q - r;
    }

    hex_cube_t result = {
        .q = q,
        .r = r,
        .s = s
    };
    return result;
}

hex_axial_t pixel_to_hex_axial(Vector2 p, float size)
{
    hex_axialf_t result = {
        .q = ((2.0 / 3.0) * p.x) / size,
        .r = (((-1.0 / 3.0) * p.x) + (SQRT_3_3 * p.y)) / size
    };
    return hex_axial_round(result);
}

Vector2 *hex_axial_pixel_corners(hex_axial_t axial, float size)
{
    Vector2 pos = hex_axial_to_pixel(axial, size);
    return hex_pixel_corners(pos, size);
}

Vector2 *hex_flat_top_pixel_corners(Vector2 pos, float size)
{
    static Vector2 corners[7];

    // right
    corners[0].x = pos.x + size;
    corners[0].y = pos.y;

    // up right
    corners[1].x = pos.x + (0.5 * size);
    corners[1].y = pos.y - (SQRT_3_2 * size);

    // up left
    corners[2].x = pos.x - (0.5 * size);
    corners[2].y = pos.y - (SQRT_3_2 * size);

    // left
    corners[3].x = pos.x - size;
    corners[3].y = pos.y;

    // down left
    corners[4].x = pos.x - (0.5 * size);
    corners[4].y = pos.y + (SQRT_3_2 * size);

    // down right
    corners[5].x = pos.x + (0.5 * size);
    corners[5].y = pos.y + (SQRT_3_2 * size);

    // repeat first corner for easier polyline drawing
    corners[6] = corners[0];

    return corners;
}

Vector2 *hex_pointy_top_pixel_corners(Vector2 pos, float size)
{
    static Vector2 corners[7];

    corners[0].x = pos.x + (SQRT_3_2 * size);
    corners[0].y = pos.y + (0.5 * size);

    corners[1].x = pos.x;
    corners[1].y = pos.y + size;

    corners[2].x = pos.x - (SQRT_3_2 * size);
    corners[2].y = pos.y + (0.5 * size);

    corners[3].x = pos.x - (SQRT_3_2 * size);
    corners[3].y = pos.y - (0.5 * size);

    corners[4].x = pos.x;
    corners[4].y = pos.y - size;

    corners[5].x = pos.x + (SQRT_3_2 * size);
    corners[5].y = pos.y - (0.5 * size);

    // repeat first corner for easier polyline drawing
    corners[6] = corners[0];

    return corners;
}

Vector2 *hex_pixel_corners(Vector2 pos, float size)
{
    return hex_flat_top_pixel_corners(pos, size);
}

Vector2 *hex_axial_pixel_edge_midpoints(hex_axial_t axial, float size)
{
    Vector2 pos = hex_axial_to_pixel(axial, size);
    return hex_pointy_top_pixel_corners(pos, size * SQRT_3_2);
}


/*** offset coordinate conversion ***/

static hex_offset_t hex_axial_to_oddr(hex_axial_t axial)
{
    hex_offset_t offset = {
        .type = HEX_OFFSET_ODD_R,
        .row = axial.r,
        .col = axial.q + (axial.r - (axial.r&1)) / 2
    };
    return offset;
}

static hex_axial_t hex_oddr_to_axial(hex_offset_t off)
{
    assert(off.type == HEX_OFFSET_ODD_R);
    hex_axial_t axial = {
        .q = off.col - (off.row - (off.row & 1)) / 2,
        .r = off.row
    };
    return axial;
}

static hex_offset_t hex_axial_to_evenr(hex_axial_t axial)
{
    hex_offset_t offset = {
        .type = HEX_OFFSET_EVEN_R,
        .row = axial.r,
        .col = axial.q + (axial.r + (axial.r&1)) / 2
    };
    return offset;
}

static hex_axial_t hex_evenr_to_axial(hex_offset_t off)
{
    assert(off.type == HEX_OFFSET_EVEN_R);
    hex_axial_t axial = {
        .q = off.col - (off.row + (off.row & 1)) / 2,
        .r = off.row
    };
    return axial;
}

static hex_offset_t hex_axial_to_oddq(hex_axial_t axial)
{
    hex_offset_t offset = {
        .type = HEX_OFFSET_ODD_Q,
        .row = axial.r + (axial.r - (axial.r&1)) / 2,
        .col = axial.q
    };
    return offset;
}

static hex_axial_t hex_oddq_to_axial(hex_offset_t off)
{
    assert(off.type == HEX_OFFSET_ODD_Q);
    hex_axial_t axial = {
        .q = off.col,
        .r = off.row - (off.row - (off.row & 1)) / 2
    };
    return axial;
}


static hex_offset_t hex_axial_to_evenq(hex_axial_t axial)
{
    hex_offset_t offset = {
        .type = HEX_OFFSET_EVEN_Q,
        .row = axial.r + (axial.r - (axial.r&1)) / 2,
        .col = axial.q
    };
    return offset;
}

static hex_axial_t hex_evenq_to_axial(hex_offset_t off)
{
    assert(off.type == HEX_OFFSET_EVEN_Q);
    hex_axial_t axial = {
        .q = off.col,
        .r = off.row - (off.row - (off.row & 1)) / 2
    };
    return axial;
}

hex_offset_t hex_axial_to_offset(hex_axial_t axial, hex_offset_type_t type)
{
    switch (type) {
    case HEX_OFFSET_ODD_R:
        return hex_axial_to_oddr(axial);

    case HEX_OFFSET_EVEN_R:
        return hex_axial_to_evenr(axial);

    case HEX_OFFSET_ODD_Q:
        return hex_axial_to_oddq(axial);

    case HEX_OFFSET_EVEN_Q:
        return hex_axial_to_evenq(axial);
    }
    __builtin_unreachable();
}

hex_axial_t hex_offset_to_axial(hex_offset_t off)
{
    switch (off.type) {
    case HEX_OFFSET_ODD_R:
        return hex_oddr_to_axial(off);

    case HEX_OFFSET_EVEN_R:
        return hex_evenr_to_axial(off);

    case HEX_OFFSET_ODD_Q:
        return hex_oddq_to_axial(off);

    case HEX_OFFSET_EVEN_Q:
        return hex_evenq_to_axial(off);
    }
    __builtin_unreachable();
}
