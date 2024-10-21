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

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "common.h"

#include <getopt.h>
#include <libgen.h>
#include <time.h>
#include <sys/stat.h>

#include "cJSON/cJSON.h"

#include "raygui/style/dark_alt.h"
#include "tinyfiledialogs/tinyfiledialogs.h"

#include "options.h"
#include "color.h"

#include "tile_draw.h"

#include "level.h"
#include "collection.h"
#include "shader.h"

#include "gui_browser.h"
#include "gui_options.h"
#include "gui_random.h"

#include "nvdata.h"
#include "nvdata_finished.h"

/* #if defined(PLATFORM_DESKTOP) */
/* /\* good *\/ */
/* #else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB */
/* # error "Only PLATFORM_DESKTOP (GLSL 330) supported!" */
/* #endif */

const char *progversion = PACKAGE_VERSION;
const char *progname    = PACKAGE_NAME;

#define DEBUG_RESIZE 1

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

bool level_finished = false;

level_t *current_level = NULL;
collection_t *current_collection = NULL;

bool modal_ui_active = false;
ui_result_t modal_ui_result;
bool show_name_edit_box = false;
bool show_ask_save_box = false;
bool show_open_file_box = false;

bool edit_tool_cycle = true;
bool edit_tool_erase = false;
path_type_t edit_tool_state = PATH_TYPE_NONE;

char const * file_filter_patterns[2] = {
    "*." COLLECTION_FILENAME_EXT,
    "*." LEVEL_FILENAME_EXT
};

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

#if defined(PLATFORM_DESKTOP)
void show_name_edit_dialog(void)
{
    if (current_level) {
        memcpy(current_level->name_backup,
               current_level->name,
               NAME_MAXLEN);
        show_name_edit_box = true;
    }
}
#endif

static void set_edit_tool(path_type_t type)
{
    edit_tool_state = type;
    if (options->verbose) {
        infomsg("Using Edit Tool: %s", path_type_name(edit_tool_state));
    }
}

static void return_from_level_callback(UNUSED level_t *level, UNUSED void *data)
{
    if (current_level) {
        level_unload();
    }
    prev_game_mode();
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

#if defined(PLATFORM_DESKTOP)
void open_game_file(const char *path)
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
        set_game_mode(GAME_MODE_PLAY_COLLECTION);
    } else if (current_collection->dirpath) {
        set_game_mode(GAME_MODE_PLAY_COLLECTION);
    } else {
        assert(0);
    }
}

const char *default_open_file_path(void)
{
    static char path[MAX_FILEPATH_LENGTH];
    const char *wd = GetWorkingDirectory();
    strcpy(path, wd);
    int len = strlen(path);
    path[len]     = '/';
    path[len + 1] = '\0';
    return path;
}

static void reset_window_to_center(void)
{
    SetWindowSize(OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH,
                  OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT);

    int m = GetCurrentMonitor();
    int x = (GetMonitorWidth(m)  / 2) - (OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH  / 2);
    int y = (GetMonitorHeight(m) / 2) - (OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT / 2);

    SetWindowPosition(x, y);
}
#endif

#define print_popup(...) {                                          \
        popup_text = TextFormat(__VA_ARGS__);                       \
        popup_text_active_until = GetTime() + popup_text_fade_time; \
    } while(0)

#define popup_infomsg(...) {                    \
        infomsg(__VA_ARGS__);                   \
        print_popup(__VA_ARGS__);               \
    } while(0)

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
    resize_gui_browser();
    resize_gui_options();
    resize_gui_random();

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
#if defined(PLATFORM_DESKTOP)
    if (WindowShouldClose()) {
        //infomsg("Window Closed");
        running = false;
        return true;
    }
#endif

    if (IsWindowResized()) {
        if (skip_next_resize_event) {
            skip_next_resize_event = false;
        } else {
            schedule_resize();
        }
    }

#if defined(PLATFORM_DESKTOP)
    if (IsKeyPressed(KEY_R) && is_any_shift_down()) {
        reset_window_to_center();
    }

    if (IsKeyPressed(KEY_F11)) {
        //ToggleFullscreen();
        ToggleBorderlessWindowed();
    }
#endif

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

#if defined(PLATFORM_DESKTOP)
    if (IsKeyPressed(KEY_ESCAPE)) {
        infomsg("etc - quit");
        running = false;
        return true;
    }

    if (IsKeyPressed(KEY_F1)) {
        if (current_level) {
            cJSON *json = level_to_json(current_level);
            char *json_str = cJSON_PrintUnformatted(json);
            printf("JSON>>>\n%s\n<<<JSON\n", json_str);
            free(json_str);
            cJSON_Delete(json);
        }
    }
