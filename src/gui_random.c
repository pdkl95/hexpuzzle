/****************************************************************************
 *                                                                          *
 * gui_random.c                                                             *
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

#include "common.h"

#include <limits.h>

#include "options.h"
#include "gui_random.h"
#include "color.h"
#include "tile.h"
#include "tile_pos.h"
#include "tile_draw.h"
#include "level.h"
#include "gui_dialog.h"
#include "gui_popup_message.h"
#include "raylib_gui_numeric.h"
#include "generate_level.h"
#include "blueprint_string.h"

Rectangle gui_random_panel_rect;
Rectangle gui_random_area_rect;
Rectangle gui_random_play_button_rect;
Rectangle gui_random_radius_label_rect;
Rectangle gui_random_radius_left_button_rect;
Rectangle gui_random_radius_display_rect;
Rectangle gui_random_radius_right_button_rect;
Rectangle gui_random_save_button_rect;
Rectangle gui_random_color_label_rect;
Rectangle gui_random_symmetry_label_rect;
Rectangle gui_random_symmetry_button_none_rect;
Rectangle gui_random_symmetry_button_reflect_rect;
Rectangle gui_random_symmetry_button_rotate_rect;
Rectangle gui_random_symmetry_button_none_bg_rect;
Rectangle gui_random_symmetry_button_reflect_bg_rect;
Rectangle gui_random_symmetry_button_rotate_bg_rect;
Rectangle gui_random_seed_rect;
Rectangle gui_random_seed_bg_rect;
Rectangle gui_random_enter_seed_rect;
Rectangle gui_random_rng_seed_rect;
Rectangle gui_random_preview_rect;

Vector2 radius_display_text_location;
Vector2 radius_display_text_shadow_location;
Vector2 seed_text_location;

char gui_random_panel_text[] = "Random Level";
char gui_random_play_button_text[] = "Play";
char gui_random_continue_button_text[] = "Continue";
char gui_random_radius_label_text[] = "Tile Radius";
char gui_random_radius_left_button_text[6];
char gui_random_radius_right_button_text[6];
char gui_random_color_label_text[] = "Colors";
char gui_random_fixed_label_text[] = "Fixed Tiles";
char gui_random_hidden_label_text[] = "Hidden Tiles";
char gui_random_minimum_tile_density_label_text[] = "Path Density";
char gui_random_symmetry_label_text[] = "Symmetry";
char gui_random_symmetry_button_none_text[] = "#79#None";
char gui_random_symmetry_button_reflect_text[] = "#40#Reflect";
char gui_random_symmetry_button_rotate_text[] = "#77#Rotate";
char gui_random_seed_text[] = "RNG Seed";
char gui_random_rng_seed_text[] = "Randomize";
char gui_random_enter_seed_text_str[] = "Enter";
char *gui_random_enter_seed_text = NULL;

char gui_random_save_button_text_str[] = "Save";
#define GUI_RANDOM_SAVE_BUTTON_TEXT_LENGTH (6 + sizeof(gui_random_save_button_text_str))
char gui_random_save_button_text[GUI_RANDOM_SAVE_BUTTON_TEXT_LENGTH];

bool gui_random_color[PATH_TYPE_COUNT];
int gui_random_color_count = PATH_TYPE_COUNT - 1;

gui_int_range_t *gui_range_fixed  = NULL;
gui_int_range_t *gui_range_hidden = NULL;
raylib_gui_numeric_t *gui_random_density = NULL;
float gui_random_density_float = 0.0f;

bool played_level = false;
level_t *gui_random_level = NULL;
level_t *gui_random_level_preview = NULL;
uint64_t gui_random_seed;
char *gui_random_seed_str = NULL;

static void set_random_seed(int value)
{
    gui_random_seed = (uint64_t)value;
    if (gui_random_seed_str) {
        free(gui_random_seed_str);
    }
    safe_asprintf(&gui_random_seed_str, "%d", value);
}

static void new_random_seed(void)
{
    set_random_seed(rand());
}

static int rng_color_count(void)
{
    int count = 0;

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        if (gui_random_color[type]) {
            count++;
        }
    }

    return count;
}

static int update_rng_color_count(void)
{
    gui_random_color_count = rng_color_count();
    return gui_random_color_count;
}

static void toggle_color(path_type_t type)
{
    gui_random_color[type] = !gui_random_color[type];
    if (update_rng_color_count() < 1) {
        // cannot allow zero colors - undo the toggle
        gui_random_color[type] = !gui_random_color[type];
        update_rng_color_count();
    }
}

#if 0
static void enable_color(path_type_t type)
{
    gui_random_color[type] = true;
    update_rng_color_count();
}

static void disable_color(path_type_t type)
{
    gui_random_color[type] = false;
    if (update_rng_color_count() < 1) {
        // cannot allow zero colors
        gui_random_color[type] = true;;
        update_rng_color_count();
    }
}
#endif

static level_t *gen_random_level(const char *purpose)
{
    uint64_t seed = gui_random_seed;

    if (options->rng_seed_str) {
        if (!parse_random_seed_str(options->rng_seed_str, &seed)) {
            errmsg("RNG seed \"%s\" is empty or unusable", options->rng_seed_str);
            new_random_seed();
            warnmsg("Using random RNG seed %d instead!", gui_random_seed);
        }
    }

#if 0
    int num_colors = update_rng_color_count();
    printf(">>> colors enabled: %d\n", num_colors);
    for (path_type_t type=0; type<PATH_TYPE_COUNT; type++) {
        printf("\t- path_%d = %s\n", type, gen_param.color[type] ? "true" : "false");
    }
#else
    update_rng_color_count();
#endif

    generate_level_param_t param = {
        .mode = GENERATE_LEVEL_RANDOM,
        .seed = seed,
        .tile_radius = options->create_level_radius,
        .color = {
            gui_random_color[0],
            gui_random_color[1],
            gui_random_color[2],
            gui_random_color[3],
            gui_random_color[4]
        },
        .color_count = gui_random_color_count,
        .fixed  = {
            gui_range_fixed->range->min,
            gui_range_fixed->range->max
        },
        .hidden = {
            gui_range_hidden->range->min,
            gui_range_hidden->range->max
        },
        .symmetry_mode = options->create_level_symmetry_mode,
        .path_density = options->create_level_minimum_path_density
    };

    return generate_random_level(&param, purpose);
}

static void regen_level(void)
{
    if (gui_random_level) {
        destroy_level(gui_random_level);
    }

    gui_random_level = gen_random_level("regen");
    played_level = false;
}

static void gen_level_with_params(generate_level_param_t param)
{
    if (gui_random_level) {
        destroy_level(gui_random_level);
    }

    gui_random_level = generate_random_level(&param, "blueprint");
    played_level = false;
}

void regen_level_preview(void)
{
    assert_not_null(gui_random_level);

    if (gui_random_level_preview) {
        destroy_level(gui_random_level_preview);
    }

    gui_random_seed = gui_random_level->seed + 1;
    gui_random_level_preview = gen_random_level("preview");
}

void promote_preview_to_level(void)
{
    if (gui_random_level) {
        destroy_level(gui_random_level);
    }

    gui_random_level = gui_random_level_preview;
    gui_random_level_preview = NULL;
}

void init_gui_random_minimal(void)
{
    gui_random_level = NULL;

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        gui_random_color[type] = true; 
    }

    new_random_seed();
}

const char *gui_random_density_get_text(raylib_gui_numeric_t *gn)
{
    float value = numeric_to_float(gn->value) / 100.0f;
    return TextFormat("%3.2f", value);
}

void init_gui_random(void)
{
    init_gui_random_minimal();

    SAFEFREE(gui_random_enter_seed_text);

    gui_random_enter_seed_text = strdup(GuiIconText(ICON_PENCIL, NULL));

    memcpy(gui_random_save_button_text,
           GuiIconText(ICON_FILE_SAVE_CLASSIC, gui_random_save_button_text_str),
           GUI_RANDOM_SAVE_BUTTON_TEXT_LENGTH);

    gui_range_fixed     = create_gui_int_range(&options->create_level_fixed,
                                               gui_random_fixed_label_text,
                                               LEVEL_MIN_FIXED,
                                               LEVEL_MAX_FIXED);

    gui_range_hidden    = create_gui_int_range(&options->create_level_hidden,
                                               gui_random_hidden_label_text,
                                               LEVEL_MIN_HIDDEN,
                                               LEVEL_MAX_HIDDEN);

    gui_random_density = create_gui_numeric_int(gui_random_minimum_tile_density_label_text,
                                                &options->create_level_minimum_path_density,
                                                LEVEL_MIN_MINIMUM_PATH_DENSITY,
                                                LEVEL_MAX_MINIMUM_PATH_DENSITY,
                                                25);
    gui_random_density->get_text = gui_random_density_get_text;
}

void cleanup_gui_random(void)
{
    if (gui_random_density) {
        destroy_gui_numeric(gui_random_density);
        gui_random_density = NULL;
    }

    if (gui_random_level) {
        destroy_level(gui_random_level);
        gui_random_level = NULL;
    }

    if (gui_range_hidden) {
        destroy_gui_int_range(gui_range_hidden);
        gui_range_hidden = NULL;
    }

    if (gui_range_fixed) {
        destroy_gui_int_range(gui_range_fixed);
        gui_range_fixed = NULL;
    }

    SAFEFREE(gui_random_seed_str);
    SAFEFREE(gui_random_enter_seed_text);
}

void resize_gui_random(void)
{
    gui_random_panel_rect.width  = window_size.x * 0.4;
    gui_random_panel_rect.height = window_size.y * 0.45;

    MINVAR(gui_random_panel_rect.width,  480);
    MINVAR(gui_random_panel_rect.height, 550);

    gui_random_panel_rect.x = (window_size.x / 2) - (gui_random_panel_rect.width  / 2);
    gui_random_panel_rect.y = (window_size.y / 2) - (gui_random_panel_rect.height / 2);

    float panel_bottom = gui_random_panel_rect.y + gui_random_panel_rect.height;

    gui_random_area_rect.x      = gui_random_panel_rect.x + PANEL_INNER_MARGIN;
    gui_random_area_rect.y      = gui_random_panel_rect.y + PANEL_INNER_MARGIN + RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
    gui_random_area_rect.width  = gui_random_panel_rect.width - (2 * PANEL_INNER_MARGIN);
    gui_random_area_rect.height = panel_bottom - PANEL_INNER_MARGIN - gui_random_area_rect.y;

    memcpy(gui_random_radius_left_button_text,  GuiIconText(ICON_ARROW_LEFT_FILL, NULL), 6);
    memcpy(gui_random_radius_right_button_text, GuiIconText(ICON_ARROW_RIGHT_FILL, NULL), 6);

    Vector2 gui_random_radius_label_text_size = measure_gui_text(gui_random_radius_label_text);

    gui_random_radius_label_rect.x      = gui_random_area_rect.x;
    gui_random_radius_label_rect.y      = gui_random_area_rect.y;
    gui_random_radius_label_rect.width  = gui_random_radius_label_text_size.x + (4 * BUTTON_MARGIN);
    gui_random_radius_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_radius_left_button_rect.x      = gui_random_radius_label_rect.x + gui_random_radius_label_rect.width + RAYGUI_ICON_SIZE;;
    gui_random_radius_left_button_rect.y      = gui_random_radius_label_rect.y;
    gui_random_radius_left_button_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_radius_left_button_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_radius_display_rect.x      = gui_random_radius_left_button_rect.x + gui_random_radius_left_button_rect.width + BUTTON_MARGIN;
    gui_random_radius_display_rect.y      = gui_random_radius_left_button_rect.y;
    gui_random_radius_display_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_radius_display_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_radius_right_button_rect.x      = gui_random_radius_display_rect.x + gui_random_radius_display_rect.width + BUTTON_MARGIN;
    gui_random_radius_right_button_rect.y      = gui_random_radius_display_rect.y;
    gui_random_radius_right_button_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_radius_right_button_rect.height = TOOL_BUTTON_HEIGHT;

    radius_display_text_location.x = gui_random_radius_display_rect.x + 8;
    radius_display_text_location.y = gui_random_radius_display_rect.y + 3;
    gui_random_radius_display_rect.x      += 1;
    gui_random_radius_display_rect.width  -= 2;
    gui_random_radius_display_rect.height += 1;

    radius_display_text_shadow_location = radius_display_text_location;
    radius_display_text_shadow_location.x += 1.0f;
    radius_display_text_shadow_location.y += 1.0f;

    Vector2 gui_random_save_button_text_size = measure_gui_text(gui_random_save_button_text);

    gui_random_save_button_rect.x      = gui_random_area_rect.x + gui_random_area_rect.width - gui_random_save_button_text_size.x - PANEL_INNER_MARGIN;
    gui_random_save_button_rect.y      = gui_random_radius_right_button_rect.y;
    gui_random_save_button_rect.width  = gui_random_save_button_text_size.x;
    gui_random_save_button_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_area_rect.y      += gui_random_radius_label_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_radius_label_rect.height + RAYGUI_ICON_SIZE;

    Vector2 color_label_text_size = measure_gui_text(gui_random_color_label_text);
 
    gui_random_color_label_rect.x      = gui_random_area_rect.x;
    gui_random_color_label_rect.y      = gui_random_area_rect.y;
    gui_random_color_label_rect.width  = color_label_text_size.x;
    gui_random_color_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_area_rect.y      += gui_random_color_label_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_color_label_rect.height + RAYGUI_ICON_SIZE;

    gui_numeric_resize(gui_random_density, &gui_random_area_rect);

    Vector2 gui_random_symmetry_label_text_size = measure_gui_text(gui_random_symmetry_label_text);
    Vector2 gui_random_symmetry_button_none_text_size = measure_gui_text(gui_random_symmetry_button_none_text);
    Vector2 gui_random_symmetry_button_reflect_text_size = measure_gui_text(gui_random_symmetry_button_reflect_text);
    Vector2 gui_random_symmetry_button_rotate_text_size = measure_gui_text(gui_random_symmetry_button_rotate_text);

    gui_random_symmetry_label_rect.x      = gui_random_area_rect.x;
    gui_random_symmetry_label_rect.y      = gui_random_area_rect.y;
    gui_random_symmetry_label_rect.width  = gui_random_symmetry_label_text_size.x + (4 * BUTTON_MARGIN);
    gui_random_symmetry_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_symmetry_button_none_rect.x      = gui_random_symmetry_label_rect.x + gui_random_symmetry_label_rect.width + RAYGUI_ICON_SIZE;
    gui_random_symmetry_button_none_rect.y      = gui_random_symmetry_label_rect.y;
    gui_random_symmetry_button_none_rect.width  = gui_random_symmetry_button_none_text_size.x;
    gui_random_symmetry_button_none_rect.height = gui_random_symmetry_label_rect.height;

    gui_random_symmetry_button_reflect_rect.x      = gui_random_symmetry_button_none_rect.x + gui_random_symmetry_button_none_rect.width + RAYGUI_ICON_SIZE;
    gui_random_symmetry_button_reflect_rect.y      = gui_random_symmetry_label_rect.y;
    gui_random_symmetry_button_reflect_rect.width  = gui_random_symmetry_button_reflect_text_size.x;
    gui_random_symmetry_button_reflect_rect.height = gui_random_symmetry_label_rect.height;

    gui_random_symmetry_button_rotate_rect.x      = gui_random_symmetry_button_reflect_rect.x + gui_random_symmetry_button_reflect_rect.width + RAYGUI_ICON_SIZE;
    gui_random_symmetry_button_rotate_rect.y      = gui_random_symmetry_label_rect.y;
    gui_random_symmetry_button_rotate_rect.width  = gui_random_symmetry_button_rotate_text_size.x;
    gui_random_symmetry_button_rotate_rect.height = gui_random_symmetry_label_rect.height;

    float symmetry_highlight_thickness = BUTTON_SELECTED_HIGHLIGHT_THICKNESS;
    gui_random_symmetry_button_none_bg_rect    = ExpandRectangle(gui_random_symmetry_button_none_rect,
                                                                 symmetry_highlight_thickness);
    gui_random_symmetry_button_reflect_bg_rect = ExpandRectangle(gui_random_symmetry_button_reflect_rect,
                                                                 symmetry_highlight_thickness);
    gui_random_symmetry_button_rotate_bg_rect  = ExpandRectangle(gui_random_symmetry_button_rotate_rect,
                                                                 symmetry_highlight_thickness);

    gui_random_symmetry_button_none_bg_rect.x        -= 1.0;
    gui_random_symmetry_button_reflect_bg_rect.x     -= 1.0;
    gui_random_symmetry_button_rotate_bg_rect.x      -= 1.0;

    gui_random_symmetry_button_none_bg_rect.width    += 1.0;
    gui_random_symmetry_button_reflect_bg_rect.width += 1.0;
    gui_random_symmetry_button_rotate_bg_rect.width  += 1.0;

    gui_random_area_rect.y      += gui_random_symmetry_label_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_symmetry_label_rect.height + RAYGUI_ICON_SIZE;

    resize_gui_int_range(gui_range_fixed,  &gui_random_area_rect);
    resize_gui_int_range(gui_range_hidden, &gui_random_area_rect);

    float range_opt_label_width = MAX(gui_range_fixed->label_rect.width,
                                      gui_range_hidden->label_rect.width);

    gui_int_range_set_label_width(gui_range_fixed,  range_opt_label_width);
    gui_int_range_set_label_width(gui_range_hidden, range_opt_label_width);

    Vector2 gui_random_seed_text_size = measure_gui_text(gui_random_seed_text);

    gui_random_seed_rect.x      = gui_random_area_rect.x;
    gui_random_seed_rect.y      = gui_random_area_rect.y;
    gui_random_seed_rect.width  = gui_random_seed_text_size.x + (2 * BUTTON_MARGIN);
    gui_random_seed_rect.height = TOOL_BUTTON_HEIGHT;

    seed_text_location.x = gui_random_seed_rect.x + gui_random_seed_rect.width + RAYGUI_ICON_SIZE;
    seed_text_location.y = gui_random_seed_rect.y + 1;

    Vector2 gui_random_rng_seed_text_size   = measure_gui_text(gui_random_rng_seed_text);

    gui_random_rng_seed_rect.y      = gui_random_seed_rect.y;
    gui_random_rng_seed_rect.width  = gui_random_rng_seed_text_size.x + (4 * BUTTON_MARGIN);
    gui_random_rng_seed_rect.height = TOOL_BUTTON_HEIGHT;
    gui_random_rng_seed_rect.x      = gui_random_area_rect.x + gui_random_area_rect.width - gui_random_rng_seed_rect.width;

    gui_random_enter_seed_rect.y      = seed_text_location.y;
    gui_random_enter_seed_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_enter_seed_rect.height = TOOL_BUTTON_HEIGHT;
    gui_random_enter_seed_rect.x      = gui_random_rng_seed_rect.x - RAYGUI_ICON_SIZE - gui_random_enter_seed_rect.width;

    gui_random_seed_bg_rect.x      = seed_text_location.x;
    gui_random_seed_bg_rect.y      = seed_text_location.y;
    gui_random_seed_bg_rect.width  = 2 + gui_random_enter_seed_rect.x - gui_random_seed_bg_rect.x - RAYGUI_ICON_SIZE;
    gui_random_seed_bg_rect.height = 3 + gui_random_enter_seed_rect.height;

    gui_random_area_rect.y      += gui_random_rng_seed_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_rng_seed_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_play_button_text_size = measure_big_button_text(gui_random_play_button_text);
    Vector2 gui_random_continue_button_text_size = measure_big_button_text(gui_random_continue_button_text);
    Vector2 play_or_continue_text_size = {
        .x = MAX(gui_random_play_button_text_size.x, gui_random_continue_button_text_size.x),
        .y = MAX(gui_random_play_button_text_size.y, gui_random_continue_button_text_size.y)
    };
    gui_random_play_button_rect.height = play_or_continue_text_size.y + (3 * BUTTON_MARGIN);;
    gui_random_play_button_rect.width  = gui_random_area_rect.width;
    gui_random_play_button_rect.x      = gui_random_area_rect.x;
    gui_random_play_button_rect.y      = gui_random_area_rect.y + gui_random_area_rect.height - gui_random_play_button_rect.height;

    gui_random_area_rect.height -= gui_random_play_button_rect.height + RAYGUI_ICON_SIZE;

    gui_random_preview_rect.height = gui_random_area_rect.height;
    gui_random_preview_rect.width  = MIN(gui_random_preview_rect.height, gui_random_area_rect.width);
    gui_random_preview_rect.y      = gui_random_area_rect.y;
    gui_random_preview_rect.x      = gui_random_area_rect.x + (gui_random_area_rect.width / 2) - (gui_random_preview_rect.width / 2);

    seed_text_location.x += 3;
    seed_text_location.y += 5;
    gui_random_seed_rect.y += BUTTON_MARGIN;
}

static void draw_gui_randon_symmetry_modes(void)
{
    GuiLabel(gui_random_symmetry_label_rect, gui_random_symmetry_label_text);

    float roundness = BUTTON_SELECTED_HIGHLIGHT_ROUNDNESS;
    int segments    = BUTTON_SELECTED_HIGHLIGHT_SEGMENTS;

    if (options->create_level_symmetry_mode == SYMMETRY_MODE_NONE) {
        GuiSetState(STATE_PRESSED);
        DrawRectangleRounded(gui_random_symmetry_button_none_bg_rect, roundness, segments,  highlight_border_color);
    } else {
        GuiSetState(STATE_NORMAL);
    }

    if (GuiButton(gui_random_symmetry_button_none_rect, gui_random_symmetry_button_none_text)) {
        options->create_level_symmetry_mode = SYMMETRY_MODE_NONE;
        regen_level();
    }

    if (options->create_level_symmetry_mode == SYMMETRY_MODE_REFLECT) {
        GuiSetState(STATE_PRESSED);
        DrawRectangleRounded(gui_random_symmetry_button_reflect_bg_rect, roundness, segments,  highlight_border_color);
    } else {
        GuiSetState(STATE_NORMAL);
    }

    if (GuiButton(gui_random_symmetry_button_reflect_rect, gui_random_symmetry_button_reflect_text)) {
        options->create_level_symmetry_mode = SYMMETRY_MODE_REFLECT;
        regen_level();
    }

    if (options->create_level_symmetry_mode == SYMMETRY_MODE_ROTATE) {
        GuiSetState(STATE_PRESSED);
        DrawRectangleRounded(gui_random_symmetry_button_rotate_bg_rect, roundness, segments,  highlight_border_color);
    } else {
        GuiSetState(STATE_NORMAL);
    }

    if (GuiButton(gui_random_symmetry_button_rotate_rect, gui_random_symmetry_button_rotate_text)) {
        options->create_level_symmetry_mode = SYMMETRY_MODE_ROTATE;
        regen_level();
    }

    GuiSetState(STATE_NORMAL);
}

static void draw_gui_random_colors(void)
{
    GuiLabel(gui_random_color_label_rect, gui_random_color_label_text);

    Rectangle rect = {
        .x = gui_random_color_label_rect.x + gui_random_color_label_rect.width + RAYGUI_ICON_SIZE,
        .y = gui_random_color_label_rect.y,
        .width  = TOOL_BUTTON_WIDTH,
        .height = TOOL_BUTTON_HEIGHT
    };

    int color_button_segments = 12;
    float color_button_roundness = 0.5f;
    float color_button_line_thickness = 1.0f;
    float cross_thickness = 2.0f;
    Color cross_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    cross_color = ColorAlpha(cross_color, 0.7);

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        bool hover = CheckCollisionPointRec(mouse_positionf, rect);

        if (gui_random_color[type]) {
            /* button for color ON */
            if (gui_random_color_count < 2) {
                hover = false;
            }

            DrawRectangleRounded(rect,
                                 color_button_roundness,
                                 color_button_segments,
                                 hover
                                 ? path_type_hover_color(type)
                                 : path_type_color(type));

            DrawRectangleRoundedLines(rect,
                                      color_button_roundness,
                                      color_button_segments,
                                      hover
                                      ? color_button_line_thickness + 1.0f
                                      : color_button_line_thickness,
                                      hover
                                      ? WHITE
                                      : LIGHTGRAY);

        } else {
            /* button for color OFF */
            DrawRectangleRounded(rect,
                                 color_button_roundness,
                                 color_button_segments,
                                 hover
                                 ? path_type_disabled_hover_color(type)
                                 : path_type_disabled_color(type));

            Vector2 s1 = { rect.x,              rect.y               };
            Vector2 e1 = { rect.x + rect.width, rect.y + rect.height };
            Vector2 s2 = { rect.x + rect.width, rect.y               };
            Vector2 e2 = { rect.x,              rect.y + rect.height };
            DrawLineEx(s1, e1, cross_thickness, cross_color);
            DrawLineEx(s2, e2, cross_thickness, cross_color);

            if (hover) {
                DrawRectangleRoundedLines(rect,
                                          color_button_roundness,
                                          color_button_segments,
                                          color_button_line_thickness + 1.0f,
                                          RAYWHITE);
            }
        }

        if (hover) {
            if (mouse_left_click) {
                toggle_color(type);
                regen_level();
            } else {
                set_mouse_cursor(MOUSE_CURSOR_POINTING_HAND);
            }
        }

        rect.x += TOOL_BUTTON_WIDTH + ICON_BUTTON_SIZE;
    }
}


