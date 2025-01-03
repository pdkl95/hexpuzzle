/****************************************************************************
 *                                                                          *
 * gui_dialog.c                                                             *
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
#include "color.h"
#include "gui_dialog.h"

gui_dialog_t *current_dialog;

Vector2 winsize = {0};

Rectangle dialog_panel_rect;
Rectangle dialog_question_label_rect;
Rectangle dialog_ok_button_rect;
Rectangle dialog_cancel_button_rect;
Rectangle dialog_controls_rect;

#define MAX_DIALOG_BUTTON_TEXT_LENGTH 48
char dialog_ok_button_text[MAX_DIALOG_BUTTON_TEXT_LENGTH];
char dialog_cancel_button_text[MAX_DIALOG_BUTTON_TEXT_LENGTH];

#define MINIMUM_BUTTON_WIDTH 100.0f

extern gui_dialog_t dialog;

static const char *default_dialog_title(gui_dialog_t *dialog)
{
    switch (dialog->type) {
    case GUI_DIALOG_NULL:
        assert(false && "Should not be trying to display type GUI_DIALOG_NULL");
        return "(NULL)";

    case GUI_DIALOG_YN:
        return "Y/N?";

    case GUI_DIALOG_STRING:
        return "Text Input";

    case GUI_DIALOG_OPEN_FILE:
        return "Open File";

    case GUI_DIALOG_COLOR:
        return "Pick Color";

    default:
        assert(false && "bad dialog type!");
        return "(bad dialog type)";
    }
}

#define set_btn_text(text, icon, target) do {             \
        const char *str = GuiIconText(ICON_##icon, text); \
        snprintf(dialog_##target##_button_text,           \
                 MAX_DIALOG_BUTTON_TEXT_LENGTH,           \
                 "%s",                                    \
                 str);                                    \
    } while(0)

static void set_button_text(gui_dialog_t *dialog)
{
    switch (dialog->type) {
    case GUI_DIALOG_NULL:
        assert(false && "Should not be trying to display type GUI_DIALOG_NULL");

    case GUI_DIALOG_YN:
        set_btn_text("Yes", OK_TICK, ok);
        set_btn_text("No", CROSS, cancel);
        break;

    default:
        set_btn_text("Ok", OK_TICK, ok);
        set_btn_text("Cancel", CROSS, cancel);
        break;
    }

}

#undef set_btn_text

void init_gui_dialog(void)
{
    current_dialog = NULL;
}

void cleanup_gui_dialog(void)
{
}

void prepare_gui_dialog(void)
{
    assert_not_null(current_dialog);

    if (!current_dialog->title) {
        current_dialog->title = default_dialog_title(current_dialog);
    }

    winsize = ivector2_to_vector2(window_size);

    set_button_text(current_dialog);

    float total_height = 0.0f;

    Vector2 ok_text_size = measure_gui_text(dialog_ok_button_text);
    Vector2 cancel_text_size = measure_gui_text(dialog_cancel_button_text);
    float button_width = MAX(ok_text_size.x, cancel_text_size.x);
    button_width = MAX(button_width, MINIMUM_BUTTON_WIDTH);

    float total_button_width = (2 * button_width) + PANEL_INNER_MARGIN;
    total_height += TOOL_BUTTON_HEIGHT + PANEL_INNER_MARGIN;

    float total_label_width = 0.0f;
    Vector2 label_text_size;
    if (current_dialog->question) {
        label_text_size = measure_gui_text(current_dialog->question);
        total_label_width = label_text_size.x;
        total_height += label_text_size.y + PANEL_INNER_MARGIN;
    }

    float total_gui_controls_width = 0.0f;
    switch (current_dialog->type) {
    case GUI_DIALOG_STRING:
        total_gui_controls_width = 400.0f;
        total_height +=  TOOL_BUTTON_HEIGHT;
        break;

    case GUI_DIALOG_OPEN_FILE:
        total_gui_controls_width = 0.0f;
        break;

    case GUI_DIALOG_COLOR:
        total_gui_controls_width = 192.0f;
        total_height += total_gui_controls_width;
        break;

    default:
        /* do nothing */
        break;
    }

    float total_width = MAX(MAX(total_button_width,
                                total_label_width),
                            total_gui_controls_width);

    Rectangle area = {
        .x      = window_center.x - (total_width / 2),
        .y      = window_center.y - (total_height / 2),
        .width  = total_width,
        .height = total_height
    };
    float yoffset = 0.0f;

    dialog_panel_rect.x      = area.x - PANEL_INNER_MARGIN;
    dialog_panel_rect.y      = area.y - PANEL_INNER_MARGIN - TOOL_BUTTON_HEIGHT;;
    dialog_panel_rect.width  = area.width  + (2 * PANEL_INNER_MARGIN);
    dialog_panel_rect.height = area.height + (2 * PANEL_INNER_MARGIN) + TOOL_BUTTON_HEIGHT;

    if (current_dialog->question) {
        dialog_question_label_rect.x = area.x;
        dialog_question_label_rect.y = area.y;
        dialog_question_label_rect.width  = total_label_width;
        dialog_question_label_rect.height = label_text_size.y;

        yoffset = dialog_question_label_rect.height + PANEL_INNER_MARGIN;
        area.y      += yoffset;
        area.height -= yoffset;
    }

    switch (current_dialog->type) {
    case GUI_DIALOG_STRING:
        dialog_controls_rect.x      = area.x;
        dialog_controls_rect.y      = area.y;
        dialog_controls_rect.width  = total_gui_controls_width;
        dialog_controls_rect.height = TOOL_BUTTON_HEIGHT;

        memset(current_dialog->string, 0, GUI_DIALOG_STRING_MAX_LENGTH);
        if (current_dialog->default_input) {
            snprintf(current_dialog->string,
                     GUI_DIALOG_STRING_MAX_LENGTH,
                     "%s",
                     current_dialog->default_input);
        }
        current_dialog->textbox_edit_mode = false;
        break;

    case GUI_DIALOG_OPEN_FILE:
        break;

    case GUI_DIALOG_COLOR:
        dialog_controls_rect.x      = area.x;
        dialog_controls_rect.y      = area.y;
        dialog_controls_rect.width  = total_gui_controls_width;
        dialog_controls_rect.height = dialog_controls_rect.width;
        break;

    default:
        /* do nothing */
        dialog_controls_rect.x      = area.x;
        dialog_controls_rect.y      = area.y;
        dialog_controls_rect.width  = 0.0f;
        dialog_controls_rect.height = 0.0f;
        break;
    }

    yoffset = dialog_controls_rect.height + PANEL_INNER_MARGIN;
    area.y      += yoffset;
    area.height -= yoffset;

    dialog_ok_button_rect.width      = button_width;
    dialog_cancel_button_rect.width  = button_width;
    dialog_ok_button_rect.height     = TOOL_BUTTON_HEIGHT;
    dialog_cancel_button_rect.height = TOOL_BUTTON_HEIGHT;
    dialog_ok_button_rect.y          = area.y;
    dialog_cancel_button_rect.y      = area.y;
    dialog_ok_button_rect.x          = area.x + area.width - button_width;;
    dialog_cancel_button_rect.x      = dialog_ok_button_rect.x - PANEL_INNER_MARGIN - button_width;
}

