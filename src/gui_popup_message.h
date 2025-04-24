/****************************************************************************
 *                                                                          *
 * gui_popup_message.h                                                      *
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

#ifndef GUI_POPUP_MESSAGE_H
#define GUI_POPUP_MESSAGE_H

typedef struct popup_msg {
    int id;
    char *title;
    char *message;

    Vector2 message_size;

    struct popup_msg *prev;
    struct popup_msg *next;
} popup_msg;

void init_gui_popup_message(void);
void cleanup_gui_popup_message(void);
void resize_gui_popup_message(void);
void draw_gui_popup_message(void);

int popup_message(const char *title, const char *fmt, ...);
int vpopup_message(const char *title, const char *fmt, va_list ap);

extern popup_msg *current_message;

static inline bool gui_popup_message_active(void)
{
    return !!current_message;
}

#endif /*GUI_POPUP_MESSAGE_H*/

