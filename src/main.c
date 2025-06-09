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
# include <emscripten/emscripten.h>
#endif

#include "common.h"

#include <getopt.h>
#include <libgen.h>
#include <time.h>
#include <sys/stat.h>

#include "cJSON/cJSON.h"
#include "pcg/pcg_basic.h"

#include "raygui/style/dark_alt.h"

#include "options.h"
#include "color.h"
#include "raylib_helper.h"

#include "tile_draw.h"

#include "level.h"
#include "level_draw.h"
#include "level_undo.h"
#include "collection.h"
#include "shader.h"
#include "textures.h"

#include "gui_title.h"
#include "gui_browser.h"
#include "gui_collection.h"
#include "gui_options.h"
#include "gui_random.h"
#include "gui_dialog.h"
#include "gui_popup_message.h"
#include "gui_help.h"
#include "background.h"

#include "nvdata.h"
#include "nvdata_finished.h"

#include "solver.h"

#include "classics.h"

/* #if defined(PLATFORM_DESKTOP) */
/* /\* good *\/ */
/* #else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB */
/* # error "Only PLATFORM_DESKTOP (GLSL 330) supported!" */
/* #endif */

const char *progversion = PACKAGE_VERSION;
const char *progname    = PACKAGE_NAME;

//#define DEBUG_SEMAPHORES
//#define DEBUG_RESIZE

#define CONFIG_SUBDIR_NAME PACKAGE_NAME
char *config_dir;
char *home_dir;

#ifdef DEBUG_ID_AND_DIR
int debug_id = 0;
int debug_dir = 0;
#endif

bool running = true;
int raylib_log_level = LOG_WARNING;
bool demo_mode = false;
int automatic_event_polling_semaphore = 0;
bool mouse_input_accepted = true;
bool event_waiting_active = false;
bool window_size_changed = false;
bool first_resize = true;
bool skip_next_resize_event = false;
double resize_delay = 0.3;
double resize_time = 0.0;
IVector2 window_size;
Vector2 window_sizef;
Vector2 window_center;
float window_corner_dist;
IVector2 mouse_position;
Vector2 mouse_positionf;
bool mouse_left_click  = false;
bool mouse_left_release  = false;
bool mouse_right_click = false;
bool mouse_left_doubleclick = true;
double last_mouse_click_time = -100.0;
int current_mouse_cursor = -1;
float cursor_spin = 0.0f;
float cursor_spin_step = (360.0f / 100.0f);
int frame_count = 0;
int frame_delay;
bool show_fps = false;
float current_time = 0.0f;
double double_current_time = 0.0;
solve_timer_t solve_timer;

RenderTexture2D scene_targets[2];
RenderTexture2D *scene_read_target;
RenderTexture2D *scene_write_target;

bool do_postprocessing = false;
float bloom_amount = 1.0;
float distort_amount = 0.0;
float warp_amount = 0.0;
float postprocessing_effect_amount1[4];
float postprocessing_effect_amount2[4];

float feedback_bg_zoom_ratio = 0.1;
Vector2 feedback_bg_zoom_margin = { .x = 20.0, .y = 20.0 };

pcg32_random_t global_rng;

bool level_finished = false;

background_t *background;

game_mode_t nested_game_mode = GAME_MODE_NULL;
level_t *nested_return_level = NULL;

level_t *current_level = NULL;
level_t *current_level_preview = NULL;
collection_t *current_collection = NULL;

bool modal_ui_active = false;
ui_result_t modal_ui_result;
gui_dialog_t dialog;
bool show_name_edit_box = false;
bool show_ask_save_box = false;
bool show_open_file_box = false;

bool edit_tool_cycle = true;
bool edit_tool_erase = false;
path_type_t edit_tool_state = PATH_TYPE_NONE;

Rectangle tooltip_bounds = {0};
const char *tooltip_text = NULL;

char const * file_filter_patterns[2] = {
    "*." COLLECTION_FILENAME_EXT,
    "*." LEVEL_FILENAME_EXT
};

bool return_from_level(void);
void gui_setup(void);
#if defined(PLATFORM_DESKTOP)
void open_game_file(const char *path, bool edit);
#endif

static void seed_global_rng(void)
{
    pcg32_srandom_r(&global_rng, rand(), rand());
}

int global_rng_get(int bound)
{
    if (bound <= 1) {
        return 0;
    } else {
        return (int)pcg32_boundedrand_r(&global_rng, (uint32_t)bound);
    }
}

bool global_rng_bool(int true_chances, int false_chances)
{
    int total_chances = true_chances + false_chances;
    int random_value = global_rng_get(total_chances);
    return random_value <= true_chances;
}

int global_rng_sign(int pos_chances, int neg_chances)
{
    if (global_rng_bool(pos_chances, neg_chances)) {
        return 1;
    } else {
        return -1;
    }
}

static inline bool do_level_ui_interaction(void)
{
    return current_level; // && !modal_ui_active;
}

void enable_automatic_events(void)
{
    if (options->wait_events) {
        if (0 == automatic_event_polling_semaphore) {
#ifdef DEBUG_SEMAPHORES
            if (options->verbose) {
                infomsg("Enabling automatic event polling.");
            }
#endif
            DisableEventWaiting();
            event_waiting_active = false;;
            //PollInputEvents();
        }
        automatic_event_polling_semaphore++;
#ifdef DEBUG_SEMAPHORES
        printf("automatic_event_polling_semaphore++ = %d\n", automatic_event_polling_semaphore);
#endif
    }
}

void disable_automatic_events(void)
{
    if (options->wait_events) {
        automatic_event_polling_semaphore--;

        assert(automatic_event_polling_semaphore >= 0);

        if (0 == automatic_event_polling_semaphore) {
#ifdef DEBUG_SEMAPHORES
            if (options->verbose) {
                infomsg("Disabling automatic event polling.");
            }
#endif
            EnableEventWaiting();
            event_waiting_active = true;
        }
#ifdef DEBUG_SEMAPHORES
        printf("automatic_event_polling_semaphore-- = %d\n", automatic_event_polling_semaphore);
#endif
    }
}

#if defined(PLATFORM_DESKTOP)
static void name_edit_dialog_finished(gui_dialog_t *dialog, UNUSED void *data)
{
    if (dialog->status) {
        /* accept edit / ok */
        if (current_level) {
            level_set_name(current_level, dialog->string);
        }
        modal_ui_result = UI_RESULT_NULL;
    } else {
        /* rollback edit / cancel */
        if (current_level) {
            level_set_name(current_level, current_level->name_backup);
        }
        modal_ui_result = UI_RESULT_NULL;
    }
}

static void ask_save_dialog_finished_edit_collection(gui_dialog_t *dialog, UNUSED void *data)
{
    after_dialog_action_t action = dialog->status
        ? dialog->action_after_ok
        : dialog->action_after_cancel;

    switch (action) {
    case AFTER_DIALOG_NULL:
        break;

    case AFTER_DIALOG_QUIT:
        running = false;
        break;

    case AFTER_DIALOG_RETURN:
        return_from_level();
        break;

    case AFTER_DIALOG_SAVE:
        if (current_collection) {
            collection_save(current_collection);
        }
        break;

    case AFTER_DIALOG_SAVE_AND_QUIT:
        if (current_collection) {
            collection_save(current_collection);
        }
        running = false;
        break;

    case AFTER_DIALOG_SAVE_AND_RETURN:
        if (current_collection) {
            collection_save(current_collection);
        }
        return_from_level();
        break;

    default:
        __builtin_unreachable();
    }
}

static void ask_save_dialog_finished_edit_level(gui_dialog_t *dialog, UNUSED void *data)
{
    after_dialog_action_t action = dialog->status
        ? dialog->action_after_ok
        : dialog->action_after_cancel;

    switch (action) {
    case AFTER_DIALOG_NULL:
        break;

    case AFTER_DIALOG_QUIT:
        running = false;
        break;

    case AFTER_DIALOG_RETURN:
        return_from_level();
        break;

    case AFTER_DIALOG_SAVE:
        if (current_level) {
            level_save(current_level);
        }
        break;

    case AFTER_DIALOG_SAVE_AND_QUIT:
        if (current_level) {
            level_save(current_level);
        }
        running = false;
        break;

    case AFTER_DIALOG_SAVE_AND_RETURN:
        if (current_level) {
            level_save(current_level);
        }
        return_from_level();
        break;

    default:
        __builtin_unreachable();
    }
}

static void ask_save_dialog_finished(gui_dialog_t *dialog, void *data)
{
    switch (game_mode) {
    case GAME_MODE_EDIT_COLLECTION:
        ask_save_dialog_finished_edit_collection(dialog, data);
        break;

    case GAME_MODE_EDIT_LEVEL:
        ask_save_dialog_finished_edit_level(dialog, data);
        break;

    default:
        __builtin_unreachable();
    }

    modal_ui_result = UI_RESULT_NULL;
}

static void open_file_dialog_finished(gui_dialog_t *dialog, UNUSED void *data)
{
    if (dialog->status) {
        if (options->verbose) {
            infomsg("Opening file \"%s\"\n", dialog->string);
        }
        open_game_file(dialog->string, false);
    }
}

void show_ask_save_dialog(after_dialog_action_t action_after_ok, after_dialog_action_t action_after_cancel)
{
    if (current_dialog) {
        return;
    }

    gui_dialog_clesr(&dialog,  GUI_DIALOG_YN);
    dialog.callback = ask_save_dialog_finished;
    dialog.action_after_ok     = action_after_ok;
    dialog.action_after_cancel = action_after_cancel;

    switch (game_mode) {
    case GAME_MODE_EDIT_COLLECTION:
        dialog.title    = "Save Collection?";
        dialog.question = "Save changes to level collection?";
        break;

    case GAME_MODE_EDIT_LEVEL:
        dialog.title    = "Save Level?";
        dialog.question = "Save changes to level?";
        break;

    default:
        __builtin_unreachable();
    }

    gui_dialog_show(&dialog);
}

void open_file_box(void)
{
    if (!current_dialog) {
        gui_dialog_clesr(&dialog, GUI_DIALOG_OPEN_FILE);
        dialog.question = NULL;
        dialog.callback = open_file_dialog_finished;
        gui_dialog_show(&dialog);
    }
}

