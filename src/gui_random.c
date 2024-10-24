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
#include "options.h"
#include "gui_random.h"
#include "color.h"
#include "tile.h"
#include "tile_pos.h"
#include "tile_draw.h"
#include "level.h"

#include "pcg/pcg_basic.h"

//#define RANDOM_GEN_DEBUG

Rectangle gui_random_panel_rect;
Rectangle gui_random_area_rect;
Rectangle gui_random_play_button_rect;
Rectangle gui_random_radius_rect;
Rectangle gui_random_color_label_rect;
Rectangle gui_random_gen_style_label_rect;
Rectangle gui_random_gen_style_rect;
Rectangle gui_random_difficulty_label_rect;
Rectangle gui_random_difficulty_rect;
Rectangle gui_random_seed_rect;
Rectangle gui_random_seed_bg_rect;
Rectangle gui_random_enter_seed_rect;
Rectangle gui_random_rng_seed_rect;
Rectangle gui_random_preview_rect;

Vector2 seed_text_location;

char gui_random_panel_text[] = "Random Level";
char gui_random_play_button_text[] = "Play";
char gui_random_radius_text[] = "Tile Radius  ";
char gui_random_color_label_text[] = "Colors";
char gui_random_gen_style_label_text[] = "Gen. Method";
char gui_random_difficulty_label_text[] = "Difficulty";
char gui_random_seed_text[] = "RNG Seed";
char gui_random_rng_seed_text[] = "Randomize";
char gui_random_enter_seed_text_str[] = "Enter";
char *gui_random_enter_seed_text = NULL;

const char *gui_random_gen_styles[] = {
    "Density / Scatter",
    "Hamiltonian Path - DFS"
};
char *gui_random_gen_style_text = NULL;
#define NUM_GEN_STYLES (sizeof(gui_random_gen_styles)/sizeof(char *))
bool gui_random_gen_style_edit_mode = false;

int gen_style = 1;

const char *gui_random_difficulties[] = {
    "Easy    (~2 paths/tile)",
    "Medium (3~4 paths/tile)",
    "Hard   (4~6 paths/tile)"
};
char *gui_random_difficulty_text = NULL;
#define NUM_DIFFICULTIES (sizeof(gui_random_difficulties)/sizeof(char *))
bool gui_random_difficulty_edit_mode = false;

int difficulty = 1;

bool gui_random_color[PATH_TYPE_COUNT];
int gui_random_color_count = PATH_TYPE_COUNT - 1;

level_t *gui_random_level = NULL;
uint64_t gui_random_seed;
char *gui_random_seed_str = NULL;

Color seed_bg_color;

pcg32_random_t rng;

static void set_random_seed(int value)
{
    gui_random_seed = (uint64_t)value;
    if (gui_random_seed_str) {
        free(gui_random_seed_str);
    }
    asprintf(&gui_random_seed_str, "%d", value);
}

static void new_random_seed(void)
{
    set_random_seed(rand());
}

static void rng_seed(void)
{
    pcg32_srandom_r(&rng, gui_random_seed, (uint64_t)options->create_level_radius);
}

static int rng_get(int bound)
{
    if (bound <= 1) {
        return 0;
    } else {
        return (int)pcg32_boundedrand_r(&rng, (uint32_t)bound);
    }
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

static void toggle_color(path_type_t type)
{
    gui_random_color[type] = !gui_random_color[type];
    gui_random_color_count = rng_color_count();
}

static path_type_t rng_color(void)
{
    int skip = rng_get(gui_random_color_count);

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        if (gui_random_color[type]) {
            if (skip) {
                skip--;
            } else {
                return type;
            }
        }
    }

    __builtin_unreachable();

    assert(false && "shouldn't ever reach this!");

    return PATH_TYPE_NONE;
}

