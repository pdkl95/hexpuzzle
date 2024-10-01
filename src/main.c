/******************************************************************************
 *                                                                            *
 *  main.c                                                                    *
 *                                                                            *
 *  This file is part of hexpuzzle.                                           *
 *                                                                            *
 *  hexpuzzle is free software: you can redistribute it and/or                *
 *  modify it under the terms of the GNU General Public License as published  *
 *  by the Free Software Foundation, either version 3 of the License,         *
 *  or (at your option) any later version.                                    *
 *                                                                            *
 *  hexpuzzle is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General  *
 *  Public License for more details.                                          *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License along   *
 *  with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.              *
 *                                                                            *
 ******************************************************************************/

#include "common.h"

#include <getopt.h>
#include <libgen.h>
#include <time.h>
#include <sys/stat.h>

#include "raygui/style/dark_alt.h"
#include "raygui/gui_window_file_dialog.h"

#include "options.h"
#include "color.h"

#include "tile_draw.h"

#include "level.h"
#include "collection.h"
#include "shader.h"

#if defined(PLATFORM_DESKTOP)
/* good */
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
# error "Only PLATFORM_DESKTOP (GLSL 330) supported!"
#endif

const char *progversion = PACKAGE_VERSION;
const char *progname    = PACKAGE_NAME;

#define CONFIG_SUBDIR_NAME PACKAGE_NAME
char *config_dir;

bool running = true;
int automatic_event_polling_semaphore = 0;
options_t *options = NULL;
bool event_waiting_active = false;
bool window_size_changed = false;
bool first_resize = true;
bool skip_next_resize_event = false;
double resize_delay = 0.5;
double resize_time = 0.0;
IVector2 window_size;
IVector2 mouse_position;
Vector2 mouse_positionf;
bool mouse_left_click  = false;
bool mouse_left_release  = false;
bool mouse_right_click = false;
float cursor_spin = 0.0f;
float cursor_spin_step = (360.0f / 100.0f);
int frame_count = 0;
int frame_delay;
bool show_fps = false;
float current_time = 0.0f;
double double_current_time = 0.0;

float popup_text_fade_time = 2.5f;
float popup_text_active_until = 0.0f;
const char *popup_text;
Color popup_text_color = { 0xFF, 0xFA, 0xCD, 0xFF };

game_mode_t game_mode = GAME_MODE_NULL;

bool level_finished = false;

level_t *current_level = NULL;
collection_t *current_collection = NULL;

bool modal_ui_active = false;
ui_result_t modal_ui_result;
bool show_name_edit_box = false;
bool show_ask_save_box = false;
bool show_open_file_box = false;

GuiWindowFileDialogState open_file_box_state;

#define MOUSE_TEXT_MAX_LINES 8
#define MOUSE_TEXT_MAX_LINE_LENGTH 60
char mouse_text[MOUSE_TEXT_MAX_LINES][MOUSE_TEXT_MAX_LINE_LENGTH];
int mouse_text_idx = 0;
int mouse_text_max_line_length = 0;
int mouse_text_font_size = 20;

void gui_setup(void);

static inline bool do_level_ui_interaction(void)
{
    return current_level && !modal_ui_active;
};

void enable_automatic_events(void)
{
    if (options->wait_events) {
        if (0 == automatic_event_polling_semaphore) {
#ifdef DEBUG_EVENT_POLLING
            if (options->verbose) {
                infomsg("Enabling automatic event polling.");
            }
#endif
            DisableEventWaiting();
            event_waiting_active = false;;
            //PollInputEvents();
        }
        automatic_event_polling_semaphore++;
#ifdef DEBUG_EVENT_POLLING
        printf("semaphore++ = %d\n", automatic_event_polling_semaphore);
#endif
    }
}

void disable_automatic_events(void)
{
    if (options->wait_events) {
        automatic_event_polling_semaphore--;

        assert(automatic_event_polling_semaphore >= 0);

        if (0 == automatic_event_polling_semaphore) {
#ifdef DEBUG_EVENT_POLLING
            if (options->verbose) {
                infomsg("Disabling automatic event polling.");
            }
#endif
            EnableEventWaiting();
            event_waiting_active = true;
        }
#ifdef DEBUG_EVENT_POLLING
        printf("semaphore-- = %d\n", automatic_event_polling_semaphore);
#endif
    }
}

void show_name_edit_dialog(void)
{
    if (current_level) {
        memcpy(current_level->name_backup,
               current_level->name,
               NAME_MAXLEN);
        show_name_edit_box = true;
    }
}