void show_name_edit_dialog(void)
{
    if (current_level && !current_dialog) {
        memcpy(current_level->name_backup,
               current_level->name,
               NAME_MAXLEN);

        gui_dialog_clesr(&dialog, GUI_DIALOG_STRING);
        dialog.question = "Enter Name:";
        dialog.default_input = current_level->name;
        dialog.callback = name_edit_dialog_finished;
        gui_dialog_show(&dialog);
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

static void return_from_nested_level_callback(UNUSED level_t *level, UNUSED void *data)
{
    level_t *level_copy = current_level;

    if (current_level) {
        disable_postprocessing();
        level_unload();
    }

    destroy_level(level_copy);

    switch (nested_game_mode) {
    case GAME_MODE_EDIT_LEVEL:
        level_edit(nested_return_level);
        break;

    default:
        __builtin_unreachable();
    }

    nested_return_level = NULL;
    nested_game_mode = GAME_MODE_NULL;
}

bool return_from_nested_level(void)
{
    assert_not_null(nested_return_level);

    if (current_level) {
        if (current_level->fade.active) {
            if (options->verbose) {
                infomsg("forcinng quick exit");
            }
            running = false;
            return true;
        }
        level_fade_out(current_level, return_from_nested_level_callback, NULL);
        return true;
    } else {
        return false;
    }
}

static void return_from_level_callback(UNUSED level_t *level, UNUSED void *data)
{
    if (current_level) {
        disable_postprocessing();
        level_unload();
    }

    prev_game_mode();
}

bool return_from_level(void)
{
    if (current_level) {
        if (current_level->fade.active) {
            if (options->verbose) {
                infomsg("forcinng quick exit");
            }
            running = false;
            return true;
        }
        level_fade_out(current_level, return_from_level_callback, NULL);
        return true;
    } else {
        return false;
    }
}

void create_new_level(void)
{
    level_t *level = create_level(current_collection);
    collection_add_level(current_collection, level);
    level_edit(level);
}

#if defined(PLATFORM_DESKTOP)
void open_game_file(const char *path, bool edit)
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

    game_mode_t collection_mode = GAME_MODE_PLAY_COLLECTION;
    if (edit) {
        collection_mode = GAME_MODE_EDIT_COLLECTION;
    }

    if (IS_LEVEL_FILENAME(path)) {
        level_t *level = collection_find_level_by_filename(current_collection, path);
        if (level) {
            if (edit) {
                if (options->verbose) {
                    infomsg("Editing level file \"%s\"", path);
                }
                level_edit(level);
            } else {
                if (options->verbose) {
                    infomsg("Playing level file \"%s\"", path);
                }
                level_play(level);
            }
        } else {
            errmsg("Failed to open file \"%s\"", path);
        }
    } else if (IS_COLLECTION_FILENAME(path)) {
        if (options->verbose) {
            infomsg("%s collection file \"%s\"", edit ? "Editing" : "Playing", path);
        }
        set_game_mode(collection_mode);
    } else if (current_collection->dirpath) {
        if (options->verbose) {
            infomsg("%s collection directory \"%s\"", edit ? "Editing" : "Playing", path);
        }
        set_game_mode(collection_mode);
    } else {
        assert(0);
    }
}

static void play_game_file(const char *path)
{
    if (options->verbose) {
        infomsg("ACTION: play file \"%s\"", path);
    }
    open_game_file(path, false);
}

static void edit_game_file(const char *path)
{
    if (options->verbose) {
        infomsg("ACTION: edit file \"%s\"", path);
    }
    open_game_file(path, true);
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

void savequit_current_level(void)
{
    running = false;

    if (current_level) {
        save_current_level_with_nvdata();
    }
}
#endif

void play_random_game(void)
{
    set_game_mode(GAME_MODE_RANDOM);
    play_gui_random_level();
}

level_t *create_blank_level(void)
{
    level_t *level = create_level(NULL);
    level_reset(level);
    level_set_radius(level, options->create_level_radius);
    return level;
}

void edit_new_blank_level(void)
{
    level_t *blank = create_blank_level();
    level_edit(blank);
}

void play_nested_level_callback(UNUSED struct level *level, UNUSED void *data)
{
    level_t *nested_level = create_level_copy(nested_return_level);
    level_play(nested_level);
}

void play_nested_level(void)
{
    assert_not_null(current_level);
    assert_null(nested_return_level);
    assert(game_mode == GAME_MODE_EDIT_LEVEL);

    nested_game_mode = game_mode;
    nested_return_level = current_level;

    level_fade_out(current_level, play_nested_level_callback, NULL);
}

static void reset_current_level(void)
{
    if (current_level) {
        if (options->verbose) {
            infomsg("Resetting level tile positions");
        }

        undo_reset_data_t *data_from = level_undo_copy_reset_data(current_level);
        level_unwin(current_level);
        level_reset_tile_positions(current_level);
        undo_reset_data_t *data_to = level_undo_copy_reset_data(current_level);
        level_undo_add_reset(current_level, data_from, data_to);
    } else {
        warnmsg("Cannot reset level - current_level is NULL");
    }
}

static void undo_play(void)
{
    if (current_level) {
        level_undo_play(current_level);
    }
}

static void redo_play(void)
{
    if (current_level) {
        level_redo_play(current_level);
    }
}

static void undo_edit(void)
{
    if (current_level) {
        level_undo_edit(current_level);
    }
}

static void redo_edit(void)
{
    if (current_level) {
        level_redo_edit(current_level);
    }
}

static void undo(void)
{
    switch (game_mode) {
    case GAME_MODE_PLAY_LEVEL:
        undo_play();
        break;

    case GAME_MODE_EDIT_LEVEL:
        undo_edit();
        break;

    default:
        warnmsg("UNDO only works in PLAY of EDIT mode");
    }
}

static void redo(void)
{
    switch (game_mode) {
    case GAME_MODE_PLAY_LEVEL:
        redo_play();
        break;

    case GAME_MODE_EDIT_LEVEL:
        redo_edit();
        break;

    default:
        warnmsg("REDO only works in PLAY of EDIT mode");
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
unload_render_textures(
    void
) {
    if (IsRenderTextureReady(scene_targets[0])) {
        UnloadRenderTexture(scene_targets[0]);
    }
    if (IsRenderTextureReady(scene_targets[1])) {
        UnloadRenderTexture(scene_targets[1]);
    }

    scene_read_target  = NULL;
    scene_write_target = NULL;
}

static void
create_render_textures(
    void
) {

    unload_render_textures();

    scene_targets[0] = LoadRenderTexture(window_size.x, window_size.y);
    scene_targets[1] = LoadRenderTexture(window_size.x, window_size.y);
    SetTextureWrap(scene_targets[0].texture, TEXTURE_WRAP_MIRROR_REPEAT);
    SetTextureWrap(scene_targets[1].texture, TEXTURE_WRAP_MIRROR_REPEAT);
    scene_read_target  = &(scene_targets[0]);
    scene_write_target = &(scene_targets[1]);
}

static void swap_scene_targets(void)
{
    RenderTexture *tmp = scene_write_target;
    scene_write_target = scene_read_target;
    scene_read_target  = tmp;
}

static void
set_uniform_resolution(
    void
) {
    float resolution[2] = { (float)window_size.x, (float)window_size.y };
    SetShaderValue(win_border_shader, win_border_shader_loc.resolution, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(postprocessing_shader, postprocessing_shader_loc.resolution, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(background_shader, background_shader_loc.resolution, resolution, SHADER_UNIFORM_VEC2);
}

void do_resize(void)
{
    window_size.x = GetScreenWidth();
    window_size.y = GetScreenHeight();

    window_sizef.x = (float)window_size.x;
    window_sizef.y = (float)window_size.y;

    window_center.x = 0.5f * window_sizef.x;
    window_center.y = 0.5f * window_sizef.y;

    window_corner_dist = Vector2Length(window_center);

#ifdef DEBUG_RESIZE
    warnmsg("RESIZE to: %d x %d, CENTER: %4f x %4f",
            window_size.x, window_size.y,
            window_center.x, window_center.y);
#endif

    set_uniform_resolution();
    create_render_textures();
    if (current_level) {
        level_resize(current_level);
    }

    background_resize(background);

    gui_setup();
    resize_gui_popup_message();
    resize_gui_dialog();
    resize_gui_help();
    resize_gui_title();
    resize_gui_browser();
    resize_gui_collection();
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

void set_mouse_position(int new_x, int new_y)
{
    mouse_position.x = new_x;
    mouse_position.y = new_y;

    mouse_positionf.x = (float)mouse_position.x;
    mouse_positionf.y = (float)mouse_position.y;

    if (do_level_ui_interaction()) {
        level_set_hover(current_level, mouse_position);
    }
}

static void handle_mouse_events(void)
{
    mouse_left_click   = false;;
    mouse_left_release = false;
    mouse_right_click  = false;;
    mouse_left_doubleclick = false;

    if (IsCursorOnScreen()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            mouse_left_click = true;

            double click_time = GetTime();
            if ((click_time - last_mouse_click_time) < ((float)options->double_click_ms) * 0.001f) {
                mouse_left_doubleclick = true;
            }
            last_mouse_click_time = click_time;

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

#if 0
    if (mouse_left_doubleclick) {
        printf("DOUBLECLICK!\n");
    }
#endif
}

static void do_level_checks(void)
{
    if (current_level) {
        if (level_check(current_level)) {
            if (!level_finished) {
                level_finished = true;
                level_win(current_level);
            }
        } else {
            if (level_finished) {
                level_finished = false;
                level_unwin(current_level);
            }
        }
    } else {
        level_finished = false;
    }
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

    if (mouse_input_is_enabled()) {
        set_mouse_position(GetMouseX(), GetMouseY());
    }

    if (IsKeyPressed(KEY_Q)) {
        running = false;
    }

    if (IsKeyPressed(KEY_F)) {
        show_fps = !show_fps;
    }

    if (IsKeyPressed(KEY_B)) {
        options->animate_bg = !options->animate_bg;
    }

    if (IsKeyPressed(KEY_P)) {
        options->use_postprocessing = !options->use_postprocessing;
    }

    if (!demo_mode) {
        if (options->cheat_solver) {
            if (IsKeyPressed(KEY_F5)) {
                if (current_level) {
                    create_or_use_solver(current_level);
                    solver_toggle_solve(current_level->solver);
                }
            }

            if (IsKeyPressed(KEY_F6)) {
                if (current_level) {
                    create_or_use_solver(current_level);
                    solver_stop(current_level->solver);
                }
            }

            if (IsKeyPressed(KEY_F7)) {
                if (current_level) {
                    create_or_use_solver(current_level);
                    solver_toggle_undo(current_level->solver);
                }
            }
        }

#ifdef DEBUG_ID_AND_DIR
        if (IsKeyPressed(KEY_F3)) {
            debug_dir = (debug_dir + 1) % 6;
        }

        if (IsKeyPressed(KEY_F4)) {
            debug_id = (debug_id + 1) % LEVEL_MAXTILES;
        }
#endif

        if (IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_H)) {
            show_help_box = !show_help_box;
        }
    }

#if defined(PLATFORM_DESKTOP)
    if (IsKeyPressed(KEY_R) && is_any_shift_down()) {
        reset_window_to_center();
    }
#endif

    if (demo_mode) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            running = false;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            //regen_level_preview();
            play_gui_random_level_preview();

            if (options->cheat_autowin) {
                level_win(current_level);
            } else {
                create_or_use_solver(current_level);
                solver_toggle_solve(current_level->solver);
            }
        }

        do_level_checks();

        return true;
    }

    if (modal_ui_active) {
        if (show_help_box) {
            if (IsKeyPressed(KEY_ESCAPE) ||
                IsKeyPressed(KEY_ENTER)) {
                show_help_box = false;
            }
        } else {
            if (IsKeyPressed(KEY_ESCAPE)) {
                modal_ui_result = UI_RESULT_CANCEL;
            } else if (IsKeyPressed(KEY_ENTER)) {
                modal_ui_result = UI_RESULT_OK;
            } else {
                modal_ui_result = UI_RESULT_PENDING;;
            }
        }

        return true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        switch (game_mode) {
        case GAME_MODE_OPTIONS:
            prev_game_mode();
            break;

        case GAME_MODE_EDIT_LEVEL:
            /* fall thrugh */
        case GAME_MODE_WIN_LEVEL:
            /* fall thrugh */
        case GAME_MODE_PLAY_LEVEL:
            if (return_from_level()) {
                break;
            }
            /* fall through */
        default:
#if defined(PLATFORM_DESKTOP)
            running = false;
            return true;
#else
            break;
#endif
        }
    }

    if (IsKeyPressed(KEY_ENTER)) {
        switch (game_mode) {
        case GAME_MODE_RANDOM:
            play_gui_random_level();
            break;

        default:
            /* do nothing */
            break;
        }
    }

    if (IsKeyPressed(KEY_U) || is_key_pressed_with_control_or_super(KEY_Z)) {
        undo();
    }

    if (IsKeyPressed(KEY_R) || is_key_pressed_with_control_or_super(KEY_Y)) {
        redo();
    }

    if (mouse_input_is_enabled()) {
        handle_mouse_events();
    }

    do_level_checks();

    return true;
}

Rectangle name_text_rect;
Rectangle name_panel_rect;
Rectangle name_edit_button_rect;

Rectangle edit_radius_panel_rect;
Rectangle edit_radius_label_rect;
Rectangle edit_radius_left_button_rect;
Rectangle edit_radius_display_rect;
Rectangle edit_radius_right_button_rect;
Rectangle edit_mode_panel_rect;
Rectangle edit_mode_label_rect;
Rectangle edit_mode_toggle_rect;
Rectangle open_file_button_rect;
Rectangle tool_panel_rect;
Rectangle edit_tool_label_shadow_rect;
Rectangle edit_tool_label_rect;
Rectangle cycle_tool_button_rect;
Rectangle erase_tool_button_rect;
Rectangle edit_shuffle_panel_rect;
Rectangle goto_next_level_panel_rect;
Rectangle goto_next_level_no_preview_panel_rect;
Rectangle goto_next_level_label_rect;
Rectangle goto_next_level_button_rect;
Rectangle goto_next_level_preview_rect;
Rectangle goto_next_seed_panel_rect;
Rectangle goto_next_seed_no_preview_panel_rect;
Rectangle goto_next_seed_label_rect;
Rectangle goto_next_seed_button_rect;
Rectangle goto_next_seed_preview_rect;
Rectangle main_gui_area_rect;

float tool_panel_content_height;

Vector2 edit_radius_label_location;
Vector2 edit_radius_display_text_location;
Vector2 edit_radius_display_text_shadow_location;
Vector2 edit_mode_label_location;
Vector2 edit_tool_label_location;
Vector2 edit_tool_label_shadow_location;
Vector2 edit_shuffle_label_location;
Vector2 goto_next_level_label_location;
Vector2 goto_next_seed_label_location;

struct right_side_button {
    Rectangle rect;

    float top_y;
    float vert_spacing;
    float area_width;

    float single_line_y_offset;
    float double_line_y_offset;
};
struct right_side_button right_side_button;

char close_button_text_str[] = "Quit";
#define CLOSE_BUTTON_TEXT_LENGTH (6 + sizeof(close_button_text_str))
char close_button_text[CLOSE_BUTTON_TEXT_LENGTH];

char options_button_text_str[] = "Options";
#define OPTIONS_BUTTON_TEXT_LENGTH (6 + sizeof(options_button_text_str))
char options_button_text[OPTIONS_BUTTON_TEXT_LENGTH];

char browser_button_text_str[] = "Browse";
#define BROWSER_BUTTON_TEXT_LENGTH (6 + sizeof(browser_button_text_str))
char browser_button_text[BROWSER_BUTTON_TEXT_LENGTH];

char random_button_text_str[] = "Random";
#define RANDOM_BUTTON_TEXT_LENGTH (6 + sizeof(random_button_text_str))
char random_button_text[RANDOM_BUTTON_TEXT_LENGTH];

char levels_button_text_str[] = "Levels";
#define LEVELS_BUTTON_TEXT_LENGTH (6 + sizeof(levels_button_text_str))
char levels_button_text[LEVELS_BUTTON_TEXT_LENGTH];

char save_button_text_str[] = "Save";
#define SAVE_BUTTON_TEXT_LENGTH (6 + sizeof(save_button_text_str))
char save_button_text[SAVE_BUTTON_TEXT_LENGTH];

char test_button_text_str[] = "Test";
#define TEST_BUTTON_TEXT_LENGTH (6 + sizeof(test_button_text_str))
char test_button_text[TEST_BUTTON_TEXT_LENGTH];

char edit_button_text_str[] = "Edit";
#define EDIT_BUTTON_TEXT_LENGTH (6 + sizeof(edit_button_text_str))
char edit_button_text[EDIT_BUTTON_TEXT_LENGTH];

char unedit_button_text_str[] = "Stop Edit";
#define UNEDIT_BUTTON_TEXT_LENGTH (6 + sizeof(unedit_button_text_str))
char unedit_button_text[UNEDIT_BUTTON_TEXT_LENGTH];

char savequit_button_text_str[] = "Save&Quit";
#define SAVEQUIT_BUTTON_TEXT_LENGTH (6 + sizeof(savequit_button_text_str))
char savequit_button_text[SAVEQUIT_BUTTON_TEXT_LENGTH];

char reset_button_text_str[] = "Reset";
#define RESET_BUTTON_TEXT_LENGTH (6 + sizeof(reset_button_text_str))
char reset_button_text[RESET_BUTTON_TEXT_LENGTH];

char return_button_text_str[] = "Back";
#define RETURN_BUTTON_TEXT_LENGTH (6 + sizeof(return_button_text_str))
char return_button_text[RETURN_BUTTON_TEXT_LENGTH];

char retnest_button_text_str[] = "Back";
#define RETNEST_BUTTON_TEXT_LENGTH (6 + sizeof(retnest_button_text_str))
char retnest_button_text[RETNEST_BUTTON_TEXT_LENGTH];

char undo_button_text_str[] = "Undo";
#define UNDO_BUTTON_TEXT_LENGTH (6 + sizeof(undo_button_text_str))
char undo_button_text[UNDO_BUTTON_TEXT_LENGTH];

char redo_button_text_str[] = "Redo";
#define REDO_BUTTON_TEXT_LENGTH (6 + sizeof(redo_button_text_str))
char redo_button_text[REDO_BUTTON_TEXT_LENGTH];

char open_file_button_text_str[] = "Open File";
#define OPEN_FILE_BUTTON_TEXT_LENGTH (6 + sizeof(open_file_button_text_str))
char open_file_button_text[OPEN_FILE_BUTTON_TEXT_LENGTH];

char cycle_tool_button_text_str[] = "Cycle";
#define CYCLE_TOOL_BUTTON_TEXT_LENGTH (6 + sizeof(cycle_tool_button_text_str))
char cycle_tool_button_text[CYCLE_TOOL_BUTTON_TEXT_LENGTH];

char erase_tool_button_text_str[] = "Erase";
#define ERASE_TOOL_BUTTON_TEXT_LENGTH (6 + sizeof(erase_tool_button_text_str))
char erase_tool_button_text[ERASE_TOOL_BUTTON_TEXT_LENGTH];

char *browser_button_lines[2] = {
    browser_button_text_str,
    levels_button_text_str
};

char *random_button_lines[2] = {
    random_button_text_str,
    levels_button_text_str
};

char game_source_button_source_str[] = "Source";
char game_source_button_code_str[] = "Code";
const char *game_source_button_lines[3] = {
    game_source_button_source_str,
    game_source_button_code_str
};

char edit_radius_label_text[] = "Radius";
char edit_mode_label_text[] = "Mode";
char edit_tool_label_text[] = "Edit Tool";
char edit_shuffle_label_text[] = "Shuffle";
char goto_next_level_label_text[] = "Next level?";
char goto_next_seed_label_text[] = "Next RNG Seed?";

char edit_radius_left_button_text[6];
char edit_radius_right_button_text[6];

char cancel_ok_with_icons[25];
char no_yes_with_icons[25];

int edit_mode_toggle_active;

Vector2 inner_margin_vec2 = { .x = PANEL_INNER_MARGIN, .y = PANEL_INNER_MARGIN };

static void gui_setup_name_panel(void)
{
    name_text_rect.x = WINDOW_MARGIN + PANEL_INNER_MARGIN;
    name_text_rect.y = WINDOW_MARGIN + PANEL_INNER_MARGIN;
    name_text_rect.height = name_font.size;

    name_panel_rect.x = WINDOW_MARGIN;
    name_panel_rect.y = WINDOW_MARGIN;
    name_panel_rect.height = 36 + (2 * PANEL_INNER_MARGIN);

    name_edit_button_rect.x = name_text_rect.x + name_text_rect.width + ICON_BUTTON_SIZE;
    name_edit_button_rect.y = name_text_rect.y;

    name_edit_button_rect.width  = ICON_BUTTON_SIZE;
    name_edit_button_rect.height = ICON_BUTTON_SIZE;
}

static void gui_setup_edit_mode_panel(void)
{
    edit_mode_panel_rect.x = name_panel_rect.x;
    edit_mode_panel_rect.y =
        name_panel_rect.y
        + name_panel_rect.height
        + WINDOW_MARGIN;

    Vector2 edit_mode_label_text_size = measure_panel_text(edit_mode_label_text);

    edit_mode_label_rect.x = edit_mode_panel_rect.x + PANEL_INNER_MARGIN;
    edit_mode_label_rect.y = edit_mode_panel_rect.y + edit_mode_panel_rect.height + PANEL_INNER_MARGIN;
    edit_mode_label_rect.width  = edit_mode_label_text_size.x;
    edit_mode_label_rect.height = edit_mode_label_text_size.y;

    edit_mode_label_location.x = edit_mode_label_rect.x;
    edit_mode_label_location.y = edit_mode_label_rect.y;

    edit_mode_toggle_rect.x = edit_mode_panel_rect.x + PANEL_INNER_MARGIN;
    edit_mode_toggle_rect.y = edit_mode_label_rect.y + edit_mode_label_rect.height + PANEL_INNER_MARGIN;
    edit_mode_toggle_rect.width  = 120;
    edit_mode_toggle_rect.height = 24;

    edit_mode_panel_rect.width =
        edit_mode_toggle_rect.x
        + edit_mode_toggle_rect.width
        + PANEL_INNER_MARGIN
        - edit_mode_panel_rect.x;

    edit_mode_panel_rect.height =
        edit_mode_label_rect.height
        + (2 * edit_mode_toggle_rect.height)
        + (3 * PANEL_INNER_MARGIN);
}

static void gui_setup_edit_radius_panel(void)
{
    edit_radius_panel_rect.x = edit_mode_panel_rect.x;
    edit_radius_panel_rect.y = edit_mode_panel_rect.y + edit_mode_panel_rect.height + PANEL_INNER_MARGIN;

    Vector2 edit_radius_label_text_size = measure_panel_text(edit_radius_label_text);

    edit_radius_label_rect.x = name_text_rect.x;
    edit_radius_label_rect.y = edit_radius_panel_rect.y + PANEL_INNER_MARGIN;
    edit_radius_label_rect.width  = edit_radius_label_text_size.x;
    edit_radius_label_rect.height = edit_radius_label_text_size.y;

    edit_radius_label_location.x = edit_radius_label_rect.x;
    edit_radius_label_location.y = edit_radius_label_rect.y;

    memcpy(edit_radius_left_button_text,  GuiIconText(ICON_ARROW_LEFT_FILL, NULL), 6);
    memcpy(edit_radius_right_button_text, GuiIconText(ICON_ARROW_RIGHT_FILL, NULL), 6);

    edit_radius_left_button_rect.x      = edit_radius_panel_rect.x + PANEL_INNER_MARGIN;
    edit_radius_left_button_rect.y      = edit_radius_label_rect.y + edit_radius_label_rect.height + PANEL_INNER_MARGIN;
    edit_radius_left_button_rect.width  = TOOL_BUTTON_WIDTH;
    edit_radius_left_button_rect.height = TOOL_BUTTON_HEIGHT;

    edit_radius_display_rect.x      = edit_radius_left_button_rect.x + edit_radius_left_button_rect.width + BUTTON_MARGIN;
    edit_radius_display_rect.y      = edit_radius_left_button_rect.y;
    edit_radius_display_rect.width  = TOOL_BUTTON_WIDTH;
    edit_radius_display_rect.height = TOOL_BUTTON_HEIGHT;

    edit_radius_right_button_rect.x      = edit_radius_display_rect.x + edit_radius_display_rect.width + BUTTON_MARGIN;
    edit_radius_right_button_rect.y      = edit_radius_display_rect.y;
    edit_radius_right_button_rect.width  = TOOL_BUTTON_WIDTH;
    edit_radius_right_button_rect.height = TOOL_BUTTON_HEIGHT;

    edit_radius_display_text_location.x = edit_radius_display_rect.x + 8;
    edit_radius_display_text_location.y = edit_radius_display_rect.y + 3;
    edit_radius_display_rect.x      += 1;
    edit_radius_display_rect.width  -= 2;
    edit_radius_display_rect.height += 1;

    edit_radius_display_text_shadow_location = edit_radius_display_text_location;
    edit_radius_display_text_shadow_location.x += 1.0f;
    edit_radius_display_text_shadow_location.y += 1.0f;

    edit_radius_panel_rect.width =
        edit_radius_right_button_rect.x
        + edit_radius_right_button_rect.width
        + PANEL_INNER_MARGIN
        - edit_radius_panel_rect.x;

    edit_radius_panel_rect.height =
        edit_radius_right_button_rect.y
        + edit_radius_right_button_rect.height
        + PANEL_INNER_MARGIN
        - edit_radius_panel_rect.y;
}

static void gui_setup_edit_tool_panel(void)
{
    memcpy(cycle_tool_button_text,  GuiIconText(ICON_MUTATE_FILL, cycle_tool_button_text_str), CYCLE_TOOL_BUTTON_TEXT_LENGTH);
    memcpy(erase_tool_button_text,  GuiIconText(ICON_RUBBER, erase_tool_button_text_str), ERASE_TOOL_BUTTON_TEXT_LENGTH);

    Vector2 edit_tool_label_text_size   = measure_panel_text(edit_tool_label_text);

    Vector2 cycle_tool_button_text_size = measure_gui_text(cycle_tool_button_text);
    Vector2 erase_tool_button_text_size = measure_gui_text(erase_tool_button_text);

    tool_panel_content_height = MAX(edit_tool_label_text_size.y,
                                    TOOL_BUTTON_HEIGHT);

    tool_panel_rect.x = edit_radius_panel_rect.x;
    tool_panel_rect.width = (4 * PANEL_INNER_MARGIN)
        + ((PATH_TYPE_COUNT - 0) * (2 * ICON_BUTTON_SIZE))
        + edit_tool_label_text_size.x
        + cycle_tool_button_text_size.x
        + erase_tool_button_text_size.x;
    tool_panel_rect.height = (2 * PANEL_INNER_MARGIN) + tool_panel_content_height;
    tool_panel_rect.y = window_size.y - WINDOW_MARGIN - tool_panel_rect.height;

    edit_tool_label_rect.x =  tool_panel_rect.x + PANEL_INNER_MARGIN;
    edit_tool_label_rect.y =  tool_panel_rect.y + PANEL_INNER_MARGIN;
    edit_tool_label_rect.width = edit_tool_label_text_size.x + (4 * BUTTON_MARGIN);
    edit_tool_label_rect.height = tool_panel_content_height;

    edit_tool_label_location = getVector2FromRectangle(edit_tool_label_rect);
    edit_tool_label_location.x += 3.0 + (1 * BUTTON_MARGIN);
    edit_tool_label_location.y += 3.0;
    edit_tool_label_shadow_location = Vector2Add(edit_tool_label_location, VEC2_SHADOW);

    cycle_tool_button_rect.x = edit_tool_label_rect.x + edit_tool_label_rect.width + PANEL_INNER_MARGIN;
    cycle_tool_button_rect.y = edit_tool_label_rect.y;
    cycle_tool_button_rect.width  = cycle_tool_button_text_size.x;
    cycle_tool_button_rect.height = tool_panel_content_height;

    erase_tool_button_rect.x = cycle_tool_button_rect.x + cycle_tool_button_rect.width + ICON_BUTTON_SIZE;
    erase_tool_button_rect.y = cycle_tool_button_rect.y;
    erase_tool_button_rect.width  = erase_tool_button_text_size.x;
    erase_tool_button_rect.height = tool_panel_content_height;
}

static void gui_setup_edit_shuffle_panel(void)
{
    Vector2 edit_shuffle_label_text_size = measure_panel_text(edit_shuffle_label_text);

    edit_shuffle_label_location = edit_tool_label_location;

    edit_shuffle_panel_rect.x = tool_panel_rect.x;
    edit_shuffle_panel_rect.y = tool_panel_rect.y;
    edit_shuffle_panel_rect.height = tool_panel_rect.height;
    edit_shuffle_panel_rect.width = edit_shuffle_label_text_size.x + (4 * PANEL_INNER_MARGIN);
}

static void gui_setup_goto_next_level_panel(void)
{
    Vector2 goto_next_level_label_text_size = measure_panel_text(goto_next_level_label_text);

    Vector2 panel_bottom_right = {
        .x = window_size.x - WINDOW_MARGIN,
        .y = window_size.x - WINDOW_MARGIN
    };
    Vector2 content_bottom_right = Vector2Subtract(panel_bottom_right, inner_margin_vec2);

    float preview_size = GOTO_NEXT_LEVEL_PREVIEW_SIZE;
    goto_next_level_label_rect.width    = goto_next_level_label_text_size.x;
    goto_next_level_label_rect.height   = goto_next_level_label_text_size.y;
    goto_next_level_preview_rect.width  = preview_size;
    goto_next_level_preview_rect.height = preview_size;

    Rectangle inner_rect;
    inner_rect.width  = MAX(goto_next_level_label_rect.width,
                            goto_next_level_preview_rect.width);
    inner_rect.height =
        goto_next_level_preview_rect.height
        + goto_next_level_label_rect.height
        + PANEL_INNER_MARGIN;

    inner_rect.x = content_bottom_right.x - inner_rect.width;
    inner_rect.y = content_bottom_right.y - inner_rect.height;

    goto_next_level_label_location.x = inner_rect.x;
    goto_next_level_label_location.y =
        inner_rect.y
        + inner_rect.height
        - goto_next_level_label_rect.height;

    goto_next_level_label_rect.x = goto_next_level_label_location.x;
    goto_next_level_label_rect.y = goto_next_level_label_location.y;

    goto_next_level_preview_rect.x = inner_rect.x;
    goto_next_level_preview_rect.y = inner_rect.y;

    if (goto_next_level_label_rect.width > goto_next_level_preview_rect.width) {
        float delta = goto_next_level_label_rect.width - goto_next_level_preview_rect.width;
        goto_next_level_preview_rect.x += delta * 0.5;
    } else if (goto_next_level_label_rect.width < goto_next_level_preview_rect.width) {
        float delta = goto_next_level_preview_rect.width - goto_next_level_label_rect.width;
        delta *= 0.5;
        goto_next_level_label_rect.x     += delta;
        goto_next_level_label_location.x += delta;
    }

    goto_next_level_panel_rect.width  = inner_rect.width  + (2 * PANEL_INNER_MARGIN);
    goto_next_level_panel_rect.height = inner_rect.height + (2 * PANEL_INNER_MARGIN);
    goto_next_level_panel_rect.x      = inner_rect.x - PANEL_INNER_MARGIN;
    goto_next_level_panel_rect.y      = inner_rect.y - PANEL_INNER_MARGIN;

    goto_next_level_no_preview_panel_rect = goto_next_level_panel_rect;
    float height_delta = goto_next_level_preview_rect.height + PANEL_INNER_MARGIN;
    goto_next_level_no_preview_panel_rect.y += height_delta;
    goto_next_level_no_preview_panel_rect.height -= height_delta;
}

static void gui_setup_goto_next_seed_panel(void)
{
    Vector2 goto_next_seed_label_text_size = measure_panel_text(goto_next_seed_label_text);

    Vector2 panel_bottom_right = {
        .x = window_size.x - WINDOW_MARGIN,
        .y = window_size.x - WINDOW_MARGIN
    };
    Vector2 content_bottom_right = Vector2Subtract(panel_bottom_right, inner_margin_vec2);

    float preview_size = GOTO_NEXT_SEED_PREVIEW_SIZE;
    goto_next_seed_label_rect.width    = goto_next_seed_label_text_size.x;
    goto_next_seed_label_rect.height   = goto_next_seed_label_text_size.y;
    goto_next_seed_preview_rect.width  = preview_size;
    goto_next_seed_preview_rect.height = preview_size;

    Rectangle inner_rect;
    inner_rect.width  = MAX(goto_next_seed_label_rect.width,
                            goto_next_seed_preview_rect.width);
    inner_rect.height =
        goto_next_seed_preview_rect.height
        + goto_next_seed_label_rect.height
        + PANEL_INNER_MARGIN;

    inner_rect.x = content_bottom_right.x - inner_rect.width;
    inner_rect.y = content_bottom_right.y - inner_rect.height;

    goto_next_seed_label_location.x = inner_rect.x;
    goto_next_seed_label_location.y =
        inner_rect.y
        + inner_rect.height
        - goto_next_seed_label_rect.height;

    goto_next_seed_label_rect.x = goto_next_seed_label_location.x;
    goto_next_seed_label_rect.y = goto_next_seed_label_location.y;

    goto_next_seed_preview_rect.x = inner_rect.x;
    goto_next_seed_preview_rect.y = inner_rect.y;

    if (goto_next_seed_label_rect.width > goto_next_seed_preview_rect.width) {
        float delta = goto_next_seed_label_rect.width - goto_next_seed_preview_rect.width;
        goto_next_seed_preview_rect.x += delta * 0.5;
    } else if (goto_next_seed_label_rect.width < goto_next_seed_preview_rect.width) {
        float delta = goto_next_seed_preview_rect.width - goto_next_seed_label_rect.width;
        delta *= 0.5;
        goto_next_seed_label_rect.x     += delta;
        goto_next_seed_label_location.x += delta;
    }

    goto_next_seed_panel_rect.width  = inner_rect.width  + (2 * PANEL_INNER_MARGIN);
    goto_next_seed_panel_rect.height = inner_rect.height + (2 * PANEL_INNER_MARGIN);
    goto_next_seed_panel_rect.x      = inner_rect.x - PANEL_INNER_MARGIN;
    goto_next_seed_panel_rect.y      = inner_rect.y - PANEL_INNER_MARGIN;

    goto_next_seed_no_preview_panel_rect = goto_next_seed_panel_rect;
    float height_delta = goto_next_seed_preview_rect.height + PANEL_INNER_MARGIN;
    goto_next_seed_no_preview_panel_rect.y += height_delta;
    goto_next_seed_no_preview_panel_rect.height -= height_delta;
}

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

    gui_setup_name_panel();
    gui_setup_edit_mode_panel();
    gui_setup_edit_radius_panel();
    gui_setup_edit_tool_panel();
    gui_setup_edit_shuffle_panel();
    gui_setup_goto_next_level_panel();
    gui_setup_goto_next_seed_panel();

    memcpy(   close_button_text, GuiIconText(ICON_EXIT,                 close_button_text_str),    CLOSE_BUTTON_TEXT_LENGTH);
    memcpy(    test_button_text, GuiIconText(ICON_PLAYER_PLAY,           test_button_text_str),     TEST_BUTTON_TEXT_LENGTH);
    memcpy(    edit_button_text, GuiIconText(ICON_TOOLS,                 edit_button_text_str),     EDIT_BUTTON_TEXT_LENGTH);
    memcpy(  unedit_button_text, GuiIconText(ICON_TOOLS,               unedit_button_text_str),   UNEDIT_BUTTON_TEXT_LENGTH);
    memcpy(    save_button_text, GuiIconText(ICON_FILE_SAVE_CLASSIC,     save_button_text_str),     SAVE_BUTTON_TEXT_LENGTH);
    memcpy(savequit_button_text, GuiIconText(ICON_FILE_SAVE_CLASSIC, savequit_button_text_str), SAVEQUIT_BUTTON_TEXT_LENGTH);
    memcpy(   reset_button_text, GuiIconText(ICON_EXPLOSION,            reset_button_text_str),    RESET_BUTTON_TEXT_LENGTH);
    memcpy(  return_button_text, GuiIconText(ICON_ARROW_LEFT,          return_button_text_str),   RETURN_BUTTON_TEXT_LENGTH);
    memcpy( retnest_button_text, GuiIconText(ICON_ARROW_LEFT,         retnest_button_text_str),  RETNEST_BUTTON_TEXT_LENGTH);
    memcpy(    undo_button_text, GuiIconText(ICON_UNDO_FILL,             undo_button_text_str),     UNDO_BUTTON_TEXT_LENGTH);
    memcpy(    redo_button_text, GuiIconText(ICON_REDO_FILL,             redo_button_text_str),     REDO_BUTTON_TEXT_LENGTH);
    memcpy( options_button_text, GuiIconText(ICON_GEAR,               options_button_text_str),  OPTIONS_BUTTON_TEXT_LENGTH);
    memcpy( browser_button_text, GuiIconText(ICON_FOLDER_FILE_OPEN,   browser_button_text_str),  BROWSER_BUTTON_TEXT_LENGTH);
    memcpy(  random_button_text, GuiIconText(ICON_SHUFFLE_FILL,        random_button_text_str),   RANDOM_BUTTON_TEXT_LENGTH);
    memcpy(  levels_button_text, GuiIconText(ICON_NONE,                levels_button_text_str),   LEVELS_BUTTON_TEXT_LENGTH);

    int right_side_button_text_width = 0;

#define rsbw(name) do {                                                             \
        Vector2 name##_button_text_size = measure_gui_text(name##_button_text_str); \
        right_side_button_text_width = MAX(right_side_button_text_width,            \
                                           name##_button_text_size.x);              \
    } while(0)

#define rsbnw(name) do {                                                                   \
        Vector2 name##_button_text_size = measure_gui_narrow_text(name##_button_text_str); \
        right_side_button_text_width = MAX(right_side_button_text_width,                   \
                                           name##_button_text_size.x);                     \
    } while(0)

    rsbw(close);
    rsbw(edit);
    rsbw(save);
    rsbw(return);
    rsbw(browser);
    rsbw(options);
    rsbw(reset);
    rsbnw(savequit);
#undef rsbw

    right_side_button_text_width += 2 * BUTTON_MARGIN;

    right_side_button.rect.y      = 1.5 * WINDOW_MARGIN;
    right_side_button.rect.width  = ICON_BUTTON_SIZE +
        right_side_button_text_width;
    right_side_button.rect.x      = window_size.x - WINDOW_MARGIN - right_side_button.rect.width;
    right_side_button.rect.height = ICON_BUTTON_SIZE;

    right_side_button.top_y = right_side_button.rect.y;
    right_side_button.vert_spacing = 1.0 * WINDOW_MARGIN;

    right_side_button.single_line_y_offset = right_side_button.rect.height + right_side_button.vert_spacing;
    right_side_button.double_line_y_offset = (2 * right_side_button.rect.height) + right_side_button.vert_spacing;

    right_side_button.area_width = right_side_button.rect.width + (2 * WINDOW_MARGIN);

    main_gui_area_rect.x      = WINDOW_MARGIN;
    main_gui_area_rect.y      = WINDOW_MARGIN;
    main_gui_area_rect.width  = window_size.x - WINDOW_MARGIN - right_side_button.area_width;
    main_gui_area_rect.height = window_size.y - (2 * WINDOW_MARGIN);

    Vector2 open_file_button_text_size = measure_gui_text(open_file_button_text_str);
    open_file_button_rect.x      = window_size.x - WINDOW_MARGIN - ICON_BUTTON_SIZE - open_file_button_text_size.x;
    open_file_button_rect.y      = window_size.y - WINDOW_MARGIN - ICON_BUTTON_SIZE;
    open_file_button_rect.width  = ICON_BUTTON_SIZE + open_file_button_text_size.x;
    open_file_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(open_file_button_text,  GuiIconText(ICON_FILE_OPEN, open_file_button_text_str), OPEN_FILE_BUTTON_TEXT_LENGTH);

    feedback_bg_zoom_ratio = 0.002;
    feedback_bg_zoom_margin.x = feedback_bg_zoom_ratio * ((float)window_size.x);
    feedback_bg_zoom_margin.y = feedback_bg_zoom_ratio * ((float)window_size.y);
}

static void draw_name_header(void)
{
    if (!current_level) {
        return;
    }

    assert_not_null(current_level->name);

    char *name = current_level->name;

    Vector2 textsize = measure_name_text(name);

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
            set_mouse_cursor(MOUSE_CURSOR_POINTING_HAND);
        }
    }
#else
    bool hover = false;
#endif

    Color bg_color   = hover ? panel_bg_hover_color : panel_bg_color;
    Color edge_color = hover ? panel_edge_hover_color : panel_edge_color;
    Color text_color = hover ? panel_header_text_hover_color : panel_header_text_color;

    DrawRectangleRounded(name_panel_rect, PANEL_ROUNDNES, 0, bg_color);
    DrawRectangleRoundedLines(name_panel_rect, PANEL_ROUNDNES, 0, 1.0, edge_color);

    Vector2 text_pos = getVector2FromRectangle(name_text_rect);
    Vector2 text_shadow_pos = {
        .x = text_pos.x + 1,
        .y = text_pos.y + 1
    };

    draw_name_text(name, text_shadow_pos, text_shadow_color);
    draw_name_text(name, text_pos, text_color);
}

static void drsw_edit_tile_radius_gui(void)
{
    DrawRectangleRounded(edit_radius_panel_rect, PANEL_ROUNDNES, 0, panel_bg_color);
    DrawRectangleRoundedLines(edit_radius_panel_rect, PANEL_ROUNDNES, 0, 1.0, panel_edge_color);

    draw_panel_text(edit_radius_label_text,
                    edit_radius_label_location,
                    panel_header_text_color);

    if (GuiButton(edit_radius_left_button_rect, edit_radius_left_button_text)) {
        if (current_level->radius > LEVEL_MIN_RADIUS) {
            int new_radius = current_level->radius - 1;
            level_undo_add_set_radius_event(current_level, current_level->radius, new_radius);
            level_set_radius(current_level, new_radius);
        }
    }
    DrawRectangleRec(edit_radius_display_rect, panel_header_label_bg_color);

    draw_panel_text(TextFormat("%d", current_level->radius),
                    edit_radius_display_text_shadow_location,
                    text_shadow_color);

    draw_panel_text(TextFormat("%d", current_level->radius),
                    edit_radius_display_text_location,
                    RAYWHITE);

    if (GuiButton(edit_radius_right_button_rect, edit_radius_right_button_text)) {
        if (current_level->radius < LEVEL_MAX_RADIUS) {
            int new_radius = current_level->radius + 1;
            level_undo_add_set_radius_event(current_level, current_level->radius, new_radius);
            level_set_radius(current_level, new_radius);
        }
    }
}

static void drsw_edit_tile_mode_gui(void)
{
    if (!current_level) {
        return;
    }

    DrawRectangleRounded(edit_mode_panel_rect, PANEL_ROUNDNES, 0, panel_bg_color);
    DrawRectangleRoundedLines(edit_mode_panel_rect, PANEL_ROUNDNES, 0, 1.0, panel_edge_color);

    draw_panel_text(edit_mode_label_text,
                    edit_mode_label_location,
                    panel_header_text_color);

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

    GuiToggleGroup(edit_mode_toggle_rect, "#112#Solved\n#62#Scrambled", &edit_mode_toggle_active);

    if ((edit_mode_toggle_active == 0 && current_level->currently_used_tiles == USED_TILES_UNSOLVED) ||
        (edit_mode_toggle_active == 1 && current_level->currently_used_tiles == USED_TILES_SOLVED)
    ) {
        used_tiles_t old_used_tiles = current_level->currently_used_tiles;
        level_toggle_currently_used_tiles(current_level);
        level_undo_add_use_tiles_event(current_level, old_used_tiles, current_level->currently_used_tiles);
    }
}

static void draw_shuffle_panel(void)
{
    bool hover = CheckCollisionPointRec(mouse_positionf, edit_shuffle_panel_rect);

    Color bg_color   = panel_bg_color;
    Color edge_color = panel_edge_color;
    Color text_color = panel_header_text_color;

    if (hover) {
        bg_color   = panel_bg_hover_color;
        edge_color = panel_edge_hover_color;
            text_color = panel_header_text_hover_color;
    }

    DrawRectangleRounded(edit_shuffle_panel_rect, PANEL_ROUNDNES, 0, bg_color);
    DrawRectangleRoundedLines(edit_shuffle_panel_rect, PANEL_ROUNDNES, 0, 1.0, edge_color);

    Vector2 text_pos = edit_shuffle_label_location;
    Vector2 text_shadow_pos = {
        .x = text_pos.x + 1,
        .y = text_pos.y + 1
    };

    draw_panel_text(edit_shuffle_label_text,
                    text_shadow_pos,
                    text_shadow_color);

    draw_panel_text(edit_shuffle_label_text,
                    text_pos,
                    text_color);

    if (hover && mouse_left_click && current_level) {
        undo_shuffle_data_t *from = level_undo_copy_shuffle_data(current_level);
        level_shuffle_tiles(current_level);
        undo_shuffle_data_t *to = level_undo_copy_shuffle_data(current_level);
        level_undo_add_shuffle(current_level, from, to);
    }
}


static void draw_tool_panel(void)
{
    Color bg   = panel_bg_color;
    Color edge = panel_edge_color;
    DrawRectangleRounded(tool_panel_rect, PANEL_ROUNDNES, 0, bg);
    DrawRectangleRoundedLines(tool_panel_rect, PANEL_ROUNDNES, 0, 1.0, edge);

    DrawRectangleRec(edit_tool_label_rect, panel_header_label_bg_color);

    draw_panel_text(edit_tool_label_text,
                    edit_tool_label_shadow_location,
                    text_shadow_color);

    draw_panel_text(edit_tool_label_text,
                    edit_tool_label_location,
                    panel_header_text_color);

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
        .width  = tool_panel_content_height,
        .height = tool_panel_content_height
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
                set_mouse_cursor(MOUSE_CURSOR_POINTING_HAND);
            }
        }

        rect.x += TOOL_BUTTON_WIDTH + ICON_BUTTON_SIZE;
    }
}