static void generate_random_paths(level_t *level, int num_tiles)
{
    for (int i=0; i<num_tiles; i++) {
        int path_delta = options->create_level_max_path - options->create_level_min_path;
        int path_rand  = rng_get(path_delta);
        int num_paths  = path_rand + options->create_level_min_path;

        tile_t *tile = level->enabled_tiles[i];
        tile_pos_t *pos = tile->solved_pos;

        for (hex_direction_t dir=0; dir<6; dir++) {
            if (tile->path[dir]) {
                num_paths--;
            } else {
                tile_pos_t *neighbor = pos->neighbors[dir];
                if (neighbor && neighbor->tile && !neighbor->tile->enabled) {
                    num_paths--;
                }
            }
        }

        //print_tile_pos(pos);
        //pint(num_paths);

        while (num_paths > 0) {
            int offset = rng_get(6);
            for (hex_direction_t dir=0; dir<6; dir++) {
                hex_direction_t idx = (dir + offset) % 6;

                if (tile->path[idx]) {
                    continue;
                }

                tile_pos_t *neighbor = pos->neighbors[idx];
                if (neighbor && neighbor->tile && neighbor->tile->enabled) {
                    hex_direction_t opp_idx = hex_opposite_direction(idx);
                    tile->path[idx] = rng_color();
                    neighbor->tile->path[opp_idx] = tile->path[idx];
                    num_paths--;
                    break;
                }
            }
        }
    }
}

static void set_all_hover(level_t *level, bool value)
{
    for (int i=0; i<LEVEL_MAXTILES; i++) {
        tile_t *tile = &(level->tiles[i]);
        tile->unsolved_pos->hover = value;
    }
}

static void dfs_tile_pos(tile_pos_t *pos)
{
    bool todo[6] = {0};
    int todo_count = 0;

    pos->hover = true;

    for (hex_direction_t dir=0; dir<6; dir++) {
        tile_pos_t *neighbor = pos->neighbors[dir];
        if (neighbor && neighbor->tile && neighbor->tile->enabled && !neighbor->hover) {
            todo[dir] = true;
            todo_count++;
        }
    }

    while (todo_count > 0) {
        int offset = rng_get(6);
        for (hex_direction_t dir=0; dir<6; dir++) {
            hex_direction_t idx = (dir + offset) % 6;
            if (todo[idx]) {
                todo[idx] = false;

                tile_pos_t *neighbor = pos->neighbors[idx];

                if (neighbor && neighbor->tile && neighbor->tile->enabled && !neighbor->hover) {
                    hex_direction_t opp_idx = hex_opposite_direction(idx);
                    pos->tile->path[idx] = rng_color();
                    neighbor->tile->path[opp_idx] = pos->tile->path[idx];

                    dfs_tile_pos(neighbor);
                }
                break;
            }
        }

        todo_count--;
    }
}

static void generate_dfs_path(level_t *level, int num_tiles)
{
    int start_idx = rng_get(num_tiles);
    tile_t *start = level->enabled_tiles[start_idx];
    tile_pos_t *start_pos = start->solved_pos;

    set_all_hover(level, false);
    dfs_tile_pos(start_pos);
    set_all_hover(level, false);
}

static void shuffle_tiles(level_t *level)
{
    level_use_unsolved_tile_pos(level);
    int num_positions = level_get_enabled_positions(level);

    for (int i=num_positions-1; i>0; i--) {
        int j = i;
        while (i == j) {
            j = rng_get(i + 1);
        }

        tile_pos_t *pos_i = level->enabled_positions[i];
        tile_pos_t *pos_j = level->enabled_positions[j];
        level_swap_tile_pos(level, pos_i, pos_j);
        level->enabled_positions[i] = pos_j;
        level->enabled_positions[j] = pos_i;
    }
}

struct level *generate_random_level(void)
{
    assert(rng_color_count() > 0);

    rng_seed();

    level_t *level = create_level(NULL);
    level_reset(level);

    snprintf(level->name, NAME_MAXLEN, "%s", TextFormat("%d", gui_random_seed));
    level_set_radius(level, options->create_level_radius);
    int n = level_get_enabled_tiles(level);

    switch (difficulty) {
    case 0: /* easy */
        options->create_level_min_path = OPTIONS_DEFAULT_CREATE_LEVEL_EASY_MIN_PATH;
        options->create_level_max_path = OPTIONS_DEFAULT_CREATE_LEVEL_EASY_MAX_PATH;
        break;

    case 1: /* medium */
        options->create_level_min_path = OPTIONS_DEFAULT_CREATE_LEVEL_MEDIUM_MIN_PATH;
        options->create_level_max_path = OPTIONS_DEFAULT_CREATE_LEVEL_MEDIUM_MAX_PATH;
        break;

    case 2: /* hard */
        options->create_level_min_path = OPTIONS_DEFAULT_CREATE_LEVEL_HARD_MIN_PATH;
        options->create_level_max_path = OPTIONS_DEFAULT_CREATE_LEVEL_HARD_MAX_PATH;
        break;

    default:
        __builtin_unreachable();
        assert(false && "should never reach here");
    }

