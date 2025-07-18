/****************************************************************************
 *                                                                          *
 * const.h                                                                  *
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

#ifndef CONST_H
#define CONST_H

#define COLLECTION_ZIP_INDEX_FILENAME "levels.index"

#define COLLECTION_FILENAME_EXT "hexlevelpack"
#define LEVEL_FILENAME_EXT      "hexlevel"

#define PROJECT_STATE_JSON_NAMESPACE              PACKAGE_NAME "/state"
#define PROJECT_FINISHED_LEVEL_LOG_JSON_NAMESPACE PACKAGE_NAME "/finished_levels"

#define LEVEL_PACK_DIRPATH_MARKER "{" COLLECTION_FILENAME_EXT "}"
#define COLLECTION_DEFAULT_FILENAME_PREFIX "level-"
#define COLLECTION_DEFAULT_FILENAME_SUFFIX "." LEVEL_FILENAME_EXT

#define BLUEPRINT_STRING_PREFIX "HEXLVL"
#define BLUEPRINT_STRING_PREFIX_LENGTH 6
#define BLUEPRINT_STRING_SUFFIX "zBP"
#define BLUEPRINT_STRING_SUFFIX_LENGTH 3

#define GUI_RAMDOM_SAVE_PREFIX "random"

#define LEVEL_DEFAULT_NAME "Untitled"
#define LEVEL_DEFAULT_RADIUS LEVEL_MIN_RADIUS

#define LEVEL_MIN_RADIUS 1
#define LEVEL_MAX_RADIUS 4
#define LEVEL_MIN_FIXED 0
#define LEVEL_MAX_FIXED 9
#define LEVEL_MIN_HIDDEN 0
#define LEVEL_MAX_HIDDEN 9
#define LEVEL_MIN_PATH_ITER 1
#define LEVEL_MAX_PATH_ITER 6

#define LEVEL_MIN_MINIMUM_PATH_DENSITY 250
#define LEVEL_MAX_MINIMUM_PATH_DENSITY 450
#define TITLE_MINIMUM_PATH_DENSITY 375

#define TILE_RESET_TIME 0.35

#define TILE_LEVEL_WIDTH  ((2 * LEVEL_MAX_RADIUS) + 1)
#define TILE_LEVEL_HEIGHT TILE_LEVEL_WIDTH

#define LEVEL_MAXTILES (TILE_LEVEL_HEIGHT * TILE_LEVEL_WIDTH)

#define LEVEL_CENTER_POSITION  \
    ((hex_axial_t){            \
        .q = LEVEL_MAX_RADIUS, \
        .r = LEVEL_MAX_RADIUS  \
     })

#define MAX_PATH_DENSITY_ITER LEVEL_MAXTILES

#define LEVEL_FADE_TRANSITION_LENGTH 0.7
#define LEVEL_FADE_TRANSITION_FRAMES (LEVEL_FADE_TRANSITION_LENGTH * options->max_fps)
#define LEVEL_FADE_DELTA (1.0 / (LEVEL_FADE_TRANSITION_FRAMES))

#define PATH_TYPE_COUNT 5
#define PATH_COLOR_COUNT (PATH_TYPE_COUNT - 1)

//#define NAME_MAXLEN 22
#define NAME_MAXLEN 50
#define UI_NAME_MAXLEN  (6 + NAME_MAXLEN)

// ['#', '1', '2', '3', '#', '\0']
#define ICON_STR_MAXLEN 6

#define UNIQUE_ID_LENGTH 37
#define COLLECTION_ID_LENGTH UNIQUE_ID_LENGTH
#define ID_MAXLEN 255

#define FINISHED_HUE_STEP 4.0
#define FINISHED_ANIM_PREDELAY_LENGTH 0.5
#define FINISHED_ANIM_LENGTH 5.0

#define WIN_ANIM_STARTUP_TIME  1.9
#define WIN_ANIM_SHUTDOWN_TIME 0.7

// 1MB
#define LEVEL_SERIALIZE_BUFSIZE (1024 * 1024)


#define DEFAULT_GUI_FONT      font18
#define DEFAULT_GUI_FONT_SIZE 26
#define DEFAULT_GUI_FONT_SPACING 2.0

#define PANEL_LABEL_FONT         font18
#define PANEL_LABEL_FONT_SIZE    28
#define PANEL_LABEL_FONT_SPACING 2.0


#ifndef RAYGUI_ICON_SIZE
#define RAYGUI_ICON_SIZE 16
#endif

#if !defined(RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT)
#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT        24
#endif

#if !defined(RAYGUI_MESSAGEBOX_BUTTON_PADDING)
#define RAYGUI_MESSAGEBOX_BUTTON_PADDING 12
#endif

#if !defined(RAYGUI_MESSAGEBOX_BUTTON_HEIGHT)
#define RAYGUI_MESSAGEBOX_BUTTON_HEIGHT 24
#endif

#define WINDOW_MARGIN RAYGUI_ICON_SIZE
#define PANEL_INNER_MARGIN 10

#define BUTTON_MARGIN 4
#define ICON_BUTTON_SIZE (RAYGUI_ICON_SIZE + (2 * BUTTON_MARGIN))
#define ICON_FONT_SIZE DEFAULT_GUI_FONT_SIZE

#define NAME_FONT         font20
#define NAME_FONT_SIZE    20
#define NAME_FONT_SPACING 2.0

#define CURSOR_MIN_SCALE 1
#define CURSOR_MAX_SCALE 4

#define DOUBLE_CLICK_MS_MIN 100
#define DOUBLE_CLICK_MS_MAX 2000

#define PANEL_ROUNDNES 0.2
#define BUTTON_ROUNDNES 0.1
#define TOOL_BUTTON_HEIGHT MAX(RAYGUI_ICON_SIZE, DEFAULT_GUI_FONT_SIZE)
#define TOOL_BUTTON_WIDTH TOOL_BUTTON_HEIGHT

#define BUTTON_SELECTED_HIGHLIGHT_THICKNESS 2.0f
#define BUTTON_SELECTED_HIGHLIGHT_ROUNDNESS 0.2f
#define BUTTON_SELECTED_HIGHLIGHT_SEGMENTS  4

#define DEFAULT_LEVEL_PREVIEW_SIZE 100

#define BROWSER_LEVEL_PREVIEW_SIZE 192
#define COLLECTION_LEVEL_PREVIEW_SIZE 256
#define GOTO_NEXT_LEVEL_PREVIEW_SIZE DEFAULT_LEVEL_PREVIEW_SIZE
#define GOTO_NEXT_SEED_PREVIEW_SIZE GOTO_NEXT_LEVEL_PREVIEW_SIZE

#define CREATE_DIR_MODE (S_IRWXU | S_IRWXG | (S_IROTH | S_IXOTH))

#define NVDATA_STATE_FILE_NAME "state.json"
#define NVDATA_FINISHED_LEVEL_FILE_NAME "finished_levels.dat"
#define NVDATA_DEFAULT_BROWSE_PATH_NAME "levels"
#define NVDATA_SAVED_CURRENT_LEVEL_FILE_NAME_PREFIX "level_in_progress"

#ifndef MAX_FILEPATH_LENGTH
    #if defined(_WIN32)
        #define MAX_FILEPATH_LENGTH      256
    #else
        #define MAX_FILEPATH_LENGTH     4096
    #endif
#endif

#ifndef MAX_TEXT_BUFFER_LENGTH
    #define MAX_TEXT_BUFFER_LENGTH 1024
#endif

#ifndef MAX_TEXTSPLIT_COUNT
    #define MAX_TEXTSPLIT_COUNT 128
#endif

#define SOLVER_SOLVE_SWAP_TIME               0.2
#define SOLVER_DEMO_SOLVE_SWAP_TIME          1.2
#define SOLVER_DEMO_SOLVE_MOVE_POINTER_TIME  0.7
#define SOLVER_UNDO_SWAP_TIME                0.15

#endif /*CONST_H*/

