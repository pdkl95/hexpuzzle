/****************************************************************************
 *                                                                          *
 * level_undo.c                                                             *
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
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.                 *
 *                                                                          *
 ****************************************************************************/

#include "common.h"
#include "level.h"
#include "level_undo.h"

//#define DEBUG_UNDO_LIST

#define UNDO_LIST_MAX_EVENTS 64
struct undo_list {
    undo_event_t events[UNDO_LIST_MAX_EVENTS];
    int last;
    int current;
    struct undo_list *next;
    struct undo_list *prev;
};
typedef struct undo_list undo_list_t;

static const char *undo_play_event_type_str(undo_play_event_type_t type)
{
    switch (type) {
    case UNDO_PLAY_TYPE_NONE:
        return "(NULL)";
    case UNDO_PLAY_TYPE_SWAP:
        return "SWAP";
    default:
        return "(INVALID PLAY EVENT TYPE)";
    }
}

static const char *undo_edit_event_type_str(undo_edit_event_type_t type)
{
    switch (type) {
    case UNDO_EDIT_TYPE_NONE:
        return "(NULL)";
    case UNDO_EDIT_TYPE_SWAP:
        return "SWAP";
    default:
        return "(INVALID PLAY EVENT TYPE)";
    }
}

static const char *undo_event_type_str(undo_event_t *event)
{
    switch (event->type) {
    case UNDO_EVENT_TYPE_PLAY:
        return undo_play_event_type_str(event->play.type);

    case UNDO_EVENT_TYPE_EDIT:
        return undo_edit_event_type_str(event->edit.type);

    default:
        __builtin_unreachable();
    }
}

#ifdef DEBUG_UNDO_LIST
static void print_undo_list(undo_list_t *list)
{
    printf(">>> UNDO List prev=");
    if (list->prev) {
        printf("%p", list->prev);
    } else {
        printf("NULL");
    }
    printf("  >>>\n");
    
    for (int i = 0; i < list->last; i++) {
        undo_event_t *event = &(list->events[i]);
        printf("%s#% 2d: %s\n",
               i == list->current ? "->" : "  ",
               i, undo_event_type_str(event));
    }
    printf("<<<  END List next=");
    if (list->prev) {
        printf("%p", list->next);
    } else {
        printf("NULL");
    }
    printf("  <<<\n");
}

static void print_undo_lists(undo_list_t *list)
{
    while (list) {
        print_undo_list(list);
        list = list->next;
    }
}

static void print_undo(undo_t *undo)
{
    printf("UNDO< play_count=%d edit_count=%d\n",
           undo->play_count, undo->edit_count);

    printf("[PLAY Events] \n");
    print_undo_lists(undo->play_event_list);
    printf("--\n");
    printf("[EDIT Events]: \n");
    print_undo_lists(undo->edit_event_list);
    printf("--\n");
}
#endif

static undo_list_t *create_undo_list(void)
{
    undo_list_t *list = calloc(1, sizeof(undo_list_t));list->current++;

    memset(&(list->events[0]), 0, sizeof(list->events));

    list->last    = 0;
    list->current = 0;

    list->next = NULL;
    list->prev = NULL;

    return list;
}

static void destroy_undo_list(undo_list_t *list)
{
    if (list->prev) {
        list->prev->next = NULL;
        destroy_undo_list(list->prev);
        list->prev = NULL;
    }
    if (list->next) {
        list->next->prev = NULL;
        destroy_undo_list(list->next);
        list->next = NULL;
    }
    SAFEFREE(list);
}

static undo_list_t *find_current_list(undo_list_t *list)
{
    assert_not_null(list);

    while (list->next && list->current >= UNDO_LIST_MAX_EVENTS) {
        list = list->next;
    }

    return list;
}

