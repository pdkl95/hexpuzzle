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
options_t *options = NULL;
bool event_waiting_active = false;
bool window_size_changed = false;
bool first_resize = true;
bool skip_next_resize_event = false;
double resize_delay = 0.5;
double resize_time = 0.0;
IVector2 window_size;
IVector2 mouse_position;
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

bool edit_mode = true;
grid_t *grid = NULL;
level_t *current_level = NULL;

#define MOUSE_TEXT_MAX_LINES 8
#define MOUSE_TEXT_MAX_LINE_LENGTH 60
char mouse_text[MOUSE_TEXT_MAX_LINES][MOUSE_TEXT_MAX_LINE_LENGTH];
int mouse_text_idx = 0;
int mouse_text_max_line_length = 0;
int mouse_text_font_size = 20;

void gui_setup(void);

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
    grid_resize(grid);
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

    if (IsKeyPressed(KEY_ESCAPE)) {
        //infomsg("etc - quit");
        return false;
    }

    if (IsKeyPressed(KEY_F5)) {
        edit_mode = !edit_mode;
    }

    if (IsKeyPressed(KEY_F8)) {
        grid_serialize(grid, stdout);
    }

    if (IsWindowResized()) {
        if (skip_next_resize_event) {
            skip_next_resize_event = false;
        } else {
            schedule_resize();
        }
    }

    //IVector2 old_mouse = mouse_position;
    mouse_position.x = GetMouseX();
    mouse_position.y = GetMouseY();

    grid_set_hover(grid, mouse_position);

    mouse_text_idx = 0;
    mouse_text_max_line_length = 0;

    bool any_zoom_active = false;

    if (IsCursorOnScreen()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            grid_drag_start(grid);
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            grid_drag_stop(grid);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (edit_mode) {
                grid_modify_hovered_feature(grid);
            }
        }
    }

    if (options->wait_events) {
        if (event_waiting_active) {
            /* event waiting currently ON */
            if (any_zoom_active) {
                // disablw waiting during user interaction
                DisableEventWaiting();
                event_waiting_active = false;
            }
        } else {
            /* event waiting currently OFF */
            if (!any_zoom_active) {
                EnableEventWaiting();
                event_waiting_active = true;
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

static void draw_tiles(void)
{
    grid_draw(grid);
}

#ifndef RAYGUI_ICON_SIZE
#define RAYGUI_ICON_SIZE 16
#endif

#define WINDOW_MARGIN RAYGUI_ICON_SIZE

#define BUTTON_MARGIN 4
#define ICON_BUTTON_SIZE (RAYGUI_ICON_SIZE + (2 * BUTTON_MARGIN))

Rectangle close_button_rect;
Rectangle edit_button_rect;
char close_button_text[6];
char edit_button_text[6];

void gui_setup(void)
{
    close_button_rect.x      = window_size.x - WINDOW_MARGIN - ICON_BUTTON_SIZE;
    close_button_rect.y      = WINDOW_MARGIN;
    close_button_rect.width  = ICON_BUTTON_SIZE;
    close_button_rect.height = ICON_BUTTON_SIZE;

    memcpy(close_button_text, GuiIconText(ICON_CROSS, NULL), 6);

    edit_button_rect.x      = window_size.x - WINDOW_MARGIN - ICON_BUTTON_SIZE;
    edit_button_rect.y      = close_button_rect.y + close_button_rect.height + WINDOW_MARGIN;
    edit_button_rect.width  = ICON_BUTTON_SIZE;
    edit_button_rect.height = ICON_BUTTON_SIZE;
    
    memcpy(edit_button_text,  GuiIconText(ICON_TOOLS, NULL), 6);
}

static void draw_gui_widgets(void)
{
    if (GuiButton(close_button_rect, close_button_text)) {
        running = false;
    }

    if (GuiButton(edit_button_rect, edit_button_text)) {
        edit_mode = !edit_mode;
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
        draw_tiles();
        draw_gui_widgets();
        draw_popup_text();

        if (show_fps) {
            DrawTextShadow(TextFormat("FPS: %d", GetFPS()), 15, 10, 20, WHITE);
        }

        draw_mouse_text();
    }
    EndDrawing();

    return true;
}

static bool
main_event_loop(
    void
) {
    //infomsg("Entering main event loop...");

    while (running) {
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

    SetTargetFPS(options->max_fps);
    if (options->verbose) {
        infomsg("Target FPS: %d", options->max_fps);
    }

    SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);

    GuiLoadStyleTerminal();

    set_uniform_resolution();

    if (options->wait_events) {
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

bool set_current_level(level_t *level)
{
    assert_not_null(level);

    if (grid) {
        destroy_grid(grid);
        grid = NULL;
    }

    grid = level_create_grid(level);

    return (!!grid);
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
        level_t *lv = load_level_file(filename);
        if (lv) {
            set_current_level(lv);
        }
    }
}

static void game_cleanup(void)
{
    destroy_grid(grid);
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