#endif

    //IVector2 old_mouse = mouse_position;
    mouse_position.x = GetMouseX();
    mouse_position.y = GetMouseY();

    mouse_positionf.x = (float)mouse_position.x;
    mouse_positionf.y = (float)mouse_position.y;

    if (do_level_ui_interaction()) {
        level_set_hover(current_level, mouse_position);
    }

    mouse_left_click   = false;;
    mouse_left_release = false;
    mouse_right_click  = false;;

    if (IsCursorOnScreen()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            mouse_left_click = true;
            if (do_level_ui_interaction()) {
                if (edit_mode_solved) {
                    if (edit_tool_cycle) {
                        level_modify_hovered_feature(current_level);
                    } else if (edit_tool_erase) {
                        level_set_hovered_feature(current_level, PATH_TYPE_NONE);
                    } else {
                        level_set_hovered_feature(current_level, edit_tool_state);
                    }

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
            if (!level_finished) {
                level_finished = true;
                level_win(current_level);
                nvdata_mark_finished(current_level);
            }
        } else {
            if (level_finished) {
                level_finished = false;
                level_unwin(current_level);
                nvdata_unmark_finished(current_level);
            }
        }
    } else {
        level_finished = false;
    }

    return true;
}

Rectangle name_text_rect;
Rectangle name_panel_rect;
Rectangle name_edit_button_rect;
Rectangle edit_panel_rect;
Rectangle radius_spinner_label_rect;
Rectangle radius_spinner_rect;
Rectangle close_button_rect;
/* Rectangle save_button_rect; */
/* Rectangle edit_button_rect; */
/* Rectangle return_button_rect; */
Rectangle edit_mode_toggle_rect;
Rectangle open_file_button_rect;
Rectangle tool_panel_rect;
Rectangle edit_tool_label_rect;
Rectangle cycle_tool_button_rect;
Rectangle erase_tool_button_rect;
Rectangle goto_next_level_panel_rect;
Rectangle goto_next_level_label_rect;
Rectangle goto_next_level_button_rect;

#define MAX_RIGHT_SIDE_BUTTONS 5
Rectangle right_side_button_rect[MAX_RIGHT_SIDE_BUTTONS];

char close_button_text_str[] = "Quit";
#define CLOSE_BUTTON_TEXT_LENGTH (6 + sizeof(close_button_text_str))
char close_button_text[CLOSE_BUTTON_TEXT_LENGTH];

char goto_next_level_button_text_str[] = "Next";
#define GOTO_NEXT_LEVEL_BUTTON_TEXT_LENGTH (6 + sizeof(goto_next_level_button_text_str))
char goto_next_level_button_text[GOTO_NEXT_LEVEL_BUTTON_TEXT_LENGTH];

char options_button_text_str[] = "Options";
#define OPTIONS_BUTTON_TEXT_LENGTH (6 + sizeof(options_button_text_str))
char options_button_text[OPTIONS_BUTTON_TEXT_LENGTH];

char browser_button_text_str[] = "Browse";
#define BROWSER_BUTTON_TEXT_LENGTH (6 + sizeof(browser_button_text_str))
char browser_button_text[BROWSER_BUTTON_TEXT_LENGTH];

char random_button_text_str[] = "Random";
#define RANDOM_BUTTON_TEXT_LENGTH (6 + sizeof(random_button_text_str))
char random_button_text[RANDOM_BUTTON_TEXT_LENGTH];

char save_button_text_str[] = "Save";
#define SAVE_BUTTON_TEXT_LENGTH (6 + sizeof(save_button_text_str))
char save_button_text[SAVE_BUTTON_TEXT_LENGTH];

char edit_button_text_str[] = "Edit";
#define EDIT_BUTTON_TEXT_LENGTH (6 + sizeof(edit_button_text_str))
char edit_button_text[EDIT_BUTTON_TEXT_LENGTH];

char return_button_text_str[] = "Back";
#define RETURN_BUTTON_TEXT_LENGTH (6 + sizeof(return_button_text_str))
char return_button_text[RETURN_BUTTON_TEXT_LENGTH];

char open_file_button_text_str[] = "Open File";
#define OPEN_FILE_BUTTON_TEXT_LENGTH (6 + sizeof(open_file_button_text_str))
char open_file_button_text[OPEN_FILE_BUTTON_TEXT_LENGTH];