bool draw_level_preview(level_t *level, Rectangle bounds)
{
    bool rv = false;

    bool hover = false;;
    if (!GuiIsLocked()) {
        hover = CheckCollisionPointRec(mouse_positionf, bounds);
    }

    DrawRectangleRec(bounds, BLACK);

    if (level) {
        if (current_level_preview != level) {
            current_level_preview = level;

            level_resize(level);
        }

        level_preview(level, bounds);
    } else {
        Vector2 bounds_loc  = {.x = bounds.x,     .y = bounds.y      };
        Vector2 bounds_size = {.x = bounds.width, .y = bounds.height };

        Vector2 margin   = Vector2Scale(bounds_size, 0.1f);
        Vector2 c_tl  = Vector2Add(bounds_loc, margin);
        Vector2 c_br = Vector2Subtract(Vector2Add(bounds_loc, bounds_size), margin);
        Vector2 c_tr = { .x = c_br.x, .y = c_tl.y };
        Vector2 c_bl = { .x = c_tl.x, .y = c_br.y };

        float thickness = 2.0;
        Color color = error_cross_color;

        DrawLineEx(c_tl, c_br, thickness, color);
        DrawLineEx(c_bl, c_tr, thickness, color);

        DrawLineEx(c_tl, c_tr, thickness, color);
        DrawLineEx(c_tr, c_br, thickness, color);
        DrawLineEx(c_br, c_bl, thickness, color);
        DrawLineEx(c_bl, c_tl, thickness, color);

        return false;
    }

    if (hover) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Rectangle shift_bounds = bounds;
            shift_bounds.x -= 1;
            shift_bounds.y += 1;

            DrawRectangleLinesEx(      bounds, 2.0,    text_shadow_color);
            DrawRectangleLinesEx(shift_bounds, 1.0, tile_edge_drag_color);
        } else {
            Rectangle shift_bounds = bounds;
            shift_bounds.x += 1;
            shift_bounds.y -= 1;

            DrawRectangleLinesEx(      bounds, 2.0,     text_shadow_color);
            DrawRectangleLinesEx(shift_bounds, 1.0, tile_edge_hover_color);
        }

        if (mouse_left_click) {
            rv = true;
        }
    }

    return rv;
}