    switch (gen_style) {
    case 0:
        generate_random_paths(level, n);
        break;

    case 1:
        generate_dfs_path(level, n);
        generate_random_paths(level, n);
        break;

    default:
        __builtin_unreachable();
        assert(false && "should never reach here");
    }

#ifndef RANDOM_GEN_DEBUG
    shuffle_tiles(level);
#endif

    level_use_unsolved_tile_pos(level);

    return level;
}

static void regen_level(void)
{
    if (gui_random_level) {
        destroy_level(gui_random_level);
    }

    gui_random_level = generate_random_level();
}

void init_gui_random_minimal(void)
{
    gui_random_level = NULL;

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        gui_random_color[type] = true; 
    }

    new_random_seed();
}

void init_gui_random(void)
{
    init_gui_random_minimal();

    seed_bg_color = ColorBrightness(tile_bg_color, -0.15);

    SAFEFREE(gui_random_enter_seed_text);
    SAFEFREE(gui_random_gen_style_text);
    SAFEFREE(gui_random_difficulty_text);

    //gui_random_enter_seed_text = strdup(GuiIconText(ICON_PENCIL, gui_random_enter_seed_text_str));
    gui_random_enter_seed_text = strdup(GuiIconText(ICON_PENCIL, NULL));

    const char *join_str = TextJoin(gui_random_gen_styles,   NUM_GEN_STYLES,   ";");
    gui_random_gen_style_text  = strdup(join_str);

    join_str = TextJoin(gui_random_difficulties, NUM_DIFFICULTIES, ";");
    gui_random_difficulty_text = strdup(join_str);

    regen_level();
}

void cleanup_gui_random(void)
{
    if (gui_random_level) {
        destroy_level(gui_random_level);
        gui_random_level = NULL;
    }

    SAFEFREE(gui_random_seed_str);

    SAFEFREE(gui_random_enter_seed_text);
    SAFEFREE(gui_random_gen_style_text);
    SAFEFREE(gui_random_difficulty_text);
}

