/****************************************************************************
 *                                                                          *
 * gui_popup_message.c                                                      *
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

#include <limits.h>

#include "color.h"
#include "gui_popup_message.h"

#include "sglib/sglib.h"

#define compare_popup_msg(a, b) (((a)->id) - ((b)->id))

SGLIB_DEFINE_DL_LIST_PROTOTYPES(popup_msg, compare_popup_msg, prev, next);
SGLIB_DEFINE_DL_LIST_FUNCTIONS(popup_msg, compare_popup_msg, prev, next);

Rectangle popup_message_rect;

popup_msg *current_message = NULL;
popup_msg *message_queue = NULL;
int message_queue_last_id = 0;

static void popup_msg_set_state(popup_msg *msg, popup_msg_state_t state)
{
    assert_not_null(msg);

    msg->state = state;

    switch (state) {
    case POPUP_MSG_STATE_START:
        msg->state_duration = 0.0f;
        msg->fade = 0.0f;
        break;

    case POPUP_MSG_STATE_FADEIN:
        msg->state_duration = 0.4f;
        msg->fade = 0.0f;
        break;

    case POPUP_MSG_STATE_VISIBLE:
        msg->state_duration = 3.333f;
        msg->fade = 1.0f;
        break;

    case POPUP_MSG_STATE_FADEOUT:
        msg->state_duration = 0.4f;
        msg->fade = 1.0f;
        break;
    }

    msg->enter_state_time = current_time;
    msg->exit_state_time = msg->enter_state_time + msg->state_duration;
    msg->state_progress = 0.0f;
}

popup_msg *create_popup_msg(char *message)
{
    assert_not_null(message);

    popup_msg *msg = calloc(1, sizeof(popup_msg));

    assert(message_queue_last_id < (INT_MAX / 2));
    message_queue_last_id += 1;
    msg->id = message_queue_last_id;

    msg->message = message;
    msg->prev    = NULL;
    msg->next    = NULL;

    popup_msg_set_state(msg, POPUP_MSG_STATE_START);

    popup_msg_state_t state;
    float state_duration;
    float enter_state_time;
    float exit_state_time;
    float state_progress;


    msg->message_size = measure_panel_text(msg->message);

    popup_msg *last = sglib_popup_msg_get_last(message_queue);
    sglib_popup_msg_add_after(&last, msg);

    message_queue = msg;

    return msg;
}

static void destroy_popup_msg(struct popup_msg *msg)
{
    if (msg) {
        sglib_popup_msg_delete(&message_queue, msg);

        SAFEFREE(msg->message);

        FREE(msg);
    }
}

static void popup_msg_next_state(popup_msg *msg)
{
    assert_not_null(msg);

    switch (msg->state) {
    case POPUP_MSG_STATE_START:
        popup_msg_set_state(msg, POPUP_MSG_STATE_FADEIN);
        break;

    case POPUP_MSG_STATE_FADEIN:
        popup_msg_set_state(msg, POPUP_MSG_STATE_VISIBLE);
        break;

    case POPUP_MSG_STATE_VISIBLE:
        popup_msg_set_state(msg, POPUP_MSG_STATE_FADEOUT);
        break;

    case POPUP_MSG_STATE_FADEOUT:
        current_message = NULL;
        destroy_popup_msg(msg);
        break;
    }
}

static void popup_msg_update_state(popup_msg *msg)
{
    assert_not_null(msg);

    if (current_time < msg->exit_state_time) {
        float delta_time = current_time - msg->enter_state_time;
        msg->state_progress = delta_time / msg->state_duration;

        switch (msg->state) {
        case POPUP_MSG_STATE_START:
            msg->fade = 0.0f;
            break;

        case POPUP_MSG_STATE_FADEIN:
//            msg->fade = ease_exponential_out(msg->state_progress);
            msg->fade = ease_quint_out(msg->state_progress);
            break;

        case POPUP_MSG_STATE_VISIBLE:
            msg->fade = 1.0f;
            break;

        case POPUP_MSG_STATE_FADEOUT:
//            msg->fade = ease_exponential_out(1.0 - msg->state_progress);
            msg->fade = ease_quint_out(1.0 - msg->state_progress);
            break;
        }
    } else {
        popup_msg_next_state(msg);
    }
}

void init_gui_popup_message(void)
{
    current_message = NULL;
    message_queue = NULL;
    message_queue_last_id = 0;
}

void cleanup_gui_popup_message(void)
{
    while (message_queue) {
        destroy_popup_msg(message_queue);
    }
}

void resize_gui_popup_message(void)
{
    popup_message_rect.x      = (float)GetScreenWidth()/2 - 125;
    popup_message_rect.y      = (float)GetScreenHeight()/2 - 50;
    popup_message_rect.width  = 250;
    popup_message_rect.height = 100;
}

static inline void draw_current_popup_message(void)
{
    assert_not_null(current_message);

    popup_msg_update_state(current_message);

    if  (!current_message) {
        return;
    }

    Rectangle bounds = popup_message_rect;
    float border_thickness = 2.0;
    float margin = WINDOW_MARGIN + border_thickness;
    float mode_yoffset = 0.0f;

    switch (game_mode) {
    case GAME_MODE_TITLE:
        // should be -right_side_button.double_line_y_offset
        mode_yoffset = -((2 * ICON_BUTTON_SIZE) + WINDOW_MARGIN);
        break;

    default:
        // do nothing
        break;
    }

    bounds.width =
        current_message->message_size.x
        + (2 * PANEL_INNER_MARGIN);

    bounds.height =
        current_message->message_size.y
        + (2 * PANEL_INNER_MARGIN);

    bounds.x = window_sizef.x - margin - bounds.width;
    bounds.y = window_sizef.y - margin - bounds.height + mode_yoffset;

    float xslide_length = window_sizef.x - bounds.width;
    bounds.x += (1.0 - current_message->fade) * xslide_length;

    Vector2 text_pos = getVector2FromRectangle(bounds);
    text_pos.x += PANEL_INNER_MARGIN;
    text_pos.y += PANEL_INNER_MARGIN;

    DrawRectangleRounded(bounds, PANEL_ROUNDNES, 0, Fade(BLACK, current_message->fade));
    DrawRectangleRoundedLines(bounds, PANEL_ROUNDNES, 0, border_thickness, Fade(panel_edge_color, current_message->fade));

    draw_panel_text(current_message->message, text_pos, Fade(WHITE, current_message->fade));
}

static inline void show_next_popup_message(void)
{
    assert_null(current_message);

    current_message = sglib_popup_msg_get_first(message_queue);
    sglib_popup_msg_delete(&message_queue, current_message);
}

void draw_gui_popup_message(void)
{
    if (current_message) {
        draw_current_popup_message();
    } else {
        if (message_queue) {
            show_next_popup_message();
        }
    }
}

int popup_message(const char *fmt, ...)
{
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vpopup_message(fmt, ap);
    va_end(ap);

    return rv;
}

int vpopup_message(const char *fmt, va_list ap)
{
    char *message = NULL;
    
    int rv = safe_vasprintf(&message, fmt, ap);

    create_popup_msg(message);

    return rv;
}

int popup_error_message(const char *fmt, ...)
{
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vpopup_error_message(fmt, ap);
    va_end(ap);

    return rv;
}

int vpopup_error_message(const char *fmt, va_list ap)
{
    char *message = NULL;
    char *error_message = NULL;

    int rv = safe_vasprintf(&message, fmt, ap);

    errmsg("%s", message);
    safe_asprintf(&error_message, "ERROR: %s", message);
    FREE(message);

    create_popup_msg(error_message);

    return rv;
}

int popup_bug_message(const char *fmt, ...)
{
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vpopup_bug_message(fmt, ap);
    va_end(ap);

    return rv;
}

int vpopup_bug_message(const char *fmt, va_list ap)
{
    char *message = NULL;
    char *bug_message = NULL;

    int rv = safe_vasprintf(&message, fmt, ap);

    errmsg("POSSIBLE BUG: %s", message);
    safe_asprintf(&bug_message, "POSSIBLE BUG: %s", message);
    FREE(message);

    create_popup_msg(bug_message);

    return rv;
}
