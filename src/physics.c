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
#include "physics.h"


physics_t *create_physics(struct level *level)
{
    physics_t *physics = calloc(1, sizeof(physics_t));

    physics->level = level;

    physics->state = PHYSICS_STOP;

    physics->tiles = NULL;

    physics->time = 0.0;
    physics->time_step = 1.0 / options->max_fps;

    physics->space = cpSpaceNew();

    cpSpaceSetIterations(physics->space, 20);

    //cpVect gravity = cpvzero;
    cpVect gravity = cpv(0.0, 500.0);

    cpSpaceSetGravity(physics->space, gravity);

    Vector2 size = window_center;
    size = Vector2Scale(size, 1.0);
    cpVect tl = cpv(-size.x,-size.x);
    cpVect tr = cpv( size.x,-size.x);
    cpVect bl = cpv(-size.x, size.y);
    cpVect br = cpv( size.x, size.y);

    /* cpVect px_offset = cpv(level->px_offset.x, */
    /*                        level->px_offset.y); */
    /* tl = cpvadd(tl, px_offset); */
    /* tr = cpvadd(tr, px_offset); */
    /* bl = cpvadd(bl, px_offset); */
    /* br = cpvadd(br, px_offset); */

    cpBody *wall_body = cpSpaceGetStaticBody(physics->space);
    physics->wall[0] = cpSegmentShapeNew(wall_body, tl, tr, 0);
    physics->wall[1] = cpSegmentShapeNew(wall_body, tr, br, 0);
    physics->wall[2] = cpSegmentShapeNew(wall_body, br, bl, 0);
    physics->wall[3] = cpSegmentShapeNew(wall_body, bl, tl, 0);

    for (int i=0; i<4; i++) {
        cpShapeSetFriction(physics->wall[i], 1.0);
        cpShapeSetElasticity(physics->wall[i], 1.0);
        cpSpaceAddShape(physics->space, physics->wall[i]);
    }

    return physics;
}

void destroy_physics(physics_t *physics)
{
    if (physics) {
        if (physics->tiles) {
            for (int i=0; i<physics->num_tiles; i++) {
                physics_tile_t *pt = &(physics->tiles[i]);

                cpBodyFree(pt->body);
            }
            FREE(physics->tiles);
        }

        for (int i=0; i<4; i++) {
            cpShapeFree(physics->wall[i]);
        }

        cpSpaceFree(physics->space);

        FREE(physics);
    }
}

cpFloat gravity_strength = 5.0e6f;

UNUSED static void tile_velocity_update_func(cpBody *body, UNUSED cpVect gravity, cpFloat damping, cpFloat dt)
{
    cpVect p = cpBodyGetPosition(body);
    cpFloat sqdist = cpvlengthsq(p);
    cpVect g = cpvmult(p, -gravity_strength / (sqdist * cpfsqrt(sqdist)));
    g = cpvmult(g, 20);
    //g = cpvadd(g, gravity);
    cpBodyUpdateVelocity(body, g, damping, dt);
}