char *game_mode_str(void)
{
    switch (game_mode) {
    case GAME_MODE_NULL:
        return "NULL";

    case GAME_MODE_PLAY_COLLECTION:
        return "PLAY_COLLECTION";

    case GAME_MODE_EDIT_COLLECTION:
        return "EDIT_COLLECTION";

    case GAME_MODE_PLAY_LEVEL:
        return "PLAY_LEVEL";

    case GAME_MODE_EDIT_LEVEL:
        return "EDIT_LEVEL";

    default:
        __builtin_unreachable();
    }
}

void toggle_edit_mode(void)
{
    switch (game_mode) {
    case GAME_MODE_NULL:
        warnmsg("Trying to toggle edit mode in NULL game mode?!");
        break;

    case GAME_MODE_PLAY_COLLECTION:
        game_mode = GAME_MODE_EDIT_COLLECTION;
        break;

    case GAME_MODE_EDIT_COLLECTION:
        game_mode = GAME_MODE_PLAY_COLLECTION;
        break;

    case GAME_MODE_PLAY_LEVEL:
        game_mode = GAME_MODE_EDIT_LEVEL;
        break;

    case GAME_MODE_EDIT_LEVEL:
        game_mode = GAME_MODE_PLAY_LEVEL;
        break;

    default:
        /* do nothing */
        __builtin_unreachable();
    }
}

static void return_from_level_callback(UNUSED level_t *level, UNUSED void *data)
{
    if (current_level) {
        level_unload();
    }
}

void return_from_level(void)
{
    if (current_level) {
        level_fade_out(current_level, return_from_level_callback, NULL);
    }
}

void create_new_level(void)
{
    level_t *level = create_level(current_collection);
    collection_add_level(current_collection, level);
    level_edit(level);
}

void open_game_file(char *path)
{
    if (options->verbose) {
        infomsg("Loading: \"%s\"", path);
    }
    collection_t *collection = load_collection_path(path);
    if (!collection) {
        errmsg("Cannot open \"%s\"", path);
        return;
    }
    if (current_collection) {
        destroy_collection(current_collection);
    }
    current_collection = collection;

    if (IS_LEVEL_FILENAME(path)) {
        level_t *level = collection_find_level_by_filename(current_collection, path);
        if (level) {
            level_play(level);
        }
    } else if (IS_COLLECTION_FILENAME(path)) {
        game_mode = GAME_MODE_PLAY_COLLECTION;
    } else if (current_collection->dirpath) {
        game_mode = GAME_MODE_PLAY_COLLECTION;
    } else {
        assert(0);
    }
}

#define print_popup(...) {                                          \
        popup_text = TextFormat(__VA_ARGS__);                       \
        popup_text_active_until = GetTime() + popup_text_fade_time; \
    } while(0)

#define popup_infomsg(...) {                    \
        infomsg(__VA_ARGS__);                   \
        print_popup(__VA_ARGS__);               \
    } while(0)

UNUSED static void add_mouse_text(const char *line)
{
    if (mouse_text_idx < MOUSE_TEXT_MAX_LINES) {
        strncpy(mouse_text[mouse_text_idx], line, MOUSE_TEXT_MAX_LINE_LENGTH - 1);
        mouse_text[mouse_text_idx][MOUSE_TEXT_MAX_LINE_LENGTH - 1] = '\0';

        int width = MeasureText(mouse_text[mouse_text_idx], mouse_text_font_size);
        mouse_text_max_line_length = MAX(width, mouse_text_max_line_length);

        mouse_text_idx++;
    }
}

static void
schedule_resize(
    void
) {
    window_size_changed = true;
    double curtime = GetTime();
    resize_time = curtime + resize_delay;

#if 0
    infomsg("scheduled resize at %f (now = %f, delta = %f)",
            resize_time, curtime, resize_time - curtime);
#endif
}

UNUSED static void
schedule_resize_now(
    void
) {
    window_size_changed = true;
    resize_time = 0;
}

static bool
resize_pending() {
    return window_size_changed;
}

static void
unload_textures(
    void
) {

}

static void
create_textures(
    void
) {

    unload_textures();
    
}

static void
set_uniform_resolution(
    void
) {
    float resolution[2] = { (float)window_size.x, (float)window_size.y };
    SetShaderValue(win_border_shader, win_border_shader_loc.resolution, resolution, SHADER_UNIFORM_VEC2);
}

static void
do_resize(
    void
) {
    window_size.x = GetScreenWidth();
    window_size.y = GetScreenHeight();

#ifdef DEBUG_RESIZE
    warnmsg("RESIZE to: %d x %d", window_size.x, window_size.y);
#endif

    set_uniform_resolution();
    create_textures();
    if (current_level) {
        level_resize(current_level);
    }
    gui_setup();

    window_size_changed = false;
    resize_time = 0;

    if (first_resize) {
        first_resize = false;
#ifdef DEBUG_RESIZE
        infomsg("resize: bypass");
#endif
    } else {
#ifdef DEBUG_RESIZE
        infomsg("resize: request redraw");
#endif
    }
}

