/******************************************************************************
 *                                                                            *
 *  main.c                                                                    *
 *                                                                            *
 *  This file is part of hexpuzzle.                                         *
 *                                                                            *
 *  hexpuzzle is free software: you can redistribute it and/or              *
 *  modify it under the terms of the GNU General Public License as published  *
 *  by the Free Software Foundation, either version 3 of the License,         *
 *  or (at your option) any later version.                                    *
 *                                                                            *
 *  hexpuzzle is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General  *
 *  Public License for more details.                                          *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License along   *
 *  with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.            *
 *                                                                            *
 ******************************************************************************/

#include "common.h"

#include <getopt.h>
#include <libgen.h>
#include <time.h>
#include <sys/stat.h>

#include "raygui/raygui.h"
#include "raygui/style/terminal.h"

#include "options.h"
#include "raylib_helper.h"
#include "grid.h"
#include "level.h"
#include "collection.h"

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
bool mouse_right_click = false;
float cursor_spin = 0.0f;
float cursor_spin_step = (360.0f / 100.0f);
int frame_count = 0;
int frame_delay;
bool show_fps = false;

float popup_text_fade_time = 2.5f;
float popup_text_active_until = 0.0f;
const char *popup_text;
Color popup_text_color = { 0xFF, 0xFA, 0xCD, 0xFF };

Color TRANSPARENT_BLACK  = { 0, 0, 0, 0 };
Color LIGHT_ORANGE       = { 255, 249, 187, 255 };
Color CHARTREUSE         = { 194, 241, 146, 255 };
Color DEEP_SKY_BLUE      = { 176, 224, 230, 255 };
Color DODGER_BLUE        = { 0x1E, 0x90, 0xFF, 255 };
Color DEEP_PINK          = { 0xFF, 0x14, 0x93, 255 };
Color CGOLD              = { 0xFF, 0xD7, 0x00, 255 };
Color cursor_outer_color;
Color cursor_inner_color;

game_mode_t game_mode = GAME_MODE_NULL;

grid_t *current_grid = NULL;
level_t *current_level = NULL;
collection_t *current_collection = NULL;

bool modal_ui_active = false;
ui_result_t modal_ui_result;
bool show_name_edit_box = false;
bool show_ask_save_box = false;

#define MOUSE_TEXT_MAX_LINES 8
#define MOUSE_TEXT_MAX_LINE_LENGTH 60
char mouse_text[MOUSE_TEXT_MAX_LINES][MOUSE_TEXT_MAX_LINE_LENGTH];
int mouse_text_idx = 0;
int mouse_text_max_line_length = 0;
int mouse_text_font_size = 20;

void gui_setup(void);

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

    case GAME_MODE_COLLECTION:
        return "COLLECTION";

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
    case GAME_MODE_PLAY_LEVEL:
        game_mode = GAME_MODE_EDIT_LEVEL;
        break;

    case GAME_MODE_EDIT_LEVEL:
        game_mode = GAME_MODE_PLAY_LEVEL;
        break;

    default:
        /* do nothing */
        break;
    }
}

bool set_current_level(level_t *level)
{
    assert_not_null(level);

    if (current_grid) {
        destroy_grid(current_grid);
        current_grid = NULL;
    }

    current_grid = level_create_grid(level);
    grid_resize(current_grid);

    current_level = level;

    return (!!current_grid);
}

void return_from_level(void)
{
    if (current_grid) {
        destroy_grid(current_grid);
        current_grid = NULL;
    }
    current_level = NULL;
    game_mode = GAME_MODE_COLLECTION;
}

