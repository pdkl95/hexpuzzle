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

#include "cJSON/cJSON.h"

#include "common.h"
#include "raylib_helper.h"
#include "tile.h"
#include "tile_pos.h"
#include "level.h"

char *tile_flag_string(tile_t *tile)
{
    if (tile) {
        static char buf[4];
        buf[0] = tile->enabled ? 'E' : 'e';
        buf[1] = tile->fixed   ? 'F' : 'f';
        buf[2] = tile->hidden  ? 'H' : 'h';
        buf[3] = '\0';
        return buf;
    } else {
        return "---";
    }
}

char *tile_path_string(tile_t *tile)
{
    if (tile) {
        static char buf[7];
        snprintf(buf, 7, "%d%d%d%d%d%d",
                 tile->path[0],
                 tile->path[1],
                 tile->path[2],
                 tile->path[3],
                 tile->path[4],
                 tile->path[5]);
        return buf;
    } else {
        return "(NULL)";
    }
}

char *tile_our_pos_string(struct tile_pos *pos)
{
    static char buf[30];
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
    if (tile) {
        printf("%p ", tile);
        printf("tile<flags=%s path=%s s_pos=%s u_pos=%s%s>\n",
               tile_flag_string(tile),
               tile_path_string(tile),
               tile_our_pos_string(tile->solved_pos),
               tile_our_pos_string(tile->unsolved_pos),
               tile_is_blank(tile) ? " EMPTY" : "");
    } else {
        printf("tile<NULL>\n");
    }
}

void print_tile_with_solved_neighbors(tile_t *tile)
{
    print_tile(tile);
    each_direction {
        printf("  neighbor[%d]: ", dir);
        print_tile_pos(tile->solved_pos->neighbors[dir]);
    }
}

void print_tile_with_unsolved_neighbors(tile_t *tile)
{
    print_tile(tile);
    each_direction {
        printf("  neighbor[%d]: ", dir);
        print_tile_pos(tile->unsolved_pos->neighbors[dir]);
    }
}

tile_t *init_tile(tile_t *tile)
{
    assert_not_null(tile);

    tile->enabled = false;
    tile->fixed   = false;
    tile->hidden  = false;

    tile->solved_pos   = NULL;
    tile->unsolved_pos = NULL;

    each_direction {
        tile->path[dir] = PATH_TYPE_NONE;
        tile->hidden_saved_path[dir] = PATH_TYPE_NONE;
        tile->enabled_saved_path[dir] = PATH_TYPE_NONE;
    }

#if 0
    each_direction {
        tile->path[dir] = rand() % PATH_TYPE_COUNT;
    }
#endif

    tile->start_for_path_type = PATH_TYPE_NONE;

    tile_update_path_count(tile);

    return tile;
}

tile_t *create_tile(void)
{
    tile_t *tile = calloc(1, sizeof(tile_t));
    return init_tile(tile);
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

#define BCMP(field) \
    if (t1->field && !t2->field) { return 1; } \
    if (!t1->field && t2->field) { return -1; }

    if (t1->enabled && !t2->enabled) {
        return 1;
    }
    if (!t1->enabled && t2->enabled) {
        return -1;
    }
    //BCMP(enabled);
    BCMP(hidden);
    BCMP(fixed);
#undef BCMP

#define CMP(field)                            \
    rv = ((int)t1->field) - ((int)t2->field); \
    if (rv) { return rv; }

    CMP(path[0]);
    CMP(path[1]);
    CMP(path[2]);
    CMP(path[3]);
    CMP(path[4]);
    CMP(path[5]);
#undef CMP

    return 0;
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

    each_direction {
        dst->path[dir] = src->path[dir];
    }
}

