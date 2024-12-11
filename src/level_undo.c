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
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.             *
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
    case UNDO_PLAY_TYPE_RESET:
        return "RESET";
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
    case UNDO_EDIT_TYPE_USE_TILES:
        return "USE_TILES";
    case UNDO_EDIT_TYPE_SET_RADIUS:
        return "SET_RADIUS";
    case UNDO_EDIT_TYPE_SET_FLAGS:
        return "SET_FLAGS";
    case UNDO_EDIT_TYPE_SET_FLAGS_AND_PATHS:
        return "SET_FLAGS_AND_PATHS";
    case UNDO_EDIT_TYPE_CHANGE_PATH:
        return "CHANGE_PATH";
    case UNDO_EDIT_TYPE_SHUFFLE:
        return "SHUFFLE";
    default:
        return "(INVALID EDIT EVENT TYPE)";
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

static void cleanup_undo_event(undo_event_t *event)
{
    switch (event->edit.type) {
    case UNDO_EDIT_TYPE_SHUFFLE:
        SAFEFREE(event->edit.shuffle.from);
        SAFEFREE(event->edit.shuffle.to);
        break;

    default:
        /* do nothing */
        break;
    }
}

static void destroy_undo_list(undo_list_t *list)
{
    for (int i=0; i<list->last; i++) {
        cleanup_undo_event(&(list->events[i]));
    }

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

static undo_event_t *_get_undo_event(undo_list_t *list, bool update_current)
{
    assert_not_null(list);

    list = find_current_list(list);

    while (list) {
        //printf("loop: current = %d, last = %d\n", list->current, list->last);
        if (list->current < UNDO_LIST_MAX_EVENTS) {
            if (list->current == 0) {
                if (list->prev) {
                    if (update_current) {
                        list->prev->current--;
                    }
                    return &(list->prev->events[list->prev->current]);
                } else {
                    return NULL;
                }
            } else {
                if (update_current) {
                    list->current--;
                }
                return &(list->events[list->current]);
            }
        }

        list = list->next;
    }

    return NULL;
}

static bool get_undo_event(undo_list_t *list, undo_event_t *eventp)
{
    assert_not_null(list);
    assert_not_null(eventp);

    undo_event_t *prev_event = _get_undo_event(list, true);
    if (prev_event) {
        *eventp = *prev_event;
        return true;
    } else {
        return false;
    }
}

static undo_event_t *find_prev_event(undo_list_t *list)
{
    assert_not_null(list);

    return _get_undo_event(list, false);
}

static undo_event_t *_get_redo_event(undo_list_t *list, bool update_current)
{
    assert_not_null(list);

    list = find_current_list(list);

    while (list) {
        if (list->current < list->last) {
            undo_event_t *rv = &(list->events[list->current]);
            if (update_current) {
                list->current++;
            }
            return rv;
        }

        list = list->next;
    }

    return NULL;
}

static bool get_redo_event(undo_list_t *list, undo_event_t *eventp)
{
    assert_not_null(list);
    assert_not_null(eventp);

    undo_event_t *next_event = _get_redo_event(list, true);
    if (next_event) {
        *eventp = *next_event;
        return true;
    } else {
        return false;
    }
}

UNUSED static undo_event_t *find_next_event(undo_list_t *list)
{
    assert_not_null(list);

    return _get_redo_event(list, false);
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

void chain_with_prev_edit_event(level_t *level, undo_event_t *event)
{
    undo_event_t *prev = find_prev_event(level->undo->edit_event_list);
    if (!prev) {
        return;
    }
    prev->chain_next  = true;
    event->chain_prev = true;
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
        event.type = UNDO_EVENT_TYPE_EDIT;
        event.edit.type = UNDO_EDIT_TYPE_SWAP;
        event.edit.swap.a = a;
        event.edit.swap.b = b;        
        break;

    default:
        __builtin_unreachable();
    }

    level_undo_add_event(level, event);
}

#define PLAY_EVENT(union_name, enum_name) \
    undo_event_t event;                   \
    event.type = UNDO_EVENT_TYPE_PLAY;    \
    event.play.type = UNDO_PLAY_TYPE_##enum_name;

undo_reset_data_t *level_undo_copy_reset_data(level_t *level)
{
    undo_reset_data_t *data = calloc(1, sizeof(undo_reset_data_t));

    data->finished = level->finished;

    memcpy(data->tiles, level->tiles, sizeof(data->tiles));
    memcpy(data->unsolved_positions, level->unsolved_positions, sizeof(data->unsolved_positions));
    if (level->win_anim) {
        data->have_win_anim = true;
        memcpy(&data->win_anim, level->win_anim, sizeof(data->win_anim));
    } else {
        data->have_win_anim = false;
    }

    return data;
}

void level_undo_add_reset(level_t *level, undo_reset_data_t *from, undo_reset_data_t *to)
{
    PLAY_EVENT(reset, RESET);
    event.play.reset.from = from;
    event.play.reset.to   = to;

    level_undo_add_event(level, event);
}

#define EDIT_EVENT(union_name, enum_name) \
    undo_event_t event;                   \
    event.type = UNDO_EVENT_TYPE_EDIT;    \
    event.edit.type = UNDO_EDIT_TYPE_##enum_name;

void level_undo_add_use_tiles_event(level_t *level, used_tiles_t from, used_tiles_t to)
{
    EDIT_EVENT(use_tiles, USE_TILES);
    event.edit.use_tiles.from = from;
    event.edit.use_tiles.to   = to;

    level_undo_add_event(level, event);
}

void level_undo_add_set_radius_event(level_t *level, int from, int to)
{
    EDIT_EVENT(set_radius, SET_RADIUS);
    event.edit.set_radius.from = from;
    event.edit.set_radius.to   = to;

    level_undo_add_event(level, event);
}

void level_undo_add_set_flags_event(
    level_t *level,
    bool include_prev_swap,
    tile_t *tile,
    tile_flags_t from,
    tile_flags_t to)
{
    EDIT_EVENT(set_flags, SET_FLAGS);
    event.edit.set_flags.tile = tile;
    event.edit.set_flags.from = from;
    event.edit.set_flags.to   = to;

    if (include_prev_swap) {
        chain_with_prev_edit_event(level, &event);
    }

    level_undo_add_event(level, event);
}

void level_undo_add_set_flags_event_with_neighbor_paths(
    level_t *level,
    bool include_prev_swap,
    tile_t *tile,
    tile_flags_t flags_from,
    tile_neighbor_paths_t neighbor_paths_from,
    tile_flags_t flags_to,
    tile_neighbor_paths_t neighbor_paths_to)
{
    EDIT_EVENT(set_flags_and_paths, SET_FLAGS_AND_PATHS);
    event.edit.set_flags_and_paths.tile = tile;
    event.edit.set_flags_and_paths.flags_from = flags_from;
    event.edit.set_flags_and_paths.paths_from = neighbor_paths_from;
    event.edit.set_flags_and_paths.flags_to   = flags_to;
    event.edit.set_flags_and_paths.paths_to   = neighbor_paths_to;

    if (include_prev_swap) {
        chain_with_prev_edit_event(level, &event);
    }

    level_undo_add_event(level, event);
}

void level_undo_add_change_path_event(
    level_t *level,

    tile_t *tile,
    hex_direction_t section,
    path_type_t from,
    path_type_t to)
{
    EDIT_EVENT(change_path, CHANGE_PATH);

    event.edit.change_path.tile1           = tile;
    event.edit.change_path.tile1_section   = section;
    event.edit.change_path.tile1_path_from = from;
    event.edit.change_path.tile1_path_to   = to;

    event.edit.change_path.tile2           = NULL;

    level_undo_add_event(level, event);
}

void level_undo_add_change_paths_event(
    level_t *level,

    tile_t *tile1,
    hex_direction_t tile1_section,
    path_type_t tile1_path_from,
    path_type_t tile1_path_to,

    tile_t *tile2,
    hex_direction_t tile2_section,
    path_type_t tile2_path_from,
    path_type_t tile2_path_to)
{
    EDIT_EVENT(change_path, CHANGE_PATH);

    event.edit.change_path.tile1           = tile1;
    event.edit.change_path.tile1_section   = tile1_section;
    event.edit.change_path.tile1_path_from = tile1_path_from;
    event.edit.change_path.tile1_path_to   = tile1_path_to;

    event.edit.change_path.tile2           = tile2;
    event.edit.change_path.tile2_section   = tile2_section;
    event.edit.change_path.tile2_path_from = tile2_path_from;
    event.edit.change_path.tile2_path_to   = tile2_path_to;

    level_undo_add_event(level, event);
}

undo_shuffle_data_t *level_undo_copy_shuffle_data(level_t *level)
{
    undo_shuffle_data_t *data = calloc(1, sizeof(undo_shuffle_data_t));

    memcpy(data->tiles, level->tiles, sizeof(data->tiles));
    memcpy(data->unsolved_positions, level->unsolved_positions, sizeof(data->unsolved_positions));

    return data;
}

void level_undo_add_shuffle(level_t *level, undo_shuffle_data_t *from, undo_shuffle_data_t *to)
{
    EDIT_EVENT(shuffle, SHUFFLE);
    event.edit.shuffle.from = from;
    event.edit.shuffle.to   = to;

    level_undo_add_event(level, event);
}

static void rewind_swap(level_t *level, undo_swap_event_t event)
{
    level_swap_tile_pos_by_position(level, event.a, event.b, false);
}

static void replay_swap(level_t *level, undo_swap_event_t event)
{
    rewind_swap(level, event);
}

static void rewind_use_tiles(level_t *level, undo_use_tiles_event_t event)
{
    level->currently_used_tiles = event.from;
}

static void replay_use_tiles(level_t *level, undo_use_tiles_event_t event)
{
    level->currently_used_tiles = event.to;
}

static void rewind_set_radius(level_t *level, undo_set_radius_event_t event)
{
    level_set_radius(level, event.from);
}

static void replay_set_radius(level_t *level, undo_set_radius_event_t event)
{
    level_set_radius(level, event.to);
}

static void rewind_set_flags(UNUSED level_t *level, undo_set_flags_event_t event)
{
    tile_set_flags(event.tile, event.from);
}

static void replay_set_flags(UNUSED level_t *level, undo_set_flags_event_t event)
{
    tile_set_flags(event.tile, event.to);
}

static void rewind_set_flags_and_paths(UNUSED level_t *level, undo_set_flags_and_paths_event_t event)
{
    tile_set_flags(event.tile, event.flags_from);
    tile_set_neighbor_paths(event.tile, event.paths_from);
}

static void replay_set_flags_and_paths(UNUSED level_t *level, undo_set_flags_and_paths_event_t event)
{
    tile_set_flags(event.tile, event.flags_to);
    tile_set_neighbor_paths(event.tile, event.paths_to);
}

static void rewind_change_path(UNUSED level_t *level, undo_change_path_event_t event)
{
    if (event.tile1) {
        event.tile1->path[event.tile1_section] = event.tile1_path_from;
        tile_update_path_count(event.tile1);
    }

    if (event.tile2) {
        event.tile2->path[event.tile2_section] = event.tile2_path_from;
        tile_update_path_count(event.tile2);
    }
}

static void replay_change_path(UNUSED level_t *level, undo_change_path_event_t event)
{
    if (event.tile1) {
        event.tile1->path[event.tile1_section] = event.tile1_path_to;
        tile_update_path_count(event.tile1);
    }

    if (event.tile2) {
        event.tile2->path[event.tile2_section] = event.tile2_path_to;
        tile_update_path_count(event.tile2);
    }
}

static void apply_shuffle_data(level_t *level, undo_shuffle_data_t *data)
{
    memcpy(level->tiles, data->tiles, sizeof(data->tiles));
    memcpy(level->unsolved_positions, data->unsolved_positions, sizeof(data->unsolved_positions));
}

static void rewind_shuffle(level_t *level, undo_shuffle_t event)
{
    apply_shuffle_data(level, event.from);
}

static void replay_shuffle(level_t *level, undo_shuffle_t event)
{
    apply_shuffle_data(level, event.to);
}

static void apply_reset_data(level_t *level, undo_reset_data_t *data)
{

    memcpy(level->tiles, data->tiles, sizeof(data->tiles));
    memcpy(level->unsolved_positions, data->unsolved_positions, sizeof(data->unsolved_positions));

    if (data->finished) {
        level_win(level);
    } else {
        level_unwin(level);
    }
}

static void rewind_reset(level_t *level, undo_reset_t event)
{
    apply_reset_data(level, event.from);
}

static void replay_reset(level_t *level, undo_reset_t event)
{
    apply_reset_data(level, event.to);
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
        rewind_swap(level, event.play.swap);
        break;

    case UNDO_PLAY_TYPE_RESET:
        rewind_reset(level, event.play.reset);
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
    do {

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
            rewind_swap(level, event.edit.swap);
            break;

        case UNDO_EDIT_TYPE_USE_TILES:
            rewind_use_tiles(level, event.edit.use_tiles);
            break;

        case UNDO_EDIT_TYPE_SET_RADIUS:
            rewind_set_radius(level, event.edit.set_radius);
            break;

        case UNDO_EDIT_TYPE_SET_FLAGS:
            rewind_set_flags(level, event.edit.set_flags);
            break;

        case UNDO_EDIT_TYPE_SET_FLAGS_AND_PATHS:
            rewind_set_flags_and_paths(level, event.edit.set_flags_and_paths);
            break;

        case UNDO_EDIT_TYPE_CHANGE_PATH:
            rewind_change_path(level, event.edit.change_path);
            break;

        case UNDO_EDIT_TYPE_SHUFFLE:
            rewind_shuffle(level, event.edit.shuffle);
            break;

        default:
            __builtin_unreachable();
        }

    } while(event.chain_prev);
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

    case UNDO_PLAY_TYPE_RESET:
        replay_reset(level, event.play.reset);
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
    do {

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

        case UNDO_EDIT_TYPE_USE_TILES:
            replay_use_tiles(level, event.edit.use_tiles);
            break;

        case UNDO_EDIT_TYPE_SET_RADIUS:
            replay_set_radius(level, event.edit.set_radius);
            break;

        case UNDO_EDIT_TYPE_SET_FLAGS:
            replay_set_flags(level, event.edit.set_flags);
            break;

        case UNDO_EDIT_TYPE_SET_FLAGS_AND_PATHS:
            replay_set_flags_and_paths(level, event.edit.set_flags_and_paths);
            break;

        case UNDO_EDIT_TYPE_CHANGE_PATH:
            replay_change_path(level, event.edit.change_path);
            break;

        case UNDO_EDIT_TYPE_SHUFFLE:
            replay_shuffle(level, event.edit.shuffle);
            break;

        default:
            __builtin_unreachable();
        }

    } while(event.chain_next);
}

