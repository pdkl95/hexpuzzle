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

//#define DEBUG_SOLVER

static void stop_move_anim(solver_t *solver);

const char *solver_state_name(solver_state_t state)
{
    switch (state) {
    case SOLVER_STATE_IDLE:
        return "IDLE";
    case SOLVER_STATE_SOLVE:
        return "SOLVE";
    case SOLVER_STATE_SOLVE_MOVING:
        return "SOLVE_MOVING";
    case SOLVER_STATE_SOLVE_MOVING_TO_NEXT_TILE:
        return "SOLVE_MOVING_TO_NEXT_TILE";
    case SOLVER_STATE_UNDO:
        return "UNDO";
    case SOLVER_STATE_UNDO_MOVING:
        return "UNDO_MOVING";
    default:
        __builtin_unreachable();
    }
}

solver_t *create_solver(struct level *level)
{
    assert_not_null(level);

    solver_t *solver = calloc(1, sizeof(solver_t));

    solver->level = level;
    solver->state = SOLVER_STATE_IDLE;
    solver->tile_index   = 0;
    solver->solved_index = 0;

    return solver;
}

solver_t *create_or_use_solver(struct level *level)
{
    if (!level->solver) {
        level->solver = create_solver(level);
    }

    return level->solver;
}

void destroy_solver(solver_t *solver)
{
    SAFEFREE(solver);
}

void solver_set_state(solver_t *solver, solver_state_t new_state)
{
    if (new_state == solver->state) {
        return;
    }

#ifdef DEBUG_SOLVER
    printf("solver: STATE CHANGE \"%s\" -> \"%s\"\n",
           solver_state_name(solver->state),
           solver_state_name(new_state));
#endif

    /* exiting old state */
    switch (new_state) {
    case SOLVER_STATE_IDLE:
        solver->saved_mouse_position = mouse_position;
        disable_mouse_input();
        break;

    case SOLVER_STATE_SOLVE:
        break;

    case SOLVER_STATE_SOLVE_MOVING:
        break;

    case SOLVER_STATE_SOLVE_MOVING_TO_NEXT_TILE:
        break;

    case SOLVER_STATE_UNDO:
        break;

    case SOLVER_STATE_UNDO_MOVING:
        break;

    default:
        __builtin_unreachable();
    }

    /* entering new state */
    switch (new_state) {
    case SOLVER_STATE_IDLE:
        stop_move_anim(solver);
        set_mouse_position(solver->saved_mouse_position.x,
                           solver->saved_mouse_position.y);

        solver->fast = false;

        enable_mouse_input();
        break;

    case SOLVER_STATE_SOLVE:
        break;

    case SOLVER_STATE_SOLVE_MOVING:
        break;

    case SOLVER_STATE_SOLVE_MOVING_TO_NEXT_TILE:
        break;

    case SOLVER_STATE_UNDO:
        break;

    case SOLVER_STATE_UNDO_MOVING:
        break;

    default:
        __builtin_unreachable();
    }

    solver->state = new_state;

    if (new_state != SOLVER_STATE_IDLE) {
        render_next_frame_no_waiting = true;
    }
}

void solver_toggle_solve(solver_t *solver)
{
    switch (solver->state) {
    case SOLVER_STATE_IDLE:
        solver_set_state(solver, SOLVER_STATE_SOLVE);
        break;

    case SOLVER_STATE_SOLVE_MOVING_TO_NEXT_TILE:
        fallthrough;
    case SOLVER_STATE_SOLVE:
        fallthrough;
    case SOLVER_STATE_SOLVE_MOVING:
        solver_set_state(solver, SOLVER_STATE_IDLE);
        break;

    case SOLVER_STATE_UNDO:
        fallthrough;
    case SOLVER_STATE_UNDO_MOVING:
        solver_set_state(solver, SOLVER_STATE_IDLE);
        break;

    default:
        __builtin_unreachable();
    }
}

void solver_toggle_undo(solver_t *solver)
{
    switch (solver->state) {
    case SOLVER_STATE_IDLE:
        solver_set_state(solver, SOLVER_STATE_UNDO);
        break;

    case SOLVER_STATE_SOLVE_MOVING_TO_NEXT_TILE:
        fallthrough;
    case SOLVER_STATE_SOLVE:
        fallthrough;
    case SOLVER_STATE_SOLVE_MOVING:
        solver_set_state(solver, SOLVER_STATE_IDLE);
        break;

    case SOLVER_STATE_UNDO:
        fallthrough;
    case SOLVER_STATE_UNDO_MOVING:
        solver_set_state(solver, SOLVER_STATE_IDLE);
        break;

    default:
        __builtin_unreachable();
    }
}