void tile_copy_attributes_except_enabled(tile_t *dst, tile_t *src)
{
    assert_not_null(dst);
    assert_not_null(src);

    dst->fixed   = src->fixed;
    dst->hidden  = src->hidden;

    each_direction {
        dst->path[dir] = src->path[dir];
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

void tile_update_path_count(tile_t *tile)
{
    tile->path_count = 0;

    each_direction {
        if (tile->path[dir]) {
            tile->path_count++;
        }
    }
}

path_int_t tile_count_path_types(tile_t *tile)
{
    path_int_t rv = {0};

    each_direction {
        rv.path[tile->path[dir]] += 1;
    }

    return rv;
}

tile_flags_t tile_get_flags(tile_t *tile)
{
    tile_flags_t flags = {
        .enabled = tile->enabled,
        .fixed   = tile->fixed,
        .hidden  = tile->hidden
    };
    return flags;
}

void tile_set_flags(tile_t *tile, tile_flags_t flags)
{
    tile->enabled = flags.enabled;
    tile->fixed   = flags.fixed;
    tile->hidden  = flags.hidden;
}

tile_neighbor_paths_t tile_get_neighbor_paths(tile_t *tile)
{
    assert_not_null(tile);

    tile_neighbor_paths_t paths;
    memset(&paths, 0, sizeof(paths));

    tile_pos_t *pos = tile->solved_pos;
    assert_not_null(pos);

    each_direction {
        hex_direction_t opposite_dir = hex_opposite_direction(dir);
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (neighbor) { // && neighbor->enabled)
            assert_not_null(neighbor->tile);

            paths.sections[dir].neighbor.section            = opposite_dir;
            paths.sections[dir].neighbor.tile               = neighbor->tile;
            paths.sections[dir].neighbor.path               = neighbor->tile->path[opposite_dir];
            paths.sections[dir].neighbor.hidden_saved_path  = neighbor->tile->hidden_saved_path[opposite_dir];
            paths.sections[dir].neighbor.enabled_saved_path = neighbor->tile->enabled_saved_path[opposite_dir];
        } else {
            paths.sections[dir].neighbor.tile = NULL;
        }
 
        paths.sections[dir].local.section            = dir;
        paths.sections[dir].local.tile               = pos->tile;
        paths.sections[dir].local.path               = pos->tile->path[dir];
        paths.sections[dir].local.hidden_saved_path  = pos->tile->hidden_saved_path[dir];
        paths.sections[dir].local.enabled_saved_path = pos->tile->enabled_saved_path[dir];
    }

    return paths;
}

void tile_set_neighbor_paths(tile_t *tile, tile_neighbor_paths_t paths)
{
    assert_not_null(tile);

#if 0
    printf("--\n\nTILE: ");
    print_tile(tile);
    printf("UNSOLVED: ");
    print_tile_pos(tile->unsolved_pos);
    printf("LOAD >>>\n");
#endif

    each_direction {
        tile_t *neighbor = paths.sections[dir].neighbor.tile;
        //printf("dir(%d) ", dir);
        if (neighbor) {
            //print_tile(neighbor);
            hex_direction_t section               = paths.sections[dir].neighbor.section;
            neighbor->path[section]               = paths.sections[dir].neighbor.path;
            neighbor->hidden_saved_path[section]  = paths.sections[dir].neighbor.hidden_saved_path;
            neighbor->enabled_saved_path[section] = paths.sections[dir].neighbor.enabled_saved_path;
        }

        hex_direction_t section           = paths.sections[dir].local.section;
        tile->path[section]               = paths.sections[dir].local.path;
        tile->hidden_saved_path[section]  = paths.sections[dir].local.hidden_saved_path;
        tile->enabled_saved_path[section] = paths.sections[dir].local.enabled_saved_path;

        tile_update_path_count(neighbor);;
    }

    tile_update_path_count(tile);
}

bool tile_has_path_type(tile_t *tile, path_type_t type)
{
    each_direction {
        if (tile->path[dir] == type) {
            return true;
        }
    }

    return false;
}

bool tile_is_blank(tile_t *tile)
{
    each_direction {
        if (tile->path[dir] != PATH_TYPE_NONE) {
            return false;
        }
    }

    return true;
}

bool tile_is_empty(tile_t *tile)
{
    return tile_is_blank(tile) && tile->enabled;
}

void tile_set_positions(tile_t *tile, level_t *level, hex_axial_t solved_addr, hex_axial_t unsolved_addr)
{
    tile->solved_pos = level_get_solved_tile_pos(level, solved_addr);
    tile->unsolved_pos = level_get_unsolved_tile_pos(level, unsolved_addr);

    tile->solved_pos->tile = tile;
    tile->unsolved_pos->tile = tile;
}

bool tile_from_json_addr(cJSON *json, hex_axial_t *addr)
{
    addr->q = 0;
    addr->r = 0;

    if (!cJSON_IsObject(json)) {
        errmsg("Error parsing tile addr JSON: is not an Object");
        return false;
    }

    cJSON *q_json = cJSON_GetObjectItem(json, "q");
    cJSON *r_json = cJSON_GetObjectItem(json, "r");

    if (!cJSON_IsNumber(q_json)) {
        errmsg("Error parsing tile addr JSON: Object missing Number 'q'");
        return false;
    }
    if (!cJSON_IsNumber(r_json)) {
        errmsg("Error parsing tile addr JSON: Object missing Number 'r'");
        return false;
    }

    addr->q = q_json->valueint;
    addr->r = r_json->valueint;

    return true;
}

bool tile_from_json(tile_t *tile, level_t *level, cJSON *json)
{
    if (!cJSON_IsObject(json)) {
        errmsg("Error parsing tile JSON: not an Object");
        return false;
    }

    cJSON   *solved_json = cJSON_GetObjectItem(json, "solved");
    cJSON *unsolved_json = cJSON_GetObjectItem(json, "unsolved");
    cJSON     *path_json = cJSON_GetObjectItem(json, "path");
    cJSON  *enabled_json = cJSON_GetObjectItem(json, "enabled");
    cJSON   *hidden_json = cJSON_GetObjectItem(json, "hidden");
    cJSON    *fixed_json = cJSON_GetObjectItem(json, "fixed");

    hex_axial_t solved_addr, unsolved_addr;

    if (!tile_from_json_addr(solved_json, &solved_addr)) {
        errmsg("Error parsing tile JSON: Cannot parse 'solved' address");
        return false;
    }

    if (!tile_from_json_addr(unsolved_json, &unsolved_addr)) {
        errmsg("Error parsing tile JSON: Cannot parse 'unsolved' address");
        return false;
    }

    if (!cJSON_IsArray(path_json)) {
        errmsg("Error parsing tile JSON: Object missing Array 'path'");
        return false;
    }

    if (!cJSON_IsBool(enabled_json)) {
        errmsg("Error parsing tile JSON: Object missing Bool 'enabled'");
        return false;
    }

    if (!cJSON_IsBool(hidden_json)) {
        errmsg("Error parsing tile JSON: Object missing Bool 'hidden'");
        return false;
    }

    if (!cJSON_IsBool(fixed_json)) {
        errmsg("Error parsing tile JSON: Object missing Bool 'fixed'");
        return false;
    }

    each_direction {
        cJSON *p_json = cJSON_GetArrayItem(path_json, dir);
        if (NULL == p_json) {
            errmsg("Error parsing tile JSON: Array 'path' is missing item %d", dir);
            return false;
        }

        if (!cJSON_IsNumber(p_json)) {
            errmsg("Error parsing tile JSON: Array 'path' item %d is not a Number", dir);
            return false;
        }

        tile->path[dir] = p_json->valueint;
    }

    tile->enabled = cJSON_IsTrue(enabled_json);
    tile->hidden  = cJSON_IsTrue(hidden_json);
    tile->fixed   = cJSON_IsTrue(fixed_json);

    tile_set_positions(tile, level, solved_addr, unsolved_addr);

    tile_update_path_count(tile);

    return true;
}

static cJSON *tile_to_json_addr(hex_axial_t addr)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(json, "q", addr.q) == NULL) {
        goto json_addr_err;
    }


    if (cJSON_AddNumberToObject(json, "r", addr.r) == NULL) {
        goto json_addr_err;
    }

    return json;

  json_addr_err:
    cJSON_Delete(json);
    return NULL;
}

