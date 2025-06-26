/****************************************************************************
 *                                                                          *
 * solve_timer.c                                                            *
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

#include "solve_timer.h"

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR  (60 * SECONDS_PER_MINUTE)
#define SECONDS_PER_DAY   (24 * SECONDS_PER_HOUR)

#define NSEC_PER_SEC 1000000000

#define ELAPSED_TIME_FMT "%dms%ds%dm%dh%dd"

char *elapsed_time_parts_to_str(elapsed_time_parts_t *parts)
{
    static char buf[ELAPSED_TIME_STR_MAXLEN];

    snprintf(buf,
             ELAPSED_TIME_STR_MAXLEN,
             ELAPSED_TIME_FMT,
             parts->ms,
             parts->sec,
             parts->min,
             parts->hr,
             parts->day);

    return buf;
}

char *solve_elapsed_time_to_str(solve_elapsed_time_t *elapsed_time)
{
    return elapsed_time_parts_to_str(&(elapsed_time->parts));
}

bool str_to_elapsed_time_parts(const char *str, elapsed_time_parts_t *parts)
{
    errno = 0;
    int scan_ret = sscanf(str,
                          ELAPSED_TIME_FMT,
                          &parts->ms,
                          &parts->sec,
                          &parts->min,
                          &parts->hr,
                          &parts->day);

    if (errno) {
        scan_ret = -1;
        errmsg("while trying to parse \"%s\" as an elapsed time: %s",
               str, strerror(errno));
    }

    bool rv = false;

    switch (scan_ret) {
    case 1:
        parts->sec = 0;
        fallthrough;
    case 2:
        parts->min = 0;
        fallthrough;
    case 3:
        parts->hr  = 0;
        fallthrough;
    case 4:
        parts->day = 0;
        fallthrough;
    case 5:
        /* all fields ok */
        rv = true;
        break;

    default:
        parts->ms  = 0;
        parts->sec = 0;
        parts->min = 0;
        parts->hr  = 0;
        parts->day = 0;
        rv = false;
        break;
    }

    return rv;
}
#undef ELAPSED_TIME_FMT

solve_elapsed_time_t str_to_solve_elapsed_time(const char *str)
{
    solve_elapsed_time_t elapsed_time = {0};

    if (str_to_elapsed_time_parts(str, &(elapsed_time.parts))) {
        elapsed_time.valid = true;
    } else {
        elapsed_time.valid = false;
    }

    return elapsed_time;
}

static inline void get_currernt_time(struct timespec *ts)
{
    timespec_get(ts, TIME_UTC);
}

struct timespec timespec_add(struct timespec a, struct timespec b)
{
    struct timespec res = {0};

    res.tv_sec  = a.tv_sec  + b.tv_sec;
    res.tv_nsec = a.tv_nsec + b.tv_nsec;

    while (res.tv_nsec >= NSEC_PER_SEC) {
        res.tv_sec  += 1;
        res.tv_nsec -= NSEC_PER_SEC;
    }
    
    return res;
}

struct timespec timespec_sub(struct timespec a, struct timespec b)
{
    struct timespec res = {0};

    if (a.tv_nsec < b.tv_nsec) {
        res.tv_sec  = a.tv_sec  - b.tv_sec  - 1;
        res.tv_nsec = a.tv_nsec - b.tv_nsec + 1000000000;
    } else {
        res.tv_sec  = a.tv_sec  - b.tv_sec;
        res.tv_nsec = a.tv_nsec - b.tv_nsec;
    }

    return res;
}

solve_timer_t *alloc_solve_timer(void)
{
    return calloc(1, sizeof(solve_timer_t));
}

void free_solve_timer(solve_timer_t *solve_timer)
{
    if (solve_timer) {
        free(solve_timer);
    }
}

void init_solve_timer(solve_timer_t *solve_timer)
{
    solve_timer_reset(solve_timer);
}

solve_timer_t *create_solve_timer(void)
{
    solve_timer_t *solve_timer = alloc_solve_timer();
    init_solve_timer(solve_timer);
    return solve_timer;
}

void destroy_solve_timer(solve_timer_t *solve_timer)
{
    free_solve_timer(solve_timer);
}


void solve_timer_update(solve_timer_t *solve_timer)
{
    if (!solve_timer->valid) {
        return;
    }

    if (solve_timer->state == SOLVE_TIMER_STATE_RUNNING) {
        struct timespec now = {0};
        get_currernt_time(&now);

        struct timespec delta = timespec_sub(now, solve_timer->start_time);
        solve_timer->elapsed_time.ts = timespec_add(delta, solve_timer->elapsed_time.ts);

        solve_timer->start_time = now;
    }

    struct timespec et = solve_timer->elapsed_time.ts;

    solve_timer->elapsed_time.parts.day = et.tv_sec  / SECONDS_PER_DAY;
    et.tv_sec -= solve_timer->elapsed_time.parts.day * SECONDS_PER_DAY;

    solve_timer->elapsed_time.parts.hr  = et.tv_sec  / SECONDS_PER_HOUR;
    et.tv_sec -= solve_timer->elapsed_time.parts.hr  * SECONDS_PER_HOUR;

    solve_timer->elapsed_time.parts.min = et.tv_sec  / SECONDS_PER_MINUTE;
    et.tv_sec -= solve_timer->elapsed_time.parts.min * SECONDS_PER_MINUTE;

    solve_timer->elapsed_time.parts.sec = et.tv_sec;

    assert(solve_timer->elapsed_time.ts.tv_sec == ((solve_timer->elapsed_time.parts.day * SECONDS_PER_DAY) +
                                                   (solve_timer->elapsed_time.parts.hr  * SECONDS_PER_HOUR) +
                                                   (solve_timer->elapsed_time.parts.min * SECONDS_PER_MINUTE) +
                                                   (solve_timer->elapsed_time.parts.sec)));

    solve_timer->elapsed_time.parts.ms = et.tv_nsec / 1000000;
}

void solve_timer_reset(solve_timer_t *solve_timer)
{
    solve_timer->state = SOLVE_TIMER_STATE_RESET;
    solve_timer->valid = true;
    solve_timer->elapsed_time.ts.tv_sec  = 0;
    solve_timer->elapsed_time.ts.tv_nsec = 0;
    solve_timer_update(solve_timer);
}

void solve_timer_start(solve_timer_t *solve_timer)
{
    if (solve_timer->state == SOLVE_TIMER_STATE_RESET) {
        solve_timer->elapsed_time.ts.tv_sec  = 0;
        solve_timer->elapsed_time.ts.tv_nsec = 0;
    }
    solve_timer->state = SOLVE_TIMER_STATE_RUNNING;
    get_currernt_time(&solve_timer->start_time);
    solve_timer_update(solve_timer);
}

void solve_timer_stop(solve_timer_t *solve_timer)
{
    solve_timer_update(solve_timer);
    solve_timer->state = SOLVE_TIMER_STATE_PAUSED;
}