void ask_for_random_seed_callback(gui_dialog_t *dialog, UNUSED void *data){
    if (dialog->status) {
        if (strlen(dialog->string) > 0) {
            uint64_t value = 0;
            if (parse_random_seed_str(dialog->string, &value)) {
                gui_random_seed = value;
            } else {
                popup_error_message("RNG seed \"%s\" is empty or unusable", dialog->string);
            }
        }
    }
}

bool ask_for_random_seed(void)
{
    gui_dialog_ask_for_string("Enter RNG seed (number or string)", NULL, NULL, ask_for_random_seed_callback, NULL);
    return true;
}

void draw_preview(void)
{
    bool hover = CheckCollisionPointRec(mouse_positionf, gui_random_preview_rect);

    DrawRectangleRec(gui_random_preview_rect, BLACK);
    level_preview(gui_random_level, gui_random_preview_rect);

    if (hover) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Rectangle shift_rect = gui_random_preview_rect;
            shift_rect.x -= 1;
            shift_rect.y += 1;

            DrawRectangleLinesEx(gui_random_preview_rect, 2.0, text_shadow_color);
            DrawRectangleLinesEx(shift_rect, 1.0, tile_edge_drag_color);
        } else {
            Rectangle shift_rect = gui_random_preview_rect;
            shift_rect.x += 1;
            shift_rect.y -= 1;

            DrawRectangleLinesEx(gui_random_preview_rect, 2.0, text_shadow_color);
            DrawRectangleLinesEx(shift_rect, 1.0, tile_edge_hover_color);
        }

        if (mouse_left_click) {
            play_gui_random_level();
        }
    }
}

