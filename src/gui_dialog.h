/****************************************************************************
 *                                                                          *
 * gui_dialog.h                                                             *
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

#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#define GUI_DIALOG_STRING_MAX_LENGTH 2048

struct gui_dialog;
struct color_option;

enum gui_dialog_type {
    GUI_DIALOG_NULL = 0,
    GUI_DIALOG_YN,
    GUI_DIALOG_STRING,
    GUI_DIALOG_OPEN_FILE,
    GUI_DIALOG_COLOR
};
typedef enum gui_dialog_type gui_dialog_type_t;

enum after_dialog_action {
    AFTER_DIALOG_NULL = 0,
    AFTER_DIALOG_QUIT,
    AFTER_DIALOG_RETURN,
    AFTER_DIALOG_SAVE,
    AFTER_DIALOG_SAVE_AND_QUIT,
    AFTER_DIALOG_SAVE_AND_RETURN
};
typedef enum after_dialog_action after_dialog_action_t;

typedef void (*gui_dialog_finished_cb_t)(struct gui_dialog *dialog, void *data);

struct gui_dialog {
    gui_dialog_type_t type;
    gui_dialog_finished_cb_t callback;
    void *callback_data;

    const char *title;
    const char *question;
    char *default_input;

    bool textbox_edit_mode;
    struct color_option *color_opt;

    after_dialog_action_t action_after_ok;
    after_dialog_action_t action_after_cancel;

    /* results */
    bool status;
    char string[GUI_DIALOG_STRING_MAX_LENGTH];
    Color color;
};
typedef struct gui_dialog gui_dialog_t;

void init_gui_dialog(void);
void cleanup_gui_dialog(void);
void resize_gui_dialog(void);
void draw_gui_dialog(void);
void draw_gui_modal_popup_background(void);

void gui_dialog_show(gui_dialog_t *dialog);

extern gui_dialog_t *current_dialog;

static inline bool gui_dialog_active(void)
{
    return !!current_dialog;
}

void gui_dialog_clesr(gui_dialog_t *dialog, gui_dialog_type_t type);
void gui_dialog_pick_color(struct color_option *c_opt, gui_dialog_finished_cb_t callback, void *data);
void gui_dialog_ask_for_string(const char *title, const char *question, const char *default_string, gui_dialog_finished_cb_t callback, void *data);
void gui_dialog_ask_yn(const char *title, const char *question, gui_dialog_finished_cb_t callback, void *data);

#endif /*GUI_DIALOG_H*/

