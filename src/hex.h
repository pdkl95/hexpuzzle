/****************************************************************************
 *                                                                          *
 * hex.h                                                                    *
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

#ifndef HEX_H
#define HEX_H

/* 1.0 / SQRT(3) */
#define INV_SQRT_3 0.5773502691896258

#define SQRT_3 1.7320508075688772
#define SQRT_3_2 (SQRT_3 / 2.0)
#define SQRT_3_3 (SQRT_3 / 3.0)

/* flat-top directions */
enum hex_direction {
    HEX_DIR_UP_RIGHT   = 0,
    HEX_DIR_UP         = 1,
    HEX_DIR_UP_LEFT    = 2,
    HEX_DIR_DOWN_LEFT  = 3,
    HEX_DIR_DOWN       = 4,
    HEX_DIR_DOWN_RIGHT = 5
};
typedef enum hex_direction hex_direction_t;

#define HEX_DIRECTION_MIN HEX_DIR_UP_RIGHT
#define HEX_DIRECTION_MAX HEX_DIR_DOWN_RIGHT

#define for_each_direction(var) for (hex_direction_t var=0; var<6; var++)
#define each_direction for_each_direction(dir)

const char *hex_direction_name(hex_direction_t dir);

inline static hex_direction_t hex_opposite_direction(hex_direction_t dir)
{
    return (dir + 3) % 6;
}

struct hex_direction_order {
    hex_direction_t dir[6];
};
typedef struct hex_direction_order hex_direction_order_t;

/*
 * Axial Coordinates for hexagons
 */
struct hex_axial {
    int q;
    int r;
};
typedef struct hex_axial hex_axial_t;

//#define hex_axial_is(axial, q, r) (((axial).q == (q)) && ((axial).r == (r)))

struct hex_axialf {
    float q;
    float r;
};
typedef struct hex_axialf hex_axialf_t;

typedef void (*hex_axial_cb_t)(hex_axial_t axial, void *data);


extern hex_axial_t hex_axial_direction_vectors[6];
static inline hex_axial_t hex_axial_direction(hex_direction_t dir)
{
    return hex_axial_direction_vectors[dir];
}

hex_axial_t hex_axial_add(hex_axial_t a, hex_axial_t b);

static inline hex_axial_t hex_axial_neighbor(hex_axial_t axial, hex_direction_t dir)
{
    return hex_axial_add(axial, hex_axial_direction(dir));
}

hex_axial_t hex_axial_subtract(hex_axial_t a, hex_axial_t b);
int hex_axial_distance(hex_axial_t a, hex_axial_t b);
hex_axial_t hex_axial_round(hex_axialf_t axialf);
Vector2 hex_axial_to_pixel(hex_axial_t axial, float size);
hex_axial_t pixel_to_hex_axial(Vector2 p, float size);

Vector2 *hex_axial_pixel_corners(hex_axial_t axial, float size);
Vector2 *hex_axial_pixel_edge_midpoints(hex_axial_t axial, float size);

bool hex_axial_eq(hex_axial_t a, hex_axial_t b);

hex_direction_t pixel_to_hex_axial_section(Vector2 p, float size);

inline static hex_axial_t hex_axial_scale(hex_axial_t axial, int factor)
{
    hex_axial_t rv = {
        .q = axial.q * factor,
        .r = axial.r * factor,
    };
    return rv;
}

void hex_axial_foreach_in_ring(hex_axial_t center, int radius, hex_axial_cb_t callback, void *data);
void hex_axial_foreach_in_spiral(hex_axial_t center, int radius, hex_axial_cb_t callback, void *data);

/*
 * Cube Coordinates for hexagons
 */
struct hex_cube {
    int q;
    int r;
    int s;
};
typedef struct hex_cube hex_cube_t;

struct hex_cubef {
    float q;
    float r;
    float s;
};
typedef struct hex_cubef hex_cubef_t;

static inline hex_axial_t cube_to_axial(hex_cube_t cube)
{
    hex_axial_t axial = {
        .q = cube.q,
        .r = cube.r
    };
    return axial;
}

static inline hex_cube_t axial_to_cube(hex_axial_t axial)
{
    hex_cube_t cube = {
        .q = axial.q,
        .r = axial.r,
        .s = -axial.q - axial.r
    };
    return cube;
}

extern hex_cube_t hex_cube_direction_vectors[6];
static inline hex_cube_t hex_cube_direction(hex_direction_t dir)
{
    return hex_cube_direction_vectors[dir];
}

hex_cube_t hex_cube_add(hex_cube_t a, hex_cube_t b);

static inline hex_cube_t hex_cube_neighbor(hex_cube_t cube, hex_direction_t dir)
{
    return hex_cube_add(cube, hex_cube_direction(dir));
}

hex_cube_t hex_cube_subtract(hex_cube_t a, hex_cube_t b);
int hex_cube_distance(hex_cube_t a, hex_cube_t b);
hex_cube_t hex_cube_round(hex_cubef_t cubef);

int hex_cube_distance(hex_cube_t a, hex_cube_t b);


/*
 * Offset Coordinates
 */
enum hex_offset_type {
    HEX_OFFSET_ODD_R,
    HEX_OFFSET_EVEN_R,
    HEX_OFFSET_ODD_Q,
    HEX_OFFSET_EVEN_Q,
};
typedef enum hex_offset_type hex_offset_type_t;

struct hex_offset {
    hex_offset_type_t type;
    int row;
    int col;
};
typedef struct hex_offset hex_offset_t;


/*
 * Coordinate Conversion
 */
static inline hex_axial_t hex_cube_to_axial(hex_cube_t cube)
{
    hex_axial_t axial = {
        .q = cube.q,
        .r = cube.r
    };
    return axial;
}

static inline hex_cube_t hex_axial_to_cube(hex_axial_t axial)
{
    hex_cube_t cube = {
        .q = axial.q,
        .r = axial.r,
        .s = -(axial.q) - (axial.r)
    };
    return cube;
}

static inline hex_axialf_t hex_cubef_to_axialf(hex_cubef_t cubef)
{
    hex_axialf_t axialf = {
        .q = cubef.q,
        .r = cubef.r
    };
    return axialf;
}

static inline hex_cubef_t hex_axialf_to_cubef(hex_axialf_t axialf)
{
    hex_cubef_t cubef = {
        .q = axialf.q,
        .r = axialf.r,
        .s = -(axialf.q) - (axialf.r)
    };
    return cubef;
}

Vector2 *hex_pixel_corners(Vector2 pos, float size);

hex_offset_t hex_axial_to_offset(hex_axial_t axial, hex_offset_type_t type);
hex_axial_t hex_offset_to_axial(hex_offset_t off);

hex_axial_t hex_axial_reflect_horiz(hex_axial_t axial, hex_axial_t reflect_point);
hex_axial_t hex_axial_rotate(hex_axial_t axial, hex_axial_t rotate_point);

#endif /*HEX_H*/