static void draw_tile_radius_gui(void)
{
    GuiLabel(gui_random_radius_label_rect, gui_random_radius_label_text);
    if (GuiButton(gui_random_radius_left_button_rect, gui_random_radius_left_button_text)) {
        if (options->create_level_radius > LEVEL_MIN_RADIUS) {
            options->create_level_radius--;
            regen_level();
        }
    }
    DrawRectangleRec(gui_random_radius_display_rect, seed_bg_color);

    draw_panel_text(TextFormat("%d", options->create_level_radius),
                    radius_display_text_shadow_location,
                    text_shadow_color);

    draw_panel_text(TextFormat("%d", options->create_level_radius),
                    radius_display_text_location,
                    RAYWHITE);

    if (GuiButton(gui_random_radius_right_button_rect, gui_random_radius_right_button_text)) {
        if (options->create_level_radius < LEVEL_MAX_RADIUS) {
            options->create_level_radius++;
            regen_level();
        }
    }
}

static char *get_play_or_continue_text(void)
{
    if (played_level) {
        return gui_random_continue_button_text;
    } else {
        return gui_random_play_button_text;
    }
}

void draw_gui_random(void)
{
    if (!gui_random_level) {
        regen_level();
    }

    GuiPanel(gui_random_panel_rect, gui_random_panel_text);

    draw_tile_radius_gui();

#if defined(PLATFORM_DESKTOP)
    bool save_ok = false;
    if (gui_random_level && (gui_random_level->seed > 0)) {
        save_ok = true;
    }

    if (!save_ok) {
        GuiDisable();
    }

    if (GuiButton(gui_random_save_button_rect, gui_random_save_button_text)) {
        save_gui_random_level();
    }

    if (!save_ok) {
        GuiEnable();
    }
#endif

    draw_gui_random_colors();

    bool colors_ok = false;
    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        colors_ok = colors_ok || gui_random_color[type];
    }

    if (draw_gui_numeric(gui_random_density)) {
        regen_level();
    }

    draw_gui_randon_symmetry_modes();

    if (draw_gui_int_range(gui_range_fixed)) {
        regen_level();
    }
    if (draw_gui_int_range(gui_range_hidden)) {
        regen_level();
    }

    GuiLabel(gui_random_seed_rect, gui_random_seed_text);

    DrawRectangleRec(gui_random_seed_bg_rect, seed_bg_color);
    draw_panel_text(gui_random_seed_str, seed_text_location, panel_header_text_color);

    if (GuiButton(gui_random_enter_seed_rect, gui_random_enter_seed_text)) {
        if (ask_for_random_seed()) {
            regen_level();
        }
    }

    if (GuiButton(gui_random_rng_seed_rect, gui_random_rng_seed_text)) {
        new_random_seed();
        regen_level();
    }

    if (!colors_ok) {
        GuiDisable();
    }

    set_big_button_font();

    if (GuiButton(gui_random_play_button_rect, get_play_or_continue_text())) {
        play_gui_random_level();
    }

    set_default_font();

    if (!colors_ok) {
        GuiEnable();
    }

    if (gui_random_level) {
        draw_preview();
    }
}