static void
resize(
    void
) {
    if (!resize_pending()) {
        return;
    }

    double curtime = GetTime();
    if (curtime < resize_time) {
        return;
    }

#ifdef DEBUG_RESIZE
    warnmsg("RESIZE event (%f < %f)", curtime, resize_time);
#endif

    do_resize();
}

static bool
handle_events(
    void
) {
    if (WindowShouldClose()) {
        //infomsg("Window Closed");
        running = false;
        return true;
    }

    if (IsWindowResized()) {
        if (skip_next_resize_event) {
            skip_next_resize_event = false;
        } else {
            schedule_resize();
        }
    }

    if (modal_ui_active) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            modal_ui_result = UI_RESULT_CANCEL;
        } else if (IsKeyPressed(KEY_ENTER)) {
            modal_ui_result = UI_RESULT_OK;
        } else {
            modal_ui_result = UI_RESULT_PENDING;;
        }

        return true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        infomsg("etc - quit");
        running = false;
        return true;
    }

    if (IsKeyPressed(KEY_F1)) {
        printf("game_mode = %s\n", game_mode_str());
        printf("collection->gui_list_scroll_index = %d\n",
               current_collection->gui_list_scroll_index);
        printf("collection->gui_list_active       = %d\n",
               current_collection->gui_list_active);
        printf("collection->gui_list_focus        = %d\n",
               current_collection->gui_list_focus);
    }

    if (IsKeyPressed(KEY_F8)) {
        if (current_level) {
            level_serialize(current_level, stdout);
        }
    }

    //IVector2 old_mouse = mouse_position;
    mouse_position.x = GetMouseX();
    mouse_position.y = GetMouseY();

    mouse_positionf.x = (float)mouse_position.x;
    mouse_positionf.y = (float)mouse_position.y;

    if (do_level_ui_interaction()) {
        level_set_hover(current_level, mouse_position);
    }

    mouse_text_idx = 0;
    mouse_text_max_line_length = 0;

    mouse_left_click   = false;;
    mouse_left_release = false;
    mouse_right_click  = false;;

    if (IsCursorOnScreen()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            mouse_left_click = true;
            if (do_level_ui_interaction()) {
                if (edit_mode_solved) {
                    level_modify_hovered_feature(current_level);
                //} else if (edit_mode_unsolved) {
                //    level_drag_start(current_level);
                } else {
                    level_drag_start(current_level);
                }
            }
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            mouse_left_release = false;
            if (do_level_ui_interaction()) {
                level_drag_stop(current_level);
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            mouse_right_click = true;
            if (do_level_ui_interaction()) {
                if (edit_mode_solved) {
                    if (is_any_shift_down()) {
                        level_clear_hovered_tile(current_level);
                    } else {
                        level_modify_hovered_feature(current_level);
                    }
                }
            }
        }
    }

    if (current_level) {
        if (level_check(current_level)) {
            level_finished = true;
            level_win(current_level);
        } else {
            level_finished = false;
            level_unwin(current_level);
        }
    } else {
        level_finished = false;
    }

    return true;
}

static void draw_mouse_text(void)
{
    if (mouse_text_idx) {
        int vmargin = 4;
        int hmargin = 8;
        int font_size = mouse_text_font_size;
        int line_height = font_size + 2;
        int x = mouse_position.x + 30;
        int y = mouse_position.y - ((mouse_text_idx / 2) * font_size) - 15;

        Rectangle rec = {
            .x      = x - hmargin,
            .y      = y - vmargin,
            .width  = mouse_text_max_line_length + (2 * hmargin),
            .height = (mouse_text_idx * line_height) + (2 * vmargin)
        };

        DrawRectangleRounded(rec, 0.2, 0, ColorAlpha(DARKPURPLE, 0.333));
        DrawRectangleRoundedLines(rec, 0.2, 0, 1.0, ColorAlpha(LIGHTGRAY, 0.666));

        for (int i=0; i<mouse_text_idx; i++) {
            DrawTextShadow(mouse_text[i], x, y, font_size, WHITE);
            y += line_height;
        }
    }
}

Rectangle name_text_rect;
Rectangle name_panel_rect;
Rectangle name_edit_button_rect;
Rectangle edit_panel_rect;
Rectangle radius_spinner_label_rect;
Rectangle radius_spinner_rect;
Rectangle close_button_rect;
Rectangle edit_button_rect;
Rectangle edit_mode_toggle_rect;
Rectangle return_button_rect;
Rectangle open_file_button_rect;

