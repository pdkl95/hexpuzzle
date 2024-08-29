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

#include "raygui/raygui.h"
#include "raygui/style/terminal.h"

#include "options.h"
#include "raylib_helper.h"
#include "grid.h"

#if defined(PLATFORM_DESKTOP)
/* good */
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
# error "Only PLATFORM_DESKTOP (GLSL 330) supported!"
#endif

const char *progversion = PACKAGE_VERSION;
const char *progname    = PACKAGE_NAME;

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

extern Vector2 tile_origin;
grid_t *grid;

tile_t *drag_target = NULL;
IVector2 drag_start;
IVector2 drag_offset;

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

    infomsg("scheduled resize at %f (now = %f, delta = %f)",
            resize_time, curtime, resize_time - curtime);
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

    warnmsg("RESIZE to: %d x %d", window_size.x, window_size.y);

    set_uniform_resolution();
    create_textures();

    window_size_changed = false;
    resize_time = 0;

    if (first_resize) {
        first_resize = false;
        printf("resize: bypass\n");
    } else {
        printf("resize: request mandel redraw\n");
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
    warnmsg("RESIZE event (%f < %f)", curtime, resize_time);
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
        infomsg("Window Closed");
        return false;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        infomsg("etc - quit");
        return false;
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

    bool any_zoom_active = false;
    if (drag_target) {
        drag_offset.x = mouse_position.x - drag_start.x;
        drag_offset.y = mouse_position.y - drag_start.y;
    } else {
        drag_offset.x = 0;
        drag_offset.y = 0;
    }

    if (IsCursorOnScreen()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (drag_target) {
                // undo prev drag
            }
            //drag_target = testtile;
            drag_start = mouse_position;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (drag_target) {
                drag_target = NULL;
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

static void draw_tiles(void)
{
    Vector2 mpos = { (float)mouse_position.x, (float)mouse_position.y };
    Vector2 mouse_tile_pos = Vector2Subtract(mpos, tile_origin);
    hex_axial_t mouse_hex = pixel_to_hex_axial(mouse_tile_pos, grid->tile_size);
    grid_set_hover(grid, mouse_hex);

    grid_draw(grid);
    //tile_draw(testtile, ivector2_to_vector2(drag_offset));
}

static void draw_gui_widgets(void)
{
}

static bool
render_frame(
    void
) {
    BeginDrawing();
    {
        ClearBackground(BLACK);
        draw_tiles();
        draw_gui_widgets();

        if (IsCursorOnScreen()) {
            Vector2 center;
            center.x = (float)mouse_position.x;
            center.y = (float)mouse_position.y;

            cursor_spin += cursor_spin_step;
            while (cursor_spin >= 360.0) {
                cursor_spin -= 360.0;
            }
            float spinosc = 0.5 * (sinf(3.0f * cursor_spin * M_PI / 180.0f) + 1.0);

#define CURSOR_NUM_SECTORS 3
#define CURSOR_SECTOR_SIZE (360.0f / CURSOR_NUM_SECTORS)
#define CURSOR_RADIUS 12.0f
#define CURSOR_INNER_RADIUS 4.0f

            DrawRing(center,
                     CURSOR_RADIUS,
                     CURSOR_RADIUS + 1.5f,
                     0.0f, 360.0f,
                     0, cursor_outer_color);

            for (int i=0; i<CURSOR_NUM_SECTORS; i++) {
                float start_angle = ((float)i) * CURSOR_SECTOR_SIZE;
                start_angle += cursor_spin;
                float wedgesize = 2.5f + spinosc; // * 0.5;
                float end_angle = start_angle + (CURSOR_SECTOR_SIZE/wedgesize);
                DrawRing(center, CURSOR_INNER_RADIUS, CURSOR_RADIUS, start_angle, end_angle, 8, cursor_inner_color);
            }
        }

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

        if (show_fps) {
            DrawTextShadow(TextFormat("FPS: %d", GetFPS()), 15, 10, 20, WHITE);
        }
    }
    EndDrawing();

    return true;
}

static bool
main_event_loop(
    void
) {
    infomsg("Entering main event loop...");

    while (true) {
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

    InitWindow(window_size.x, window_size.y, "Mandelbrot Iterations");
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

static void game_init(void)
{
    tile_origin.x = (float)(window_size.x / 2);
    tile_origin.y = (float)(window_size.y / 2);

    grid = create_grid(2);
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