static void draw_goto_next_level_panel(void)
{
    assert_not_null(current_collection);
    assert_not_null(current_level);

    level_t *next_level = collection_get_level_after(current_collection, current_level);

    if (!next_level) {
        return;
    }

    bool hover = CheckCollisionPointRec(mouse_positionf, goto_next_level_panel_rect);

    Color bg_color   = panel_bg_color;
    Color edge_color = panel_edge_color;
    Color text_color = panel_header_text_color;

    if (options->show_level_previews) {
        DrawRectangleRounded(goto_next_level_panel_rect, PANEL_ROUNDNES, 0, bg_color);
        DrawRectangleRoundedLines(goto_next_level_panel_rect, PANEL_ROUNDNES, 0, 2.0, edge_color);
    } else {
        if (hover) {
            bg_color   = panel_bg_hover_color;
            edge_color = panel_edge_hover_color;
            text_color = panel_header_text_hover_color;
        }

        DrawRectangleRounded(goto_next_level_no_preview_panel_rect, PANEL_ROUNDNES, 0, bg_color);
        DrawRectangleRoundedLines(goto_next_level_no_preview_panel_rect, PANEL_ROUNDNES, 0, 2.0, edge_color);
    }

    Vector2 text_pos = goto_next_level_label_location;
    Vector2 text_shadow_pos = {
        .x = text_pos.x + 1,
        .y = text_pos.y + 1
    };
    draw_panel_text(goto_next_level_label_text,
                    text_shadow_pos,
                    text_shadow_color);

    draw_panel_text(goto_next_level_label_text,
                    text_pos,
                    text_color);

    if (options->show_level_previews) {
        if (draw_level_preview(next_level, goto_next_level_preview_rect)) {
            level_play(next_level);
        }
    } else {
        if (hover && mouse_left_click) {
            level_play(next_level);
        }
    }
}