void solver_start(solver_t *solver)
{
    assert_not_null(solver);

#ifdef DEBUG_SOLVER
    printf("solver: START\n");
#endif

    solver->tile_index   = 0;
    solver->solved_index = 0;

    solver_set_state(solver, SOLVER_STATE_SOLVE);
}


void solver_start_fast(solver_t *solver)
{
    solver->fast = true;
    assert_not_null(solver);

#ifdef DEBUG_SOLVER
    printf("solver: START (FAST)\n");
#endif

    solver->tile_index   = 0;
    solver->solved_index = 0;

    solver_set_state(solver, SOLVER_STATE_SOLVE);
}

void solver_stop(solver_t *solver)
{
    assert_not_null(solver);

#ifdef DEBUG_SOLVER
    printf("solver: STOP\n");
#endif

    solver_set_state(solver, SOLVER_STATE_IDLE);
}

void solver_undo(solver_t *solver)
{
    assert_not_null(solver);

#ifdef DEBUG_SOLVER
    printf("solver: UNDO\n");
#endif

    solver_set_state(solver, SOLVER_STATE_UNDO);
}

static bool update_move_anim(solver_t *solver);

static void start_move_anim(solver_t *solver, float anim_time, bool is_drag)
{
    if (solver->anim_running) {
        stop_move_anim(solver);
    }

    solver->move_is_drag = is_drag;

    int total_frames = options->max_fps * anim_time;
    solver->anim_step = 1.0 / (float)total_frames;
    solver->anim_progress = 0.0;
    solver->anim_running = true;

#ifdef DEBUG_SOLVER
    printf("solver: start anim (tile_index = %d)\n", solver->tile_index);
#endif

    disable_mouse_input();

    IVector2 ipos = vector2_to_ivector2(solver->start_px);
    set_mouse_position(ipos.x, ipos.y);

    if (solver->move_is_drag) {
        level_drag_start(solver->level);
    }

    if (solver->fast) {
        solver->anim_progress = 1.0;
    }
}

static void stop_move_anim(solver_t *solver)
{
    if (!solver->anim_running) {
        return;
    }

    solver->anim_progress = 1.0;
    solver->anim_running = false;

#ifdef DEBUG_SOLVER
    printf("solver: stop anim\n");
#endif

    if (solver->move_is_drag) {
        level_drag_stop(solver->level);
    }
}

static bool update_move_anim(solver_t *solver)
{
    if (!solver->anim_running) {
        return false;
    }

    solver->anim_progress += solver->anim_step;
    CLAMPVAR(solver->anim_progress, 0.0f, 1.0f);

    float t = ease_quartic_inout(solver->anim_progress);
    Vector2 cur_pos = Vector2Lerp(solver->start_px, solver->end_px, t);
    IVector2 icur_pos = vector2_to_ivector2(cur_pos);
    set_mouse_position(icur_pos.x, icur_pos.y);

#ifdef DEBUG_SOLVER
    printf("solver: anim_progress = %f, cur_pos = <%d, %d>\n", solver->anim_progress, icur_pos.x, icur_pos.y);
#endif

    render_next_frame_no_waiting = true;

    if (solver->anim_progress >= 1.0) {
        stop_move_anim(solver);
        return true;
    } else {
        return false;
    }
}

static bool next_tile_index(solver_t *solver)
{
    int next_tile_index = solver->tile_index + 1;
    if (next_tile_index >= LEVEL_MAXTILES) {
        solver_set_state(solver, SOLVER_STATE_IDLE);
        return true;
    } else {
        solver->tile_index = next_tile_index;
        return false;
    }    
}

static bool next_solved_index(solver_t *solver)
{
    int next_solved_index = solver->solved_index + 1;
    if (next_solved_index > solver->level->enabled_tile_count) {
        solver_set_state(solver, SOLVER_STATE_IDLE);
        return true;
    } else {
        solver->solved_index = next_solved_index;
        return false;
    }    
}

static bool prev_solved_index(solver_t *solver)
{
    int prev_solved_index = solver->solved_index - 1;
    if (prev_solved_index < 0) {
        stop_move_anim(solver);
        solver_set_state(solver, SOLVER_STATE_IDLE);
        return true;
    } else {
        solver->solved_index = prev_solved_index;
        return false;
    }
}