static bool get_undo_event(undo_list_t *list, undo_event_t *eventp)
{
    assert_not_null(list);
    assert_not_null(eventp);

    list = find_current_list(list);

    while (list) {
        //printf("loop: current = %d, last = %d\n", list->current, list->last);
        if (list->current < UNDO_LIST_MAX_EVENTS) {
            if (list->current == 0) {
                if (list->prev) {
                    list->prev->current--;
                    *eventp = list->prev->events[list->prev->current];
                    return true;
                } else {
                    return false;
                }
            } else {
                list->current--;
                *eventp = list->events[list->current];
                return true;
            }
        }

        list = list->next;
    }

    return false;
}

static bool get_redo_event(undo_list_t *list, undo_event_t *eventp)
{
    assert_not_null(list);
    assert_not_null(eventp);

    list = find_current_list(list);

    while (list) {
        if (list->current < list->last) {
            *eventp = list->events[list->current];
            list->current++;
            return true;
        }

        list = list->next;
    }

    return false;
}

undo_t *create_level_undo(struct level *level)
{
    assert_not_null(level);
    undo_t *undo = calloc(1, sizeof(undo_t));

    undo->level = level;

    undo->play_count = 0;
    undo->edit_count = 0;

    undo->play_event_list = create_undo_list();
    undo->edit_event_list = create_undo_list();

    return undo;
}

void destroy_level_undo(undo_t *undo)
{
    if (undo->play_event_list) {
        destroy_undo_list(undo->play_event_list);
        undo->play_event_list = NULL;
    }

    if (undo->edit_event_list) {
        destroy_undo_list(undo->edit_event_list);
        undo->edit_event_list = NULL;
    }

    SAFEFREE(undo);
}

void level_undo_add_event(level_t *level, undo_event_t event)
{
    assert_not_null(level);
    assert_not_null(level->undo);

    undo_list_t *list = NULL;

    switch (event.type) {
    case UNDO_EVENT_TYPE_PLAY: 
        level->undo->play_count++;
        list = level->undo->play_event_list;
#ifdef DEBUG_UNDO_LIST
        printf("add undo PLAY event[%d]: %s\n",
               level->undo->play_count,
               undo_event_type_str(&event));
#endif

        break;

    case UNDO_EVENT_TYPE_EDIT:
        level->undo->edit_count++;
        list = level->undo->edit_event_list;
#ifdef DEBUG_UNDO_LIST
        printf("add undo EDIT event[%d]: %s\n",
               level->undo->edit_count,
               undo_event_type_str(&event));
#endif

        break;

    default:
        __builtin_unreachable();
    }

    while ((list->last    == UNDO_LIST_MAX_EVENTS) &&
           (list->current == UNDO_LIST_MAX_EVENTS)) {
        if (!list->next) {
            undo_list_t *newlist = create_undo_list();
            newlist->prev = list;
            list->next = newlist;
        }

        list = list->next;
    }

    list->events[list->current] = event;

    list->current++;
    list->last = list->current;

#ifdef DEBUG_UNDO_LIST
    print_undo(level->undo);
    //print_undo_lists(list);
#endif
}

void level_undo_add_swap_event(level_t *level, hex_axial_t a, hex_axial_t b)
{
    undo_event_t event;

    switch (game_mode) {
    case GAME_MODE_PLAY_LEVEL:
        event.type = UNDO_EVENT_TYPE_PLAY;
        event.play.type = UNDO_PLAY_TYPE_SWAP;
        event.play.swap.a = a;
        event.play.swap.b = b;
        break;

    case GAME_MODE_EDIT_LEVEL:
        event.type = UNDO_EVENT_TYPE_PLAY;
        event.edit.type = UNDO_EDIT_TYPE_SWAP;
        event.edit.swap.a = a;
        event.edit.swap.b = b;        
        break;

    default:
        __builtin_unreachable();
    }

    level_undo_add_event(level, event);
}

static void replay_swap(level_t *level, undo_swap_event_t event)
{
    level_swap_tile_pos_by_position(level, event.a, event.b, false);
}

void level_undo_add_play_event(level_t *level, undo_play_event_t play_event)
{
    assert_not_null(level);
    assert_not_null(level->undo);

    undo_event_t event = { .type = UNDO_EVENT_TYPE_PLAY };
    event.play = play_event;

    level_undo_add_event(level, event);}