char cycle_tool_button_text_str[] = "Cycle";
#define CYCLE_TOOL_BUTTON_TEXT_LENGTH (6 + sizeof(cycle_tool_button_text_str))
char cycle_tool_button_text[CYCLE_TOOL_BUTTON_TEXT_LENGTH];

char erase_tool_button_text_str[] = "Erase";
#define ERASE_TOOL_BUTTON_TEXT_LENGTH (6 + sizeof(erase_tool_button_text_str))
char erase_tool_button_text[ERASE_TOOL_BUTTON_TEXT_LENGTH];

char edit_tool_label_text[] = "Edit Tool";
char goto_next_level_label_text[] = "Next level?";

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

    memcpy(cycle_tool_button_text,  GuiIconText(ICON_MUTATE_FILL, cycle_tool_button_text_str), CYCLE_TOOL_BUTTON_TEXT_LENGTH);
    memcpy(erase_tool_button_text,  GuiIconText(ICON_RUBBER, erase_tool_button_text_str), ERASE_TOOL_BUTTON_TEXT_LENGTH);

    Vector2 edit_tool_label_text_size = MeasureTextEx(PANEL_LABEL_FONT,
                                                      edit_tool_label_text,
                                                      PANEL_LABEL_FONT_SIZE,
                                                      PANEL_LABEL_FONT_SPACING);

    Vector2 cycle_tool_button_text_size = MeasureGuiText(cycle_tool_button_text);
    Vector2 erase_tool_button_text_size = MeasureGuiText(erase_tool_button_text);

    tool_panel_rect.x = edit_panel_rect.x;
    tool_panel_rect.width = (4 * PANEL_INNER_MARGIN)
        + ((PATH_TYPE_COUNT - 1) * (2 * ICON_BUTTON_SIZE))
        + edit_tool_label_text_size.x
        + cycle_tool_button_text_size.x
        + erase_tool_button_text_size.x;
    tool_panel_rect.height = (2 * PANEL_INNER_MARGIN) + TOOL_BUTTON_HEIGHT;
    tool_panel_rect.y = window_size.y - WINDOW_MARGIN - tool_panel_rect.height;

    Vector2 close_button_text_size   = MeasureGuiText(close_button_text_str);
    Vector2 edit_button_text_size    = MeasureGuiText(edit_button_text_str);
    Vector2 save_button_text_size    = MeasureGuiText(save_button_text_str);
    Vector2 return_button_text_size  = MeasureGuiText(return_button_text_str);
    Vector2 browser_button_text_size = MeasureGuiText(browser_button_text_str);
    Vector2 options_button_text_size = MeasureGuiText(options_button_text_str);

    int close_button_text_width   = close_button_text_size.x;
    int edit_button_text_width    = edit_button_text_size.x;
    int save_button_text_width    = save_button_text_size.x;
    int return_button_text_width  = return_button_text_size.x;
    int browser_button_text_width = browser_button_text_size.x;
    int options_button_text_width = options_button_text_size.x;

    int right_side_button_text_width = close_button_text_width =
        MAX(MAX(MAX(close_button_text_width, edit_button_text_width),
                MAX(save_button_text_width, return_button_text_width)),
            MAX(browser_button_text_width, options_button_text_width));

    close_button_rect.x      = window_size.x - WINDOW_MARGIN - ICON_BUTTON_SIZE - close_button_text_width;
    close_button_rect.y      = WINDOW_MARGIN;
    close_button_rect.width  = ICON_BUTTON_SIZE + close_button_text_width;
    close_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(  close_button_text, GuiIconText(ICON_EXIT,               close_button_text_str),   CLOSE_BUTTON_TEXT_LENGTH);
    memcpy(   edit_button_text, GuiIconText(ICON_FILE_SAVE_CLASSIC,   edit_button_text_str),    EDIT_BUTTON_TEXT_LENGTH);
    memcpy(   save_button_text, GuiIconText(ICON_TOOLS,               save_button_text_str),    SAVE_BUTTON_TEXT_LENGTH);
    memcpy( return_button_text, GuiIconText(ICON_UNDO_FILL,         return_button_text_str),  RETURN_BUTTON_TEXT_LENGTH);
    memcpy(options_button_text, GuiIconText(ICON_GEAR,             options_button_text_str), OPTIONS_BUTTON_TEXT_LENGTH);
    memcpy(browser_button_text, GuiIconText(ICON_FOLDER_FILE_OPEN, browser_button_text_str), BROWSER_BUTTON_TEXT_LENGTH);
    memcpy( random_button_text, GuiIconText(ICON_SHUFFLE_FILL,      random_button_text_str),  RANDOM_BUTTON_TEXT_LENGTH);

    right_side_button_rect[0].x      = close_button_rect.x;
    right_side_button_rect[0].y      = close_button_rect.y + close_button_rect.height + (3 * WINDOW_MARGIN);
    right_side_button_rect[0].width  = ICON_BUTTON_SIZE + right_side_button_text_width;
    right_side_button_rect[0].height = ICON_BUTTON_SIZE;

    for (int i=1; i<MAX_RIGHT_SIDE_BUTTONS; i++) {
        right_side_button_rect[i].x      = right_side_button_rect[i-1].x;
        right_side_button_rect[i].y      = right_side_button_rect[i-1].y + right_side_button_rect[i-1].height + WINDOW_MARGIN;
        right_side_button_rect[i].width  = ICON_BUTTON_SIZE + right_side_button_text_width;
        right_side_button_rect[i].height = ICON_BUTTON_SIZE;
    }

    Vector2 open_file_button_text_size = MeasureGuiText(open_file_button_text_str);
    open_file_button_rect.x      = window_size.x - WINDOW_MARGIN - ICON_BUTTON_SIZE - open_file_button_text_size.x;
    open_file_button_rect.y      = window_size.y - WINDOW_MARGIN - ICON_BUTTON_SIZE;
    open_file_button_rect.width  = ICON_BUTTON_SIZE + open_file_button_text_size.x;
    open_file_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(open_file_button_text,  GuiIconText(ICON_FILE_OPEN, open_file_button_text_str), OPEN_FILE_BUTTON_TEXT_LENGTH);

    edit_tool_label_rect.x =  tool_panel_rect.x + PANEL_INNER_MARGIN;
    edit_tool_label_rect.y =  tool_panel_rect.y + PANEL_INNER_MARGIN;
    edit_tool_label_rect.width = edit_tool_label_text_size.x;
    edit_tool_label_rect.height = TOOL_BUTTON_HEIGHT;

    cycle_tool_button_rect.x = edit_tool_label_rect.x + edit_tool_label_rect.width + ICON_BUTTON_SIZE;
    cycle_tool_button_rect.y = edit_tool_label_rect.y;
    cycle_tool_button_rect.width  = cycle_tool_button_text_size.x;
    cycle_tool_button_rect.height = edit_tool_label_rect.height;

    erase_tool_button_rect.x = cycle_tool_button_rect.x + cycle_tool_button_rect.width + ICON_BUTTON_SIZE;
    erase_tool_button_rect.y = cycle_tool_button_rect.y;
    erase_tool_button_rect.width  = erase_tool_button_text_size.x;
    erase_tool_button_rect.height = cycle_tool_button_rect.height;

    memcpy(goto_next_level_button_text, GuiIconText(ICON_ARROW_RIGHT_FILL, goto_next_level_button_text_str), GOTO_NEXT_LEVEL_BUTTON_TEXT_LENGTH);

    Vector2 goto_next_level_label_text_size = MeasureTextEx(PANEL_LABEL_FONT,
                                                      goto_next_level_label_text,
                                                      PANEL_LABEL_FONT_SIZE,
                                                      PANEL_LABEL_FONT_SPACING);

    Vector2 goto_next_level_button_text_size = MeasureGuiText(goto_next_level_button_text_str);

    goto_next_level_panel_rect.y      = tool_panel_rect.y;

    goto_next_level_button_rect.y      = goto_next_level_panel_rect.y + PANEL_INNER_MARGIN;
    goto_next_level_button_rect.width  = goto_next_level_button_text_size.x;
    goto_next_level_button_rect.height = TOOL_BUTTON_HEIGHT;
    goto_next_level_button_rect.x      = window_size.x - WINDOW_MARGIN - goto_next_level_button_rect.width - PANEL_INNER_MARGIN;

    goto_next_level_label_rect.y      = goto_next_level_button_rect.y;
    goto_next_level_label_rect.width  = goto_next_level_label_text_size.x;
    goto_next_level_label_rect.height = TOOL_BUTTON_HEIGHT;
    goto_next_level_label_rect.x      = goto_next_level_button_rect.x - goto_next_level_label_rect.width - ICON_BUTTON_SIZE;

    goto_next_level_panel_rect.width  = goto_next_level_button_rect.width + goto_next_level_label_rect.width + ICON_BUTTON_SIZE + (2 * PANEL_INNER_MARGIN);
    goto_next_level_panel_rect.height = goto_next_level_button_rect.height + (2 * PANEL_INNER_MARGIN);
    goto_next_level_panel_rect.x      = window_size.x - WINDOW_MARGIN - goto_next_level_panel_rect.width;
}