void play_gui_random_level(void)
{
    if (!gui_random_level) {
        regen_level();
    }

    level_play(gui_random_level);
    played_level = true;
}

void play_gui_random_level_preview(void)
{
    promote_preview_to_level();
    play_gui_random_level();
}

#if defined(PLATFORM_DESKTOP)
void save_gui_random_level(void)
{
    if (gui_random_level) {
        level_save_to_local_levels(gui_random_level,
                                   GUI_RAMDOM_SAVE_PREFIX,
                                   gui_random_level->name);

        assert_not_null(gui_random_level->filename);

        popup_message("Level saved as:\"%s\"", gui_random_level->filename);
    }
}
#endif

bool create_level_from_json(cJSON *json)
{
    if (!cJSON_IsObject(json)) {
        errmsg("JSON['create_level'] should be an Object");
        return false;
    }

    if (options->load_state_create_level_radius) {
        cJSON *radius_json = cJSON_GetObjectItem(json, "radius");
        if (radius_json) {
            if (cJSON_IsNumber(radius_json)) {
                options->create_level_radius = radius_json->valueint;
            } else {
                errmsg("JSON['create_level']['radius'] not a Number");
                return false;
            }
            CLAMPVAR(options->create_level_radius,
                     LEVEL_MIN_RADIUS,
                     LEVEL_MAX_RADIUS);
        } else {
            warnmsg("JSON['create_level']['radius'] is missing");
        }
    }

    if (options->load_state_create_level_minimum_path_density) {
        cJSON *density_json = cJSON_GetObjectItem(json, "minimum_path_density");
        if (density_json) {
            if (cJSON_IsNumber(density_json)) {
                options->create_level_minimum_path_density = density_json->valuedouble;
            } else {
                errmsg("JSON['create_level']['minimum_path_density'] not a Number");
                return false;
            }
            CLAMPVAR(options->create_level_minimum_path_density,
                     LEVEL_MIN_MINIMUM_PATH_DENSITY,
                     LEVEL_MAX_MINIMUM_PATH_DENSITY);
        } else {
            warnmsg("JSON['create_level']['minimum_path_density'] is missing");
        }
    }

    if (options->load_state_create_level_symmetry_mode) {
        cJSON *symmetry_mode_json = cJSON_GetObjectItem(json, "symmetry_mode");
        if (symmetry_mode_json) {
            if (cJSON_IsString(symmetry_mode_json)) {
                options->create_level_symmetry_mode = parse_symmetry_mode_string(cJSON_GetStringValue(symmetry_mode_json));
            } else {
                errmsg("JSON['create_level']['symmetry_mode'] not a Number");
                return false;
            }
        } else {
            warnmsg("JSON['create_level']['symmetry_mode'] is missing");
        }
    }

    cJSON *fixed_json = cJSON_GetObjectItem(json, "fixed");
    if (fixed_json) {
        int_range_t fixed_range = options->create_level_fixed;
        if (!int_range_from_json(fixed_json, &fixed_range)) {
            errmsg("Error parsing program state: JSON['create_level']['fixed']");
            return false;
        }
        if (options->load_state_create_level_fixed_min) {
            options->create_level_fixed.min = fixed_range.min;
        }
        if (options->load_state_create_level_fixed_max) {
            options->create_level_fixed.max = fixed_range.max;
        }
        int_range_clamp(&options->create_level_fixed,
                        LEVEL_MIN_FIXED,
                        LEVEL_MAX_FIXED);
    } else {
        warnmsg("Program state JSON['create_level'] is missing \"fixed\"");
    }

    cJSON *hidden_json = cJSON_GetObjectItem(json, "hidden");
    if (hidden_json) {
        int_range_t hidden_range = options->create_level_hidden;
        if (!int_range_from_json(hidden_json, &options->create_level_hidden)) {
            errmsg("Error parsing program state: JSON['create_level']['hidden']");
            return false;
        }
        if (options->load_state_create_level_hidden_min) {
            options->create_level_hidden.min = hidden_range.min;
        }
        if (options->load_state_create_level_hidden_max) {
            options->create_level_hidden.max = hidden_range.max;
        }
        int_range_clamp(&options->create_level_hidden,
                        LEVEL_MIN_HIDDEN,
                        LEVEL_MAX_HIDDEN);
    } else {
        warnmsg("Program state JSON['create_level'] is missing \"hidden\"");
    }

    cJSON *color_json = cJSON_GetObjectItem(json, "enabled_colors");
    if (color_json) {
        for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
            const char *color_name = TextFormat("path_color_%d", type);

            cJSON *color_type_json = cJSON_GetObjectItem(color_json, color_name);
            if (color_type_json) {
                if (cJSON_IsBool(color_type_json)) {
                    if (cJSON_IsTrue(color_type_json)) {
                        gui_random_color[type] = true;
                    } else {
                        gui_random_color[type] = false;
                    }
                } else {
                    errmsg("Error parsing program state: JSON['create_level']['color_enabled']['%s']", color_name);
                    return false;
                }
            } else {
                warnmsg("Program state JSON['create_level']['color_enabled'] is missing \"%s\"", color_name);
            }
        }

        update_rng_color_count();
    } else {
        warnmsg("Program state JSON['create_level'] is missing \"color_enabled\"");
    }

    return true;
}