char close_button_text_str[] = "Quit";
#define CLOSE_BUTTON_TEXT_LENGTH (6 + sizeof(close_button_text_str))
char close_button_text[CLOSE_BUTTON_TEXT_LENGTH];

char edit_button_text_str[] = "Edit";
#define EDIT_BUTTON_TEXT_LENGTH (6 + sizeof(edit_button_text_str))
char edit_button_text[EDIT_BUTTON_TEXT_LENGTH];

char return_button_text_str[] = "Back";
#define RETURN_BUTTON_TEXT_LENGTH (6 + sizeof(return_button_text_str))
char return_button_text[RETURN_BUTTON_TEXT_LENGTH];

char open_file_button_text_str[] = "Open File";
#define OPEN_FILE_BUTTON_TEXT_LENGTH (6 + sizeof(open_file_button_text_str))
char open_file_button_text[OPEN_FILE_BUTTON_TEXT_LENGTH];

char cancel_ok_with_icons[25];
char no_yes_with_icons[25];

int edit_mode_toggle_active;

void gui_setup(void)
{
    cancel_ok_with_icons[0] = '\0';
    strcat(cancel_ok_with_icons, GuiIconText(ICON_CROSS,"Cancel"));
    strcat(cancel_ok_with_icons, ";");
    strcat(cancel_ok_with_icons, GuiIconText(ICON_OK_TICK,"Ok"));

    no_yes_with_icons[0] = '\0';
    strcat(no_yes_with_icons, GuiIconText(ICON_CROSS,"No"));
    strcat(no_yes_with_icons, ";");
    strcat(no_yes_with_icons, GuiIconText(ICON_OK_TICK,"Yes"));

    name_text_rect.x = WINDOW_MARGIN + PANEL_INNER_MARGIN;
    name_text_rect.y = WINDOW_MARGIN + PANEL_INNER_MARGIN;
    name_text_rect.height = NAME_FONT_SIZE;

    name_panel_rect.x = WINDOW_MARGIN;
    name_panel_rect.y = WINDOW_MARGIN;
    name_panel_rect.height = NAME_FONT_SIZE + (2 * PANEL_INNER_MARGIN);

    name_edit_button_rect.x = name_text_rect.x + name_text_rect.width + ICON_BUTTON_SIZE;
    name_edit_button_rect.y = name_text_rect.y;

    name_edit_button_rect.width  = ICON_BUTTON_SIZE;
    name_edit_button_rect.height = ICON_BUTTON_SIZE;

    edit_panel_rect.x = name_panel_rect.x;
    edit_panel_rect.y =
        name_panel_rect.y
        + name_panel_rect.height
        + WINDOW_MARGIN;

    radius_spinner_label_rect.x = name_text_rect.x;
    radius_spinner_label_rect.y =
        edit_panel_rect.y
        + PANEL_INNER_MARGIN;
    radius_spinner_label_rect.height = PANEL_LABEL_FONT_SIZE;

    radius_spinner_rect.x = name_text_rect.x;
    radius_spinner_rect.y =
        radius_spinner_label_rect.y
        + radius_spinner_label_rect.height
        + PANEL_INNER_MARGIN;
    radius_spinner_rect.width  = 180;
    radius_spinner_rect.height = 30;

    edit_mode_toggle_rect.x = radius_spinner_rect.x;
    edit_mode_toggle_rect.y =
        radius_spinner_rect.y
        + radius_spinner_rect.height
        + PANEL_INNER_MARGIN;
    edit_mode_toggle_rect.width  = radius_spinner_rect.width;
    edit_mode_toggle_rect.height = radius_spinner_rect.height;

    edit_panel_rect.width =
        radius_spinner_rect.width
        + (2 * PANEL_INNER_MARGIN);
    edit_panel_rect.height =
        radius_spinner_label_rect.height
        + radius_spinner_rect.height
        + edit_mode_toggle_rect.height
        + (4 * PANEL_INNER_MARGIN);

    int close_button_text_width  = MeasureText(close_button_text_str,  ICON_FONT_SIZE);
    int edit_button_text_width   = MeasureText(edit_button_text_str,   ICON_FONT_SIZE);
    int return_button_text_width = MeasureText(return_button_text_str, ICON_FONT_SIZE);
    close_button_text_width = edit_button_text_width = return_button_text_width =
        MAX(MAX(close_button_text_width, edit_button_text_width),
            return_button_text_width);

    close_button_rect.x      = window_size.x - WINDOW_MARGIN - ICON_BUTTON_SIZE - close_button_text_width;
    close_button_rect.y      = WINDOW_MARGIN;
    close_button_rect.width  = ICON_BUTTON_SIZE + close_button_text_width;
    close_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(close_button_text, GuiIconText(ICON_EXIT, close_button_text_str), CLOSE_BUTTON_TEXT_LENGTH);

    edit_button_rect.x      = close_button_rect.x;
    edit_button_rect.y      = close_button_rect.y + close_button_rect.height + WINDOW_MARGIN;
    edit_button_rect.width  = ICON_BUTTON_SIZE + edit_button_text_width;
    edit_button_rect.height = ICON_BUTTON_SIZE;
    
    memcpy(edit_button_text,  GuiIconText(ICON_TOOLS, edit_button_text_str), EDIT_BUTTON_TEXT_LENGTH);

    return_button_rect.x      = edit_button_rect.x;
    return_button_rect.y      = edit_button_rect.y + edit_button_rect.height + WINDOW_MARGIN;
    return_button_rect.width  = ICON_BUTTON_SIZE + return_button_text_width;
    return_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(return_button_text,  GuiIconText(ICON_UNDO_FILL, return_button_text_str), RETURN_BUTTON_TEXT_LENGTH);

    int open_file_button_text_width = MeasureText(open_file_button_text_str, ICON_FONT_SIZE);

    open_file_button_rect.x      = window_size.x - WINDOW_MARGIN - ICON_BUTTON_SIZE - open_file_button_text_width;
    open_file_button_rect.y      = window_size.y - WINDOW_MARGIN - ICON_BUTTON_SIZE;
    open_file_button_rect.width  = ICON_BUTTON_SIZE + open_file_button_text_width;
    open_file_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(open_file_button_text,  GuiIconText(ICON_FILE_OPEN, open_file_button_text_str), OPEN_FILE_BUTTON_TEXT_LENGTH);

}

