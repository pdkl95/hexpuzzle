/****************************************************************************
 *                                                                          *
 * solver.c                                                                 *
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
#include "hex.h"
#include "tile.h"
#include "tile_pos.h"
#include "level.h"
#include "solver.h"

solver_t *create_solver(struct level *level)
{
    assert_not_null(level);

    solver_t *solver = calloc(1, sizeof(solver_t));

    solver->level = level;
    solver->state = SOLVER_STATE_IDLE;
    solver->tile_index = 0;

    return solver;
}

void destroy_solver(solver_t *solver)
{
    SAFEFREE(solver);
}

void solver_start(solver_t *solver)
{
    assert_not_null(solver);

    solver->tile_index = 0;

    solver->state = SOLVER_STATE_SOLVE;
}

void solver_stop(solver_t *solver)
{
    assert_not_null(solver);
    solver->state = SOLVER_STATE_IDLE;
}

void solver_undo(solver_t *solver)
{
    assert_not_null(solver);
    solver->state = SOLVER_STATE_UNDO;
}

static void start_move_anim(solver_t *solver)
{
    solver->anim_step = SOLVER_SWAP_TIME / options->max_fps;
    solver->anim_progress = 0.0;
    solver->anim_running = true;
}

static bool update_move_anim(solver_t *solver)
{
    if (!solver->anim_running) {
        return false;
    }

    solver->anim_progress += solver->anim_step;

    if (solver->anim_progress >= 1.0) {
        solver->anim_progress = 1.0;
        solver->anim_running = false;
        return false;
    } else {
        return true;
    }
}

static bool next_tile_index(solver_t *solver)
{
    int next_tile_index = solver->tile_index + 1;
    if (next_tile_index >= solver->level->tile_count) {
        solver->state = SOLVER_STATE_IDLE;
        return false;
    } else {
        solver->tile_index = next_tile_index;
        return true;
    }    
}

static bool prev_tile_index(solver_t *solver)
{
    int prev_tile_index = solver->tile_index - 1;
    if (prev_tile_index < 0) {
        solver->state = SOLVER_STATE_IDLE;
        return false;
    } else {
        solver->tile_index = prev_tile_index;
        return true;
    }    
}

static void prepare_tile_swap(solver_t *solver)
{
    assert_not_null(solver->swap_a);
    assert_not_null(solver->swap_b);\

    start_move_anim(solver);
}

void solver_update_solve(solver_t *solver)
{
    tile_t *tile;

  retry_solve_with_next_index:
    tile = &(solver->level->tiles[solver->tile_index]);
    if (tile_is_solved(tile)) {
        if (next_tile_index(solver)) {
            goto retry_solve_with_next_index;
        }
    }

    hex_axial_t solved_p   = tile->solved_pos->position;
    hex_axial_t unsolved_p = tile->unsolved_pos->position;

    solver->saved_positions[solver->tile_index].tile              = tile;
    solver->saved_positions[solver->tile_index].solved_position   = solved_p;
    solver->saved_positions[solver->tile_index].unsolved_position = unsolved_p;

    solver->swap_a = level_get_unsolved_tile_pos(solver->level, solved_p);
    solver->swap_b = level_get_unsolved_tile_pos(solver->level, unsolved_p);
    prepare_tile_swap(solver);

    next_tile_index(solver);
}

void solver_update_undo(solver_t *solver)
{
    tile_t *tile;

  retry_undo_with_next_index:
    tile = &(solver->level->tiles[solver->tile_index]);
    if (tile_is_solved(tile)) {
        if (prev_tile_index(solver)) {
            goto retry_undo_with_next_index;
        }
    }

    saved_position_t sp = solver->saved_positions[solver->tile_index];

    solver->swap_a = level_get_unsolved_tile_pos(solver->level, sp.solved_position);
    solver->swap_b = level_get_unsolved_tile_pos(solver->level, sp.unsolved_position);
    prepare_tile_swap(solver);

    prev_tile_index(solver);
}

void solver_update(solver_t *solver)
{
    assert_not_null(solver);
 
    if (update_move_anim(solver)) {
        return;
    }

    switch (solver->state) {
    case SOLVER_STATE_IDLE:
        /* do nothing */
        break;

    case SOLVER_STATE_SOLVE:
        solver_update_solve(solver);
        break;

    case SOLVER_STATE_UNDO:
        solver_update_undo(solver);
        break;

    default:
        __builtin_unreachable();
    }
}

