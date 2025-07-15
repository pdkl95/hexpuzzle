/****************************************************************************
 *                                                                          *
 * solve_timer.h                                                            *
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

#ifndef SOLVE_TIMER_H
#define SOLVE_TIMER_H

#include <time.h>

#define ELAPSED_TIME_STR_MAXLEN 256

struct elapsed_time_parts {
    int day;
    int hr;
    int min;
    int sec;
    int ms;
};
typedef struct elapsed_time_parts elapsed_time_parts_t;

#define ELAPSED_TIME_PARTS_NULL ((elapsed_time_parts_t){ .day = 0, .hr = 0, .min = 0, .sec = 0, .ms = 0 })

char *elapsed_time_parts_to_str(elapsed_time_parts_t *parts);
bool str_to_elapsed_time_parts(const char *str, elapsed_time_parts_t *parts);

struct solve_elapsed_time {
    bool valid;

    struct timespec ts;
    elapsed_time_parts_t parts;
};
typedef struct solve_elapsed_time solve_elapsed_time_t;

enum solve_timer_state {
    SOLVE_TIMER_STATE_RESET = 0,
    SOLVE_TIMER_STATE_RUNNING,
    SOLVE_TIMER_STATE_PAUSED,
};
typedef enum solve_timer_state solve_timer_state_t;

struct solve_timer {
    solve_timer_state_t state;

    /*
     * set true on reset; becomes false in cases
     * where validity was disrupted.
     */
    bool valid;

    /* 
     * Time of the most recent transition to RUNNING stater
     * OR the most recent call to solve_timer_update(), whichever
     * is cloxer.
     */
    struct timespec start_time;

    /*
     * Total cumulative elqapwsd time the solve_timer
     * has been in the RUNNING state.
     */
    solve_elapsed_time_t elapsed_time;
};
typedef struct solve_timer solve_timer_t;

char *solve_elapsed_time_to_str(solve_elapsed_time_t *elapsed_time);
solve_elapsed_time_t str_to_solve_elapsed_time(const char *str);
const char *elapsed_time_parts_to_readable_string(elapsed_time_parts_t *elapsed_time_parts);

solve_timer_t *alloc_solve_timer(void);
void free_solve_timer(solve_timer_t *solve_timer);
void init_solve_timer(solve_timer_t *solve_timer);
solve_timer_t *create_solve_timer(void);
void destroy_solve_timer(solve_timer_t *solve_timer);

void solve_timer_update(solve_timer_t *solve_timer);

void solve_timer_reset(solve_timer_t *solve_timer);
void solve_timer_start(solve_timer_t *solve_timer);
void solve_timer_stop(solve_timer_t *solve_timer);

#endif /*SOLVE_TIMER_H*/