Color panel_bg_color   = { 0x72, 0x1C, 0xB8, 0xaa };
Color panel_edge_color = { 0x94, 0x83, 0xA2, 0xcc };
Color panel_header_text_color = { 0xD0, 0xC0, 0xFF, 0xff };

static void draw_name_header(char *name)
{
    int textwidth = MeasureText(name, NAME_FONT_SIZE);

    name_text_rect.width  = textwidth;
    name_panel_rect.width = textwidth + (2 * PANEL_INNER_MARGIN);

    bool hover = CheckCollisionPointRec(mouse_positionf, name_text_rect);
    if (game_mode != GAME_MODE_EDIT_LEVEL) {
        hover = false;
    }

    if (hover && mouse_left_click) {
        show_name_edit_dialog();
    }

    Color bg   = hover ? panel_edge_color : panel_bg_color;
    Color edge = hover ? panel_bg_color : panel_edge_color;
    DrawRectangleRounded(name_panel_rect, PANEL_ROUNDNES, 0, bg);
    DrawRectangleRoundedLines(name_panel_rect, PANEL_ROUNDNES, 0, 1.0, edge);

    DrawText(name, name_text_rect.x, name_text_rect.y, NAME_FONT_SIZE, panel_header_text_color);
}

static void draw_edit_panel(void)
{
    Color bg   = panel_bg_color;
    Color edge = panel_edge_color;
    DrawRectangleRounded(edit_panel_rect, PANEL_ROUNDNES, 0, bg);
    DrawRectangleRoundedLines(edit_panel_rect, PANEL_ROUNDNES, 0, 1.0, edge);

    DrawText("Board Radius", radius_spinner_label_rect.x, radius_spinner_label_rect.y,
             PANEL_LABEL_FONT_SIZE, panel_header_text_color);

    int radius = 1;
    if (current_level) {
        radius = current_level->radius;
    }

    GuiSpinner(radius_spinner_rect, NULL, &radius, LEVEL_MIN_RADIUS, LEVEL_MAX_RADIUS, false);
    if (current_level->radius != radius) {
        level_set_radius(current_level, radius);
    }

    int edit_mode_toggle_active;
    switch (current_level->currently_used_tiles) {
    case USED_TILES_NULL:
        printf("return (NULL)\n");
        return;

    case USED_TILES_SOLVED:
        edit_mode_toggle_active = 0;
        break;

    case USED_TILES_UNSOLVED:
        edit_mode_toggle_active = 1;
        break;
    }

    GuiToggleSlider(edit_mode_toggle_rect, "#112#Solved;#62#Scrambled", &edit_mode_toggle_active);

    if ((edit_mode_toggle_active == 0 && current_level->currently_used_tiles == USED_TILES_UNSOLVED) ||
        (edit_mode_toggle_active == 1 && current_level->currently_used_tiles == USED_TILES_SOLVED)
    ) {
        level_toggle_currently_used_tiles(current_level);
    }
}