void resize_gui_random(void)
{
    gui_random_panel_rect.width  = window_size.x * 0.4;
    gui_random_panel_rect.height = window_size.y * 0.45;

    MINVAR(gui_random_panel_rect.width,  350);
    MINVAR(gui_random_panel_rect.height, 550);

    gui_random_panel_rect.x = (window_size.x / 2) - (gui_random_panel_rect.width  / 2);
    gui_random_panel_rect.y = (window_size.y / 2) - (gui_random_panel_rect.height / 2);

    float panel_bottom = gui_random_panel_rect.y + gui_random_panel_rect.height;

    gui_random_area_rect.x      = gui_random_panel_rect.x + PANEL_INNER_MARGIN;
    gui_random_area_rect.y      = gui_random_panel_rect.y + PANEL_INNER_MARGIN + RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT;
    gui_random_area_rect.width  = gui_random_panel_rect.width - (2 * PANEL_INNER_MARGIN);
    gui_random_area_rect.height = panel_bottom - PANEL_INNER_MARGIN - gui_random_area_rect.y;

    Vector2 gui_random_radius_text_size = MeasureGuiText(gui_random_radius_text);

    gui_random_radius_rect.x      = gui_random_area_rect.x + gui_random_radius_text_size.x;
    gui_random_radius_rect.y      = gui_random_area_rect.y;
    gui_random_radius_rect.width  = 90; //gui_random_area_rect.width - gui_random_radius_text_size.x;
    gui_random_radius_rect.height = 30;

    gui_random_area_rect.y      += gui_random_radius_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_radius_rect.height + RAYGUI_ICON_SIZE;

    Vector2 color_label_text_size = MeasureGuiText(gui_random_color_label_text);
 
    gui_random_color_label_rect.x      = gui_random_area_rect.x;
    gui_random_color_label_rect.y      = gui_random_area_rect.y;
    gui_random_color_label_rect.width  = color_label_text_size.x;
    gui_random_color_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_area_rect.y      += gui_random_color_label_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_color_label_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_gen_style_label_text_size = MeasureGuiText(gui_random_gen_style_label_text);

    gui_random_gen_style_label_rect.x      = gui_random_area_rect.x;
    gui_random_gen_style_label_rect.y      = gui_random_area_rect.y;
    gui_random_gen_style_label_rect.width  = gui_random_gen_style_label_text_size.x;
    gui_random_gen_style_label_rect.height = TOOL_BUTTON_HEIGHT;;

    gui_random_gen_style_rect.x      = gui_random_gen_style_label_rect.x + gui_random_gen_style_label_rect.width + RAYGUI_ICON_SIZE;
    gui_random_gen_style_rect.y      = gui_random_gen_style_label_rect.y;
    gui_random_gen_style_rect.width  = gui_random_area_rect.width - (gui_random_gen_style_rect.x - gui_random_gen_style_label_rect.x);
    gui_random_gen_style_rect.height = gui_random_gen_style_label_rect.height;

    gui_random_area_rect.y      += gui_random_gen_style_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_gen_style_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_difficulty_label_text_size = MeasureGuiText(gui_random_difficulty_label_text);

    gui_random_difficulty_label_rect.x      = gui_random_area_rect.x;
    gui_random_difficulty_label_rect.y      = gui_random_area_rect.y;
    gui_random_difficulty_label_rect.width  = gui_random_difficulty_label_text_size.x;
    gui_random_difficulty_label_rect.height = TOOL_BUTTON_HEIGHT;

    gui_random_difficulty_rect.x      = gui_random_difficulty_label_rect.x + gui_random_difficulty_label_rect.width + RAYGUI_ICON_SIZE;
    gui_random_difficulty_rect.y      = gui_random_difficulty_label_rect.y;
    gui_random_difficulty_rect.width  = gui_random_area_rect.width - (gui_random_difficulty_rect.x - gui_random_difficulty_label_rect.x);
    gui_random_difficulty_rect.height = gui_random_difficulty_label_rect.height;

    gui_random_area_rect.y      += gui_random_difficulty_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_difficulty_rect.height + RAYGUI_ICON_SIZE;

    Vector2 gui_random_seed_text_size = MeasureGuiText(gui_random_seed_text);

    gui_random_seed_rect.x      = gui_random_area_rect.x;
    gui_random_seed_rect.y      = gui_random_area_rect.y;
    gui_random_seed_rect.width  = gui_random_seed_text_size.x;
    gui_random_seed_rect.height = TOOL_BUTTON_HEIGHT;

    seed_text_location.x = gui_random_seed_rect.x + gui_random_seed_rect.width + RAYGUI_ICON_SIZE;
    seed_text_location.y = gui_random_seed_rect.y;

    Vector2 gui_random_rng_seed_text_size   = MeasureGuiText(gui_random_rng_seed_text);

    gui_random_rng_seed_rect.y      = seed_text_location.y;
    gui_random_rng_seed_rect.width  = gui_random_rng_seed_text_size.x;
    gui_random_rng_seed_rect.height = TOOL_BUTTON_HEIGHT;
    gui_random_rng_seed_rect.x      = gui_random_area_rect.x + gui_random_area_rect.width - gui_random_rng_seed_rect.width;

    gui_random_enter_seed_rect.y      = seed_text_location.y;
    gui_random_enter_seed_rect.width  = TOOL_BUTTON_WIDTH;
    gui_random_enter_seed_rect.height = TOOL_BUTTON_HEIGHT;
    gui_random_enter_seed_rect.x      = gui_random_rng_seed_rect.x - RAYGUI_ICON_SIZE - gui_random_enter_seed_rect.width;

    gui_random_seed_bg_rect.x      = seed_text_location.x - 6;
    gui_random_seed_bg_rect.y      = seed_text_location.y - 2;
    gui_random_seed_bg_rect.width  = 2 + gui_random_enter_seed_rect.x - gui_random_seed_bg_rect.x - RAYGUI_ICON_SIZE;
    gui_random_seed_bg_rect.height = 3 + gui_random_enter_seed_rect.height;

    gui_random_area_rect.y      += gui_random_rng_seed_rect.height + RAYGUI_ICON_SIZE;
    gui_random_area_rect.height -= gui_random_rng_seed_rect.height + RAYGUI_ICON_SIZE;

    gui_random_play_button_rect.height = 3 * RAYGUI_ICON_SIZE;
    gui_random_play_button_rect.width  = gui_random_area_rect.width;
    gui_random_play_button_rect.x      = gui_random_area_rect.x;
    gui_random_play_button_rect.y      = gui_random_area_rect.y + gui_random_area_rect.height - gui_random_play_button_rect.height;

    gui_random_area_rect.height -= gui_random_play_button_rect.height + RAYGUI_ICON_SIZE;

    gui_random_preview_rect.height = gui_random_area_rect.height;
    gui_random_preview_rect.width  = MIN(gui_random_preview_rect.height, gui_random_area_rect.width);
    gui_random_preview_rect.y      = gui_random_area_rect.y;
    gui_random_preview_rect.x      = gui_random_area_rect.x + (gui_random_area_rect.width / 2) - (gui_random_preview_rect.width / 2);
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
    float color_button_roundness = 0.5;
    float color_button_line_thickness = 1.0;
    float cross_thickness = 2.0;
    Color cross_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    cross_color = ColorAlpha(cross_color, 0.7);

    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        bool hover = CheckCollisionPointRec(mouse_positionf, rect);

        DrawRectangleRounded(rect, color_button_roundness, color_button_segments,
                             hover ? path_type_highlight_color(type) : path_type_color(type));
        if (gui_random_color[type]) {
            DrawRectangleRoundedLines(rect, color_button_roundness, color_button_segments, color_button_line_thickness, WHITE); 
        } else {
            Vector2 s1 = { rect.x,              rect.y               };
            Vector2 e1 = { rect.x + rect.width, rect.y + rect.height };
            Vector2 s2 = { rect.x + rect.width, rect.y               };
            Vector2 e2 = { rect.x,              rect.y + rect.height };
            DrawLineEx(s1, e1, cross_thickness, cross_color);
            DrawLineEx(s2, e2, cross_thickness, cross_color);
        }

        if (hover) {
            if (mouse_left_click) {
                toggle_color(type);
                regen_level();
            } else {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            }
        }

        rect.x += TOOL_BUTTON_WIDTH + ICON_BUTTON_SIZE;
    }
}