static void draw_goto_next_seed_panel(void)
{
    assert_null(current_collection);
    assert_not_null(current_level);

    bool hover = CheckCollisionPointRec(mouse_positionf, goto_next_seed_panel_rect);

    Color bg_color   = panel_bg_color;
    Color edge_color = panel_edge_color;
    Color text_color = panel_header_text_color;

    if (options->show_level_previews) {
        DrawRectangleRounded(goto_next_seed_panel_rect, PANEL_ROUNDNES, 0, bg_color);
        DrawRectangleRoundedLines(goto_next_seed_panel_rect, PANEL_ROUNDNES, 0, 2.0, edge_color);
    } else {
        if (hover) {
            bg_color   = panel_bg_hover_color;
            edge_color = panel_edge_hover_color;
            text_color = panel_header_text_hover_color;
        }

        DrawRectangleRounded(goto_next_seed_no_preview_panel_rect, PANEL_ROUNDNES, 0, bg_color);
        DrawRectangleRoundedLines(goto_next_seed_no_preview_panel_rect, PANEL_ROUNDNES, 0, 2.0, edge_color);
    }

    Vector2 text_pos = goto_next_seed_label_location;
    Vector2 text_shadow_pos = {
        .x = text_pos.x + 1,
        .y = text_pos.y + 1
    };
    draw_panel_text(goto_next_seed_label_text,
                    text_shadow_pos,
                    text_shadow_color);

    draw_panel_text(goto_next_seed_label_text,
                    text_pos,
                    text_color);

    if (options->show_level_previews) {
        if (draw_level_preview(gui_random_level_preview, goto_next_seed_preview_rect)) {
            play_gui_random_level_preview();
        }
    } else {
        if (hover && mouse_left_click) {
            play_gui_random_level_preview();
        }
    }
}