static void draw_name_header(char *name)
{
    Vector2 textsize = MeasureTextEx(NAME_FONT, name, NAME_FONT_SIZE, NAME_FONT_SPACING);

    name_text_rect.width  = textsize.x;
    name_panel_rect.width = textsize.x + (2 * PANEL_INNER_MARGIN);

#if defined(PLATFORM_DESKTOP)
    bool hover = CheckCollisionPointRec(mouse_positionf, name_text_rect);
    if (game_mode != GAME_MODE_EDIT_LEVEL) {
        hover = false;
    }

    if (hover) {
        if (mouse_left_click) {
            show_name_edit_dialog();
        } else {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
    }
#else
    bool hover = false;
#endif

    Color bg   = hover ? panel_edge_color : panel_bg_color;
    Color edge = hover ? panel_bg_color : panel_edge_color;
    DrawRectangleRounded(name_panel_rect, PANEL_ROUNDNES, 0, bg);
    DrawRectangleRoundedLines(name_panel_rect, PANEL_ROUNDNES, 0, 1.0, edge);

    DrawTextEx(NAME_FONT, name, getVector2FromRectangle(name_text_rect), NAME_FONT_SIZE, NAME_FONT_SPACING, panel_header_text_color);
}

static void draw_edit_panel(void)
{
    Color bg   = panel_bg_color;
    Color edge = panel_edge_color;
    DrawRectangleRounded(edit_panel_rect, PANEL_ROUNDNES, 0, bg);
    DrawRectangleRoundedLines(edit_panel_rect, PANEL_ROUNDNES, 0, 1.0, edge);

    DrawTextEx(PANEL_LABEL_FONT, "Board Radius", getVector2FromRectangle(radius_spinner_label_rect),
               PANEL_LABEL_FONT_SIZE, PANEL_LABEL_FONT_SPACING, panel_header_text_color);

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

static void draw_tool_panel(void)
{
    Color bg   = panel_bg_color;
    Color edge = panel_edge_color;
    DrawRectangleRounded(tool_panel_rect, PANEL_ROUNDNES, 0, bg);
    DrawRectangleRoundedLines(tool_panel_rect, PANEL_ROUNDNES, 0, 1.0, edge);

    DrawTextEx(PANEL_LABEL_FONT, edit_tool_label_text, getVector2FromRectangle(edit_tool_label_rect),
               PANEL_LABEL_FONT_SIZE, PANEL_LABEL_FONT_SPACING, panel_header_text_color);

    GuiToggle(cycle_tool_button_rect, cycle_tool_button_text, &edit_tool_cycle);
    if (edit_tool_cycle) {
        edit_tool_erase = false;
    }
    GuiToggle(erase_tool_button_rect, erase_tool_button_text, &edit_tool_erase);
    if (edit_tool_erase) {
        edit_tool_cycle = false;
    }

    if (edit_tool_cycle || edit_tool_erase) {
        edit_tool_state = PATH_TYPE_NONE;
    }

    Rectangle rect = {
        .x = erase_tool_button_rect.x + erase_tool_button_rect.width + ICON_BUTTON_SIZE,
        .y = erase_tool_button_rect.y,
        .width  = TOOL_BUTTON_WIDTH,
        .height = TOOL_BUTTON_HEIGHT
    };

    float line_thickness = 1.0;
    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        DrawRectangleRounded(rect, BUTTON_ROUNDNES, 0, path_type_color(type));
        if (edit_tool_state == type) {
            DrawRectangleRoundedLines(rect, BUTTON_ROUNDNES, 4, line_thickness, WHITE); 
        }

        bool hover = CheckCollisionPointRec(mouse_positionf, rect);
        if (hover) {
            if (mouse_left_click) {
                edit_tool_cycle = false;
                edit_tool_erase = false;
                set_edit_tool(type);
            } else {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            }
        }

        rect.x += TOOL_BUTTON_WIDTH + ICON_BUTTON_SIZE;
    }
}

static void draw_goto_next_level_panel(void)
{
    assert_not_null(current_collection);
    assert_not_null(current_level);

    level_t *next_level = collection_get_level_after(current_collection, current_level);

    DrawRectangleRounded(goto_next_level_panel_rect, PANEL_ROUNDNES, 0, panel_bg_color);
    DrawRectangleRoundedLines(goto_next_level_panel_rect, PANEL_ROUNDNES, 0, 1.0, panel_edge_color);

    DrawTextEx(PANEL_LABEL_FONT, goto_next_level_label_text, getVector2FromRectangle(goto_next_level_label_rect),
               PANEL_LABEL_FONT_SIZE, PANEL_LABEL_FONT_SPACING, panel_header_text_color);

    if (!next_level) {
        GuiDisable();
    }

    if (GuiButton(goto_next_level_button_rect, goto_next_level_button_text)) {
        level_play(next_level);
    }

    if (!next_level) {
        GuiEnable();
    }
}

static void draw_win_panels(void)
{
    if (current_collection && current_level) {
        draw_goto_next_level_panel();
    }
}

static void draw_gui_widgets(void)
{
    int prev_align = GuiGetStyle(BUTTON, TEXT_ALIGNMENT);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

#if defined(PLATFORM_DESKTOP)
    if (GuiButton(close_button_rect, close_button_text)) {
        running = false;
    }
#endif

    int rsb = 0;

    switch (game_mode) {
    case GAME_MODE_BROWSER:
        if (GuiButton(right_side_button_rect[rsb++], options_button_text)) {
            set_game_mode(GAME_MODE_OPTIONS);
        }

        if (GuiButton(right_side_button_rect[rsb++], random_button_text)) {
            set_game_mode(GAME_MODE_RANDOM);
        }

#if defined(PLATFORM_DESKTOP)
        if (GuiButton(open_file_button_rect, open_file_button_text)) {
            show_open_file_box = true;
        }
#endif
        break;

    case GAME_MODE_OPTIONS:
        if (GuiButton(right_side_button_rect[rsb++], return_button_text)) {
            prev_game_mode();
        }
        break;

    case GAME_MODE_RANDOM:
        if (GuiButton(right_side_button_rect[rsb++], options_button_text)) {
            set_game_mode(GAME_MODE_OPTIONS);
        }

        if (GuiButton(right_side_button_rect[rsb++], return_button_text)) {
            set_game_mode(GAME_MODE_BROWSER);
        }
        break;

    case GAME_MODE_EDIT_LEVEL:
        draw_name_header(current_level->name);

        draw_edit_panel();
        if (edit_mode_solved) {
            draw_tool_panel();
        }

        if (GuiButton(right_side_button_rect[rsb++], return_button_text)) {
            printf("return\n");
            show_ask_save_box = true;
        }
        break;

    case GAME_MODE_PLAY_LEVEL:
        draw_name_header(current_level->name);

        if (GuiButton(right_side_button_rect[rsb++], return_button_text)) {
            return_from_level();
        }
        break;

    case GAME_MODE_WIN_LEVEL:
        draw_name_header(current_level->name);
        draw_win_panels();

        if (GuiButton(right_side_button_rect[rsb++], return_button_text)) {
            return_from_level();
        }
        break;

    case GAME_MODE_EDIT_COLLECTION:
        if (GuiButton(right_side_button_rect[rsb++], options_button_text)) {
            set_game_mode(GAME_MODE_OPTIONS);
        }

        if (GuiButton(right_side_button_rect[rsb++], browser_button_text)) {
            set_game_mode(GAME_MODE_BROWSER);
        }

        if (GuiButton(right_side_button_rect[rsb++], edit_button_text)) {
            toggle_edit_mode();
        }

#if defined(PLATFORM_DESKTOP)
        if (current_collection && current_collection->changed) {
            if (GuiButton(right_side_button_rect[rsb++], save_button_text)) {
                show_ask_save_box = true;
            }
        }
#endif
        break;

    case GAME_MODE_PLAY_COLLECTION:
        if (GuiButton(right_side_button_rect[rsb++], options_button_text)) {
            set_game_mode(GAME_MODE_OPTIONS);
        }

        if (GuiButton(right_side_button_rect[rsb++], browser_button_text)) {
            set_game_mode(GAME_MODE_BROWSER);
        }

        if (GuiButton(right_side_button_rect[rsb++], edit_button_text)) {
            toggle_edit_mode();
        }

        break;

    default:
        break;
    }

    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, prev_align);
}