void physics_build_tiles(physics_t *physics)
{
    //level_t *level = physics->level;

    physics->num_tiles = level_get_enabled_positions(physics->level);

    physics->tiles = calloc(physics->num_tiles, sizeof(physics_tile_t));

    tile_pos_t *center_pos = level_get_center_tile_pos(physics->level);

    /* cpVect px_offset = cpv(level->px_offset.x, */
    /*                        level->px_offset.y); */

    for (int i=0; i<physics->num_tiles; i++) {
        tile_pos_t *pos = physics->level->enabled_positions[i];
        physics_tile_t *pt = &(physics->tiles[i]);

        pt->pos = pos;

        Vector2 center_position = Vector2Subtract(pos->win.center, window_center);
        cpVect position = cpv(center_position.x,
                              center_position.y);

        cpVect verts[6];
        each_direction {
            Vector2 corner = pos->rel.corners[dir];
            corner = Vector2Scale(corner, 0.95);
            verts[dir] = cpv(corner.x, corner.y);
        }
        /* cpVect verts[6]; */
        /* cpFloat size = pos->size * 1.0; */
        /* for(int i=0; i<6; i++){ */
        /*     cpFloat angle = -2.0f*CP_PI*i/((cpFloat)6); */
        /*     verts[i] = cpv(size*cos(angle), pos->size*sin(angle)); */
        /* } */

        pt->radius = pos->size;
        pt->mass = 10;//pos->size;
        pt->moment = cpMomentForPoly(pt->mass, 6, verts, cpvzero, 0.0f);

        pt->body = cpSpaceAddBody(physics->space, cpBodyNew(pt->mass, pt->moment));
        pt->shape = cpSpaceAddShape(physics->space, cpPolyShapeNew(pt->body, 6, verts, cpTransformIdentity, 0.0));

        cpBodySetUserData(pt->body, pt);

        cpBodySetPosition(pt->body, position);

        if (pos == center_pos) {
            //cpBodySetAngularVelocity(pt->body, 1);

            cpShapeSetElasticity(pt->shape, 1.0f);
            cpShapeSetFriction(pt->shape, 1.0f);
        } else {
            //cpFloat r = cpvlength(position);
            //cpFloat v = cpfsqrt(gravity_strength / r) / r;

            /* Vector2 vel = Vector2Scale(Vector2Normalize(Vector2Rotate(center_position, */
            /*                                                           TAU/4.0f)), */
            /*                            20); */

            //cpVect cpv_vel = cpv(vel.x, vel.y);

            //cpBodySetVelocity(pt->body, cpv_vel);
            //cpBodySetAngularVelocity(pt->body, v / 5.0f);

            cpShapeSetElasticity(pt->shape, 0.0f);
            cpShapeSetFriction(pt->shape, 0.4f);

            //cpBodySetVelocityUpdateFunc(pt->body, tile_velocity_update_func);
            //cpBodySetVelocityUpdateFunc(pt->body, cpBodyUpdateVelocity);
            //cpBodySetPositionUpdateFunc(pt->body, cpBodyUpdatePosition);
        }
    }
}

void physics_start(physics_t *physics)
{
    assert_not_null(physics);

    physics_build_tiles(physics);

    physics->state = PHYSICS_RUNNING;
}

void physics_update(physics_t *physics)
{
    assert_not_null(physics);

    if (physics->state != PHYSICS_RUNNING) {
        return;
    }

    Vector2 winsize = ivector2_to_vector2(window_size);
    //winsize = Vector2Scale(winsize, 0.2);
    float tau_12 = TAU/12.0;

    for (int i=0; i<physics->num_tiles; i++) {
        physics_tile_t *pt = &(physics->tiles[i]);

        cpVect position = cpBodyGetPosition(pt->body);
        cpVect velocity = cpBodyGetVelocity(pt->body);
        //position = cpBodyWorldToLocal(pt->body, position);
        Vector2 vec2_position = cpVectToVector2(position);
        pt->pos->physics_velocity = cpVectToVector2(velocity);
        pt->pos->physics_position = vec2_position;
        float scale = 0.2;
        pt->pos->physics_position = Vector2Scale(pt->pos->physics_position, scale);
        //pt->pos->physics_position.x /= winsize.x * scale;
        //pt->pos->physics_position.y /= winsize.y * scale;
        pt->pos->extra_translate = pt->pos->physics_position;
        pt->pos->extra_rotate = cpBodyGetAngle(pt->body) - tau_12;

        //cpVect force = cpvmult(cpvnormalize(cpvperp(position)), cpvlength(position) * 0.01);
        //cpBodyApplyImpulseAtWorldPoint(pt->body, force, position);
    }

    physics->time += physics->time_step;
    cpSpaceStep(physics->space, physics->time_step/2.0);
    cpSpaceStep(physics->space, physics->time_step/2.0);
}