static void draw_gui_widgets(void)
{
    if (GuiButton(close_button_rect, close_button_text)) {
        running = false;
    }

    switch (game_mode) {
    case GAME_MODE_EDIT_LEVEL:
        draw_name_header(current_level->name);

        draw_edit_panel();

        if (GuiButton(return_button_rect, return_button_text)) {
            printf("return\n");
            show_ask_save_box = true;
        }
        break;

    case GAME_MODE_PLAY_LEVEL:
        draw_name_header(current_level->name);

        // NOTE: not return_button_rect; when the edit button
        //       isn't shown, move up into edit button's place.
        if (GuiButton(edit_button_rect, return_button_text)) {
            return_from_level();
        }
        break;

    case GAME_MODE_EDIT_COLLECTION:
        /* fall through */

    case GAME_MODE_PLAY_COLLECTION:
        if (GuiButton(edit_button_rect, edit_button_text)) {
            toggle_edit_mode();
        }

        if (GuiButton(open_file_button_rect, open_file_button_text)) {
            show_open_file_box = true;
        }

        break;

    default:
        break;
    }
}

static void draw_name_edit_dialog(void)
{
    if (show_name_edit_box) {
        Rectangle edit_box_rect = {
            (float)GetScreenWidth()/2 - 120,
            (float)GetScreenHeight()/2 - 60,
            240,
            140
        };

        GuiUnlock();
        const char *icon = GuiIconText(ICON_PENCIL, "Edit Level Name");
        int result = GuiTextInputBox(edit_box_rect,
                                     icon,
                                     "Level Name:",
                                     cancel_ok_with_icons,
                                     current_level->name,
                                     NAME_MAXLEN,
                                     NULL);
        GuiLock();

        if ((result == 2) || (modal_ui_result == UI_RESULT_OK)) {
           /* accept edit / ok */
            show_name_edit_box = false;
            modal_ui_result = UI_RESULT_NULL;
        }

        if ((result == 1) || (modal_ui_result == UI_RESULT_CANCEL)) {
            /* rollback edit / cancel */
            if (current_level) {
                memcpy(current_level->name, current_level->name_backup, NAME_MAXLEN);
            }
            show_name_edit_box = false;
            modal_ui_result = UI_RESULT_NULL;
        }
    }
}

static void draw_ask_save_dialog(void)
{
    Rectangle edit_box_rect = {
        (float)GetScreenWidth()/2 - 120,
        (float)GetScreenHeight()/2 - 60,
        240,
        140
    };

    GuiUnlock();
    const char *iconmsg = GuiIconText(ICON_FILE_SAVE_CLASSIC, "Save Level?");
    int result = GuiMessageBox(edit_box_rect,
                               iconmsg,
                               "Save changes to level?",
                               no_yes_with_icons);
    GuiLock();

    if ((result == 2) || (modal_ui_result == UI_RESULT_OK)) {
        /* yes */
        printf("collection_save()\n");
        collection_save(current_collection);
        show_ask_save_box = false;
        printf("return_from_level()\n");
        return_from_level();
        modal_ui_result = UI_RESULT_NULL;
    }

    if ((result == 1) || (modal_ui_result == UI_RESULT_CANCEL)) {
        /* no */
        show_ask_save_box = false;
        return_from_level();
        modal_ui_result = UI_RESULT_NULL;
    }
}

static void draw_open_file_dialog(void)
{
    open_file_box_state.windowActive = true;

    GuiUnlock();
    GuiWindowFileDialog(&open_file_box_state);
    GuiLock();

    if (open_file_box_state.SelectFilePressed) {
        const char *path = concat_dir_and_filename(open_file_box_state.dirPathText,
                                                       open_file_box_state.fileNameText);
        printf("open file \"%s\"\n", path);
        show_open_file_box = false;
    } else if (open_file_box_state.CancelFilePressed) {
        show_open_file_box = false;
    } else if (!open_file_box_state.windowActive) {
        show_open_file_box = false;
    }
}

static void draw_popup_panels(void)
{
    if (modal_ui_result == UI_RESULT_CANCEL) {
        show_name_edit_box = false;
        show_ask_save_box = false;
        show_open_file_box = false;

        modal_ui_result = UI_RESULT_NULL;
    }

    if (show_ask_save_box) {
        draw_name_edit_dialog();
    }

    if (show_ask_save_box) {
        draw_ask_save_dialog();
    }

    if (show_open_file_box) {
        draw_open_file_dialog();
    }
}