static void draw_win_panels(void)
{
    if (current_level) {
        if (current_collection) {
            draw_goto_next_level_panel();
        } else {
            draw_goto_next_seed_panel();
        }
    }
}

static inline void reset_right_side_button(void)
{
    right_side_button.rect.y = right_side_button.top_y;
}

static inline void set_bottom_single_right_side_button(void)
{
    right_side_button.rect.y = window_size.y - right_side_button.double_line_y_offset;
}

static inline void set_bottom_double_right_side_button(void)
{
    right_side_button.rect.y = window_size.y - right_side_button.double_line_y_offset;
}

static inline void shift_down_right_side_button(float yoffset)
{
    right_side_button.rect.y += yoffset;
}

static inline bool rsb_single_line_button(const char *text)
{
    bool pressed = GuiButton(right_side_button.rect, text);
    shift_down_right_side_button(right_side_button.single_line_y_offset);
    return pressed;
}

static inline bool rsb_single_line_button_or_disabled(const char *text, bool is_enabled)
{
    if (is_enabled) {
        return rsb_single_line_button(text);
    } else {
        GuiDisable();
        bool rv = rsb_single_line_button(text);
        GuiEnable();
        return rv;
    }
}

static inline bool rsb_double_line_button(const char **lines, int line_count, int icon)
{
    bool pressed = GuiButtonMultiLine(right_side_button.rect, lines, line_count, icon);
    shift_down_right_side_button(right_side_button.double_line_y_offset);
    return pressed;
}

static bool game_mode_button(void *ptr, game_mode_t mode, int line_count, int icon)
{
    bool rv = false;

    bool selected = game_mode == mode;

    if (selected) {
        Rectangle src_rect = right_side_button.rect;

        int border_width = GuiGetStyle(BUTTON, BORDER_WIDTH);
        float line_height = src_rect.height - (2 * border_width) - 2;
        src_rect.height = (line_height * line_count) + ((line_count) * border_width);

        Rectangle bg_rect = ExpandRectangle(src_rect,
                                            BUTTON_SELECTED_HIGHLIGHT_THICKNESS);
        DrawRectangleRounded(bg_rect,
                             BUTTON_SELECTED_HIGHLIGHT_ROUNDNESS,
                             BUTTON_SELECTED_HIGHLIGHT_SEGMENTS,
                             highlight_border_color);
        //GuiDisable();
        GuiSetState(STATE_PRESSED);
    }

    bool pressed = false;
    if (line_count > 1) {
        const char **lines = ptr;
        pressed = rsb_double_line_button(lines, line_count, icon);
    } else {
        const char *text = ptr;
        pressed = rsb_single_line_button(text);
    }

    if (pressed) {
        if (selected) {
            set_game_mode(GAME_MODE_TITLE);
        } else {
            set_game_mode(mode);
        }
        rv = true;
    }

    if (selected) {
        //GuiEnable();
        GuiSetState(STATE_NORMAL);
    }

    return rv;
}

static void standard_buttons(void)
{
    game_mode_button(options_button_text,  GAME_MODE_OPTIONS, 1, 0);
    game_mode_button(browser_button_lines, GAME_MODE_BROWSER, 2, ICON_FOLDER_FILE_OPEN);
    game_mode_button( random_button_lines, GAME_MODE_RANDOM,  2, ICON_SHUFFLE_FILL);
}

static inline void rsb_undo_button(void)
{
    if (current_level) {
        if (rsb_single_line_button_or_disabled(undo_button_text, level_can_undo(current_level))) {
            undo_play();
        }
    }
}

static inline void rsb_redo_button(void)
{
    if (current_level) {
        if (rsb_single_line_button_or_disabled(redo_button_text, level_can_redo(current_level))) {
            redo_play();
        }
    }
}