void draw_gui_random(void)
{
    GuiPanel(gui_random_panel_rect, gui_random_panel_text);

    int old_radius = options->create_level_radius;

    int radius = (int)options->create_level_radius;
    GuiSpinner(gui_random_radius_rect, gui_random_radius_text, &radius, LEVEL_MIN_RADIUS, LEVEL_MAX_RADIUS, false);
    options->create_level_radius = (long)radius;

    if (old_radius != options->create_level_radius) {
        regen_level();
    }

    draw_gui_random_colors();

    bool colors_ok = false;
    for (path_type_t type = (PATH_TYPE_NONE + 1); type < PATH_TYPE_COUNT; type++) {
        colors_ok = colors_ok || gui_random_color[type];
    }

    GuiLabel(gui_random_seed_rect, gui_random_seed_text);

    DrawRectangleRec(gui_random_seed_bg_rect, seed_bg_color);
    DrawTextEx(PANEL_LABEL_FONT, gui_random_seed_str, seed_text_location,
               PANEL_LABEL_FONT_SIZE, PANEL_LABEL_FONT_SPACING, panel_header_text_color);


    if (GuiButton(gui_random_enter_seed_rect, gui_random_enter_seed_text)) {
        regen_level();
    }

    if (GuiButton(gui_random_rng_seed_rect, gui_random_rng_seed_text)) {
        new_random_seed();
        regen_level();
    }

    if (!colors_ok) {
        GuiDisable();
    }

    if (GuiButton(gui_random_play_button_rect, gui_random_play_button_text)) {
        level_play(gui_random_level);
    }

    if (!colors_ok) {
        GuiEnable();
    }

    if (gui_random_level) {
        DrawRectangleRec(gui_random_preview_rect, BLACK);
        level_preview(gui_random_level, gui_random_preview_rect);
    }

    GuiLabel(gui_random_difficulty_label_rect, gui_random_difficulty_label_text);
    if (GuiDropdownBox(gui_random_difficulty_rect, gui_random_difficulty_text, &difficulty, gui_random_difficulty_edit_mode)) {
        gui_random_difficulty_edit_mode = !gui_random_difficulty_edit_mode;
        regen_level();
    }

    GuiLabel(gui_random_gen_style_label_rect, gui_random_gen_style_label_text);
    if (GuiDropdownBox(gui_random_gen_style_rect, gui_random_gen_style_text, &gen_style, gui_random_gen_style_edit_mode)) {
        gui_random_gen_style_edit_mode = !gui_random_gen_style_edit_mode;
        regen_level();
    }
}
