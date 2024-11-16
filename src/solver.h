/****************************************************************************
 *                                                                          *
 * solver.h                                                                 *
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

#ifndef SOLVER_H
#define SOLVER_H

enum solver_state {
    SOLVER_STATE_IDLE = 0,
    SOLVER_STATE_SOLVE,
    SOLVER_STATE_UNDO
};
typedef enum solver_state solver_state_t;

struct saved_position {
    struct tile *tile;
    hex_axial_t solved_position;
    hex_axial_t unsolved_position;
};
typedef struct saved_position saved_position_t;

struct solver {
    struct level *level;
    saved_position_t saved_positions[LEVEL_MAXTILES];

    solver_state_t state;
    bool anim_running;
    float anim_progress;
    float anim_step;

    int tile_index;
    struct tile_pos *swap_a;
    struct tile_pos *swap_b;
};
typedef struct solver solver_t;

solver_t *create_solver(struct level *kevek);
void destroy_solver(solver_t *solver);

void solver_start(solver_t *solver);
void solver_stop(solver_t *solver);
void solver_undo(solver_t *solver);
void solver_update(solver_t *solver);

#endif /*SOLVER_H*/