static void draw_popup_text(void)
{
    float curtime = GetTime();
    if (curtime < popup_text_active_until) {
        float remaining = popup_text_active_until - curtime;
        float fade = ease_quintic_in(remaining / popup_text_fade_time);
        int corner_padding = 40;
        int fade_font_size = 24;
        int text_x = corner_padding;
        int text_y = window_size.y - corner_padding - fade_font_size;
        Color fade_text_color = ColorAlpha(popup_text_color, fade);
        DrawText(popup_text, text_x, text_y, fade_font_size, fade_text_color);
    }
}

static void draw_cartesian_grid(bool draw_labels)
{
    rlPushMatrix();

   Vector2 hwin = {
        .x = window_size.x / 2.0,
        .y = window_size.y / 2.0
    };

    rlTranslatef(hwin.x,
                 hwin.y,
                 0.0);

    float rot_x = 2.0 * sinf(current_time / 10.0);
    //float rot_x = rot * (360.0 / TAU) * level->fade_rotate_speed;
    rlRotatef(rot_x, 0.0, 0.0, 1.0);

    rlTranslatef(-hwin.x,
                 -hwin.y,
                 0.0);

    Color minor_color = ColorAlpha(purple, 0.5);

    int minor_size = 25;
    int minor_per_major = 4;
    int major_size = minor_size * minor_per_major;
    float wrap_size = (float)major_size;
    float half_wrap = wrap_size / 2.0;

    static float speed = 1.5;;
    static float oldspeed = 1.5;;
    static float newspeed = 1.5;;
    static Vector2 off = { 1.0, 0.0 };
    static Vector2 dir = { 1.0, 0.0 };
    static Vector2 olddir = {0};
    static Vector2 newdir = {0};
    static int dir_lerp_frames = 0;
#define TOTAL_LERP_FRAMES options->max_fps

    if (!options->wait_events) {
        if ((frame_count % (3 * options->max_fps)) == 0) {
            oldspeed = speed;
            newspeed = 1.0 + (2.0 * drand48());
            float angle = (0.4 * TAU) * drand48() - (0.2 * TAU);
            olddir = dir;
            newdir = Vector2Normalize(Vector2Rotate(dir, angle));
            dir_lerp_frames = TOTAL_LERP_FRAMES;
        }
        if (dir_lerp_frames > 0) {
            dir_lerp_frames--;
            float t = 1.0 - (((float)dir_lerp_frames) / ((float)TOTAL_LERP_FRAMES));
            speed = Lerp(oldspeed, newspeed, t);
            dir = Vector2Lerp(olddir, newdir, t);
        }

        off = Vector2Add(off, Vector2Scale(dir, speed));

        if      (off.x < -half_wrap) { off.x += wrap_size; }
        else if (off.x >  half_wrap) { off.x -= wrap_size; }
        if      (off.y < -half_wrap) { off.y += wrap_size; }
        else if (off.y >  half_wrap) { off.y -= wrap_size; }
    }

    for (int x=-major_size; x<window_size.x + major_size; x += minor_size) {
        DrawLine(x+off.x, 0, x+off.x, window_size.y, minor_color);
    }

    for (int y=-major_size; y<window_size.y + major_size; y += minor_size) {
        DrawLine(0, y+off.y, window_size.x, y+off.y, minor_color);
    }

    for (int x=-major_size; x<window_size.x + major_size; x += major_size) {
        DrawLine(x+off.x, 0, x+off.x, window_size.y, royal_blue);
        if (draw_labels) {
            DrawText(TextFormat("%d", x), (float)x + 3.0, 8.0, 16, YELLOW);
        }
    }

    for (int y=-major_size; y<window_size.y + major_size; y += major_size) {
        DrawLine(0, y+off.y, window_size.x, y+off.y, magenta);
        if (draw_labels) {
            DrawText(TextFormat("%d", y), 3.0, (float)y + 3.9, 16, YELLOW);
        }
    }

    rlPopMatrix();
}

static bool
render_frame(
    void
) {
    BeginDrawing();
    {
        ClearBackground(BLACK);
        draw_cartesian_grid(false);

        switch (game_mode) {
        case GAME_MODE_PLAY_LEVEL:
            if (current_level) {
                level_draw(current_level, level_finished);
            }
            break;

        case GAME_MODE_EDIT_LEVEL:
            if (current_level) {
                level_draw(current_level, false);
            }
            break;

        case GAME_MODE_PLAY_COLLECTION:
            /* fall through */
        case GAME_MODE_EDIT_COLLECTION:
            if (current_collection) {
                collection_draw(current_collection);
            }
            break;

        default:
            /* do nothing */
            break;
        }

        draw_gui_widgets();
        draw_popup_panels();
        draw_popup_text();

        if (show_fps) {
            DrawTextShadow(TextFormat("FPS: %d", GetFPS()), 15, 10, 20, WHITE);
        }

        draw_mouse_text();
    }
    EndDrawing();

    return true;
}