cJSON *create_level_to_json(void)
{
    cJSON *json = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(json, "radius", options->create_level_radius) == NULL) {
        goto create_level_to_json_error;
    }

    if (cJSON_AddNumberToObject(json, "minimum_path_density", options->create_level_minimum_path_density) == NULL) {
        goto create_level_to_json_error;
    }

    if (cJSON_AddStringToObject(json, "symmetry_mode", symmetry_mode_string(options->create_level_symmetry_mode)) == NULL) {
        goto create_level_to_json_error;
    }

    cJSON *fixed_json = int_range_to_json(&options->create_level_fixed);
    if (fixed_json) {
        if (!cJSON_AddItemToObject(json, "fixed", fixed_json)) {
            goto create_level_to_json_error;
        }
    } else {
        goto create_level_to_json_error;
    }

    cJSON *hidden_json = int_range_to_json(&options->create_level_hidden);
    if (hidden_json) {
        if (!cJSON_AddItemToObject(json, "hidden", hidden_json)) {
            goto create_level_to_json_error;
        }
    } else {
        goto create_level_to_json_error;
    }

    cJSON *color_json = cJSON_AddObjectToObject(json, "enabled_colors");
    if (color_json) {
        for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
            const char *color_name = TextFormat("path_color_%d", type);
            if (gui_random_color[type]) {
                if (cJSON_AddTrueToObject(color_json, color_name) == NULL) {
                    goto create_level_to_json_error;
                }
            } else {
                if (cJSON_AddFalseToObject(color_json, color_name) == NULL) {
                    goto create_level_to_json_error;
                }
            }
        }
    } else {
        goto create_level_to_json_error;
    }

    return json;

  create_level_to_json_error:
    cJSON_Delete(json);
    return NULL;
}

void gui_random_copy_blueprint_to_clipboard(void)
{
    if (!gui_random_level) {
        return;
    }

    level_copy_blueprint_to_clipboard(gui_random_level);
}

void gui_random_paste_blueprint_from_clipboard(void)
{
    const char *blueprint = GetClipboardText();

    if (options->verbose) {
        infomsg("Received %d bytes from the clipboard: \"%s\"", strlen(blueprint), blueprint);
    }

    generate_level_param_t param = {0};
    if (deserialize_generate_level_params(blueprint, &param)) {
        gen_level_with_params(param);
    } else {
        popup_error_message("Blueprint string was not formatted correctly.");
    }
}