#if defined(PLATFORM_DESKTOP)
static void draw_name_edit_dialog(void)
{
    char *default_input = "";
    if (current_level) {
        default_input = current_level->name;
    }

    char *result = tinyfd_inputBox("Edit Name",
                                   "Name?",
                                   default_input);
    if (result) {
        /* accept edit / ok */
        memcpy(current_level->name, result, strlen(result) + 1);
        modal_ui_result = UI_RESULT_NULL;
        if (current_collection) {
            collection_update_level_names(current_collection);
        }
    } else {
        /* rollback edit / cancel */
        if (current_level) {
            memcpy(current_level->name, current_level->name_backup, NAME_MAXLEN);
        }
        modal_ui_result = UI_RESULT_NULL;
    }

    show_name_edit_box = false;
}

static void draw_ask_save_dialog(void)
{
    char *title_text, *desc_text;
    switch (game_mode) {
    case GAME_MODE_EDIT_COLLECTION:
        title_text = "Save Collection?";
        desc_text  = "Save changes to level collection?";
        break;

    case GAME_MODE_EDIT_LEVEL:
        title_text = "Save Level?";
        desc_text  = "Save changes to level?";
        break;

    default:
        __builtin_unreachable();
    }

    int result = tinyfd_messageBox(title_text,
                                   desc_text,
                                   "yesnocancel",
                                   "question",
                                   1);

    switch (result) {
    case 0:
        /* fall through */
    default:
        break;

    case 1:
        /* yes */
        switch (game_mode) {
        case GAME_MODE_EDIT_COLLECTION:
            if (current_collection) {
                collection_save(current_collection);
            }
            break;

        case GAME_MODE_EDIT_LEVEL:
            if (current_level) {
                level_save(current_level);
            }
            return_from_level();
            break;

        default:
            __builtin_unreachable();
        }

        modal_ui_result = UI_RESULT_NULL;
        break;

    case 2:
        /* no */
        switch (game_mode) {
        case GAME_MODE_EDIT_COLLECTION:
            break;

        case GAME_MODE_EDIT_LEVEL:
            if (current_collection) {
                current_collection->changed = true;
            }
            return_from_level();
            break;

        default:
            __builtin_unreachable();
        }

        modal_ui_result = UI_RESULT_NULL;
        break;
    }

    show_ask_save_box = false;
}