static void early_frame_setup(void)
{
    double_current_time = GetTime();
    current_time = (float)double_current_time;

    if (level_finished) {
        SetShaderValue(win_border_shader, win_border_shader_loc.time, &current_time, SHADER_UNIFORM_FLOAT);
    }

    if (show_name_edit_box ||
        show_ask_save_box ||
        show_open_file_box
    ) {
        modal_ui_active = true;
        GuiLock();
    } else {
        modal_ui_active = false;
        GuiUnlock();
    }
}

static bool
main_event_loop(
    void
) {
    //infomsg("Entering main event loop...");

    while (running) {
        early_frame_setup();

        if (window_size_changed) {
            resize();
        }

        if (!handle_events()) {
            return false;
        }

        if (!render_frame()) {
            return false;
        }

        frame_count += 1;
    };

    return true;
}

void gfx_init(void)
{
    if (options->verbose) {
        SetTraceLogLevel(LOG_INFO);
    } else {
        SetTraceLogLevel(LOG_WARNING);
    }

    unsigned int flags = 0;
    flags |= FLAG_VSYNC_HINT;
    flags |= FLAG_WINDOW_RESIZABLE;
    flags |= FLAG_MSAA_4X_HINT;
    SetConfigFlags(flags);

    InitWindow(window_size.x, window_size.y, "Hex Puzzle");
    SetWindowMinSize(OPTIONS_WINDOW_MIN_WIDTH,
                     OPTIONS_WINDOW_MIN_HEIGHT);
    SetWindowMaxSize(OPTIONS_WINDOW_MAX_WIDTH,
                     OPTIONS_WINDOW_MAX_HEIGHT);

    SetExitKey(KEY_NULL); // handle ESC ourself
    SetTargetFPS(options->max_fps);
    if (options->verbose) {
        infomsg("Target FPS: %d", options->max_fps);
    }

    //SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);

    GuiLoadStyleDark();
    //GuiLoadStyleBiLightingFont();

    set_uniform_resolution();

    if (options->wait_events) {
        if (options->verbose) {
            infomsg("Disabling automatic event polling.");
        }
        EnableEventWaiting();
        event_waiting_active = true;
    }

    prepare_global_colors();

    open_file_box_state = InitGuiWindowFileDialog(GetWorkingDirectory());

    load_shaders();
}

static void
gfx_cleanup(
    void
) {
    unload_shaders();
    unload_textures();
    CloseWindow();
}

static void game_init(void)
{
    if (-1 == mkdir(config_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
        if (errno == EEXIST) {
            if (options->verbose) {
                infomsg("Found config dir \"%s\"", config_dir);
            }
        } else {
            errmsg("Could not created config dir \"%s\": %s", config_dir, strerror(errno));
        }
    } else {
        if (options->verbose) {
            infomsg("Created config dir \"%s\"", config_dir);
        }
    }

    if (options->extra_argc == 1) {
        char *filename = options->extra_argv[0];
        open_game_file(filename);
    }
}

static void game_cleanup(void)
{
}

int
main(
    int   argc,
    char *argv[]
) {
    progname = basename(argv[0]);

    srand48((long int)time(NULL));
    srand(time(NULL));

    char *xdg_config_dir = getenv("XDG_CONFIG_HOME");
    char *home_dir       = getenv("HOME");
    if (xdg_config_dir) {
        asprintf(&config_dir, "%s/%s", xdg_config_dir, CONFIG_SUBDIR_NAME);
    } else {
        asprintf(&config_dir, "%s/.config/%s", home_dir, CONFIG_SUBDIR_NAME);
    }

    options = create_options();

    /* configure options */
    if (!options_parse_args(options, argc, argv)) {
        fprintf(stderr, "ERROR: bad args");
        return EXIT_FAILURE;
    }

    window_size.x = options->initial_window_width;
    window_size.y = options->initial_window_height;

    frame_delay = (1000 / options->max_fps);

    gfx_init();
    game_init();
    do_resize();

    if (options->verbose) {
        infomsg("Entering Main Loop...");
    }

    bool run_ok = main_event_loop();

    if (options->verbose) {
        infomsg("Main Loop exit was %s", run_ok ? "SUCCESS" : "FAILURE");
    }

    game_cleanup();
    gfx_cleanup();

    destroy_options(options);

    if (run_ok) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