static void draw_gui_widgets(void)
{
    reset_right_side_button();

    int prev_align = GuiGetStyle(BUTTON, TEXT_ALIGNMENT);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

#if defined(PLATFORM_DESKTOP)
    bool quit_pressed = rsb_single_line_button(close_button_text);

    if (quit_pressed) {
        running = false;
    }
#endif

    switch (game_mode) {
    case GAME_MODE_TITLE:
        standard_buttons();

        set_bottom_double_right_side_button();

        if (rsb_double_line_button(game_source_button_lines, 2, ICON_FILETYPE_TEXT)) {
            popup_message("Opening %s", PACKAGE_URL);
            OpenURLBackground(PACKAGE_URL);
        }
        break;

    case GAME_MODE_RANDOM:
        standard_buttons();
        break;

    case GAME_MODE_BROWSER:
        standard_buttons();

#if 0
#if defined(PLATFORM_DESKTOP)
        if (GuiButton(open_file_button_rect, open_file_button_text)) {
            open_file_box();
        }
#endif
#endif
        break;

    case GAME_MODE_OPTIONS:
        if (rsb_single_line_button(return_button_text)) {
            prev_game_mode();
        }
        break;

    case GAME_MODE_EDIT_LEVEL:
#if defined(PLATFORM_DESKTOP)
        if (quit_pressed) {
            if (current_level && current_level->changed) {
                show_ask_save_dialog(AFTER_DIALOG_SAVE_AND_QUIT,
                                     AFTER_DIALOG_QUIT);
                running = true;
            }
        }
#endif

        draw_name_header();

        drsw_edit_tile_mode_gui();

        if (edit_mode_solved) {
            drsw_edit_tile_radius_gui();
            draw_tool_panel();
        }

        if (edit_mode_unsolved) {
            draw_shuffle_panel();
        }

        game_mode_button(options_button_text,  GAME_MODE_OPTIONS, 1, 0);

        if (rsb_single_line_button(return_button_text)) {
#if defined(PLATFORM_DESKTOP)
            if (current_level && current_level->changed) {
                show_ask_save_dialog(AFTER_DIALOG_SAVE_AND_RETURN,
                                     AFTER_DIALOG_RETURN);
            } else {
                return_from_level();
            }
#endif
        }

        // skip one position
        shift_down_right_side_button(right_side_button.single_line_y_offset);

        rsb_undo_button();
        rsb_redo_button();

#if defined(PLATFORM_DESKTOP)
        if (current_level && current_level->changed) {
            if (rsb_single_line_button(save_button_text)) {
                show_ask_save_dialog(AFTER_DIALOG_SAVE,
                                     AFTER_DIALOG_NULL);
            }
        } else {
            GuiDisable();
            rsb_single_line_button(save_button_text);
            GuiEnable();
        }
#endif

        set_bottom_single_right_side_button();

        if (rsb_single_line_button(test_button_text)) {
            play_nested_level();
        }

        break;

    case GAME_MODE_PLAY_LEVEL:
        draw_name_header();

        if (rsb_single_line_button(options_button_text)) {
            set_game_mode(GAME_MODE_OPTIONS);
        }

#if defined(PLATFORM_DESKTOP)
        set_gui_narrow_font();
        if (rsb_single_line_button(savequit_button_text)) {
            savequit_current_level();
        }
        set_default_font();
#endif

        if (rsb_single_line_button(reset_button_text)) {
            reset_current_level();
        }

        if (nested_return_level) {
            if (rsb_single_line_button(retnest_button_text)) {
                return_from_nested_level();
            }
        } else {
            if (rsb_single_line_button(return_button_text)) {
                return_from_level();
            }
        }

        // skip one position
        shift_down_right_side_button(right_side_button.single_line_y_offset);

        rsb_undo_button();
        rsb_redo_button();

        break;

    case GAME_MODE_WIN_LEVEL:
        draw_name_header();
        draw_win_panels();
        game_mode_button(options_button_text,  GAME_MODE_OPTIONS, 1, 0);

        if (rsb_single_line_button(reset_button_text)) {
            reset_current_level();
        }

        if (rsb_single_line_button(return_button_text)) {
            return_from_level();
        }

        // skip two positions
        shift_down_right_side_button(right_side_button.double_line_y_offset);

        rsb_undo_button();
        rsb_redo_button();

        break;

    case GAME_MODE_EDIT_COLLECTION:
        if (quit_pressed) {
            if (current_collection && current_collection->changed) {
                show_ask_save_dialog(AFTER_DIALOG_SAVE_AND_QUIT,
                                     AFTER_DIALOG_QUIT);
                running = true;
            }
        }

        standard_buttons();

        set_gui_narrow_font();
        if (rsb_single_line_button(unedit_button_text)) {
            toggle_edit_mode();
        }
        set_default_font();

#if defined(PLATFORM_DESKTOP)
        if (current_collection && current_collection->changed) {
            if (rsb_single_line_button(save_button_text)) {
                show_ask_save_dialog(AFTER_DIALOG_SAVE,
                                     AFTER_DIALOG_NULL);
            }
        }
#endif
        break;

    case GAME_MODE_PLAY_COLLECTION:
        standard_buttons();

        if (rsb_single_line_button(edit_button_text)) {
            toggle_edit_mode();
        }

        break;

    default:
        break;
    }

    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, prev_align);
}
#undef gm_button

static void draw_solve_timer(void)
{
    const char *str = NULL;
    Color text_color = WHITE;
    Color border_color = { 0x77, 0xDF, 0x73, 0xcc };
    Color bg_color = shade_overlay_color;

    bool draw_border = false;
    float roundness = 0.3;

    if (solve_timer.state == SOLVE_TIMER_STATE_PAUSED) {
        text_color = LIGHTGRAY;
    }

    if (solve_timer.valid) {
        solve_timer_update(&solve_timer);

        if (solve_timer.elapsed_time.day > 0) {
            str = TextFormat("%d %s, %02d:%02d:%02d.%03d",
                             solve_timer.elapsed_time.day,
                             ((solve_timer.elapsed_time.day == 1) ? "day" : "days"),
                             solve_timer.elapsed_time.hr,
                             solve_timer.elapsed_time.min,
                             solve_timer.elapsed_time.sec,
                             solve_timer.elapsed_time.ms);
        } else if (solve_timer.elapsed_time.hr > 0) {
            str = TextFormat("%02d:%02d:%02d.%03d",
                             solve_timer.elapsed_time.hr,
                             solve_timer.elapsed_time.min,
                             solve_timer.elapsed_time.sec,
                             solve_timer.elapsed_time.ms);
        } else {
            str = TextFormat("%02d:%02d.%03d",
                             solve_timer.elapsed_time.min,
                             solve_timer.elapsed_time.sec,
                             solve_timer.elapsed_time.ms);
        }

        switch (game_mode) {
        case GAME_MODE_WIN_LEVEL:
            draw_border = true;
            bg_color = ColorAlpha(BLACK, 0.9);
            break;

        default:
            break;
        }


    } else {
        /* invalid */

        switch (game_mode) {
        case GAME_MODE_WIN_LEVEL:
            return;

        default:
            str = "--:--";
            text_color = DARKGRAY;
            break;
        }
    }


    Vector2 text_size = measure_panel_text(str);
    Rectangle bg_rec = {
        .x = (window_sizef.x / 2.0f) - 45.0f,
        .y = WINDOW_MARGIN,
        .width  = text_size.x,
        .height = text_size.y,
    };

    Vector2 text_pos = getVector2FromRectangle(bg_rec);
    bg_rec = ExpandRectangle(bg_rec, 5.0f);

    DrawRectangleRounded(bg_rec, roundness, 0, bg_color);

    if (draw_border) {
        DrawRectangleRoundedLines(bg_rec, roundness, 0, 2.0, border_color);
    }

    draw_panel_text(str, text_pos, text_color);
}


static void draw_cursor(void)
{
    IVector2 icon_pos;

    if (demo_mode) {
        if (play_level_mode) {
            icon_pos = mouse_position;
        } else {
            return;
        }
    } else {
        if (IsCursorOnScreen() && mouse_input_is_enabled()) {
            icon_pos = mouse_position;
        } else {
            icon_pos.x = GetMouseX();
            icon_pos.y = GetMouseY();
        }
    }

    int iconid = ICON_CURSOR_POINTER;
    int icon_scale = options->cursor_scale;

    switch (current_mouse_cursor) {
    case MOUSE_CURSOR_POINTING_HAND:
        iconid = ICON_CURSOR_HAND;
        icon_pos.x -= 2 * options->cursor_scale;;
        icon_pos.y -= 1 * options->cursor_scale;;
        break;

    default:
        // do nothing */
        break;
    }

    bool save_locked = GuiIsLocked();
    GuiUnlock();

    GuiDrawIcon(iconid, icon_pos.x + 1, icon_pos.y + 1, icon_scale, cursor_shadow_color);
    GuiDrawIcon(iconid, icon_pos.x, icon_pos.y, icon_scale, cursor_color);

    if (save_locked) {
            GuiLock();
    }
}

static void draw_feedback_bg(void)
{
    Rectangle src = {
        .x      = 0.0f,
        .y      = 0.0f,
        .width  = (float) scene_read_target->texture.width,
        .height = (float)-scene_read_target->texture.height,
    };
    Rectangle dst = {
        .x      = -feedback_bg_zoom_margin.x + window_center.x,
        .y      = -feedback_bg_zoom_margin.y + window_center.y,
        .width  = (float)scene_read_target->texture.width  + (2.0f * feedback_bg_zoom_margin.x),
        .height = (float)scene_read_target->texture.height + (2.0f * feedback_bg_zoom_margin.y),
    };
    float rot = sin(current_time * 0.17) * (TAU / 36.0);
    DrawTexturePro(scene_read_target->texture, src, dst, window_center, rot, feedback_bg_tint_color);
}

static void draw_gui_tooltip(void)
{
    if (!tooltip_text) {
        return;
    }

    if (!options->show_tooltips) {
        return;
    }

    Vector2 text_size = MeasureTextEx(GuiGetFont(),
                                      tooltip_text,
                                      (float)GuiGetStyle(DEFAULT, TEXT_SIZE),
                                      (float)GuiGetStyle(DEFAULT, TEXT_SPACING));

    if ((tooltip_bounds.x + text_size.x + 16) > window_size.x) {
        tooltip_bounds.x -= (text_size.x + 16 - tooltip_bounds.width);
    }

    Rectangle panel_bounds = {
        .x      = tooltip_bounds.x,
        .y      = tooltip_bounds.y + tooltip_bounds.height + 4,
        .width  = text_size.x + 16,
        .height = GuiGetStyle(DEFAULT, TEXT_SIZE) + 8.f
    };
    GuiPanel(panel_bounds, NULL);

    int save_padding   = GuiGetStyle(LABEL, TEXT_PADDING);
    int save_alignment = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
    GuiSetStyle(LABEL, TEXT_PADDING, 0);
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

    Rectangle label_bounds = {
        .x      = tooltip_bounds.x,
        .y      = tooltip_bounds.y + tooltip_bounds.height + 4,
        .width  = text_size.x + 16,
        .height = GuiGetStyle(DEFAULT, TEXT_SIZE) + 8.f
    };
    GuiLabel(label_bounds, tooltip_text);

    GuiSetStyle(LABEL, TEXT_ALIGNMENT, save_alignment);
    GuiSetStyle(LABEL, TEXT_PADDING,   save_padding);

    tooltip_text = NULL;
}

static void draw_gui(void)
{
    switch (game_mode) {
    case GAME_MODE_TITLE:
        draw_gui_title();
        break;

    case GAME_MODE_BROWSER:
        draw_gui_browser();
        break;

    case GAME_MODE_OPTIONS:
        draw_gui_options();
        break;

    case GAME_MODE_RANDOM:
        draw_gui_random();
        break;

    case GAME_MODE_PLAY_COLLECTION:
        /* fall through */
    case GAME_MODE_EDIT_COLLECTION:
        draw_gui_collection();
        break;

    default:
        /* do nothing */
        break;
    }

    draw_gui_widgets();
    draw_gui_help();
    draw_gui_dialog();
    draw_gui_tooltip();
    draw_gui_popup_message();
}