static void draw_open_file_dialog(void)
{
    char *path = tinyfd_openFileDialog("Open Level Pack",
                                       default_open_file_path(),
                                       2,
                                       file_filter_patterns,
                                       "",
                                       false);

    if (path) {
        if (options->verbose) {
            infomsg("Opening file \"%s\"\n", path);
        }
        open_game_file(path);
    }

    show_open_file_box = false;
}

static void draw_popup_panels(void)
{
    if (modal_ui_result == UI_RESULT_CANCEL) {
        show_name_edit_box = false;
        show_ask_save_box = false;
        show_open_file_box = false;

        modal_ui_result = UI_RESULT_NULL;
    }

    if (show_name_edit_box) {
        draw_name_edit_dialog();
    }

    if (show_ask_save_box) {
        draw_ask_save_dialog();
    }

    if (show_open_file_box) {
        draw_open_file_dialog();
    }
}
#endif

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
    bool animate_bg = !options->wait_events && options->animate_bg;

    rlPushMatrix();

    Vector2 hwin = {
        .x = window_size.x / 2.0,
        .y = window_size.y / 2.0
    };

    if (animate_bg) {
        rlTranslatef(hwin.x,
                     hwin.y,
                     0.0);

        float rot_x = 2.0 * sinf(current_time / 10.0);
        //float rot_x = rot * (360.0 / TAU) * level->fade_rotate_speed;
        rlRotatef(rot_x, 0.0, 0.0, 1.0);

        rlTranslatef(-hwin.x,
                     -hwin.y,
                     0.0);
    }

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


    if (animate_bg) {
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
        case GAME_MODE_BROWSER:
            draw_gui_browser();
            break;

        case GAME_MODE_OPTIONS:
            draw_gui_options();
            break;

        case GAME_MODE_RANDOM:
            draw_gui_random();
            break;

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
#if defined(PLATFORM_DESKTOP)
        draw_popup_panels();
#endif
        draw_popup_text();

#if defined(PLATFORM_DESKTOP)
        if (show_fps) {
            DrawTextShadow(TextFormat("FPS: %d", GetFPS()), 15, 10, 20, WHITE);
        }
#endif
    }
    EndDrawing();

    return true;
}

