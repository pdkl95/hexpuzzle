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
#include "gui_popup_message.h"

#include "sglib/sglib.h"

#define compare_popup_msg(a, b) (((a)->id) - ((b)->id))

SGLIB_DEFINE_DL_LIST_PROTOTYPES(popup_msg, compare_popup_msg, prev, next);
SGLIB_DEFINE_DL_LIST_FUNCTIONS(popup_msg, compare_popup_msg, prev, next);

Rectangle popup_message_rect;

popup_msg *current_message = NULL;
popup_msg *message_queue = NULL;
int message_queue_last_id = 0;

popup_msg *create_popup_msg(char *title, char *message)
{
    assert_not_null(title);
    assert_not_null(message);

    popup_msg *msg = calloc(1, sizeof(popup_msg));

    assert(message_queue_last_id < (INT_MAX / 2));
    message_queue_last_id += 1;
    msg->id = message_queue_last_id;

    msg->title   = title;
    msg->message = message;
    msg->prev    = NULL;
    msg->next    = NULL;

    msg->message_size = measure_gui_text(msg->message);

    popup_msg *last = sglib_popup_msg_get_last(message_queue);
    sglib_popup_msg_add_after(&last, msg);

    message_queue = msg;

    return msg;
}

static void destroy_popup_msg(struct popup_msg *msg)
{
    if (msg) {
        sglib_popup_msg_delete(&message_queue, msg);

        SAFEFREE(msg->title);
        SAFEFREE(msg->message);

        FREE(msg);
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

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(DARKGRAY, 0.4f));

    Rectangle bounds = popup_message_rect;

    bounds.width =
        current_message->message_size.x +
        RAYGUI_MESSAGEBOX_BUTTON_PADDING*2;

    bounds.height =
        current_message->message_size.y +
        RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT +
        3*RAYGUI_MESSAGEBOX_BUTTON_PADDING +
        RAYGUI_MESSAGEBOX_BUTTON_HEIGHT;

    bounds = move_rect_to_screen_center(bounds);

    GuiUnlock();

    int result = GuiMessageBox(
        bounds,
        current_message->title,
        current_message->message,
        "Ok");

    if (result != -1) {
        destroy_popup_msg(current_message);
        current_message = NULL;
    }
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

int popup_message(const char *title, const char *fmt, ...)
{
    va_list ap;
    int rv = 0;

    va_start(ap, fmt);
    rv = vpopup_message(title, fmt, ap);
    va_end(ap);

    return rv;
}

int vpopup_message(const char *title, const char *fmt, va_list ap)
{
    char *message = NULL;
    
    int rv = safe_vasprintf(&message, fmt, ap);

    create_popup_msg(strdup(GuiIconText(ICON_INFO, title)),
                     message);

    return rv;
}