static bool
render_frame(
    void
) {
    float fxmag = (sinf(current_time) + 1.0f) / 2.0;

    static bool renderd_texture_last_frame = false;

    distort_amount = sqrt(bloom_amount);
    postprocessing_effect_amount1[0] = bloom_amount;
    postprocessing_effect_amount1[1] = distort_amount;
    postprocessing_effect_amount1[2] = warp_amount;
    postprocessing_effect_amount1[3] = 0.0f;

    postprocessing_effect_amount2[0] = current_level ? current_level->extra_rotate_level : 0.0f;
    postprocessing_effect_amount2[1] = fxmag;
    postprocessing_effect_amount2[2] = 0.0f;
    postprocessing_effect_amount2[3] = 0.0f;

    //SetShaderValue(win_border_shader, win_border_shader_loc.effect_amount1, &(postprocessing_effect_amount1[0]), SHADER_UNIFORM_VEC4);
    //SetShaderValue(win_border_shader, win_border_shader_loc.effect_amount2, &(postprocessing_effect_amount2[0]), SHADER_UNIFORM_VEC4);

    SetShaderValue(postprocessing_shader, postprocessing_shader_loc.effect_amount1, &(postprocessing_effect_amount1[0]), SHADER_UNIFORM_VEC4);
    SetShaderValue(postprocessing_shader, postprocessing_shader_loc.effect_amount2, &(postprocessing_effect_amount2[0]), SHADER_UNIFORM_VEC4);

    SetShaderValue(postprocessing_shader, postprocessing_shader_loc.time, &current_time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(background_shader, background_shader_loc.time, &current_time, SHADER_UNIFORM_FLOAT);

    bool do_postprocessing_this_frame = do_postprocessing && options->use_postprocessing;

    if (do_postprocessing_this_frame) {
        BeginTextureMode(*scene_write_target);
    } else {
        BeginDrawing();
    }

    {
        ClearBackground(BLACK);

        if (do_postprocessing_this_frame && renderd_texture_last_frame) {
            draw_feedback_bg();
        }

        BeginShaderMode(background_shader);
        {
            background_draw(background);
        }
        EndShaderMode();

        switch (game_mode) {
        case GAME_MODE_WIN_LEVEL:
            /* fall thrugh */
        case GAME_MODE_PLAY_LEVEL:
            if (current_level) {
                level_draw(current_level, level_finished);
            }
            if (options->use_solve_timer) {
                draw_solve_timer();
            }
            break;

        case GAME_MODE_EDIT_LEVEL:
            if (current_level) {
                level_draw(current_level, false);
            }
            break;

        default:
            /* do nothing */
            break;
        }
    }

    if (do_postprocessing_this_frame) {
        EndTextureMode();

        BeginDrawing();
        {
            BeginShaderMode(postprocessing_shader);
            {
                Rectangle src_rect = {
                    .x      = 0.0f,
                    .y      = 0.0f,
                    .width  = (float) scene_write_target->texture.width,
                    .height = (float)-scene_write_target->texture.height
                };
                Vector2 position = {
                    .x = 0.0f,
                    .y = 0.0f
                };
                DrawTextureRec(scene_write_target->texture, src_rect, position, WHITE);
            }
            EndShaderMode();

#if 0
            DrawTextShadow(TextFormat("PostProcFX %s", do_postprocessing_this_frame ? "ON" : "OFF"),
                           15, window_size.y - 15 - DEFAULT_GUI_FONT_SIZE - DEFAULT_GUI_FONT_SIZE,
                           DEFAULT_GUI_FONT_SIZE, WHITE);
#endif
        }

        renderd_texture_last_frame = true;
    } else {
        renderd_texture_last_frame = false;
    }

    if (!demo_mode) {
        draw_gui();

        switch (game_mode) {
        case GAME_MODE_WIN_LEVEL:
            /* fall thrugh */
        case GAME_MODE_PLAY_LEVEL:
            if (options->use_solve_timer) {
                draw_solve_timer();
            }
            break;

        default:
            break;
        }
    }

    if (show_fps) {
        DrawTextShadow(TextFormat("FPS: %d", GetFPS()), 15, 10, DEFAULT_GUI_FONT_SIZE, WHITE);
    }

#ifdef DEBUG_ID_AND_DIR
    DrawTextShadow(TextFormat("debug: id=%d dir=%d", debug_id, debug_dir), 15,
                   window_size.y - DEFAULT_GUI_FONT_SIZE - DEFAULT_GUI_FONT_SIZE,
                   DEFAULT_GUI_FONT_SIZE, WHITE);
#endif

    draw_cursor();

    EndDrawing();

    swap_scene_targets();

    return true;
}

static void early_frame_setup(void)
{
    double_current_time = GetTime();
    current_time = (float)double_current_time;

    set_mouse_cursor(MOUSE_CURSOR_DEFAULT);

    if (current_level && current_level->solver) {
        solver_update(current_level->solver);
    }

    if (level_finished) {
        SetShaderValue(win_border_shader, win_border_shader_loc.time, &current_time, SHADER_UNIFORM_FLOAT);
    }

    if (gui_dialog_active() || show_help_box) {
        modal_ui_active = true;
        GuiLock();
    } else {
        modal_ui_active = false;
        GuiUnlock();
    }
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

static const char *raylih_log_type_string(int logLevel)
{
    switch (logLevel) {
    case LOG_TRACE:   return "TRACE";
    case LOG_DEBUG:   return "DEBUG";
    case LOG_INFO:    return "INFO";
    case LOG_WARNING: return "WARNING";
    case LOG_ERROR:   return "ERROR";
    case LOG_FATAL:   return "FATAL";
    default:
        __builtin_unreachable();
        return "(NULL)";
    }
}

void raylib_trace_log_cb(int logLevel, const char *text, va_list args)
{
    if (logLevel < raylib_log_level) {
        return;
    }

    const char *type = raylih_log_type_string(logLevel);

    printf("raylib: %s: ", type);
    vprintf(text, args);
    putchar('\n');
    fflush(stdout);

    if (logLevel == LOG_FATAL) {
        exit(EXIT_FAILURE);
    }
}

void gfx_init(void)
{
    if (options->verbose) {
        SetTraceLogLevel(LOG_INFO);
        raylib_log_level = LOG_INFO;
    } else {
        SetTraceLogLevel(LOG_WARNING);
        raylib_log_level = LOG_WARNING;
    }

    SetTraceLogCallback(raylib_trace_log_cb);

    unsigned int flags = 0;
    flags |= FLAG_VSYNC_HINT;
#ifdef WINDOW_RESIZE_ENABLED
    flags |= FLAG_WINDOW_RESIZABLE;
#endif
    flags |= FLAG_MSAA_4X_HINT;
    SetConfigFlags(flags);

    InitWindow(window_size.x, window_size.y, "Hex Puzzle");

#if defined(PLATFORM_DESKTOP)
#ifdef WINDOW_RESIZE_ENABLED
    SetWindowMinSize(OPTIONS_WINDOW_MIN_WIDTH,
                     OPTIONS_WINDOW_MIN_HEIGHT);
    SetWindowMaxSize(OPTIONS_WINDOW_MAX_WIDTH,
                     OPTIONS_WINDOW_MAX_HEIGHT);
    SetWindowAspectRatio(OPTIONS_DEFAULT_INITIAL_WINDOW_WIDTH,
                         OPTIONS_DEFAULT_INITIAL_WINDOW_HEIGHT);
#endif
#endif

    SetExitKey(KEY_NULL); // handle ESC ourself
#if defined(PLATFORM_DESKTOP)
    SetTargetFPS(options->max_fps);
    if (options->verbose) {
        infomsg("Target FPS: %d", options->max_fps);
    }
#endif

    //set_mouse_cursor(MOUSE_CURSOR_CROSSHAIR);
    HideCursor();

    load_fonts();

    GuiLoadStyleDark();

    set_gui_font();

    GuiSetStyle(DEFAULT, TEXT_PADDING, 4);
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DROPDOWNBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DROPDOWNBOX, TEXT_PADDING, 10);

    GuiEnableTooltip();

    if (options->wait_events) {
        if (options->verbose) {
            infomsg("Disabling automatic event polling.");
        }
        EnableEventWaiting();
        event_waiting_active = true;
    }

    prepare_global_colors();

    load_shaders();
    load_textures();

    set_uniform_resolution();

    rlSetBlendFactors(RL_CONSTANT_ALPHA, RL_ONE_MINUS_CONSTANT_ALPHA, RL_FUNC_ADD);
    rlEnableColorBlend();
}

#if !defined(PLATFORM_WEB)
static void
gfx_cleanup(
    void
) {
    rlDisableColorBlend();

    unload_textures();
    unload_shaders();
    unload_render_textures();

    GuiUnloadStyleDark();

    unload_fonts();
    CloseWindow();
}
#endif

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
    init_gui_help();
    init_gui_collection();
    init_gui_random();
    init_gui_title();
    init_gui_dialog();
    init_gui_popup_message();

    init_solve_timer(&solve_timer);

    background = create_background();

    //set_game_mode(GAME_MODE_BROWSER);
    //set_game_mode(GAME_MODE_RANDOM);
    set_game_mode(GAME_MODE_TITLE);

    win_anim_mode_config_reset_to_defaut(&(win_anim_mode_config[0]));

    load_nvdata();
}

#if defined(PLATFORM_DESKTOP)
static void start_given_file(void)
{
    switch (options->startup_action) {
    case STARTUP_ACTION_PLAY:
        play_game_file(options->file_path);
        break;

    case STARTUP_ACTION_RANDOM:
        play_random_game();
        break;

    case STARTUP_ACTION_EDIT:
        edit_game_file(options->file_path);
        break;

    case STARTUP_ACTION_DEMO_WIN_ANIM:
        options->cheat_autowin = true;
        warnmsg("DEMO WIN ANIM ENABLED: auto-win levels");
        /* fall through */
    case STARTUP_ACTION_DEMO_SOLVE:
        demo_mode = true;
        warnmsg("DEMO MODE ENABLED");

        play_random_game();

        create_or_use_solver(current_level);
        solver_toggle_solve(current_level->solver);
        break;

    default:
        if (options->extra_argc == 1) {
            char *filename = options->extra_argv[0];
            open_game_file(filename, false);
        }
        break;
    }
}
#endif


#if !defined(PLATFORM_WEB)
static void game_cleanup(void)
{
    save_nvdata();

    cleanup_gui_popup_message();
    cleanup_gui_dialog();
    cleanup_gui_random();
    cleanup_gui_collection();
    cleanup_gui_browser();
    cleanup_gui_help();
    cleanup_gui_title();
    cleanup_nvdata();
    cleanup_gui_options();

    destroy_background(background);
}
#endif

int
main(
    int   argc,
    char *argv[]
) {
    srand48((long int)time(NULL));
    srand(time(NULL));
    seed_global_rng();

#if defined(PLATFORM_DESKTOP)
    progname = basename(argv[0]);

    char *xdg_config_dir = getenv("XDG_CONFIG_HOME");
    home_dir = getenv("HOME");
    if (xdg_config_dir) {
        safe_asprintf(&config_dir, "%s/%s", xdg_config_dir, CONFIG_SUBDIR_NAME);
    } else {
        safe_asprintf(&config_dir, "%s/.config/%s", home_dir, CONFIG_SUBDIR_NAME);
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
        errmsg("bad args");
        return EXIT_FAILURE;
    }

#if defined(PLATFORM_DESKTOP)
    if (run_startup_action()) {
        if (startup_action_ok) {
            return EXIT_SUCCESS;
        } else {
            return EXIT_FAILURE;
        }
    }
#endif

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

    start_given_file();

#if 0
    open_classics_game_pack(1);
    level_play(current_collection->levels);
#endif
#if 0
    open_classics_game_pack(2);
    set_game_mode(GAME_MODE_EDIT_COLLECTION);
    level_edit(current_collection->levels);
#endif

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