void resize_gui_dialog(void)
{
    if (current_dialog) {
        prepare_gui_dialog();
    }
}

void gui_dialog_finish(void)
{
    if (current_dialog->callback) {
        current_dialog->callback(current_dialog);
    }

    current_dialog = NULL;
}

void gui_dialog_ok(void)
{
    current_dialog->status = true;
    gui_dialog_finish();
}

void gui_dialog_cancel(void)
{
    current_dialog->status = false;
    gui_dialog_finish();
}

void draw_gui_dialog(void)
{
    if (!current_dialog) {
        return;
    }

    GuiUnlock();

    switch (modal_ui_result) {
    case UI_RESULT_PENDING:
        /* do nothing */
        break;

    case UI_RESULT_NULL:
        return;

    case UI_RESULT_CANCEL:
        gui_dialog_cancel();
        return;

    case UI_RESULT_OK:
        gui_dialog_ok();
        return;
    }

    DrawRectangleV(VEC2_ZERO, winsize, modal_dialog_shading_color);

    GuiPanel(dialog_panel_rect, current_dialog->title);
    GuiLabel(dialog_question_label_rect, current_dialog->question);

    switch (current_dialog->type) {
    case GUI_DIALOG_STRING:
        if (GuiTextBox(dialog_controls_rect,
                       &(current_dialog->string[0]),
                       GUI_DIALOG_STRING_MAX_LENGTH,
                       current_dialog->textbox_edit_mode)) {
            current_dialog->textbox_edit_mode = !current_dialog->textbox_edit_mode;
        }
        break;

    case GUI_DIALOG_OPEN_FILE:
        break;

    case GUI_DIALOG_COLOR:
        GuiColorPicker(dialog_controls_rect, NULL, &current_dialog->color);
        break;

    default:
        /* do nothing */
        break;
    }

    if (GuiButton(dialog_ok_button_rect, dialog_ok_button_text)) {
        gui_dialog_ok();
    }

    if (GuiButton(dialog_cancel_button_rect, dialog_cancel_button_text)) {
        gui_dialog_cancel();
    }
}

void gui_dialog_show(gui_dialog_t *dialog)
{
    if (current_dialog) {
        return;
    }

    assert_not_null(dialog);

    current_dialog = dialog;
    prepare_gui_dialog();
}

void gui_dialog_clesr(gui_dialog_t *dialog, gui_dialog_type_t type)
{
    memset(dialog, 0, sizeof(gui_dialog_t));
    dialog->type = type;
    dialog->title = default_dialog_title(dialog);
}

void gui_dialog_pick_color(struct color_option *c_opt, gui_dialog_finished_cb_t callback)
{
    if (current_dialog) {
        return;
    }

    gui_dialog_clesr(&dialog, GUI_DIALOG_COLOR);
    dialog.question = NULL;
    dialog.color = c_opt->color;
    dialog.color_opt = c_opt;
    dialog.callback = callback;
    gui_dialog_show(&dialog);
}