static void prepare_tile_swap(solver_t *solver, float anim_time)
{
    assert_not_null(solver->swap_a);
    assert_not_null(solver->swap_b);
    assert(anim_time > 0.0);

    solver->start_px = Vector2Add(solver->swap_a->win.center, solver->level->px_offset);
    solver->end_px   = Vector2Add(solver->swap_b->win.center, solver->level->px_offset);

#ifdef DEBUG_SOLVER
    printf("solver: swap pos <%d,%d> and <%d,%d>\n",
           solver->swap_a->position.q,
           solver->swap_a->position.r,
           solver->swap_b->position.q,
           solver->swap_b->position.r);
    printf("solver:       px <%f,%f> and <%f,%f>\n",
           solver->start_px.x,
           solver->start_px.y,
           solver->end_px.x,
           solver->end_px.y);
#endif

    start_move_anim(solver, anim_time, true);
}

static void solver_setup_tile_swap(solver_t *solver)
{
    prepare_tile_swap(solver, SOLVER_SOLVE_SWAP_TIME);
    solver_set_state(solver, SOLVER_STATE_SOLVE_MOVING);
}

static void solver_setup_tile_swap_after_move_pointer(solver_t *solver)
{
    prepare_tile_swap(solver, SOLVER_DEMO_SOLVE_SWAP_TIME);
    solver_set_state(solver, SOLVER_STATE_SOLVE_MOVING);
}

static void prepare_move_pointer(solver_t *solver, float anim_time)
{
    assert_not_null(solver->swap_a);
    assert_not_null(solver->swap_b);
    assert(anim_time > 0.0);

    solver->start_px = mouse_positionf;
    solver->end_px = Vector2Add(solver->swap_a->win.center, solver->level->px_offset);

    if (Vector2Equals(solver->start_px, solver->end_px)) {
        /* skip the move pointer anim time if we are
           alresdy in the correct position */
        solver_setup_tile_swap_after_move_pointer(solver);
    } else {
        start_move_anim(solver, anim_time, false);
    }
}

static void solver_setup_move_pointer(solver_t *solver)
{
    prepare_move_pointer(solver, SOLVER_DEMO_SOLVE_MOVE_POINTER_TIME);
    solver_set_state(solver, SOLVER_STATE_SOLVE_MOVING_TO_NEXT_TILE);
}

void solver_update_solve(solver_t *solver)
{
    tile_t *tile;

  retry_solve_with_next_index:
    tile = &(solver->level->tiles[solver->tile_index]);
    if (tile_is_solved(tile)) {

#ifdef DEBUG_SOLVER
        printf("solver: skip already solved tile: ");
        print_tile(tile);
#endif
        if (next_tile_index(solver)) {
#ifdef DEBUG_SOLVER
            printf("early return\n");
#endif
            return;
        }

        goto retry_solve_with_next_index;
    }

    hex_axial_t solved_p   = tile->solved_pos->position;
    hex_axial_t unsolved_p = tile->unsolved_pos->position;

    solver->saved_positions[solver->solved_index].tile              = tile;
    solver->saved_positions[solver->solved_index].solved_position   = solved_p;
    solver->saved_positions[solver->solved_index].unsolved_position = unsolved_p;

    solver->swap_b = level_get_unsolved_tile_pos(solver->level, solved_p);
    solver->swap_a = level_get_unsolved_tile_pos(solver->level, unsolved_p);

    if (demo_mode) {
        solver_setup_move_pointer(solver);
    } else {
        solver_setup_tile_swap(solver);
    }

    next_tile_index(solver);
    next_solved_index(solver);
}

void solver_update_undo(solver_t *solver)
{
    saved_position_t sp = solver->saved_positions[solver->solved_index];
    solver->tile_index = sp.tile_index;

    //tile_t *tile = &(solver->level->tiles[solver->tile_index]);

    solver->swap_a = level_get_unsolved_tile_pos(solver->level, sp.solved_position);
    solver->swap_b = level_get_unsolved_tile_pos(solver->level, sp.unsolved_position);

    prepare_tile_swap(solver, SOLVER_UNDO_SWAP_TIME);
    solver_set_state(solver, SOLVER_STATE_UNDO_MOVING);

    prev_solved_index(solver);
}

void solver_update(solver_t *solver)
{
    assert_not_null(solver);

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

    case SOLVER_STATE_SOLVE_MOVING_TO_NEXT_TILE:
        if (update_move_anim(solver)) {
            solver_setup_tile_swap_after_move_pointer(solver);
        }
        break;

    case SOLVER_STATE_SOLVE_MOVING:
        if (update_move_anim(solver)) {
            solver_set_state(solver, SOLVER_STATE_SOLVE);
        }
        break;

    case SOLVER_STATE_UNDO_MOVING:
        if (update_move_anim(solver)) {
            solver_set_state(solver, SOLVER_STATE_UNDO);
        }
        break;

    default:
        __builtin_unreachable();
    }
}