cJSON *tile_to_json(tile_t *tile)
{
    cJSON *json = cJSON_CreateObject();

    cJSON   *solved_addr = tile_to_json_addr(tile->solved_pos->position);
    if (!solved_addr) {
        goto json_tile_err;
    }
    if (!cJSON_AddItemToObject(json, "solved", solved_addr)) {
        goto json_tile_err;
    }

    cJSON *unsolved_addr = tile_to_json_addr(tile->unsolved_pos->position); \
    if (!unsolved_addr) {
        goto json_tile_err;
    }
    if (!cJSON_AddItemToObject(json, "unsolved", unsolved_addr)) {
        goto json_tile_err;
    }

    cJSON *path = cJSON_CreateIntArray((const int *)tile->path, 6);
    if (!cJSON_AddItemToObject(json, "path", path)) {
        goto json_tile_err;
    }

    if (cJSON_AddBoolToObject(json, "enabled", tile->enabled) == NULL) {
        goto json_tile_err;
    }

    if (cJSON_AddBoolToObject(json, "hidden", tile->hidden) == NULL) {
        goto json_tile_err;
    }

    if (cJSON_AddBoolToObject(json, "fixed", tile->fixed) == NULL) {
        goto json_tile_err;
    }

    return json;

  json_tile_err:
    cJSON_Delete(json);
    return NULL;
}

bool tile_is_solved(tile_t *tile)
{
    return hex_axial_eq(tile->solved_pos->position,
                        tile->unsolved_pos->position);
}