static void early_frame_setup(void)
{
    double_current_time = GetTime();
    current_time = (float)double_current_time;

    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    if (level_finished) {
        SetShaderValue(win_border_shader, win_border_shader_loc.time, &current_time, SHADER_UNIFORM_FLOAT);
    }

#if defined(PLATFORM_DESKTOP)
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
#endif
}

bool do_one_frame(void)
{
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

    return true;
}

#if defined(PLATFORM_WEB)
void em_do_one_frame(void)
{
    do_one_frame();
}
#else
static bool
main_event_loop(
    void
) {
    //infomsg("Entering main event loop...");

    while (running) {
        if (!do_one_frame()) {
            return false;
        }
    };

    return true;
}
#endif

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
    //flags |= FLAG_MSAA_4X_HINT;
    SetConfigFlags(flags);

    InitWindow(window_size.x, window_size.y, "Hex Puzzle");
#if defined(PLATFORM_DESKTOP)
    SetWindowMinSize(OPTIONS_WINDOW_MIN_WIDTH,
                     OPTIONS_WINDOW_MIN_HEIGHT);
    SetWindowMaxSize(OPTIONS_WINDOW_MAX_WIDTH,
                     OPTIONS_WINDOW_MAX_HEIGHT);
#endif

    SetExitKey(KEY_NULL); // handle ESC ourself