void level_undo_add_edit_event(level_t *level, undo_edit_event_t edit_event)
{
    assert_not_null(level);
    assert_not_null(level->undo);

    undo_event_t event = { .type = UNDO_EVENT_TYPE_EDIT };
    event.edit = edit_event;

    level_undo_add_event(level, event);
}

void level_undo_play(level_t *level)
{
    assert_not_null(level);
    assert_not_null(level->undo);

    /* printf("\nundo_play\n"); */
    /* print_undo(level->undo); */

    undo_event_t event;
    if (!get_undo_event(level->undo->play_event_list, &event)) {
        if (options->verbose) {
            infomsg("UNDO(play): nothing to undo");
        }
        return;
    }

    assert(event.type == UNDO_EVENT_TYPE_PLAY);

    level->undo->play_count--;
#ifdef DEBUG_UNDO_LIST
    printf("\nafter undo_play\n");
    print_undo(level->undo);
    //print_undo_lists(level->undo->play_event_list);
#endif

    if (options->verbose) {
        infomsg("UNDO(play): %s event #%d",
                undo_event_type_str(&event),
                level->undo->play_count);
    }

    switch (event.edit.type) {
    case UNDO_PLAY_TYPE_SWAP:
        replay_swap(level, event.play.swap);
        break;

    default:
        __builtin_unreachable();
    }
}

void level_undo_edit(level_t *level)
{
    assert_not_null(level);
    assert_not_null(level->undo);

    undo_event_t event;
    if (!get_undo_event(level->undo->edit_event_list, &event)) {
        if (options->verbose) {
            infomsg("UNDO(edit): nothing to undo");
        }
        return;
    }

    assert(event.type == UNDO_EVENT_TYPE_EDIT);

    level->undo->edit_count--;

#ifdef DEBUG_UNDO_LIST
    print_undo_lists(level->undo->edit_event_list);
#endif

    if (options->verbose) {
        infomsg("UNDO(edit): %s event #%d",
                undo_event_type_str(&event),
                level->undo->edit_count);
    }

    switch (event.edit.type) {
    case UNDO_EDIT_TYPE_SWAP:
        replay_swap(level, event.edit.swap);
        break;

    default:
        __builtin_unreachable();
    }
}

void level_redo_play(level_t *level)
{
    assert_not_null(level);
    assert_not_null(level->undo);

    undo_event_t event;
    if (!get_redo_event(level->undo->play_event_list, &event)) {
        if (options->verbose) {
            infomsg("REDO(play): nothing to redo");
        }
        return;
    }

    assert(event.type == UNDO_EVENT_TYPE_PLAY);

    level->undo->play_count++;

#ifdef DEBUG_UNDO_LIST
    print_undo_lists(level->undo->play_event_list);
#endif

    if (options->verbose) {
        infomsg("REDO (play mode) %s event #%d",
                undo_event_type_str(&event),
                level->undo->play_count);
    }

    switch (event.edit.type) {
    case UNDO_PLAY_TYPE_SWAP:
        replay_swap(level, event.play.swap);
        break;

    default:
        __builtin_unreachable();
    }
}

void level_redo_edit(level_t *level)
{
    assert_not_null(level);
    assert_not_null(level->undo);

    undo_event_t event;
    if (!get_redo_event(level->undo->edit_event_list, &event)) {
        if (options->verbose) {
            infomsg("REDO(edit): nothing to redo");
        }
        return;
    }

    assert(event.type == UNDO_EVENT_TYPE_EDIT);

    level->undo->edit_count++;

#ifdef DEBUG_UNDO_LIST
    print_undo_lists(level->undo->edit_event_list);
#endif

    if (options->verbose) {
        infomsg("REDO (edit mode) %s event #%d",
                undo_event_type_str(&event),
                level->undo->edit_count);
    }

    switch (event.edit.type) {
    case UNDO_EDIT_TYPE_SWAP:
        replay_swap(level, event.edit.swap);
        break;

    default:
        __builtin_unreachable();
    }
}

