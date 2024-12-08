/****************************************************************************
 *                                                                          *
 * physics.h                                                                *
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

#ifndef PHYSICS_H
#define PHYSICS_H

#include <chipmunk.h>

struct tile_pos;
struct level;

static inline Vector2 cpVectToVector2(cpVect v)
{
    Vector2 rv = {
        .x = v.x,
        .y = v.y
    };
    return rv;
}

static inline cpVect Vector2TocpVect(Vector2 v)
{
    return cpv(v.x, v.y);
}

enum physics_state {
    PHYSICS_STOP    = 0,
    PHYSICS_RUNNING = 1
};
typedef enum physics_state physics_state_t;

struct physics_tile {
    struct tile *tile;
    cpShape *shape;
    cpBody *body;

    bool path_has_spring[3];
    cpConstraint *path_spring[3];
    cpConstraint *path_rotary_limit[3];

    cpFloat radius;
    cpFloat mass;
    cpFloat moment;
};
typedef struct physics_tile physics_tile_t;

struct physics {
    struct level *level;

    physics_state_t state;

    cpSpace *space;

    cpShape *wall[4];

    int num_tiles;
    physics_tile_t tiles[LEVEL_MAXTILES];
    bool tiles_ready;

    cpFloat time_step;
    cpFloat time;
};
typedef struct physics physics_t;

physics_t *create_physics(struct level *level);
void destroy_physics(physics_t *physics);

void physics_build_tiles(physics_t *physics);

void physics_start(physics_t *physics);
void physics_stop(physics_t *physics);
void physics_update(physics_t *physics);

#endif /*PHYSICS_H*/