#if defined(PLATFORM_DESKTOP)
    SetTargetFPS(options->max_fps);
    if (options->verbose) {
        infomsg("Target FPS: %d", options->max_fps);
    }
#endif

    //SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);

    load_fonts();

    GuiLoadStyleDark();

    set_default_gui_font();

    GuiSetStyle(DEFAULT, TEXT_PADDING, 4);
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);

    set_uniform_resolution();

    if (options->wait_events) {
        if (options->verbose) {
            infomsg("Disabling automatic event polling.");
        }
        EnableEventWaiting();
        event_waiting_active = true;
    }

    prepare_global_colors();

    load_shaders();
}

static void
gfx_cleanup(
    void
) {
    unload_shaders();
    unload_textures();

    GuiUnloadStyleDark();

    unload_fonts();
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

    init_gui_options();
    init_nvdata();
    init_gui_browser();
    init_gui_random();

    set_game_mode(GAME_MODE_BROWSER);
    //set_game_mode(GAME_MODE_RANDOM);

#if defined(PLATFORM_DESKTOP)
    if (options->extra_argc == 1) {
        char *filename = options->extra_argv[0];
        open_game_file(filename);
    }
#endif

    load_nvdata();
}

static void game_cleanup(void)
{
    save_nvdata();

    cleanup_gui_random();
    cleanup_gui_browser();
    cleanup_nvdata();
    cleanup_gui_options();
}

int
main(
    int   argc,
    char *argv[]
) {
    srand48((long int)time(NULL));
    srand(time(NULL));

#if defined(PLATFORM_DESKTOP)
    progname = basename(argv[0]);

    char *xdg_config_dir = getenv("XDG_CONFIG_HOME");
    char *home_dir       = getenv("HOME");
    if (xdg_config_dir) {
        asprintf(&config_dir, "%s/%s", xdg_config_dir, CONFIG_SUBDIR_NAME);
    } else {
        asprintf(&config_dir, "%s/.config/%s", home_dir, CONFIG_SUBDIR_NAME);
    }
#endif

#if defined(PLATFORM_WEB)
    progname = PACKAGE_NAME;

    EM_ASM({
            const mountdir = '/' + UTF8ToString($0);
            FS.mkdir(mountdir);
            FS.mount(IDBFS,
                     { autoPersist: true },
                     mountdir);

            FS.syncfs(true, function (err) {
                    assert(!err);
                    //ccall('test', 'v');
                });
        }, progname);
#endif

    options = create_options();

    /* configure options */
    if (!options_parse_args(options, argc, argv)) {
        fprintf(stderr, "ERROR: bad args");
        return EXIT_FAILURE;
    }

#if defined(PLATFORM_WEB)
    options->wait_events = true;
#endif

    window_size.x = options->initial_window_width;
    window_size.y = options->initial_window_height;

    frame_delay = (1000 / options->max_fps);

    gfx_init();
    game_init();
    do_resize();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(em_do_one_frame, 0, 1);
    return EXIT_SUCCESS;
#else
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
#endif
}

