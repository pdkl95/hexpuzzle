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

    memset(physics->tiles, 0, sizeof(physics->tiles));

    physics->time = 0.0;
    physics->time_step = 1.0 / options->max_fps;

    physics->space = cpSpaceNew();

    cpSpaceSetIterations(physics->space, 20);

    cpVect gravity = cpv(0.0, 250.0);
    cpSpaceSetGravity(physics->space, gravity);

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
        cpShapeSetElasticity(physics->wall[i], 1.0);
        cpSpaceAddShape(physics->space, physics->wall[i]);
    }

    return physics;
}

void destroy_physics(physics_t *physics)
{
    if (physics) {
        for (int i=0; i<LEVEL_MAXTILES; i++) {
            physics_tile_t *pt = &(physics->tiles[i]);

            for (hex_direction_t dir=0; dir<3; dir++) {
                if (pt->path_constraint[dir]) {
                    cpConstraintFree(pt->path_constraint[dir]);
                }
            }

            cpShapeFree(pt->shape);
            cpBodyFree(pt->body);
        }

        for (int i=0; i<4; i++) {
            cpShapeFree(physics->wall[i]);
        }

        cpSpaceFree(physics->space);

        FREE(physics);
    }
}

void physics_build_tiles(physics_t *physics)
{
    level_t *level = physics->level;

    physics->num_tiles = level_get_enabled_tiles(level);

    for (int i=0; i<physics->num_tiles; i++) {
        tile_t *tile = level->enabled_tiles[i];\
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
            //corner = Vector2Scale(corner, 0.95);
            verts[dir] = cpv(corner.x, corner.y);
        }
        pt->radius = pos->size;
        pt->mass = 11; //pos->size;
        pt->moment = cpMomentForPoly(pt->mass, 6, verts, cpvzero, 0.0f);

        pt->body = cpSpaceAddBody(physics->space, cpBodyNew(pt->mass, pt->moment));
        pt->shape = cpSpaceAddShape(physics->space, cpPolyShapeNew(pt->body, 6, verts, cpTransformIdentity, 0.0));

        cpBodySetUserData(pt->body, pt);

        cpBodySetPosition(pt->body, position);

        cpShapeSetElasticity(pt->shape, 0.4f);
        cpShapeSetFriction(pt->shape, 0.4f);
    }
#if 0
    for (int i=0; i<physics->num_tiles; i++) {
        physics_tile_t *pt = &(physics->tiles[i]);
        tile_t *tile = pt->tile;;
        tile_pos_t *pos = tile->unsolved_pos;
        tile_pos_t *solved_pos = tile->solved_pos;

        pt->constraint_count = 0;

        for (hex_direction_t dir=0; dir<3; dir++) {
            if (tile->path[dir] == PATH_TYPE_NONE) {
                continue;
            }
            hex_direction_t opposite_dir = hex_opposite_direction(dir);

            tile_pos_t *solved_neighbor = solved_pos->neighbors[dir];
            assert_not_null(solved_neighbor);
            assert(solved_neighbor->tile->enabled);
            assert(tile->path[dir] == solved_neighbor->tile->path[opposite_dir]);
            tile_pos_t *neighbor = solved_neighbor->tile->unsolved_pos;
            assert_not_null(neighbor);

            cpConstraint *c = cpDampedSpringNew(
                pt->body,
                neighbor->tile->physics_tile->body,
                Vector2TocpVect(pos->rel.midpoints[dir]),
                Vector2TocpVect(neighbor->rel.midpoints[opposite_dir]),
                pos->size * 2,
                10.0f,
                0.5f);

            cpSpaceAddConstraint(physics->space, c);

            pt->path_constraint[pt->constraint_count] = c;
            pt->constraint_count++;
        }
    }
#endif
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

    physics->time += physics->time_step;
    cpSpaceStep(physics->space, physics->time_step);
}
