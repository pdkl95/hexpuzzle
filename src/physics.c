/****************************************************************************
 *                                                                          *
 * physics.c                                                                *
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
#include "level.h"
#include "win_anim.h"
#include "physics.h"

float gravity_strength = 250.0;

void init_physics(physics_t *physics, struct level *level)
{
    physics->level = level;

    physics->state = PHYSICS_STOP;

    memset(physics->tiles, 0, sizeof(physics->tiles));

    physics->time = 0.0;
    physics->time_step = 1.0 / options->max_fps;

    physics->time *= 0.1;

    physics->space = cpSpaceNew();

    cpSpaceSetIterations(physics->space, 20);

    Vector2 size = window_center;
    size = Vector2Scale(size, 1.0);
    cpVect tl = cpv(-size.x,-size.x);
    cpVect tr = cpv( size.x,-size.x);
    cpVect bl = cpv(-size.x, size.y);
    cpVect br = cpv( size.x, size.y);

    cpBody *wall_body = cpSpaceGetStaticBody(physics->space);
    physics->wall[0] = cpSegmentShapeNew(wall_body, tl, tr, 0);
    physics->wall[1] = cpSegmentShapeNew(wall_body, tr, br, 0);
    physics->wall[2] = cpSegmentShapeNew(wall_body, br, bl, 0);
    physics->wall[3] = cpSegmentShapeNew(wall_body, bl, tl, 0);

    for (int i=0; i<4; i++) {
        cpShapeSetFriction(physics->wall[i], 1.0);
        cpShapeSetElasticity(physics->wall[i], 0.75);
        cpSpaceAddShape(physics->space, physics->wall[i]);
    }

    physics_reset(physics);
}

physics_t *create_physics(struct level *level)
{
    physics_t *physics = calloc(1, sizeof(physics_t));
    init_physics(physics, level);
    return physics;
}

void cleanup_phyics_tile(physics_tile_t *pt)
{
    assert_not_null(pt);

    for (hex_direction_t dir=0; dir<3; dir++) {
        if (pt->path_spring[dir]) {
            cpConstraintFree(pt->path_spring[dir]);
            pt->path_spring[dir] = NULL;
        }
        if (pt->path_rotary_limit[dir]) {
            cpConstraintFree(pt->path_rotary_limit[dir]);
            pt->path_rotary_limit[dir] = NULL;
        }
    }

    if (pt->shape) {
        cpShapeFree(pt->shape);
        pt->shape = NULL;
    }
    if (pt->body) {
        cpBodyFree(pt->body);
        pt->body = NULL;
    }
}

void destroy_physics(physics_t *physics)
{
    if (physics) {
        for (int i=0; i<LEVEL_MAXTILES; i++) {
            physics_tile_t *pt = &(physics->tiles[i]);
            cleanup_phyics_tile(pt);
        }

        for (int i=0; i<4; i++) {
            cpShapeFree(physics->wall[i]);
        }

        cpSpaceFree(physics->space);

        FREE(physics);
    }
}

static void shuffle_int(int *list, int len)
{
    int i, j, tmp;
    for (i = len - 1; i > 0; i--) {
        j = global_rng_get(i + 1);

        tmp = list[j];

        list[j] = list[i];
        list[i] = tmp;
    }
}

static bool constrain_random_spanning_tree(physics_t *physics, tile_t *tile, hex_direction_t *exclude)
{
    if ((!tile) ||
        (tile->visited) ||
        (!tile->enabled) ||
        (!tile->unsolved_pos) ||
        (!tile->physics_tile)) {
        return false;
    }

    tile->visited = true;

    physics_tile_t *pt = tile->physics_tile;

    hex_direction_order_t order;
    for (int i=0; i<6; i++) {
        order.dir[i] = i;
    }

    shuffle_int((int *)(&(order.dir[0])), 6);


    int path_constraint_count = 0;

    for (int i=0; i<6; i++) {
        hex_direction_t dir = order.dir[i];

        if (exclude) {
            if (*exclude == dir) {
                continue;
            }
        }

        if ((tile->path[dir] == PATH_TYPE_NONE) ||
            (!tile->solved_pos)) {
            continue;
        }

        assert_not_null(tile);
        assert_not_null(tile->solved_pos);
        assert_not_null(tile->unsolved_pos);

        tile_t *neighbor_tile = tile->solved_pos->neighbors[dir]->tile;

        if ((!neighbor_tile) ||
            (!neighbor_tile->enabled)) {
            continue;
        }

        hex_direction_t opposite_dir = hex_opposite_direction(dir);
        if (constrain_random_spanning_tree(physics, neighbor_tile, &opposite_dir)) {
            physics_tile_t *neighbor_pt = neighbor_tile->physics_tile;

#if 1
            float arcsize = TAU/6.0f;
            cpConstraint *c1 = cpRotaryLimitJointNew(
                pt->body,
                neighbor_pt->body,
                -arcsize,
                arcsize);

            cpConstraintSetMaxForce(c1, 10000);
            //cpConstraintSetMaxBias(c1, 1)
            cpSpaceAddConstraint(physics->space, c1);

            pt->path_rotary_limit[path_constraint_count] = c1;
#endif
#if 1
            cpConstraint *c2 = cpDampedSpringNew(
                pt->body,
                neighbor_pt->body,
                Vector2TocpVect(tile->unsolved_pos->rel.midpoints[dir]),
                Vector2TocpVect(neighbor_tile->unsolved_pos->rel.midpoints[opposite_dir]),
                2.0f,
                120.0f,
                125.0f);

            cpConstraintSetMaxForce(c2, 10000);
            //cpConstraintSetMaxBias(c2, 1)
            cpSpaceAddConstraint(physics->space, c2);
            pt->path_spring[path_constraint_count] = c2;
#endif

            pt->path_has_spring[dir] = true;

            path_constraint_count++;
        }
    }

    return true;
}

static cpVect tile_spin_velocity(physics_tile_t *pt)
{
    cpVect pos = cpBodyGetPosition(pt->body);
    cpVect vel = cpBodyGetVelocity(pt->body);
    cpVect v = cpv(-pos.y, pos.x);
    cpVect nv = cpvnormalize(v);
    float plen = cpvlength(pos);
    plen = sqrtf(plen);
    plen = MAX(plen, 25.0f);
    plen *= 2.0;
    nv = cpvmult(nv, plen);
    v = cpvadd(nv, vel);
    return v;
}

float fade_in_factor = 0.0;

static void spin_velocity_update_func(cpBody *body, UNUSED cpVect gravity, cpFloat damping, cpFloat dt)
{
    physics_tile_t *pt = cpBodyGetUserData(body);

    cpVect orbit_vel = tile_spin_velocity(pt);
    orbit_vel = cpvmult(orbit_vel, 0.5 * fade_in_factor);
    cpBodyUpdateVelocity(body, orbit_vel, damping, dt);
}

void physics_build_tiles(physics_t *physics)
{
    if (physics->tiles_ready) {
        return;
    }
    //printf("physics_build_tiles()\n");

    level_t *level = physics->level;
    win_anim_mode_t mode = level->win_anim->mode;

    switch (mode) {
    case WIN_ANIM_MODE_PHYSICS_FALL:
        cpVect gravity = cpv(0.0, gravity_strength);
        cpSpaceSetGravity(physics->space, gravity);
        break;

    case WIN_ANIM_MODE_PHYSICS_SWIRL:
        cpSpaceSetGravity(physics->space, cpvzero);
        break;

    default:
        assert(false && "invalid win_anim_mode_t");
        __builtin_unreachable();
    }

    //tile_pos_t *center = level_get_center_tile_pos(level);

    physics->num_tiles = level_get_enabled_tiles(level);

    for (int i=0; i<physics->num_tiles; i++) {
        tile_t *tile = level->enabled_tiles[i];
        assert_not_null(tile);
        tile_pos_t *pos = tile->unsolved_pos;
        assert(tile->enabled);
        assert_not_null(pos);
        physics_tile_t *pt = &(physics->tiles[i]);
        assert_not_null(pt);

        pt->tile = tile;
        tile->physics_tile = pt;

        Vector2 center_position = Vector2Subtract(pos->win.center, window_center);
        cpVect position = cpv(center_position.x,
                              center_position.y);

        cpVect verts[6];
        each_direction {
            Vector2 corner = pos->rel.corners[dir];
            corner = Vector2Scale(corner, 0.95);
            verts[dir] = cpv(corner.x, corner.y);
        }
        pt->radius = pos->size;
        pt->mass = 2.0f; //pos->size;
        pt->moment = cpMomentForPoly(pt->mass, 6, verts, cpvzero, 0.0f);

        pt->body = cpSpaceAddBody(physics->space, cpBodyNew(pt->mass, pt->moment));
        pt->shape = cpSpaceAddShape(physics->space, cpPolyShapeNew(pt->body, 6, verts, cpTransformIdentity, 0.0));

        cpBodySetUserData(pt->body, pt);

        cpBodySetPosition(pt->body, position);

        switch (mode) {
        case WIN_ANIM_MODE_PHYSICS_FALL:
            cpBodySetVelocity(pt->body, cpvzero);

            cpShapeSetElasticity(pt->shape, 0.4f);
            cpShapeSetFriction(pt->shape, 0.7f);
            break;

        case WIN_ANIM_MODE_PHYSICS_SWIRL:
            cpBodySetVelocity(pt->body, tile_spin_velocity(pt));

            cpBodySetVelocityUpdateFunc(pt->body, spin_velocity_update_func);

            cpShapeSetElasticity(pt->shape, 0.4f);
            cpShapeSetFriction(pt->shape, 0.7f);
            break;

        default:
            assert(false && "invalid win_anim_mode_t");
            __builtin_unreachable();
        }

        tile->visited = false;
    }

    physics_tile_t *start_pt = &(physics->tiles[global_rng_get(physics->num_tiles)]);
    constrain_random_spanning_tree(physics, start_pt->tile, NULL);

    physics->tiles_ready = true;
}

void physics_reset(physics_t *physics)
{
    assert_not_null(physics);

    if (!physics->tiles_ready) {
        //printf("SKIP physics_reset()\n");
        return;
    }
    //printf("physics_reset()\n");
    level_t *level = physics->level;

    for (int i=0; i<physics->num_tiles; i++) {
        tile_t *tile = level->enabled_tiles[i];
        assert_not_null(tile);
        tile_pos_t *pos = tile->unsolved_pos;
        assert(tile->enabled);
        assert_not_null(pos);
        physics_tile_t *pt = &(physics->tiles[i]);
        assert_not_null(pt);

        Vector2 center_position = Vector2Subtract(pos->win.center, window_center);
        center_position = Vector2Add(center_position, level->px_offset);
        cpVect position = cpv(center_position.x,
                              center_position.y);

        cpBodySetPosition(pt->body, position);
        cpBodySetVelocity(pt->body, cpvzero);
        cpBodySetAngularVelocity(pt->body, 0.0f);

        pos->physics_velocity = VEC2_ZERO;
        pos->physics_position = pos->extra_translate = VEC2_ZERO;
        pos->physics_rotation = pos->extra_rotate = 0.0f;
    }
}

void physics_start(physics_t *physics)
{
    assert_not_null(physics);

    if (physics->state == PHYSICS_RUNNING) {
        //printf("SKIP physics_start()\n");
        return;
    }
    //printf("physics_start()\n");

    if (!physics->tiles_ready) {
        physics_build_tiles(physics);
    }

    physics->state = PHYSICS_RUNNING;
    physics_reset(physics);
}

void physics_stop(physics_t *physics)
{
    assert_not_null(physics);

    if (physics->state == PHYSICS_STOP) {
        //printf("SKIP physics_stop()\n");
        return;
    }
    //printf("physics_stop()\n");

    physics->state = PHYSICS_STOP;

    physics_reset(physics);
}

void physics_update(physics_t *physics)
{
    assert_not_null(physics);

    if (physics->state != PHYSICS_RUNNING) {
        return;
    }

    Vector2 screen_offset = Vector2Subtract(window_center, physics->level->px_offset);

    for (int i=0; i<physics->num_tiles; i++) {
        physics_tile_t *pt = &(physics->tiles[i]);
        tile_t *tile = pt->tile;
        tile_pos_t *pos = tile->unsolved_pos;

        cpVect velocity = cpBodyGetVelocity(pt->body);
        pos->physics_velocity = cpVectToVector2(velocity);

        cpVect position = cpBodyGetPosition(pt->body);
        Vector2 vec2_position = cpVectToVector2(position);
        vec2_position = Vector2Add(vec2_position, screen_offset);
        pos->physics_position = vec2_position;
        pos->extra_translate = Vector2Subtract(pos->physics_position, pos->win.center);

        pos->extra_rotate = cpBodyGetAngle(pt->body);
    }

    fade_in_factor = physics->level->win_anim->fade[3];
    physics->time += physics->time_step;
    cpSpaceStep(physics->space, physics->time_step);
}