void create_new_level(void)
{
    level_t *level = create_level();
    collection_add_level(current_collection, level);
    set_current_level(level);

    game_mode = GAME_MODE_EDIT_LEVEL;

    show_name_edit_dialog();
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
    //float resolution[2] = { (float)window_size.x, (float)window_size.y };
    //SetShaderValue(mandel_to_texture_prog, mandel_to_texture_loc.resolution, resolution, SHADER_UNIFORM_VEC2);
    //SetShaderValue(sobel_prog, sobel_loc.resolution, resolution, SHADER_UNIFORM_VEC2);
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
    if (current_grid) {
        grid_resize(current_grid);
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

double modifier_key_adjust(double value)
{
    if (IsKeyDown(KEY_LEFT_SHIFT) ||
        IsKeyDown(KEY_RIGHT_SHIFT)) {
        value *= 1.0/3.0;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) ||
        IsKeyDown(KEY_RIGHT_CONTROL)) {
        value *= 1.0/5.0;
    }

    if (IsKeyDown(KEY_LEFT_ALT) ||
        IsKeyDown(KEY_RIGHT_ALT)) {
        value *= 1.0/10.0;
    }

    if (IsKeyDown(KEY_LEFT_SUPER) ||
        IsKeyDown(KEY_RIGHT_SUPER)) {
        value *= 1.0/100.0;
    }

    return value;
}

static bool
handle_events(
    void
) {
    if (WindowShouldClose()) {
        //infomsg("Window Closed");
        return false;
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
        return false;
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

    if (IsKeyPressed(KEY_F5)) {
        toggle_edit_mode();
    }

    if (IsKeyPressed(KEY_F8)) {
        grid_serialize(current_grid, stdout);
    }

    //IVector2 old_mouse = mouse_position;
    mouse_position.x = GetMouseX();
    mouse_position.y = GetMouseY();

    mouse_positionf.x = (float)mouse_position.x;
    mouse_positionf.y = (float)mouse_position.y;

    grid_set_hover(current_grid, mouse_position);

    mouse_text_idx = 0;
    mouse_text_max_line_length = 0;

    mouse_left_click  = false;;
    mouse_right_click = false;;

    if (IsCursorOnScreen()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            mouse_left_click = true;
            if (current_grid) {
                grid_drag_start(current_grid);
            }
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (current_grid) {
                grid_drag_stop(current_grid);
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            mouse_right_click = true;
            if (edit_mode) {
                if (current_grid) {
                    grid_modify_hovered_feature(current_grid);
                }
            }
        }
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

#ifndef RAYGUI_ICON_SIZE
#define RAYGUI_ICON_SIZE 16
#endif

#define WINDOW_MARGIN RAYGUI_ICON_SIZE
#define PANEL_INNER_MARGIN 12

#define BUTTON_MARGIN 4
#define ICON_BUTTON_SIZE (RAYGUI_ICON_SIZE + (2 * BUTTON_MARGIN))

Rectangle info_panel_rect;
Rectangle name_text_rect;
Rectangle name_edit_button_rect;
Rectangle close_button_rect;
Rectangle edit_button_rect;
Rectangle return_button_rect;
Rectangle new_level_button_rect;
char name_edit_button_text[6];
char close_button_text[6];
char edit_button_text[6];
char return_button_text[6];
char new_level_button_text[6];

char cancel_ok_with_icons[25];
char no_yes_with_icons[25];

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

    info_panel_rect.x      = WINDOW_MARGIN;
    info_panel_rect.y      = WINDOW_MARGIN;

    name_text_rect.x = WINDOW_MARGIN * PANEL_INNER_MARGIN;
    name_text_rect.y = WINDOW_MARGIN + PANEL_INNER_MARGIN;
    name_text_rect.width = window_size.x * 0.22;
    name_text_rect.height = 30;

    name_edit_button_rect.x = name_text_rect.x + name_text_rect.width + ICON_BUTTON_SIZE;
    name_edit_button_rect.y = name_text_rect.y;

    name_edit_button_rect.width  = ICON_BUTTON_SIZE;
    name_edit_button_rect.height = ICON_BUTTON_SIZE;

    info_panel_rect.width  =
        name_edit_button_rect.x
        + name_edit_button_rect.width
        + PANEL_INNER_MARGIN
        - info_panel_rect.x;

    info_panel_rect.height =
        name_text_rect.y
        + MAX(name_text_rect.height, name_edit_button_rect.height)
        + (2 * PANEL_INNER_MARGIN)
        - info_panel_rect.y;;

    memcpy(name_edit_button_text, GuiIconText(ICON_PENCIL, NULL), 6);

    close_button_rect.x      = window_size.x - WINDOW_MARGIN - ICON_BUTTON_SIZE;
    close_button_rect.y      = WINDOW_MARGIN;
    close_button_rect.width  = ICON_BUTTON_SIZE;
    close_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(close_button_text, GuiIconText(ICON_EXIT, NULL), 6);

    edit_button_rect.x      = close_button_rect.x;
    edit_button_rect.y      = close_button_rect.y + close_button_rect.height + WINDOW_MARGIN;
    edit_button_rect.width  = ICON_BUTTON_SIZE;
    edit_button_rect.height = ICON_BUTTON_SIZE;
    
    memcpy(edit_button_text,  GuiIconText(ICON_TOOLS, NULL), 6);

    return_button_rect.x      = edit_button_rect.x;
    return_button_rect.y      = edit_button_rect.y + edit_button_rect.height + WINDOW_MARGIN;
    return_button_rect.width  = ICON_BUTTON_SIZE;
    return_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(return_button_text,  GuiIconText(ICON_UNDO_FILL, NULL), 6);

    new_level_button_rect.x      = return_button_rect.x;
    new_level_button_rect.y      = return_button_rect.y + return_button_rect.height + WINDOW_MARGIN;
    new_level_button_rect.width  = ICON_BUTTON_SIZE;
    new_level_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(new_level_button_text,  GuiIconText(ICON_FILE_ADD, NULL), 6);
}

#define NAME_FONT_SIZE 22
#define NAME_PANEL_ROUNDNES 0.2

Color name_header_panel_bg_color   = { 0x72, 0x1C, 0xB8, 0xaa };
Color name_header_panel_edge_color = { 0x94, 0x83, 0xA2, 0xcc };
Color name_header_text_color       = { 0xD0, 0xC0, 0xFF, 0xff };

static void draw_name_header(char *name)
{
    int textwidth = MeasureText(name, NAME_FONT_SIZE);

    Rectangle text_rect = {
        .x = WINDOW_MARGIN + PANEL_INNER_MARGIN,
        .y = WINDOW_MARGIN + PANEL_INNER_MARGIN,
        .width  = textwidth,
        .height = NAME_FONT_SIZE
    };

    Rectangle panel_rect = {
        .x = WINDOW_MARGIN,
        .y = WINDOW_MARGIN,
        .width  = textwidth      + (2 * PANEL_INNER_MARGIN),
        .height = NAME_FONT_SIZE + (2 * PANEL_INNER_MARGIN)
    };

    //printf("panel_rect\n"); printrect(panel_rect);
    //printf(" text_rect\n"); printrect(text_rect);

    bool hover = CheckCollisionPointRec(mouse_positionf, text_rect);
    if (hover && mouse_left_click) {
        show_name_edit_dialog();
    }

    Color bg   = hover ? name_header_panel_edge_color : name_header_panel_bg_color;
    Color edge = hover ? name_header_panel_bg_color : name_header_panel_edge_color;
    DrawRectangleRounded(panel_rect, NAME_PANEL_ROUNDNES, 0, bg);
    DrawRectangleRoundedLines(panel_rect, NAME_PANEL_ROUNDNES, 0, 1.0, edge);

    DrawText(name, text_rect.x, text_rect.y, NAME_FONT_SIZE, name_header_text_color);
}

static void draw_gui_widgets(void)
{
    if (GuiButton(close_button_rect, close_button_text)) {
        running = false;
    }

    if (GuiButton(edit_button_rect, edit_button_text)) {
        toggle_edit_mode();
    }

    switch (game_mode) {
    case GAME_MODE_EDIT_LEVEL:
        draw_name_header(current_level->name);

        if (GuiButton(return_button_rect, return_button_text)) {
            printf("return\n");
            show_ask_save_box = true;
        }
        break;

    case GAME_MODE_PLAY_LEVEL:
        draw_name_header(current_level->name);

        if (GuiButton(return_button_rect, return_button_text)) {
            return_from_level();
        }
        break;

    case GAME_MODE_COLLECTION:
        if (GuiButton(new_level_button_rect, new_level_button_text)) {
            create_new_level();
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
        const char *icon = GuiIconText(ICON_PENCIL, "Edit Level Name");

        int result = GuiTextInputBox(edit_box_rect,
                                     icon,
                                     "Level Name:",
                                     cancel_ok_with_icons,
                                     current_level->name,
                                     NAME_MAXLEN,
                                     NULL);

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
    if (show_ask_save_box) {
        Rectangle edit_box_rect = {
            (float)GetScreenWidth()/2 - 120,
            (float)GetScreenHeight()/2 - 60,
            240,
            140
        };

        const char *iconmsg = GuiIconText(ICON_FILE_SAVE_CLASSIC, "Save Level?");
        int result = GuiMessageBox(edit_box_rect,
                                   iconmsg,
                                   "Save changes to level?",
                                   no_yes_with_icons);

        if ((result == 2) || (modal_ui_result == UI_RESULT_OK)) {
            /* yes */
            printf("collection_extract_level_from_grid()\n");
            level_extract_from_grid(current_level, current_grid);
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
}

static void draw_popup_panels(void)
{
    draw_name_edit_dialog();
    draw_ask_save_dialog();
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
    Color minor_color = { 112, 31, 126, 128 };

    for (int x=0; x<window_size.x; x += 25) {
        DrawLine(x, 0, x, window_size.y, minor_color);
    }

    for (int y=0; y<window_size.y; y += 25) {
        DrawLine(0, y, window_size.x, y, minor_color);
    }

    for (int x=0; x<window_size.x; x += 100) {
        DrawLine(x, 0, x, window_size.y, DARKPURPLE);
        if (draw_labels) {
            DrawText(TextFormat("%d", x), (float)x + 3.0, 8.0, 16, YELLOW);
        }
    }

    for (int y=0; y<window_size.y; y += 100) {
        DrawLine(0, y, window_size.x, y, DARKPURPLE);
        if (draw_labels) {
            DrawText(TextFormat("%d", y), 3.0, (float)y + 3.9, 16, YELLOW);
        }
    }
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
            /* fall through */
        case GAME_MODE_EDIT_LEVEL:
            if (current_grid) {
                grid_draw(current_grid);
            }
            break;

        case GAME_MODE_COLLECTION:
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
    if (show_name_edit_box ||
        show_ask_save_box
    ) {
        modal_ui_active = true;
    } else {
        modal_ui_active = false;
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
    //flags |= FLAG_MSAA_4X_HINT;
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

    SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);

    GuiLoadStyleTerminal();

    set_uniform_resolution();

    if (options->wait_events) {
        if (options->verbose) {
            infomsg("Disabling automatic event polling.");
        }
        EnableEventWaiting();
        event_waiting_active = true;
    }

    //cursor_outer_color = DODGER_BLUE;
    cursor_outer_color = CGOLD;
    cursor_outer_color = ColorAlpha(cursor_outer_color, 0.86);

    cursor_inner_color = DEEP_PINK;
    cursor_inner_color = ColorAlpha(cursor_inner_color, 0.65);
}

static void
gfx_cleanup(
    void
) {
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
        if (options->verbose) {
            infomsg("Loading: \"%s\"", filename);
        }
        current_collection = load_collection_path(filename);
        if (IS_LEVEL_FILENAME(filename)) {
            level_t *level = collection_find_level_by_filename(current_collection, filename);
            if (level) {
                set_current_level(level);
            }
        } else if (IS_COLLECTION_FILENAME(filename)) {
            game_mode = GAME_MODE_COLLECTION;
        } else if (current_collection->dirpath) {
            game_mode = GAME_MODE_COLLECTION;
        } else {
            assert(0);
        }
    }
}

static void game_cleanup(void)
{
    destroy_grid(current_grid);
}

int
main(
    int   argc,
    char *argv[]
) {
    progname = basename(argv[0]);

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

    bool run_ok = main_event_loop();

    game_cleanup();
    gfx_cleanup();

    destroy_options(options);

    if (run_ok) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

