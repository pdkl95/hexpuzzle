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
#include "tile_pos.h"

static char *tile_flag_string(tile_t *tile)
{
    static char buf[4];
    buf[0] = tile->enabled ? 'E' : 'e';
    buf[1] = tile->fixed   ? 'F' : 'f';
    buf[2] = tile->hidden  ? 'H' : 'h';
    buf[3] = '\0';
    return buf;
}

static char *tile_path_string(tile_t *tile)
{
    static char buf[7];
    snprintf(buf, 7, "%d%d%d%d%d%d",
             tile->path[0],
             tile->path[1],
             tile->path[2],
             tile->path[3],
             tile->path[4],
             tile->path[5]);
    return buf;
}

static char *tile_our_pos_string(tile_pos_t *pos)
{
    static char buf[20];
    if (pos) {
        snprintf(buf, 29, "(%d, %d)", pos->position.q, pos->position.r);
    } else {
        char *nullstr = "NULL";
        memcpy(buf, nullstr, strlen(nullstr) + 1);
    }
    return buf;
}

void print_tile(tile_t *tile)
{
    printf("tile<flags=%s path=%s s_pos=%s u_pos=%s>\n",
           tile_flag_string(tile),
           tile_path_string(tile),
           tile_our_pos_string(tile->solved_pos),
           tile_our_pos_string(tile->unsolved_pos));
}

char *path_type_name(path_type_t type)
{
    switch (type) {
    default:
        return "(BAD VALUE)";

    case PATH_TYPE_NONE:
        return "NONE";

    case PATH_TYPE_RED:
        return "RED";

    case PATH_TYPE_BLUE:
        return "BLUE";

    case PATH_TYPE_YELLOW:
        return "YELLOW";

    case PATH_TYPE_GREEN:
        return "GREEN";
    }
}

tile_t *init_tile(tile_t *tile)
{
    assert_not_null(tile);

    tile->enabled = false;
    tile->fixed = false;
    tile->hidden = false;

#if 0
    for (int i=0; i<6; i++) {
        tile->path[i] = rand() % PATH_TYPE_COUNT;
    }
#endif

    return tile;
}

tile_t *create_tile(void)
{
    tile_t *tile = calloc(1, sizeof(tile_t));
    return init_tile(tile);
}

void tile_set_flag_from_char(tile_t *tile, char c)
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

void destroy_tile(tile_t *tile)
{
    if (tile) {
        SAFEFREE(tile);
    }
}

bool tile_eq(tile_t *tile, tile_t *other)
{
#define CMP(field) if (tile->field != other->field) { return false; }
    CMP(enabled);
    CMP(fixed);
    CMP(hidden);
    CMP(path[0]);
    CMP(path[1]);
    CMP(path[2]);
    CMP(path[3]);
    CMP(path[4]);
    CMP(path[5]);
#undef CMP
    return true;
}

int compare_tiles(const void *p1, const void *p2)
{
    tile_t *t1 = (tile_t *)p1;
    tile_t *t2 = (tile_t *)p2;

    int rv;

#define CMP(field) \
    rv = ((int)t1->field) - ((int)t2->field); \
    if (rv) { return rv; }

    CMP(enabled);
    CMP(hidden);
    CMP(fixed);

    CMP(path[0]);
    CMP(path[1]);
    CMP(path[2]);
    CMP(path[3]);
    CMP(path[4]);
    CMP(path[5]);
#undef CMP

    return 0;

#if 0
    rv = ((int)t1->enabled) - ((int)t2->enabled);
    if (rv) { return rv; }

    rv = ((int)t1->hidden) - ((int)t2->hidden);
    if (rv) { return rv; }

    rv = ((int)t1->fixed) - ((int)t2->fixed);
    if (rv) { return rv; }
#endif
}

void tile_copy_attributes(tile_t *dst, tile_t *src)
{
    assert_not_null(dst);
    assert_not_null(src);

#if 0
    printf("tile_copy [%d, %d] -> [%d, %d]\n",
           src->position.q,
           src->position.r,
           dst->position.q,
           dst->position.r);
#endif

    dst->enabled = src->enabled;
    dst->fixed   = src->fixed;
    dst->hidden  = src->hidden;

    for (int i=0; i<6; i++) {
        dst->path[i] = src->path[i];
    }
}

void tile_copy_attributes_except_enabled(tile_t *dst, tile_t *src)
{
    assert_not_null(dst);
    assert_not_null(src);

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

path_int_t tile_count_path_types(tile_t *tile)
{
    path_int_t rv = {0};

    for (hex_direction_t i=0; i<6; i++) {
        rv.path[tile->path[i]] += 1;
    }

    return rv;
}

void tile_serialize(tile_t *tile, FILE *f)
{
    fprintf(f, "tile %d,%d %d,%d %d%d%d%d%d%d %s%s%s\n",
            tile->solved_pos->position.q,
            tile->solved_pos->position.r,
            tile->unsolved_pos->position.q,
            tile->unsolved_pos->position.r,

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
